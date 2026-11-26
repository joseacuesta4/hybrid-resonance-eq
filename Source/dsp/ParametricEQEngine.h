#pragma once

#include "BandFilter.h"
#include <array>

namespace dspcore
{

class ParametricEQEngine
{
public:
    static constexpr int maxBands = 16;

    void prepare(double newSampleRate, int /*maxBlockSize*/)
    {
        sampleRate = newSampleRate;
        for (auto& band : bands)
            band.prepare(newSampleRate);
    }

    void reset()
    {
        for (auto& band : bands)
            band.reset();
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
            BandParams soloBand = bands[static_cast<size_t>(soloIndex)].getParameters();
            soloBand.shape = FilterShape::BandPass;
            soloBand.q = juce::jlimit(0.5f, 16.0f, soloBand.q * 1.5f);
            soloMonitorFilter.prepare(sampleRate);
            soloMonitorFilter.setParameters(soloBand);
            soloMonitorFilter.processBlock(left, right, numSamples, 1.0f);
            return;
        }

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
    BandFilter soloMonitorFilter;
};

} // namespace dspcore
