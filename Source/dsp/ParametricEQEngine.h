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

    void prepare(double newSampleRate, int maxBlockSize)
    {
        sampleRate = juce::jmax(8000.0, newSampleRate);
        for (auto& band : bands)
            band.prepare(sampleRate);

        soloFilter.prepare(sampleRate);

        // Allocate the Solo scratch buffer before audio processing starts.
        soloDryBuffer.setSize(
            2,
            juce::jmax(1, maxBlockSize),
            false,
            false,
            true);
        soloDryBuffer.clear();
        soloMixSmooth = 0.0f;
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
            const int scratchCapacity = soloDryBuffer.getNumSamples();
            if (scratchCapacity <= 0)
                return; // prepare() must run before processing.

            const auto& srcBand = bands[static_cast<size_t>(soloIndex)].getParameters();
            BandParams inspectParams = srcBand;
            inspectParams.enabled = true;
            inspectParams.solo = false;
            inspectParams.dynamicEnabled = false;
            inspectParams.shape = FilterShape::BandPass;
            inspectParams.q = juce::jlimit(0.4f, 6.0f, srcBand.q > 0.1f ? srcBand.q : 1.0f);

            soloFilter.setParameters(inspectParams);
            const float coeff = std::exp(-1.0f / (0.015f * static_cast<float>(sampleRate)));

            // Process oversized blocks in chunks using the preallocated buffer.
            for (int offset = 0; offset < numSamples; offset += scratchCapacity)
            {
                const int chunkSize = juce::jmin(scratchCapacity, numSamples - offset);
                soloDryBuffer.copyFrom(0, 0, left + offset, chunkSize);
                soloDryBuffer.copyFrom(1, 0, right + offset, chunkSize);

                soloFilter.processBlock(left + offset, right + offset, chunkSize, 1.0f);

                const auto* dryL = soloDryBuffer.getReadPointer(0);
                const auto* dryR = soloDryBuffer.getReadPointer(1);
                for (int i = 0; i < chunkSize; ++i)
                {
                    soloMixSmooth = coeff * soloMixSmooth + (1.0f - coeff);
                    left[offset + i] = dryL[i] * (1.0f - soloMixSmooth)
                                     + left[offset + i] * soloMixSmooth;
                    right[offset + i] = dryR[i] * (1.0f - soloMixSmooth)
                                      + right[offset + i] * soloMixSmooth;
                }
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
