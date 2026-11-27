
#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstring>
#include <vector>

namespace
{
constexpr double kPi = 3.14159265358979323846;

juce::String bandParamId(int index, const juce::String& suffix)
{
    return "band_" + juce::String(index) + "_" + suffix;
}

float dbToGain(float db) noexcept
{
    return std::pow(10.0f, db * 0.05f);
}

float gainToDb(float gain) noexcept
{
    return 20.0f * std::log10(juce::jmax(1.0e-7f, gain));
}

juce::dsp::IIR::Coefficients<float>::Ptr makeCoefficientsForShape(
    dspcore::FilterShape shape,
    double sampleRate,
    float freqHz,
    float gainDb,
    float qVal,
    float /*slopeDb*/)
{
    const double sr = juce::jmax(8000.0, sampleRate);
    const double nyquist = sr * 0.495;
    const double clampedFreq = juce::jlimit(
        20.0, nyquist, static_cast<double>(freqHz));
    const double clampedQ = juce::jlimit(
        0.1, 24.0, static_cast<double>(qVal));

    const float linearGain = dbToGain(
        juce::jlimit(-30.0f, 30.0f, gainDb));

    switch (shape)
    {
        case dspcore::FilterShape::Bell:
            return juce::dsp::IIR::Coefficients<float>::makePeakFilter(
                sr, clampedFreq, clampedQ, linearGain);

        case dspcore::FilterShape::LowShelf:
            return juce::dsp::IIR::Coefficients<float>::makeLowShelf(
                sr, clampedFreq, clampedQ, linearGain);

        case dspcore::FilterShape::HighShelf:
            return juce::dsp::IIR::Coefficients<float>::makeHighShelf(
                sr, clampedFreq, clampedQ, linearGain);

        case dspcore::FilterShape::LowCut:
            return juce::dsp::IIR::Coefficients<float>::makeHighPass(
                sr, clampedFreq, clampedQ);

        case dspcore::FilterShape::HighCut:
            return juce::dsp::IIR::Coefficients<float>::makeLowPass(
                sr, clampedFreq, clampedQ);

        case dspcore::FilterShape::Notch:
            return juce::dsp::IIR::Coefficients<float>::makeNotch(
                sr, clampedFreq, clampedQ);

        case dspcore::FilterShape::BandPass:
            return juce::dsp::IIR::Coefficients<float>::makeBandPass(
                sr, clampedFreq, clampedQ);

        case dspcore::FilterShape::TiltShelf:
            return juce::dsp::IIR::Coefficients<float>::makeHighShelf(
                sr, clampedFreq,
                juce::jmax(0.35, clampedQ * 0.7),
                linearGain);
    }

    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sr, clampedFreq, clampedQ, 1.0f);
}

float evalBiquadMagnitudeDb(
    const juce::dsp::IIR::Coefficients<float>::Ptr& coeffs,
    double sampleRate,
    double freqHz) noexcept
{
    if (coeffs == nullptr || coeffs->coefficients.size() < 5)
        return 0.0f;

    const auto& c = coeffs->coefficients;

    const double sr = juce::jmax(8000.0, sampleRate);
    const double w = 2.0 * kPi
        * juce::jlimit(10.0, sr * 0.499, freqHz) / sr;

    const std::complex<double> z1 =
        std::exp(std::complex<double>(0.0, -w));

    const std::complex<double> z2 =
        std::exp(std::complex<double>(0.0, -2.0 * w));

    const std::complex<double> num =
        static_cast<double>(c[0])
        + static_cast<double>(c[1]) * z1
        + static_cast<double>(c[2]) * z2;

    const std::complex<double> den =
        1.0
        + static_cast<double>(c[3]) * z1
        + static_cast<double>(c[4]) * z2;

    const double mag = std::abs(num)
        / juce::jmax(1.0e-12, std::abs(den));

    return static_cast<float>(
        20.0 * std::log10(juce::jmax(1.0e-7, mag)));
}
} // namespace

HybridResonanceEQProcessor::HybridResonanceEQProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    fftWorkspace.assign(
        static_cast<size_t>(fftSize * 2), 0.0f);

    preSpectrumSmooth.fill(-100.0f);
    postSpectrumSmooth.fill(-100.0f);
    reductionSmooth.fill(0.0f);
}

