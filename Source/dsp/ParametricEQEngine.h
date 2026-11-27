#pragma once

#include "BandFilter.h"
#include <array>
#include <cmath>

namespace dspcore
{

class ParametricEQEngine
{
public:
    static constexpr int maxBands = 16;

    void prepare(double newSampleRate, int /*maxBlockSize*/)
    {
        sampleRate = juce::jmax(8000.0, newSampleRate);
        for (auto& band : bands)
            band.prepare(newSampleRate);

        soloFilter.prepare(sampleRate);
        soloMixSmooth = 0.0f;
        soloDryBuffer.clear();
    }

    void reset()
    {
        for (auto& band : bands)
            band.reset();

        soloFilter.reset();
        soloMixSmooth = 0.0f;
    }

    void setBandParameters(int index, const BandParams& params)
    {
        if (index >= 0 && index < maxBands)
            bands[static_cast<size_t>(index)].setParameters(params);
    }

    const BandParams& getBandParameters(int index) const noexcept
    {
        return bands[static_cast<size_t>(juce::jlimit(0, maxBands - 1, index))].getParameters();
    }

    float getBandDynamicGainDb(int index) const noexcept
    {
        if (index >= 0 && index < maxBands)
            return bands[static_cast<size_t>(index)].getCurrentDynamicGainDb();
        return 0.0f;
    }

    void processBlock(juce::AudioBuffer<float>& buffer, float gainScale = 1.0f)
    {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();
        if (numSamples <= 0 || numChannels < 2)
            return;

        int soloIndex = -1;
        for (int i = 0; i < maxBands; ++i)
        {
            const auto& p = bands[static_cast<size_t>(i)].getParameters();
            if (p.enabled && p.solo)
            {
                soloIndex = i;
                break;
            }
        }

        auto* left = buffer.getWritePointer(0);
        auto* right = buffer.getWritePointer(1);

        if (soloIndex >= 0)
        {
            if (soloDryBuffer.getNumSamples() < numSamples)
                soloDryBuffer.setSize(2, numSamples, false, false, true);

            soloDryBuffer.copyFrom(0, 0, left, numSamples);
            soloDryBuffer.copyFrom(1, 0, right, numSamples);

            const auto& srcBand = bands[static_cast<size_t>(soloIndex)].getParameters();
            BandParams inspectParams = srcBand;
            inspectParams.enabled = true;
            inspectParams.solo = false;
            inspectParams.dynamicEnabled = false;
            inspectParams.shape = FilterShape::BandPass;
            inspectParams.q = juce::jlimit(0.4f, 6.0f, srcBand.q > 0.1f ? srcBand.q : 1.0f);

            soloFilter.setParameters(inspectParams);
            soloFilter.processBlock(left, right, numSamples, 1.0f);

            const auto* dryL = soloDryBuffer.getReadPointer(0);
            const auto* dryR = soloDryBuffer.getReadPointer(1);
            const float coeff = std::exp(-1.0f / (0.015f * static_cast<float>(sampleRate)));

            for (int i = 0; i < numSamples; ++i)
            {
                soloMixSmooth = coeff * soloMixSmooth + (1.0f - coeff) * 1.0f;
                left[i] = dryL[i] * (1.0f - soloMixSmooth) + left[i] * soloMixSmooth;
                right[i] = dryR[i] * (1.0f - soloMixSmooth) + right[i] * soloMixSmooth;
            }
            return;
        }

        soloMixSmooth = 0.0f;

        for (int i = 0; i < maxBands; ++i)
        {
            bands[static_cast<size_t>(i)].processBlock(left, right, numSamples, gainScale);
        }
    }

    float getBandResponseDb(int bandIndex, double freqHz, float gainScale = 1.0f) const noexcept
    {
        if (bandIndex < 0 || bandIndex >= maxBands)
            return 0.0f;
        return bands[static_cast<size_t>(bandIndex)].getMagnitudeDbAtFrequency(freqHz, gainScale);
    }

    float getCombinedResponseDb(double freqHz, float gainScale = 1.0f) const noexcept
    {
        float sumDb = 0.0f;
        for (int i = 0; i < maxBands; ++i)
        {
            const auto& p = bands[static_cast<size_t>(i)].getParameters();
            if (p.enabled)
                sumDb += bands[static_cast<size_t>(i)].getMagnitudeDbAtFrequency(freqHz, gainScale);
        }
        return juce::jlimit(-48.0f, 48.0f, sumDb);
    }

private:
    double sampleRate = 48000.0;
    std::array<BandFilter, maxBands> bands;
    BandFilter soloFilter;
    juce::AudioBuffer<float> soloDryBuffer;
    float soloMixSmooth = 0.0f;
};

} // namespace dspcore
