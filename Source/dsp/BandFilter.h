#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>
#include <complex>

namespace dspcore
{

enum class FilterShape : int
{
    Bell = 0,
    LowShelf,
    HighShelf,
    LowCut,
    HighCut,
    Notch,
    BandPass,
    TiltShelf
};

enum class ChannelRoute : int
{
    Stereo = 0,
    Mid,
    Side,
    Left,
    Right
};

struct BandParams
{
    bool enabled = false;
    bool solo = false;
    bool dynamicEnabled = false;
    bool autoThreshold = true;
    bool spectralEnabled = false;

    FilterShape shape = FilterShape::Bell;
    ChannelRoute route = ChannelRoute::Stereo;

    float freqHz = 1000.0f;
    float gainDb = 0.0f;
    float q = 1.0f;
    float slopeDbOct = 12.0f;

    float dynRangeDb = -6.0f;
    float dynThresholdDb = -24.0f;
    float dynAttackMs = 15.0f;
    float dynReleaseMs = 120.0f;
};

class BandFilter
{
public:
    static bool supportsSlope(FilterShape shape) noexcept
    {
        return shape == FilterShape::LowCut || shape == FilterShape::HighCut;
    }

    static bool supportsGain(FilterShape shape) noexcept
    {
        return shape == FilterShape::Bell
            || shape == FilterShape::LowShelf
            || shape == FilterShape::HighShelf
            || shape == FilterShape::TiltShelf;
    }

    void prepare(double newSampleRate)
    {
        sampleRate = juce::jmax(8000.0, newSampleRate);
        reset();
        updateCoefficients(currentParams, 0.0f, 1.0f);
    }

    void reset()
    {
        for (auto& s : stagesL) s.reset();
        for (auto& s : stagesR) s.reset();
        envLevel = 0.0f;
        autoThreshEnvDb = -24.0f;
        currentDynamicGainDb = 0.0f;
    }

    void setParameters(const BandParams& p)
    {
        currentParams = p;
    }

    const BandParams& getParameters() const noexcept
    {
        return currentParams;
    }

    float getCurrentDynamicGainDb() const noexcept
    {
        return currentDynamicGainDb;
    }

    void processBlock(float* left, float* right, int numSamples, float gainScale = 1.0f)
    {
        if (!currentParams.enabled || numSamples <= 0)
        {
            currentDynamicGainDb = 0.0f;
            return;
        }

        const float clampedScale = juce::jlimit(0.0f, 2.0f, gainScale);
        float appliedDynDb = 0.0f;

        if (currentParams.dynamicEnabled)
        {
            const float attackCoeff = std::exp(-1.0f / (0.001f * juce::jmax(0.5f, currentParams.dynAttackMs) * static_cast<float>(sampleRate)));
            const float releaseCoeff = std::exp(-1.0f / (0.001f * juce::jmax(5.0f, currentParams.dynReleaseMs) * static_cast<float>(sampleRate)));

            float peakEnv = 0.0f;
            for (int i = 0; i < numSamples; ++i)
            {
                const float detectorIn = std::max(std::abs(left[i]), std::abs(right[i]));
                if (detectorIn > envLevel)
                    envLevel = attackCoeff * envLevel + (1.0f - attackCoeff) * detectorIn;
                else
                    envLevel = releaseCoeff * envLevel + (1.0f - releaseCoeff) * detectorIn;

                if (envLevel > peakEnv)
                    peakEnv = envLevel;
            }

            const float levelDb = juce::Decibels::gainToDecibels(peakEnv + 1.0e-7f);

            float effectiveThreshDb = currentParams.dynThresholdDb;
            if (currentParams.autoThreshold)
            {
                autoThreshEnvDb = 0.985f * autoThreshEnvDb + 0.015f * juce::jlimit(-54.0f, -6.0f, levelDb - 6.0f);
                effectiveThreshDb = autoThreshEnvDb;
            }

            const float overDb = juce::jmax(0.0f, levelDb - effectiveThreshDb);
            const float ratioFactor = juce::jlimit(0.0f, 1.0f, overDb / 16.0f);
            appliedDynDb = currentParams.dynRangeDb * ratioFactor;
        }
        else
        {
            envLevel = 0.0f;
            appliedDynDb = 0.0f;
        }

        currentDynamicGainDb = appliedDynDb * clampedScale;
        updateCoefficients(currentParams, appliedDynDb, clampedScale);

        const int numActiveStages = getActiveStageCount(currentParams);

        for (int i = 0; i < numSamples; ++i)
        {
            const float inL = left[i];
            const float inR = right[i];

            float procL = inL;
            float procR = inR;

            for (int st = 0; st < numActiveStages; ++st)
            {
                procL = stagesL[static_cast<size_t>(st)].processSample(procL);
                procR = stagesR[static_cast<size_t>(st)].processSample(procR);
            }

            switch (currentParams.route)
            {
                case ChannelRoute::Stereo:
                    left[i] = procL;
                    right[i] = procR;
                    break;

                case ChannelRoute::Left:
                    left[i] = procL;
                    break;

                case ChannelRoute::Right:
                    right[i] = procR;
                    break;

                case ChannelRoute::Mid:
                {
                    const float sideIn = 0.5f * (inL - inR);
                    const float midProc = 0.5f * (procL + procR);
                    left[i] = midProc + sideIn;
                    right[i] = midProc - sideIn;
                    break;
                }

                case ChannelRoute::Side:
                {
                    const float midIn = 0.5f * (inL + inR);
                    const float sideProc = 0.5f * (procL - procR);
                    left[i] = midIn + sideProc;
                    right[i] = midIn - sideProc;
                    break;
                }
            }
        }
    }