HybridResonanceEQProcessor::~HybridResonanceEQProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout
HybridResonanceEQProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "ana_resolution", "Analyzer Resolution",
        juce::StringArray { "Low", "Medium", "High", "Maximum" }, 2));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "spec_resolution", "Spectral Processing Resolution",
        juce::StringArray { "Low Latency 2048", "High Quality 4096" }, 1));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "ana_speed", "Analyzer Speed",
        juce::StringArray { "Slow", "Medium", "Fast" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "ana_tilt", "Analyzer Tilt",
        juce::StringArray { "0 dB", "3 dB", "4.5 dB", "6 dB" }, 2));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "display_range", "Display Range",
        juce::StringArray { "+/- 6 dB", "+/- 12 dB", "+/- 30 dB" }, 1));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "character_mode", "Global Character",
        juce::StringArray { "Clean", "Subtle", "Warm" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "ana_show_pre", "Show Pre Spectrum", true));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "ana_show_post", "Show Post Spectrum", true));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "out_auto_gain", "Output Auto Gain", false));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "phase_invert", "Phase Invert", false));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "global_bypass", "Global Bypass", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "gain_scale", "Gain Scale",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "output_gain", "Output Gain",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "spec_delta", "Spectral Delta", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "spec_depth", "Spectral Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.65f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "spec_sharpness", "Spectral Sharpness",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.60f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "spec_smoothing", "Spectral Smoothing",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.50f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "spec_mix", "Spectral Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));

    for (int i = 0; i < dspcore::ParametricEQEngine::maxBands; ++i)
    {
        const juce::String prefix =
            "band_" + juce::String(i) + "_";

        const juce::String labelPrefix =
            "Band " + juce::String(i + 1) + " ";

        const bool defaultEnabled = (i < 3);

        const float defaultFreq =
            (i == 0) ? 120.0f
            : ((i == 1) ? 850.0f
            : ((i == 2) ? 4200.0f : 1000.0f));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            prefix + "enabled", labelPrefix + "Enabled",
            defaultEnabled));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            prefix + "shape", labelPrefix + "Shape",
            juce::StringArray {
                "Bell", "Low Shelf", "High Shelf", "Low Cut",
                "High Cut", "Notch", "Band Pass", "Tilt Shelf"
            }, 0));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            prefix + "route", labelPrefix + "Routing",
            juce::StringArray {
                "Stereo", "Mid", "Side", "Left", "Right"
            }, 0));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            prefix + "character", labelPrefix + "Character",
            juce::StringArray { "Clean", "Subtle", "Warm" }, 0));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            prefix + "dyn_enabled", labelPrefix + "Dynamic Enabled",
            false));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            prefix + "dyn_auto", labelPrefix + "Dynamic Auto Thresh",
            true));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            prefix + "spec_enabled", labelPrefix + "Spectral Enabled",
            false));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            prefix + "solo", labelPrefix + "Solo", false));

        juce::NormalisableRange<float> freqRange(
            20.0f, 20000.0f, 0.1f);

        freqRange.setSkewForCentre(1000.0f);

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            prefix + "freq", labelPrefix + "Freq",
            freqRange, defaultFreq));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            prefix + "gain", labelPrefix + "Gain",
            juce::NormalisableRange<float>(
                -24.0f, 24.0f, 0.05f), 0.0f));

        juce::NormalisableRange<float> qRange(
            0.1f, 18.0f, 0.01f);

        qRange.setSkewForCentre(1.0f);

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            prefix + "q", labelPrefix + "Q", qRange, 1.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            prefix + "slope", labelPrefix + "Slope",
            juce::NormalisableRange<float>(
                6.0f, 48.0f, 6.0f), 12.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            prefix + "dyn_range", labelPrefix + "Dynamic Range",
            juce::NormalisableRange<float>(
                -24.0f, 24.0f, 0.1f), -6.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            prefix + "dyn_thresh", labelPrefix + "Dynamic Threshold",
            juce::NormalisableRange<float>(
                -60.0f, 0.0f, 0.1f), -24.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            prefix + "dyn_attack", labelPrefix + "Dynamic Attack",
            juce::NormalisableRange<float>(
                0.5f, 200.0f, 0.1f), 15.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            prefix + "dyn_release", labelPrefix + "Dynamic Release",
            juce::NormalisableRange<float>(
                10.0f, 1000.0f, 0.5f), 120.0f));
    }

    return { params.begin(), params.end() };
}

