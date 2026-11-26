#pragma once

#include "BandFilter.h"
#include <array>

namespace dspcore
{

class ParametricEQEngine
{
public:
    static constexpr int kNumBands = 16;

    void prepare(double sampleRate, int maxBlockSize)
    {
        for (auto& band : bands)
            band.prepare(sampleRate, maxBlockSize);
    }

    void reset()
    {
        for (auto& band : bands)
            band.reset();
    }

    void setBandConfig(int index, const BandConfig& cfg) noexcept
    {
        if (index >= 0 && index < kNumBands)
            bands[static_cast<size_t>(index)].setConfig(cfg);
    }

    const BandConfig& getBandConfig(int index) const noexcept
    {
        static const BandConfig emptyCfg;
        if (index >= 0 && index < kNumBands)
            return bands[static_cast<size_t>(index)].getConfig();
        return emptyCfg;
    }

    float getBandDynamicGainDb(int index) const noexcept
    {
        if (index >= 0 && index < kNumBands)
            return bands[static_cast<size_t>(index)].getCurrentDynamicGainDb();
        return 0.0f;
    }

    float getTotalMagnitudeDb(float freqHz) const noexcept
    {
        float sumDb = 0.0f;
        for (const auto& band : bands)
            sumDb += band.getMagnitudeDb(freqHz);
        return sumDb;
    }

    void processBlock(juce::AudioBuffer<float>& buffer) noexcept
    {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();
        if (numSamples <= 0 || numChannels < 2)
            return;

        bool anySolo = false;
        for (const auto& band : bands)
        {
            if (band.getConfig().enabled && band.getConfig().solo)
            {
                anySolo = true;
                break;
            }
        }

        auto* leftPtr = buffer.getWritePointer(0);
        auto* rightPtr = buffer.getWritePointer(1);

        if (!anySolo)
        {
            for (int n = 0; n < numSamples; ++n)
            {
                float l = leftPtr[n];
                float r = rightPtr[n];
                for (auto& band : bands)
                    band.processSample(l, r);
                leftPtr[n] = l;
                rightPtr[n] = r;
            }
            return;
        }

        for (int n = 0; n < numSamples; ++n)
        {
            const float dryL = leftPtr[n];
            const float dryR = rightPtr[n];
            float soloSumL = 0.0f;
            float soloSumR = 0.0f;

            for (auto& band : bands)
            {
                if (band.getConfig().enabled && band.getConfig().solo)
                {
                    float tl = dryL;
                    float tr = dryR;
                    band.processSample(tl, tr);
                    soloSumL += tl;
                    soloSumR += tr;
                }
            }

            leftPtr[n] = soloSumL;
            rightPtr[n] = soloSumR;
        }
    }

private:
    std::array<BandFilter, kNumBands> bands;
};

} // namespace dspcore
