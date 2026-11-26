#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>

namespace dspcore
{

struct SpectralConfig
{
    bool enabled = true;
    bool deltaListen = false;

    float depth = 0.65f;
    float sharpness = 0.60f;
    float smoothing = 0.50f;
    float mix = 1.0f;

    float minFreqHz = 80.0f;
    float maxFreqHz = 16000.0f;
};

class SpectralResonanceEngine
{
public:
    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int hopSize = 512;
    static constexpr int numBins = fftSize / 2 + 1;

    SpectralResonanceEngine()
        : fft(fftOrder),
          window(fftSize, juce::dsp::WindowingFunction<float>::hann)
    {
        clearBuffers();
    }

    void prepare(double newSampleRate, int /*maxBlockSize*/)
    {
        sampleRate = juce::jmax(8000.0, newSampleRate);
        clearBuffers();
    }

    int getLatencySamples() const noexcept
    {
        return config.enabled ? fftSize : 0;
    }

    void setConfig(const SpectralConfig& cfg) noexcept
    {
        config = cfg;
    }

    const SpectralConfig& getConfig() const noexcept
    {
        return config;
    }

    void getMagnitudeSpectrumDb(std::array<float, numBins>& outDb) const noexcept
    {
        for (int i = 0; i < numBins; ++i)
            outDb[static_cast<size_t>(i)] = latestMagDb[static_cast<size_t>(i)];
    }

    void processBlock(juce::AudioBuffer<float>& buffer) noexcept
    {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();
        if (numSamples <= 0 || numChannels < 2)
            return;

        if (!config.enabled)
            return;

        auto* leftPtr = buffer.getWritePointer(0);
        auto* rightPtr = buffer.getWritePointer(1);

        const float wetMix = juce::jlimit(0.0f, 1.0f, config.mix);
        const float dryMix = 1.0f - wetMix;

        for (int n = 0; n < numSamples; ++n)
        {
            const float inL = leftPtr[n];
            const float inR = rightPtr[n];

            const float delayedL = delayLineL[static_cast<size_t>(delayWritePos)];
            const float delayedR = delayLineR[static_cast<size_t>(delayWritePos)];
            delayLineL[static_cast<size_t>(delayWritePos)] = inL;
            delayLineR[static_cast<size_t>(delayWritePos)] = inR;
            delayWritePos = (delayWritePos + 1) % fftSize;

            inputFifoL[static_cast<size_t>(fifoWritePos)] = inL;
            inputFifoR[static_cast<size_t>(fifoWritePos)] = inR;
            fifoWritePos = (fifoWritePos + 1) % fftSize;

            const float procL = outputAccumL[static_cast<size_t>(outReadPos)];
            const float procR = outputAccumR[static_cast<size_t>(outReadPos)];
            outputAccumL[static_cast<size_t>(outReadPos)] = 0.0f;
            outputAccumR[static_cast<size_t>(outReadPos)] = 0.0f;
            outReadPos = (outReadPos + 1) % fftSize;

            if (config.deltaListen)
            {
                const float deltaL = delayedL - procL;
                const float deltaR = delayedR - procR;
                leftPtr[n] = dryMix * delayedL + wetMix * deltaL;
                rightPtr[n] = dryMix * delayedR + wetMix * deltaR;
            }
            else
            {
                leftPtr[n] = dryMix * delayedL + wetMix * procL;
                rightPtr[n] = dryMix * delayedR + wetMix * procR;
            }

            samplesSinceHop++;
            if (samplesSinceHop >= hopSize)
            {
                samplesSinceHop = 0;
                processFrame();
            }
        }
    }

private:
    void clearBuffers() noexcept
    {
        inputFifoL.fill(0.0f);
        inputFifoR.fill(0.0f);
        outputAccumL.fill(0.0f);
        outputAccumR.fill(0.0f);
        delayLineL.fill(0.0f);
        delayLineR.fill(0.0f);
        fftWorkL.fill(0.0f);
        fftWorkR.fill(0.0f);
        magL.fill(0.0f);
        magR.fill(0.0f);
        smoothedEnv.fill(1.0e-6f);
        binGains.fill(1.0f);
        latestMagDb.fill(-120.0f);

        fifoWritePos = 0;
        outReadPos = 0;
        delayWritePos = 0;
        samplesSinceHop = 0;
    }