    float getMagnitudeDbAtFrequency(double freq, float gainScale = 1.0f) const noexcept
    {
        if (!currentParams.enabled)
            return 0.0f;

        const double clampedFreq = juce::jlimit(10.0, sampleRate * 0.495, freq);
        const double w = 2.0 * juce::MathConstants<double>::pi * clampedFreq / sampleRate;
        const std::complex<double> zInv1(std::cos(w), -std::sin(w));
        const std::complex<double> zInv2 = zInv1 * zInv1;

        double totalMagSq = 1.0;
        const int numActiveStages = getActiveStageCount(currentParams);

        BiquadCoeffs evalCoeffs[4];
        computeCoeffsInternal(currentParams, currentDynamicGainDb, juce::jlimit(0.0f, 2.0f, gainScale), evalCoeffs);

        for (int st = 0; st < numActiveStages; ++st)
        {
            const auto& c = evalCoeffs[st];
            const std::complex<double> num = static_cast<double>(c.b0)
                                           + static_cast<double>(c.b1) * zInv1
                                           + static_cast<double>(c.b2) * zInv2;
            const std::complex<double> den = 1.0
                                           + static_cast<double>(c.a1) * zInv1
                                           + static_cast<double>(c.a2) * zInv2;

            const double denNorm = std::norm(den);
            if (denNorm > 1.0e-18)
                totalMagSq *= (std::norm(num) / denNorm);
        }

        const float mag = static_cast<float>(std::sqrt(juce::jmax(1.0e-12, totalMagSq)));
        return juce::jlimit(-48.0f, 48.0f, juce::Decibels::gainToDecibels(mag));
    }

private:
    struct BiquadCoeffs
    {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
        float a1 = 0.0f, a2 = 0.0f;
    };

    struct BiquadState
    {
        float z1 = 0.0f, z2 = 0.0f;
        BiquadCoeffs c;

        void reset() noexcept { z1 = 0.0f; z2 = 0.0f; }

        float processSample(float x) noexcept
        {
            const float y = c.b0 * x + z1;
            z1 = c.b1 * x - c.a1 * y + z2;
            z2 = c.b2 * x - c.a2 * y;
            return y;
        }
    };

    static int getActiveStageCount(const BandParams& p) noexcept
    {
        if (p.shape == FilterShape::LowCut || p.shape == FilterShape::HighCut)
        {
            if (p.slopeDbOct >= 42.0f) return 4;
            if (p.slopeDbOct >= 30.0f) return 3;
            if (p.slopeDbOct >= 18.0f) return 2;
            return 1;
        }
        return 1;
    }