void HybridResonanceEQProcessor::resetDSPState()
{
    for (auto& st : bandStates)
    {
        st.envLevelDb = -120.0f;
        st.currentDynDb = 0.0f;

        st.filterL.reset();
        st.filterR.reset();

        st.detectorL.reset();
        st.detectorR.reset();
    }

    soloMonitorFilter.reset();
    spectralEngine.reset();

    dryDelayLineL.fill(0.0f);
    dryDelayLineR.fill(0.0f);
    dryDelayWritePosition = 0;

    preSpectrumSmooth.fill(-100.0f);
    postSpectrumSmooth.fill(-100.0f);
    reductionSmooth.fill(0.0f);

    for (int b = 0; b < numBins; ++b)
    {
        publishedPreSpectrum[static_cast<size_t>(b)].store(
            -100.0f, std::memory_order_relaxed);

        publishedPostSpectrum[static_cast<size_t>(b)].store(
            -100.0f, std::memory_order_relaxed);

        publishedReduction[static_cast<size_t>(b)].store(
            0.0f, std::memory_order_relaxed);
    }

    const bool bypassed =
        apvts.getRawParameterValue("global_bypass")->load() > 0.5f;

    bypassMixSmooth.setCurrentAndTargetValue(
        bypassed ? 0.0f : 1.0f);

    outputGainSmooth.setCurrentAndTargetValue(
        dbToGain(
            apvts.getRawParameterValue("output_gain")->load()));

    autoGainCompDb.store(0.0f);
}

void HybridResonanceEQProcessor::prepareToPlay(
    double sampleRate,
    int samplesPerBlock)
{
    currentSampleRate = juce::jmax(8000.0, sampleRate);
    currentBlockSize = juce::jmax(1, samplesPerBlock);

    juce::dsp::ProcessSpec spec {
        currentSampleRate,
        static_cast<juce::uint32>(currentBlockSize),
        1
    };

    for (auto& st : bandStates)
    {
        st.filterL.prepare(spec);
        st.filterR.prepare(spec);

        st.detectorL.prepare(spec);
        st.detectorR.prepare(spec);
    }

    soloMonitorFilter.prepare(currentSampleRate);

    spectralEngine.prepare(
        currentSampleRate, currentBlockSize);

    dryScratch.setSize(
        2, currentBlockSize, false, false, true);

    bandScratch.setSize(
        2, currentBlockSize, false, false, true);

    alignedDryScratch.setSize(
        2, currentBlockSize, false, false, true);

    setLatencySamples(0);
    lastReportedLatency = 0;

    bypassMixSmooth.reset(currentSampleRate, 0.012);
    outputGainSmooth.reset(currentSampleRate, 0.015);

    resetDSPState();
    updateBandFilters(currentSampleRate);
}

void HybridResonanceEQProcessor::releaseResources()
{
}

bool HybridResonanceEQProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const
{
    if (layouts.getMainInputChannelSet()
            != juce::AudioChannelSet::stereo()
        || layouts.getMainOutputChannelSet()
            != juce::AudioChannelSet::stereo())
    {
        return false;
    }

    return true;
}

float HybridResonanceEQProcessor::applyBandCharacterSample(
    float sample,
    int characterMode) const noexcept
{
    if (characterMode == 1)
    {
        const float drive = 1.25f;
        return std::tanh(sample * drive) / std::tanh(drive);
    }

    if (characterMode == 2)
    {
        const float drive = 2.1f;
        const float asym = sample + 0.06f * sample * sample;

        return std::tanh(asym * drive)
            / std::tanh(drive * 1.04f);
    }

    return sample;
}

