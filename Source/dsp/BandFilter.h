#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>

namespace dspcore
{

enum class BandType
{
    Bell = 0,
    LowShelf,
    HighShelf,
    LowCut,
    HighCut,
    Notch
};

enum class ChannelRoute
{
    Stereo = 0,
    Left,
    Right,
    Mid,
    Side
};

struct BandConfig
{
    bool enabled = false;
    bool solo = false;
    bool dynamicEnabled = false;

    BandType type = BandType::Bell;
    ChannelRoute route = ChannelRoute::Stereo;

    float freqHz = 1000.0f;
    float gainDb = 0.0f;
    float q = 1.0f;

    float thresholdDb = -18.0f;
    float attackMs = 15.0f;
    float releaseMs = 120.0f;
    float dynamicRangeDb = -6.0f;
};

class BandFilter
{
public:
    void prepare(double newSampleRate, int /*maxBlockSize*/)
    {
        sampleRate = juce::jmax(8000.0, newSampleRate);
        reset();
    }

    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            s1[ch] = 0.0;
            s2[ch] = 0.0;
            cutS1[ch] = 0.0;
            cutS2[ch] = 0.0;
        }
        envDb = -90.0f;
        currentDynamicGainDb = 0.0f;
    }

    float getMagnitudeDb(float fHz) const noexcept
    {
        if (!config.enabled)
            return 0.0f;

        const float effectiveGainDb = config.gainDb + currentDynamicGainDb;
        const float w = juce::MathConstants<float>::twoPi * fHz / static_cast<float>(sampleRate);
        const float wc = juce::MathConstants<float>::twoPi * juce::jlimit(10.0f, 0.49f * static_cast<float>(sampleRate), config.freqHz) / static_cast<float>(sampleRate);
        const float ratio = w / juce::jmax(1.0e-5f, wc);
        const float qSafe = juce::jmax(0.1f, config.q);

        switch (config.type)
        {
            case BandType::Bell:
            {
                const float A = std::pow(10.0f, effectiveGainDb / 40.0f);
                const float num = std::sqrt(std::pow(ratio * ratio - 1.0f, 2.0f) + std::pow(ratio / (qSafe * A), 2.0f));
                const float den = std::sqrt(std::pow(ratio * ratio - 1.0f, 2.0f) + std::pow(ratio * A / qSafe, 2.0f));
                const float mag = (A * A) * (num / juce::jmax(1.0e-6f, den));
                return juce::Decibels::gainToDecibels(juce::jmax(1.0e-6f, mag));
            }
            case BandType::LowShelf:
            {
                const float A = std::pow(10.0f, effectiveGainDb / 40.0f);
                const float p = std::pow(ratio, 2.0f * qSafe);
                const float mag = (A * A) * ((1.0f + (1.0f / (A * A)) * p) / (1.0f + p));
                return juce::Decibels::gainToDecibels(juce::jmax(1.0e-6f, mag));
            }
            case BandType::HighShelf:
            {
                const float A = std::pow(10.0f, effectiveGainDb / 40.0f);
                const float p = std::pow(ratio, 2.0f * qSafe);
                const float mag = (A * A) * ((p + (1.0f / (A * A))) / (p + 1.0f));
                return juce::Decibels::gainToDecibels(juce::jmax(1.0e-6f, mag));
            }
            case BandType::Notch:
            {
                const float num = std::abs(1.0f - ratio * ratio);
                const float den = std::sqrt(std::pow(1.0f - ratio * ratio, 2.0f) + std::pow(ratio / qSafe, 2.0f));
                return juce::Decibels::gainToDecibels(juce::jmax(1.0e-6f, num / juce::jmax(1.0e-6f, den)));
            }
            case BandType::LowCut:
            {
                const float p = std::pow(ratio, 4.0f);
                return juce::Decibels::gainToDecibels(juce::jmax(1.0e-6f, p / (1.0f + p)));
            }
            case BandType::HighCut:
            {
                const float p = std::pow(ratio, 4.0f);
                return juce::Decibels::gainToDecibels(juce::jmax(1.0e-6f, 1.0f / (1.0f + p)));
            }
        }
        return 0.0f;
    }

    float getCurrentDynamicGainDb() const noexcept { return currentDynamicGainDb; }
    const BandConfig& getConfig() const noexcept { return config; }
    void setConfig(const BandConfig& newCfg) noexcept { config = newCfg; }

    void processSample(float& left, float& right) noexcept
    {
        if (!config.enabled)
        {
            currentDynamicGainDb = 0.0f;
            return;
        }

        float c0 = left;
        float c1 = right;

        if (config.route == ChannelRoute::Mid || config.route == ChannelRoute::Side)
        {
            const float mid = 0.5f * (left + right);
            const float side = 0.5f * (left - right);
            c0 = mid;
            c1 = side;
        }

        float dynOffsetDb = 0.0f;
        if (config.dynamicEnabled)
        {
            const float detSample = (config.route == ChannelRoute::Side) ? c1
                                  : (config.route == ChannelRoute::Mid)  ? c0
                                  : (config.route == ChannelRoute::Left) ? left
                                  : (config.route == ChannelRoute::Right)? right
                                  : 0.5f * (left + right);

            const float levelDb = juce::Decibels::gainToDecibels(std::abs(detSample) + 1.0e-7f);
            const float coeff = (levelDb > envDb)
                ? 1.0f - std::exp(-1.0f / (0.001f * juce::jmax(0.5f, config.attackMs) * static_cast<float>(sampleRate)))
                : 1.0f - std::exp(-1.0f / (0.001f * juce::jmax(5.0f, config.releaseMs) * static_cast<float>(sampleRate)));

            envDb += coeff * (levelDb - envDb);

            const float overDb = juce::jmax(0.0f, envDb - config.thresholdDb);
            const float ratioFactor = juce::jlimit(0.0f, 1.0f, overDb / 18.0f);
            dynOffsetDb = ratioFactor * config.dynamicRangeDb;
        }

        currentDynamicGainDb = dynOffsetDb;
        const float totalGainDb = config.gainDb + dynOffsetDb;

        const float fc = juce::jlimit(15.0f, 0.45f * static_cast<float>(sampleRate), config.freqHz);
        const double g = std::tan(juce::MathConstants<double>::pi * static_cast<double>(fc) / sampleRate);
        const double k = 1.0 / juce::jmax(0.15, static_cast<double>(config.q));

        auto applyToChannel = [&](int ch, float& x) noexcept
        {
            const bool activeChannel =
                (config.route == ChannelRoute::Stereo) ||
                (config.route == ChannelRoute::Left && ch == 0) ||
                (config.route == ChannelRoute::Right && ch == 1) ||
                (config.route == ChannelRoute::Mid && ch == 0) ||
                (config.route == ChannelRoute::Side && ch == 1);

            if (!activeChannel)
                return;

            if (config.type == BandType::LowCut || config.type == BandType::HighCut)
            {
                x = processSvfCut(ch, x, g, k, config.type == BandType::HighCut);
                return;
            }

            const double A = std::pow(10.0, static_cast<double>(totalGainDb) / 40.0);
            const double hp = (static_cast<double>(x) - (k + g) * s1[ch] - s2[ch]) / (1.0 + g * (k + g));
            const double bp = g * hp + s1[ch];
            const double lp = g * bp + s2[ch];
            s1[ch] = bp + g * hp;
            s2[ch] = lp + g * bp;

            double out = static_cast<double>(x);
            switch (config.type)
            {
                case BandType::Bell:
                    out = static_cast<double>(x) + (A * A - 1.0) * k * bp;
                    break;
                case BandType::LowShelf:
                    out = static_cast<double>(x) + (A * A - 1.0) * lp;
                    break;
                case BandType::HighShelf:
                    out = static_cast<double>(x) + (A * A - 1.0) * hp;
                    break;
                case BandType::Notch:
                    out = lp + hp;
                    break;
                default:
                    break;
            }
            x = static_cast<float>(out);
        };

        if (config.route == ChannelRoute::Mid || config.route == ChannelRoute::Side)
        {
            applyToChannel(0, c0);
            applyToChannel(1, c1);
            left = c0 + c1;
            right = c0 - c1;
        }
        else
        {
            applyToChannel(0, left);
            applyToChannel(1, right);
        }
    }

private:
    float processSvfCut(int ch, float x, double g, double k, bool isHighCut) noexcept
    {
        auto stepOnce = [&](double in, double& st1, double& st2) noexcept -> double
        {
            const double hp = (in - (k + g) * st1 - st2) / (1.0 + g * (k + g));
            const double bp = g * hp + st1;
            const double lp = g * bp + st2;
            st1 = bp + g * hp;
            st2 = lp + g * bp;
            return isHighCut ? lp : hp;
        };

        const double stage1 = stepOnce(static_cast<double>(x), s1[ch], s2[ch]);
        const double stage2 = stepOnce(stage1, cutS1[ch], cutS2[ch]);
        return static_cast<float>(stage2);
    }

    double sampleRate = 48000.0;
    BandConfig config;

    double s1[2] { 0.0, 0.0 };
    double s2[2] { 0.0, 0.0 };
    double cutS1[2] { 0.0, 0.0 };
    double cutS2[2] { 0.0, 0.0 };

    float envDb = -90.0f;
    float currentDynamicGainDb = 0.0f;
};

} // namespace dspcore