    void computeCoeffsInternal(const BandParams& p, float dynamicGainOffsetDb, float gainScale, BiquadCoeffs out[4]) const noexcept
    {
        const double f0 = juce::jlimit(15.0, sampleRate * 0.48, static_cast<double>(p.freqHz));
        const double qVal = juce::jlimit(0.1, 24.0, static_cast<double>(p.q));
        const double baseGain = supportsGain(p.shape) ? static_cast<double>(p.gainDb) : 0.0;
        const double totalGainDb = juce::jlimit(-36.0, 36.0, (baseGain + static_cast<double>(dynamicGainOffsetDb)) * static_cast<double>(gainScale));

        const double w0 = 2.0 * juce::MathConstants<double>::pi * f0 / sampleRate;
        const double cosW0 = std::cos(w0);
        const double sinW0 = std::sin(w0);
        const double alpha = sinW0 / (2.0 * qVal);
        const double A = std::pow(10.0, totalGainDb / 40.0);

        const int stages = getActiveStageCount(p);

        for (int st = 0; st < 4; ++st)
        {
            double b0 = 1.0, b1 = 0.0, b2 = 0.0;
            double a0 = 1.0, a1 = 0.0, a2 = 0.0;

            if (st < stages)
            {
                switch (p.shape)
                {
                    case FilterShape::Bell:
                    {
                        b0 = 1.0 + alpha * A;
                        b1 = -2.0 * cosW0;
                        b2 = 1.0 - alpha * A;
                        a0 = 1.0 + alpha / A;
                        a1 = -2.0 * cosW0;
                        a2 = 1.0 - alpha / A;
                        break;
                    }
                    case FilterShape::LowShelf:
                    {
                        const double sqrtA2Alpha = 2.0 * std::sqrt(A) * alpha;
                        b0 = A * ((A + 1.0) - (A - 1.0) * cosW0 + sqrtA2Alpha);
                        b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cosW0);
                        b2 = A * ((A + 1.0) - (A - 1.0) * cosW0 - sqrtA2Alpha);
                        a0 = (A + 1.0) + (A - 1.0) * cosW0 + sqrtA2Alpha;
                        a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cosW0);
                        a2 = (A + 1.0) + (A - 1.0) * cosW0 - sqrtA2Alpha;
                        break;
                    }
                    case FilterShape::HighShelf:
                    {
                        const double sqrtA2Alpha = 2.0 * std::sqrt(A) * alpha;
                        b0 = A * ((A + 1.0) + (A - 1.0) * cosW0 + sqrtA2Alpha);
                        b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cosW0);
                        b2 = A * ((A + 1.0) + (A - 1.0) * cosW0 - sqrtA2Alpha);
                        a0 = (A + 1.0) - (A - 1.0) * cosW0 + sqrtA2Alpha;
                        a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cosW0);
                        a2 = (A + 1.0) - (A - 1.0) * cosW0 - sqrtA2Alpha;
                        break;
                    }
                    case FilterShape::LowCut:
                    {
                        const double stageQ = (stages == 1) ? qVal : (0.5411961 + 0.15 * st);
                        const double stageAlpha = sinW0 / (2.0 * stageQ);
                        b0 = (1.0 + cosW0) * 0.5;
                        b1 = -(1.0 + cosW0);
                        b2 = (1.0 + cosW0) * 0.5;
                        a0 = 1.0 + stageAlpha;
                        a1 = -2.0 * cosW0;
                        a2 = 1.0 - stageAlpha;
                        break;
                    }
                    case FilterShape::HighCut:
                    {
                        const double stageQ = (stages == 1) ? qVal : (0.5411961 + 0.15 * st);
                        const double stageAlpha = sinW0 / (2.0 * stageQ);
                        b0 = (1.0 - cosW0) * 0.5;
                        b1 = 1.0 - cosW0;
                        b2 = (1.0 - cosW0) * 0.5;
                        a0 = 1.0 + stageAlpha;
                        a1 = -2.0 * cosW0;
                        a2 = 1.0 - stageAlpha;
                        break;
                    }
                    case FilterShape::Notch:
                    {
                        b0 = 1.0;
                        b1 = -2.0 * cosW0;
                        b2 = 1.0;
                        a0 = 1.0 + alpha;
                        a1 = -2.0 * cosW0;
                        a2 = 1.0 - alpha;
                        break;
                    }
                    case FilterShape::BandPass:
                    {
                        b0 = alpha;
                        b1 = 0.0;
                        b2 = -alpha;
                        a0 = 1.0 + alpha;
                        a1 = -2.0 * cosW0;
                        a2 = 1.0 - alpha;
                        break;
                    }
                    case FilterShape::TiltShelf:
                    {
                        const double halfA = std::pow(10.0, (totalGainDb * 0.5) / 40.0);
                        const double sqrtA2Alpha = 2.0 * std::sqrt(halfA) * alpha;
                        b0 = halfA * ((halfA + 1.0) + (halfA - 1.0) * cosW0 + sqrtA2Alpha);
                        b1 = -2.0 * halfA * ((halfA - 1.0) + (halfA + 1.0) * cosW0);
                        b2 = halfA * ((halfA + 1.0) + (halfA - 1.0) * cosW0 - sqrtA2Alpha);
                        a0 = (halfA + 1.0) - (halfA - 1.0) * cosW0 + sqrtA2Alpha;
                        a1 = 2.0 * ((halfA - 1.0) - (halfA + 1.0) * cosW0);
                        a2 = (halfA + 1.0) - (halfA - 1.0) * cosW0 - sqrtA2Alpha;
                        break;
                    }
                }
            }

            out[st].b0 = static_cast<float>(b0 / a0);
            out[st].b1 = static_cast<float>(b1 / a0);
            out[st].b2 = static_cast<float>(b2 / a0);
            out[st].a1 = static_cast<float>(a1 / a0);
            out[st].a2 = static_cast<float>(a2 / a0);
        }
    }

    void updateCoefficients(const BandParams& p, float dynamicGainOffsetDb, float gainScale)
    {
        BiquadCoeffs normCoeffs[4];
        computeCoeffsInternal(p, dynamicGainOffsetDb, gainScale, normCoeffs);
        for (int st = 0; st < 4; ++st)
        {
            coeffs[static_cast<size_t>(st)] = normCoeffs[st];
            stagesL[static_cast<size_t>(st)].c = normCoeffs[st];
            stagesR[static_cast<size_t>(st)].c = normCoeffs[st];
        }
    }

    double sampleRate = 48000.0;
    BandParams currentParams;
    std::array<BiquadCoeffs, 4> coeffs {};
    std::array<BiquadState, 4> stagesL {};
    std::array<BiquadState, 4> stagesR {};
    float envLevel = 0.0f;
    float autoThreshEnvDb = -24.0f;
    float currentDynamicGainDb = 0.0f;
};

} // namespace dspcore