void HybridResonanceEQProcessor::updateBandFilters(
    double sampleRate)
{
    const float gainScale =
        apvts.getRawParameterValue("gain_scale")->load();

    for (int i = 0;
         i < dspcore::ParametricEQEngine::maxBands;
         ++i)
    {
        const auto prefix = bandParamId(i, "");

        const auto shape =
            static_cast<dspcore::FilterShape>(
                static_cast<int>(
                    apvts.getRawParameterValue(
                        prefix + "shape")->load()));

        const float freq =
            apvts.getRawParameterValue(prefix + "freq")->load();

        const float baseGain =
            apvts.getRawParameterValue(prefix + "gain")->load();

        const float qVal =
            apvts.getRawParameterValue(prefix + "q")->load();

        const float slope =
            apvts.getRawParameterValue(prefix + "slope")->load();

        const float effectiveGain =
            dspcore::BandFilter::supportsGain(shape)
                ? (baseGain * gainScale
                   + bandStates[static_cast<size_t>(i)].currentDynDb)
                : bandStates[static_cast<size_t>(i)].currentDynDb;

        auto coeffs = makeCoefficientsForShape(
            shape, sampleRate, freq, effectiveGain, qVal, slope);

        auto detectorCoeffs =
            juce::dsp::IIR::Coefficients<float>::makeBandPass(
                juce::jmax(8000.0, sampleRate),
                juce::jlimit(
                    20.0,
                    sampleRate * 0.48,
                    static_cast<double>(freq)),
                juce::jlimit(
                    0.2, 18.0, static_cast<double>(qVal)));

        auto& state = bandStates[static_cast<size_t>(i)];

        if (coeffs != nullptr)
        {
            *state.filterL.coefficients = *coeffs;
            *state.filterR.coefficients = *coeffs;
        }

        if (detectorCoeffs != nullptr)
        {
            *state.detectorL.coefficients = *detectorCoeffs;
            *state.detectorR.coefficients = *detectorCoeffs;
        }
    }
}

void HybridResonanceEQProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    if (numChannels <= 0 || numSamples <= 0)
        return;

    // All scratch buffers are allocated in prepareToPlay().
    // Never resize them from the audio thread.
    if (numChannels != 2
        || numSamples > dryScratch.getNumSamples()
        || numSamples > bandScratch.getNumSamples()
        || numSamples > alignedDryScratch.getNumSamples())
    {
        return;
    }

    // Preserve the original input for detection and analysis.
    dryScratch.copyFrom(
        0, 0, buffer, 0, 0, numSamples);

    dryScratch.copyFrom(
        1, 0, buffer, 1, 0, numSamples);

    const bool bypassed =
        apvts.getRawParameterValue("global_bypass")->load() > 0.5f;

    bypassMixSmooth.setTargetValue(
        bypassed ? 0.0f : 1.0f);

    const float outGainDb =
        apvts.getRawParameterValue("output_gain")->load();

    outputGainSmooth.setTargetValue(
        dbToGain(outGainDb));

    const bool phaseInvert =
        apvts.getRawParameterValue("phase_invert")->load() > 0.5f;

    const float phaseSign = phaseInvert ? -1.0f : 1.0f;

    bool anySoloActive = false;
    bool anySpecActive = false;

    for (int i = 0;
         i < dspcore::ParametricEQEngine::maxBands;
         ++i)
    {
        const auto prefix = bandParamId(i, "");

        const bool enabled =
            apvts.getRawParameterValue(
                prefix + "enabled")->load() > 0.5f;

        if (!enabled)
            continue;

        if (apvts.getRawParameterValue(
                prefix + "solo")->load() > 0.5f)
        {
            anySoloActive = true;
        }

        if (apvts.getRawParameterValue(
                prefix + "spec_enabled")->load() > 0.5f)
        {
            anySpecActive = true;
        }
    }

    const float gainScale =
        apvts.getRawParameterValue("gain_scale")->load();

    updateBandFilters(currentSampleRate);

    for (int i = 0;
         i < dspcore::ParametricEQEngine::maxBands;
         ++i)
    {
        auto& st = bandStates[static_cast<size_t>(i)];
        const auto prefix = bandParamId(i, "");

        const bool enabled =
            apvts.getRawParameterValue(
                prefix + "enabled")->load() > 0.5f;

        const bool dynOn =
            apvts.getRawParameterValue(
                prefix + "dyn_enabled")->load() > 0.5f;

        if (!enabled || !dynOn)
        {
            st.currentDynDb = 0.0f;
            st.envLevelDb = -120.0f;

            st.detectorL.reset();
            st.detectorR.reset();

            continue;
        }

        const bool autoThresh =
            apvts.getRawParameterValue(
                prefix + "dyn_auto")->load() > 0.5f;

        const float dynRange =
            apvts.getRawParameterValue(
                prefix + "dyn_range")->load();

        const float threshDb =
            autoThresh
                ? -22.0f
                : apvts.getRawParameterValue(
                    prefix + "dyn_thresh")->load();

        const float attackMs = juce::jmax(
            0.5f,
            apvts.getRawParameterValue(
                prefix + "dyn_attack")->load());

        const float releaseMs = juce::jmax(
            10.0f,
            apvts.getRawParameterValue(
                prefix + "dyn_release")->load());

        float sumSq = 0.0f;

        for (int n = 0; n < numSamples; ++n)
        {
            const float detL =
                st.detectorL.processSample(
                    buffer.getReadPointer(0)[n]);

            const float detR =
                st.detectorR.processSample(
                    buffer.getReadPointer(1)[n]);

            sumSq += 0.5f * (detL * detL + detR * detR);
        }

        const float rms = std::sqrt(
            sumSq / static_cast<float>(
                juce::jmax(1, numSamples)));

        const float inputDb = gainToDb(rms);

        const float coeffMs =
            (inputDb > st.envLevelDb)
                ? attackMs
                : releaseMs;

        const float alpha =
            1.0f - std::exp(
                -static_cast<float>(numSamples)
                / static_cast<float>(
                    currentSampleRate * (coeffMs * 0.001)));

        st.envLevelDb +=
            (inputDb - st.envLevelDb) * alpha;

        const float overDb =
            juce::jmax(0.0f, st.envLevelDb - threshDb);

        const float ratio =
            juce::jlimit(0.0f, 1.0f, overDb / 18.0f);

        st.currentDynDb = dynRange * ratio;
    }

    updateBandFilters(currentSampleRate);

    if (anySoloActive)
    {
        int soloIndex = -1;

        for (int i = 0;
             i < dspcore::ParametricEQEngine::maxBands;
             ++i)
        {
            const auto prefix = bandParamId(i, "");

            if (apvts.getRawParameterValue(
                    prefix + "enabled")->load() > 0.5f
                && apvts.getRawParameterValue(
                    prefix + "solo")->load() > 0.5f)
            {
                soloIndex = i;
                break;
            }
        }

        if (soloIndex >= 0)
        {
            const auto prefix = bandParamId(soloIndex, "");

            dspcore::BandParams monitorParams;

            monitorParams.enabled = true;
            monitorParams.shape = dspcore::FilterShape::BandPass;

            monitorParams.freqHz =
                apvts.getRawParameterValue(
                    prefix + "freq")->load();

            monitorParams.q = juce::jlimit(
                0.35f,
                8.0f,
                apvts.getRawParameterValue(
                    prefix + "q")->load());

            monitorParams.gainDb = 0.0f;
            monitorParams.dynamicEnabled = false;
            monitorParams.route = dspcore::ChannelRoute::Stereo;

            soloMonitorFilter.setParameters(monitorParams);

            buffer.copyFrom(
                0, 0, dryScratch, 0, 0, numSamples);

            buffer.copyFrom(
                1, 0, dryScratch, 1, 0, numSamples);

            soloMonitorFilter.processBlock(
                buffer.getWritePointer(0),
                buffer.getWritePointer(1),
                numSamples);
        }
    }
    else
    {
        for (int i = 0;
             i < dspcore::ParametricEQEngine::maxBands;
             ++i)
        {
            auto& st = bandStates[static_cast<size_t>(i)];
            const auto prefix = bandParamId(i, "");

            if (apvts.getRawParameterValue(
                    prefix + "enabled")->load() <= 0.5f)
            {
                continue;
            }

            const int charMode =
                static_cast<int>(
                    apvts.getRawParameterValue(
                        prefix + "character")->load());

            bandScratch.copyFrom(
                0, 0, buffer, 0, 0, numSamples);

            bandScratch.copyFrom(
                1, 0, buffer, 1, 0, numSamples);

            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto* ptr = bandScratch.getWritePointer(ch);

                for (int n = 0; n < numSamples; ++n)
                {
                    float sample = ptr[n];

                    sample = (ch == 0)
                        ? st.filterL.processSample(sample)
                        : st.filterR.processSample(sample);

                    ptr[n] = applyBandCharacterSample(
                        sample, charMode);
                }
            }

            buffer.copyFrom(
                0, 0, bandScratch, 0, 0, numSamples);

            buffer.copyFrom(
                1, 0, bandScratch, 1, 0, numSamples);
        }
    }

    dspcore::SpectralConfig spectralConfig;

    // Keep spectral processing running during global bypass.
    // This prevents its latency from disappearing when bypass is engaged.
    spectralConfig.enabled = anySpecActive && !anySoloActive;

    spectralConfig.deltaListen =
        apvts.getRawParameterValue("spec_delta")->load() > 0.5f;

    spectralConfig.depth =
        apvts.getRawParameterValue("spec_depth")->load();

    spectralConfig.sharpness =
        apvts.getRawParameterValue("spec_sharpness")->load();

    spectralConfig.smoothing =
        apvts.getRawParameterValue("spec_smoothing")->load();

    spectralConfig.mix =
        apvts.getRawParameterValue("spec_mix")->load();

    spectralConfig.resolution =
        static_cast<dspcore::SpectralResolution>(
            juce::jlimit(
                0,
                1,
                static_cast<int>(
                    apvts.getRawParameterValue(
                        "spec_resolution")->load())));

    for (int i = 0;
         i < dspcore::ParametricEQEngine::maxBands;
         ++i)
    {
        const auto prefix = bandParamId(i, "");

        auto& spectralBand =
            spectralConfig.bandWindows[static_cast<size_t>(i)];

        spectralBand.active =
            apvts.getRawParameterValue(
                prefix + "enabled")->load() > 0.5f
            && apvts.getRawParameterValue(
                prefix + "spec_enabled")->load() > 0.5f;

        spectralBand.centerFreqHz =
            apvts.getRawParameterValue(
                prefix + "freq")->load();

        spectralBand.q =
            apvts.getRawParameterValue(
                prefix + "q")->load();

        spectralBand.rangeDb =
            -std::abs(
                apvts.getRawParameterValue(
                    prefix + "dyn_range")->load());

        spectralBand.thresholdDb =
            apvts.getRawParameterValue(
                prefix + "dyn_thresh")->load();

        spectralBand.autoThreshold =
            apvts.getRawParameterValue(
                prefix + "dyn_auto")->load() > 0.5f;

        if (spectralBand.active)
        {
            spectralConfig.attackMs =
                apvts.getRawParameterValue(
                    prefix + "dyn_attack")->load();

            spectralConfig.releaseMs =
                apvts.getRawParameterValue(
                    prefix + "dyn_release")->load();
        }
    }

    spectralEngine.setConfig(spectralConfig);
    spectralEngine.processBlock(buffer);

    const int requestedLatency =
        spectralEngine.getLatencySamples();

    if (requestedLatency != lastReportedLatency)
    {
        setLatencySamples(requestedLatency);
        lastReportedLatency = requestedLatency;
    }

    // Create a Dry signal delayed by exactly the latency reported
    // by the spectral engine. The ring buffer is fixed-size and
    // performs no allocation in the audio thread.
    const int dryDelaySamples = juce::jlimit(
        0,
        maxDryDelaySamples,
        requestedLatency);

    auto* alignedL = alignedDryScratch.getWritePointer(0);
    auto* alignedR = alignedDryScratch.getWritePointer(1);

    const auto* inputL = dryScratch.getReadPointer(0);
    const auto* inputR = dryScratch.getReadPointer(1);

    for (int n = 0; n < numSamples; ++n)
    {
        const int readPosition =
            (dryDelayWritePosition
             - dryDelaySamples
             + maxDryDelaySamples)
            % maxDryDelaySamples;

        if (dryDelaySamples == 0)
        {
            alignedL[n] = inputL[n];
            alignedR[n] = inputR[n];
        }
        else
        {
            alignedL[n] =
                dryDelayLineL[static_cast<size_t>(readPosition)];

            alignedR[n] =
                dryDelayLineR[static_cast<size_t>(readPosition)];
        }

        // Read the delayed sample before overwriting this ring slot.
        dryDelayLineL[static_cast<size_t>(dryDelayWritePosition)] =
            inputL[n];

        dryDelayLineR[static_cast<size_t>(dryDelayWritePosition)] =
            inputR[n];

        dryDelayWritePosition =
            (dryDelayWritePosition + 1) % maxDryDelaySamples;
    }

    // Advance the gain smoother once per sample, not once per channel.
    for (int n = 0; n < numSamples; ++n)
    {
        const float gOut = outputGainSmooth.getNextValue();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            buffer.getWritePointer(ch)[n] *=
                phaseSign * gOut;
        }
    }

    // Crossfade between the processed signal and the time-aligned
    // Dry signal. Both paths now have the same spectral latency.
    for (int n = 0; n < numSamples; ++n)
    {
        const float wetAmt = bypassMixSmooth.getNextValue();
        const float dryAmt = 1.0f - wetAmt;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* out = buffer.getWritePointer(ch);

            const auto* dry =
                alignedDryScratch.getReadPointer(ch);

            out[n] =
                dry[n] * dryAmt
                + out[n] * wetAmt;
        }
    }

    updateSpectrumVisuals(dryScratch, buffer);
}

