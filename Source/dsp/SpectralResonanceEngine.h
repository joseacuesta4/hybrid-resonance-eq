
#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>

namespace dspcore
{

struct SpectralBandWindow
{
    bool active = false;
    float centerFreqHz = 1000.0f;
    float q = 1.0f;
    float rangeDb = -6.0f;
    float thresholdDb = -24.0f;
    bool autoThreshold = true;
};

enum class SpectralResolution : int { Low = 0, High = 1 };

struct SpectralConfig
{
    bool enabled = false;
    bool deltaListen = false;
    float depth = 0.65f;
    float sharpness = 0.60f;
    float smoothing = 0.50f;
    float mix = 1.0f;
    float attackMs = 12.0f;
    float releaseMs = 120.0f;
    SpectralResolution resolution = SpectralResolution::High;
    std::array<SpectralBandWindow, 16> bandWindows {};
};

class SpectralResonanceEngine
{
public:
    static constexpr int fftSize = 4096;
    static constexpr int hopSize = fftSize / 4;
    static constexpr int numBins = fftSize / 2 + 1;
    static constexpr int ringSize = fftSize * 2;

    SpectralResonanceEngine()
        : fftLow(11), fftHigh(12)
    {
        juce::dsp::WindowingFunction<float>::fillWindowingTables(
            windowLowTable.data(),
            2048,
            juce::dsp::WindowingFunction<float>::hann,
            false);

        juce::dsp::WindowingFunction<float>::fillWindowingTables(
            windowHighTable.data(),
            4096,
            juce::dsp::WindowingFunction<float>::hann,
            false);

        reset();
    }

    void prepare(double newSampleRate, int) noexcept
    {
        sampleRate = juce::jmax(8000.0, newSampleRate);
        reset();
    }

    void reset() noexcept
    {
        inputL.fill(0.0f);
        inputR.fill(0.0f);
        delayL.fill(0.0f);
        delayR.fill(0.0f);
        outputL.fill(0.0f);
        outputR.fill(0.0f);
        norm.fill(0.0f);
        workL.fill(0.0f);
        workR.fill(0.0f);
        envelopeDb.fill(0.0f);
        reductionDb.fill(0.0f);

        fifoWrite = 0;
        delayWrite = 0;
        samplesInFrame = 0;
        readPosition = 0;
        samplesSeen = 0;
        wasActive = false;
    }

    void setConfig(const SpectralConfig& newConfig) noexcept
    {
        if (config.resolution != newConfig.resolution)
            reset();

        config = newConfig;
    }

    const SpectralConfig& getConfig() const noexcept
    {
        return config;
    }

    bool isActive() const noexcept
    {
        if (!config.enabled
            || config.depth <= 0.0001f
            || config.mix <= 0.0001f)
            return false;

        for (const auto& band : config.bandWindows)
            if (band.active)
                return true;

        return false;
    }

    int getLatencySamples() const noexcept
    {
        return isActive() ? activeSize() : 0;
    }

    int getActiveNumBins() const noexcept
    {
        return activeSize() / 2 + 1;
    }

    void getReduction(
        std::array<float, numBins>& out) const noexcept
    {
        out = reductionDb;
    }

    void processBlock(juce::AudioBuffer<float>& buffer) noexcept
    {
        if (buffer.getNumChannels() < 2
            || buffer.getNumSamples() == 0)
            return;

        if (!isActive())
        {
            if (wasActive)
                reset();

            return;
        }

        if (!wasActive)
        {
            reset();
            wasActive = true;
        }

        auto* left = buffer.getWritePointer(0);
        auto* right = buffer.getWritePointer(1);

        const float mix =
            juce::jlimit(0.0f, 1.0f, config.mix);

        const float dry = 1.0f - mix;

        for (int n = 0; n < buffer.getNumSamples(); ++n)
        {
            const float inL = left[n];
            const float inR = right[n];

            const int size = activeSize();

            const float delayedL =
                delayL[static_cast<size_t>(delayWrite)];

            const float delayedR =
                delayR[static_cast<size_t>(delayWrite)];

            delayL[static_cast<size_t>(delayWrite)] = inL;
            delayR[static_cast<size_t>(delayWrite)] = inR;

            delayWrite = (delayWrite + 1) % size;

            inputL[static_cast<size_t>(fifoWrite)] = inL;
            inputR[static_cast<size_t>(fifoWrite)] = inR;

            fifoWrite = (fifoWrite + 1) % size;

            samplesSeen = juce::jmin(size, samplesSeen + 1);

            const float wetL =
                outputL[static_cast<size_t>(readPosition)];

            const float wetR =
                outputR[static_cast<size_t>(readPosition)];

            const float normValue =
                norm[static_cast<size_t>(readPosition)];

            outputL[static_cast<size_t>(readPosition)] = 0.0f;
            outputR[static_cast<size_t>(readPosition)] = 0.0f;
            norm[static_cast<size_t>(readPosition)] = 0.0f;

            readPosition = (readPosition + 1) % ringSize;

            if (samplesSeen >= size && normValue > 1.0e-8f)
            {
                const float outL = wetL / normValue;
                const float outR = wetR / normValue;

                if (config.deltaListen)
                {
                    left[n] =
                        dry * delayedL + mix * (delayedL - outL);

                    right[n] =
                        dry * delayedR + mix * (delayedR - outR);
                }
                else
                {
                    left[n] = dry * delayedL + mix * outL;
                    right[n] = dry * delayedR + mix * outR;
                }
            }
            else
            {
                left[n] = 0.0f;
                right[n] = 0.0f;
            }

            ++samplesInFrame;

            if (samplesInFrame >= activeHopSize())
            {
                samplesInFrame = 0;

                if (samplesSeen >= size)
                    processFrame();
            }
        }
    }

private:
    int activeSize() const noexcept
    {
        return config.resolution == SpectralResolution::Low
            ? 2048 : 4096;
    }