    void processFrame() noexcept
    {
        for (int i = 0; i < fftSize; ++i)
        {
            const int idx = (fifoWritePos + i) % fftSize;
            fftWorkL[static_cast<size_t>(i)] = inputFifoL[static_cast<size_t>(idx)];
            fftWorkR[static_cast<size_t>(i)] = inputFifoR[static_cast<size_t>(idx)];
        }

        window.multiplyWithWindowingTable(fftWorkL.data(), static_cast<size_t>(fftSize));
        window.multiplyWithWindowingTable(fftWorkR.data(), static_cast<size_t>(fftSize));

        fft.performRealOnlyForwardTransform(fftWorkL.data(), true);
        fft.performRealOnlyForwardTransform(fftWorkR.data(), true);

        const float binHzStep = static_cast<float>(sampleRate) / static_cast<float>(fftSize);
        const float minHz = juce::jlimit(20.0f, 10000.0f, config.minFreqHz);
        const float maxHz = juce::jlimit(minHz + 100.0f, static_cast<float>(sampleRate) * 0.49f, config.maxFreqHz);

        for (int b = 0; b < numBins; ++b)
        {
            const int reIdx = 2 * b;
            const int imIdx = 2 * b + 1;

            const float mL = std::hypot(fftWorkL[static_cast<size_t>(reIdx)], fftWorkL[static_cast<size_t>(imIdx)]);
            const float mR = std::hypot(fftWorkR[static_cast<size_t>(reIdx)], fftWorkR[static_cast<size_t>(imIdx)]);
            const float mAvg = 0.5f * (mL + mR);

            magL[static_cast<size_t>(b)] = mL;
            magR[static_cast<size_t>(b)] = mR;

            const float db = juce::Decibels::gainToDecibels(mAvg / static_cast<float>(fftSize) + 1.0e-8f);
            latestMagDb[static_cast<size_t>(b)] = juce::jlimit(-120.0f, 12.0f, db);
        }

        const int halfKernel = juce::jlimit(2, 24, static_cast<int>(std::round(22.0f * (1.0f - 0.75f * config.sharpness))));
        const float smoothCoeff = juce::jlimit(0.05f, 0.95f, 0.25f + 0.65f * config.smoothing);
        const float depthAmount = juce::jlimit(0.0f, 1.0f, config.depth);

        for (int b = 0; b < numBins; ++b)
        {
            const float freqHz = static_cast<float>(b) * binHzStep;
            if (freqHz < minHz || freqHz > maxHz)
            {
                binGains[static_cast<size_t>(b)] = 1.0f;
                continue;
            }

            float sumLocal = 0.0f;
            int countLocal = 0;
            const int startK = juce::jmax(0, b - halfKernel);
            const int endK = juce::jmin(numBins - 1, b + halfKernel);
            for (int k = startK; k <= endK; ++k)
            {
                const float mK = 0.5f * (magL[static_cast<size_t>(k)] + magR[static_cast<size_t>(k)]);
                sumLocal += mK;
                ++countLocal;
            }

            const float localMean = (countLocal > 0) ? (sumLocal / static_cast<float>(countLocal)) : 1.0e-6f;
            smoothedEnv[static_cast<size_t>(b)] =
                smoothCoeff * smoothedEnv[static_cast<size_t>(b)] + (1.0f - smoothCoeff) * localMean;

            const float mCur = 0.5f * (magL[static_cast<size_t>(b)] + magR[static_cast<size_t>(b)]);
            const float ratio = mCur / juce::jmax(1.0e-6f, smoothedEnv[static_cast<size_t>(b)]);

            const float excess = juce::jmax(0.0f, ratio - (1.25f - 0.35f * config.sharpness));
            const float attenDb = -depthAmount * juce::jlimit(0.0f, 18.0f, excess * 8.0f);
            const float targetGain = juce::Decibels::decibelsToGain(attenDb);

            binGains[static_cast<size_t>(b)] =
                smoothCoeff * binGains[static_cast<size_t>(b)] + (1.0f - smoothCoeff) * targetGain;
        }

        for (int b = 0; b < numBins; ++b)
        {
            const float g = binGains[static_cast<size_t>(b)];
            const int reIdx = 2 * b;
            const int imIdx = 2 * b + 1;

            fftWorkL[static_cast<size_t>(reIdx)] *= g;
            fftWorkL[static_cast<size_t>(imIdx)] *= g;
            fftWorkR[static_cast<size_t>(reIdx)] *= g;
            fftWorkR[static_cast<size_t>(imIdx)] *= g;
        }

        fft.performRealOnlyInverseTransform(fftWorkL.data();
        fft.performRealOnlyInverseTransform(fftWorkR.data();

        constexpr float olaScale = 1.0f / 1.5f;

        for (int i = 0; i < fftSize; ++i)
        {
            const int idx = (outReadPos + i) % fftSize;
            outputAccumL[static_cast<size_t>(idx)] += fftWorkL[static_cast<size_t>(i)] * olaScale;
            outputAccumR[static_cast<size_t>(idx)] += fftWorkR[static_cast<size_t>(i)] * olaScale;
        }
    }

    double sampleRate = 48000.0;
    SpectralConfig config;

    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;

    std::array<float, fftSize> inputFifoL {};
    std::array<float, fftSize> inputFifoR {};
    std::array<float, fftSize> outputAccumL {};
    std::array<float, fftSize> outputAccumR {};
    std::array<float, fftSize> delayLineL {};
    std::array<float, fftSize> delayLineR {};

    std::array<float, fftSize * 2> fftWorkL {};
    std::array<float, fftSize * 2> fftWorkR {};

    std::array<float, numBins> magL {};
    std::array<float, numBins> magR {};
    std::array<float, numBins> smoothedEnv {};
    std::array<float, numBins> binGains {};
    std::array<float, numBins> latestMagDb {};

    int fifoWritePos = 0;
    int outReadPos = 0;
    int delayWritePos = 0;
    int samplesSinceHop = 0;
};

} // namespace dspcore