void HybridResonanceEQProcessor::updateSpectrumVisuals(
    const juce::AudioBuffer<float>& preBuffer,
    const juce::AudioBuffer<float>& postBuffer)
{
    const int numSamples = juce::jmin(
        preBuffer.getNumSamples(),
        postBuffer.getNumSamples());

    const int numChannels = juce::jmin(
        preBuffer.getNumChannels(),
        postBuffer.getNumChannels());

    if (numSamples <= 0 || numChannels <= 0)
        return;

    const int speedIdx =
        static_cast<int>(
            apvts.getRawParameterValue("ana_speed")->load());

    const float alpha =
        (speedIdx == 0) ? 0.18f
        : ((speedIdx == 1) ? 0.32f : 0.55f);

    const int tiltIdx =
        static_cast<int>(
            apvts.getRawParameterValue("ana_tilt")->load());

    const float tiltDbPerOct =
        (tiltIdx == 0) ? 0.0f
        : ((tiltIdx == 1) ? 3.0f
        : ((tiltIdx == 2) ? 4.5f : 6.0f));

    auto analyze =
        [this, numSamples, numChannels, tiltDbPerOct](
            const juce::AudioBuffer<float>& source,
            std::array<float, numBins>& smoothed)
    {
        std::fill(
            fftWorkspace.begin(),
            fftWorkspace.end(),
            0.0f);

        const int copySamples =
            juce::jmin(numSamples, fftSize);

        const int sourceStart =
            numSamples - copySamples;

        const int destStart =
            fftSize - copySamples;

        for (int n = 0; n < copySamples; ++n)
        {
            float mono = 0.0f;

            for (int ch = 0; ch < numChannels; ++ch)
            {
                mono += source.getReadPointer(ch)[
                    sourceStart + n];
            }

            fftWorkspace[static_cast<size_t>(destStart + n)] =
                mono / static_cast<float>(numChannels);
        }

        hannWindow.multiplyWithWindowingTable(
            fftWorkspace.data(),
            static_cast<size_t>(fftSize));

        forwardFFT.performRealOnlyForwardTransform(
            fftWorkspace.data(), true);

        constexpr float windowSumApprox =
            static_cast<float>(fftSize) * 0.5f;

        const float magnitudeScale =
            2.0f / windowSumApprox;

        const float binHz =
            static_cast<float>(
                currentSampleRate / static_cast<double>(fftSize));

        for (int b = 0; b < numBins; ++b)
        {
            float magnitude = 0.0f;

            if (b == 0)
            {
                magnitude =
                    std::abs(fftWorkspace[0])
                    / windowSumApprox;
            }
            else if (b == fftSize / 2)
            {
                magnitude =
                    std::abs(fftWorkspace[1])
                    / windowSumApprox;
            }
            else
            {
                const size_t re =
                    static_cast<size_t>(2 * b);

                const size_t im = re + 1;

                magnitude = std::hypot(
                    fftWorkspace[re],
                    fftWorkspace[im]) * magnitudeScale;
            }

            const float freq =
                juce::jmax(
                    1.0f,
                    static_cast<float>(b) * binHz);

            const float tilt =
                tiltDbPerOct * std::log2(freq / 1000.0f);

            const float targetDb = juce::jlimit(
                -120.0f,
                12.0f,
                juce::Decibels::gainToDecibels(
                    magnitude + 1.0e-9f) + tilt);

            const size_t idx = static_cast<size_t>(b);

            smoothed[idx] +=
                (targetDb - smoothed[idx]) * alpha;
        }
    };

    analyze(preBuffer, preSpectrumSmooth);
    analyze(postBuffer, postSpectrumSmooth);

    spectralEngine.getReduction(spectralReduction);

    for (int b = 0; b < numBins; ++b)
    {
        const size_t idx = static_cast<size_t>(b);

        const size_t spectralIndex =
            (spectralEngine.getConfig().resolution
                == dspcore::SpectralResolution::Low)
            ? juce::jmin(
                static_cast<size_t>(
                    spectralEngine.getActiveNumBins() - 1),
                idx)
            : juce::jmin(
                static_cast<size_t>(
                    spectralEngine.getActiveNumBins() - 1),
                idx * 2);

        reductionSmooth[idx] +=
            (spectralReduction[spectralIndex]
             - reductionSmooth[idx]) * 0.5f;

        publishedPreSpectrum[idx].store(
            preSpectrumSmooth[idx],
            std::memory_order_relaxed);

        publishedPostSpectrum[idx].store(
            postSpectrumSmooth[idx],
            std::memory_order_relaxed);

        publishedReduction[idx].store(
            reductionSmooth[idx],
            std::memory_order_relaxed);
    }
}