    int activeHopSize() const noexcept
    {
        return activeSize() / 4;
    }

    float bandWeight(
        float frequency,
        float& rangeDb,
        float& thresholdDb,
        bool& autoThreshold) const noexcept
    {
        float weight = 0.0f;

        rangeDb = -6.0f;
        thresholdDb = -24.0f;
        autoThreshold = true;

        for (const auto& band : config.bandWindows)
        {
            if (!band.active)
                continue;

            const float f0 = juce::jlimit(
                20.0f,
                static_cast<float>(sampleRate * 0.48),
                band.centerFreqHz);

            const float q =
                juce::jlimit(0.2f, 18.0f, band.q);

            const float oct = std::abs(
                std::log2(
                    juce::jmax(1.0f, frequency) / f0));

            const float sigma =
                juce::jlimit(0.045f, 1.0f, 0.50f / q);

            const float w =
                std::exp(-0.5f * (oct / sigma) * (oct / sigma));

            if (w > weight)
            {
                weight = w;

                rangeDb = juce::jlimit(
                    -24.0f, 0.0f, band.rangeDb);

                thresholdDb = juce::jlimit(
                    -90.0f, 0.0f, band.thresholdDb);

                autoThreshold = band.autoThreshold;
            }
        }

        return weight;
    }

    void processFrame() noexcept
    {
        const int size = activeSize();
        const int bins = size / 2 + 1;

        auto& activeFFT =
            config.resolution == SpectralResolution::Low
                ? fftLow : fftHigh;

        const auto& activeWindowTable =
            config.resolution == SpectralResolution::Low
                ? windowLowTable : windowHighTable;

        for (int i = 0; i < size; ++i)
        {
            const int index = (fifoWrite + i) % size;

            workL[static_cast<size_t>(i)] =
                inputL[static_cast<size_t>(index)];

            workR[static_cast<size_t>(i)] =
                inputR[static_cast<size_t>(index)];
        }

        for (int i = 0; i < size; ++i)
        {
            const float w = juce::jmax(
                0.05f,
                activeWindowTable[static_cast<size_t>(i)]);

            workL[static_cast<size_t>(i)] *= w;
            workR[static_cast<size_t>(i)] *= w;
        }

        activeFFT.performRealOnlyForwardTransform(
            workL.data(), true);

        activeFFT.performRealOnlyForwardTransform(
            workR.data(), true);

        const float binHz =
            static_cast<float>(
                sampleRate / static_cast<double>(size));

        const int radius = juce::jlimit(
            2,
            18,
            static_cast<int>(
                std::round(
                    3.0f + 14.0f * (1.0f - config.sharpness))));

        const float attack = std::exp(
            -static_cast<float>(activeHopSize())
            / (static_cast<float>(sampleRate)
               * 0.001f
               * juce::jmax(0.5f, config.attackMs)));

        const float release = std::exp(
            -static_cast<float>(activeHopSize())
            / (static_cast<float>(sampleRate)
               * 0.001f
               * juce::jmax(5.0f, config.releaseMs)));

        const float depth =
            juce::jlimit(0.0f, 1.0f, config.depth);

        const float smoothing =
            juce::jlimit(0.0f, 1.0f, config.smoothing);

        const float thresholdOffset =
            1.0f + 4.0f * (1.0f - config.sharpness);

        std::array<float, numBins> magnitude {};

        for (int b = 0; b < bins; ++b)
        {
            float magL = 0.0f;
            float magR = 0.0f;

            if (b == 0)
            {
                magL = std::abs(workL[0]);
                magR = std::abs(workR[0]);
            }
            else if (b == size / 2)
            {
                magL = std::abs(workL[1]);
                magR = std::abs(workR[1]);
            }
            else
            {
                const size_t re = static_cast<size_t>(2 * b);
                const size_t im = re + 1;

                magL = std::hypot(workL[re], workL[im]);
                magR = std::hypot(workR[re], workR[im]);
            }

            magnitude[static_cast<size_t>(b)] =
                0.5f * (magL + magR);
        }

        for (int b = 0; b < bins; ++b)
        {
            const size_t idx = static_cast<size_t>(b);

            const float freq = juce::jmax(
                1.0f,
                static_cast<float>(b) * binHz);

            float range = -6.0f;
            float threshold = -24.0f;
            bool autoThreshold = true;

            const float weight = bandWeight(
                freq, range, threshold, autoThreshold);

            float localSum = 0.0f;
            int count = 0;

            for (int k = juce::jmax(1, b - radius);
                 k <= juce::jmin(bins - 1, b + radius);
                 ++k)
            {
                localSum +=
                    juce::Decibels::gainToDecibels(
                        magnitude[static_cast<size_t>(k)]
                            / static_cast<float>(size)
                        + 1.0e-9f);

                ++count;
            }

            const float localDb =
                count > 0
                    ? localSum / static_cast<float>(count)
                    : -120.0f;

            const float levelDb =
                juce::Decibels::gainToDecibels(
                    magnitude[idx] / static_cast<float>(size)
                    + 1.0e-9f);

            const float effectiveThreshold =
                autoThreshold
                    ? localDb + thresholdOffset
                    : juce::jmax(
                        threshold,
                        localDb + thresholdOffset);

            const float excess =
                juce::jmax(0.0f, levelDb - effectiveThreshold);

            const float targetReduction =
                (weight > 0.001f && excess > 0.0f)
                    ? range * depth * weight
                        * juce::jlimit(0.0f, 1.0f, excess / 12.0f)
                    : 0.0f;

            const float coeff =
                targetReduction < envelopeDb[idx]
                    ? attack : release;

            envelopeDb[idx] =
                coeff * envelopeDb[idx]
                + (1.0f - coeff) * targetReduction;

            const float smoothCoeff =
                0.15f + 0.70f * smoothing;

            reductionDb[idx] +=
                (envelopeDb[idx] - reductionDb[idx])
                * (1.0f - smoothCoeff);

            const float gain =
                juce::Decibels::decibelsToGain(reductionDb[idx]);

            if (b == 0)
            {
                workL[0] *= gain;
                workR[0] *= gain;
            }
            else if (b == size / 2)
            {
                workL[1] *= gain;
                workR[1] *= gain;
            }
            else
            {
                const size_t re = static_cast<size_t>(2 * b);
                const size_t im = re + 1;

                workL[re] *= gain;
                workL[im] *= gain;
                workR[re] *= gain;
                workR[im] *= gain;
            }
        }

        activeFFT.performRealOnlyInverseTransform(workL.data());
        activeFFT.performRealOnlyInverseTransform(workR.data());

        // Weighted overlap-add using the same Hann window.
        for (int i = 0; i < size; ++i)
        {
            const int ring = (readPosition + i) % ringSize;

            const float w = juce::jmax(
                0.05f,
                activeWindowTable[static_cast<size_t>(i)]);

            outputL[static_cast<size_t>(ring)] +=
                workL[static_cast<size_t>(i)]
                * w / static_cast<float>(size);

            outputR[static_cast<size_t>(ring)] +=
                workR[static_cast<size_t>(i)]
                * w / static_cast<float>(size);

            norm[static_cast<size_t>(ring)] += w * w;
        }
    }

    double sampleRate = 48000.0;
    SpectralConfig config;

    juce::dsp::FFT fftLow;
    juce::dsp::FFT fftHigh;

    std::array<float, fftSize> windowLowTable {};
    std::array<float, fftSize> windowHighTable {};

    std::array<float, fftSize> inputL {}, inputR {}, delayL {}, delayR {};
    std::array<float, ringSize> outputL {}, outputR {}, norm {};
    std::array<float, fftSize * 2> workL {}, workR {};
    std::array<float, numBins> envelopeDb {}, reductionDb {};

    int fifoWrite = 0;
    int delayWrite = 0;
    int samplesInFrame = 0;
    int readPosition = 0;
    int samplesSeen = 0;
    bool wasActive = false;
};

} // namespace dspcore