double HybridResonanceEQProcessor::getCurrentSampleRateHz() const noexcept
{
    return currentSampleRate;
}

float HybridResonanceEQProcessor::getBandEQResponseDb(
    int bandIndex,
    double freqHz) const noexcept
{
    if (bandIndex < 0
        || bandIndex >= dspcore::ParametricEQEngine::maxBands)
    {
        return 0.0f;
    }

    const auto prefix = bandParamId(bandIndex, "");

    const bool enabled =
        apvts.getRawParameterValue(
            prefix + "enabled")->load() > 0.5f;

    if (!enabled)
        return 0.0f;

    const auto& coeffs =
        bandStates[static_cast<size_t>(bandIndex)]
            .filterL.coefficients;

    return evalBiquadMagnitudeDb(
        coeffs, currentSampleRate, freqHz);
}

float HybridResonanceEQProcessor::getCombinedEQResponseDb(
    double freqHz) const noexcept
{
    float totalDb = 0.0f;

    for (int i = 0;
         i < dspcore::ParametricEQEngine::maxBands;
         ++i)
    {
        totalDb += getBandEQResponseDb(i, freqHz);
    }

    return totalDb;
}

float HybridResonanceEQProcessor::getBandDynamicGainDb(
    int bandIndex) const noexcept
{
    if (bandIndex < 0
        || bandIndex >= dspcore::ParametricEQEngine::maxBands)
    {
        return 0.0f;
    }

    return bandStates[
        static_cast<size_t>(bandIndex)].currentDynDb;
}

float HybridResonanceEQProcessor::getAutoGainCompensationDb() const noexcept
{
    return autoGainCompDb.load();
}

void HybridResonanceEQProcessor::getVisualData(
    std::array<float, analyzerNumBins>& preOut,
    std::array<float, analyzerNumBins>& postOut,
    std::array<float, analyzerNumBins>& redOut) const noexcept
{
    const int copyCount = analyzerNumBins;

    for (int i = 0; i < copyCount; ++i)
    {
        const size_t idx = static_cast<size_t>(i);

        preOut[idx] =
            publishedPreSpectrum[idx].load(
                std::memory_order_relaxed);

        postOut[idx] =
            publishedPostSpectrum[idx].load(
                std::memory_order_relaxed);

        redOut[idx] =
            publishedReduction[idx].load(
                std::memory_order_relaxed);
    }
}

juce::AudioProcessorEditor*
HybridResonanceEQProcessor::createEditor()
{
    return new HybridResonanceEQEditor(*this);
}

bool HybridResonanceEQProcessor::hasEditor() const
{
    return true;
}

const juce::String HybridResonanceEQProcessor::getName() const
{
    return JucePlugin_Name;
}

bool HybridResonanceEQProcessor::acceptsMidi() const
{
    return false;
}

bool HybridResonanceEQProcessor::producesMidi() const
{
    return false;
}

bool HybridResonanceEQProcessor::isMidiEffect() const
{
    return false;
}

double HybridResonanceEQProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int HybridResonanceEQProcessor::getNumPrograms()
{
    return 1;
}

int HybridResonanceEQProcessor::getCurrentProgram()
{
    return 0;
}

void HybridResonanceEQProcessor::setCurrentProgram(int /*index*/)
{
}

const juce::String
HybridResonanceEQProcessor::getProgramName(int /*index*/)
{
    return {};
}

void HybridResonanceEQProcessor::changeProgramName(
    int /*index*/,
    const juce::String& /*newName*/)
{
}

void HybridResonanceEQProcessor::getStateInformation(
    juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();

    std::unique_ptr<juce::XmlElement> xml(
        state.createXml());

    copyXmlToBinary(*xml, destData);
}

void HybridResonanceEQProcessor::setStateInformation(
    const void* data,
    int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(
        getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr
        && xmlState->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(
            juce::ValueTree::fromXml(*xmlState));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HybridResonanceEQProcessor();
}
