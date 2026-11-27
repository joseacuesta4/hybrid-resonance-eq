#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

namespace
{
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

juce::dsp::IIR::Coefficients<float>::Ptr makeCoefficientsForShape(dspcore::FilterShape shape,
                                                                 double sampleRate,
                                                                 float freqHz,
                                                                 float gainDb,
                                                                 float qVal,
                                                                 float /*slopeDb*/)
{
    const double sr = juce::jmax(8000.0, sampleRate);
    const double nyquist = sr * 0.495;
    const double clampedFreq = juce::jlimit(20.0, nyquist, static_cast<double>(freqHz));
    const double clampedQ = juce::jlimit(0.1, 24.0, static_cast<double>(qVal));
    const float linearGain = dbToGain(juce::jlimit(-30.0f, 30.0f, gainDb));

    switch (shape)
    {
        case dspcore::FilterShape::Bell:
            return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sr, clampedFreq, clampedQ, linearGain);

        case dspcore::FilterShape::LowShelf:
            return juce::dsp::IIR::Coefficients<float>::makeLowShelf(sr, clampedFreq, clampedQ, linearGain);

        case dspcore::FilterShape::HighShelf:
            return juce::dsp::IIR::Coefficients<float>::makeHighShelf(sr, clampedFreq, clampedQ, linearGain);

        case dspcore::FilterShape::LowCut:
            return juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, clampedFreq, clampedQ);

        case dspcore::FilterShape::HighCut:
            return juce::dsp::IIR::Coefficients<float>::makeLowPass(sr, clampedFreq, clampedQ);

        case dspcore::FilterShape::Notch:
            return juce::dsp::IIR::Coefficients<float>::makeNotch(sr, clampedFreq, clampedQ);

        case dspcore::FilterShape::BandPass:
            return juce::dsp::IIR::Coefficients<float>::makeBandPass(sr, clampedFreq, clampedQ);

        case dspcore::FilterShape::TiltShelf:
            return juce::dsp::IIR::Coefficients<float>::makeHighShelf(sr, clampedFreq, juce::jmax(0.35, clampedQ * 0.7), linearGain);
    }

    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sr, clampedFreq, clampedQ, 1.0f);
}
} // namespace

HybridResonanceEQProcessor::HybridResonanceEQProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    fifoL.assign(static_cast<size_t>(fftSize), 0.0f);
    fifoR.assign(static_cast<size_t>(fftSize), 0.0f);
    fftWorkspace.assign(static_cast<size_t>(fftSize * 2), 0.0f);
    olaBufferL.assign(static_cast<size_t>(fftSize * 2), 0.0f);
    olaBufferR.assign(static_cast<size_t>(fftSize * 2), 0.0f);
    olaNormBuffer.assign(static_cast<size_t>(fftSize * 2), 0.0f);
    binEnvelope.assign(static_cast<size_t>(numBins), 0.0f);
    binReductionDb.assign(static_cast<size_t>(numBins), 0.0f);

    preSpectrumSmooth.fill(-100.0f);
    postSpectrumSmooth.fill(-100.0f);
    reductionSmooth.fill(0.0f);
}

HybridResonanceEQProcessor::~HybridResonanceEQProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout HybridResonanceEQProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "ana_resolution", "Analyzer Resolution",
        juce::StringArray { "Low", "Medium", "High", "Maximum" }, 2));

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

    params.push_back(std::make_unique<juce::AudioParameterBool>("ana_show_pre", "Show Pre Spectrum", true));
    params.push_back(std::make_unique<juce::AudioParameterBool>("ana_show_post", "Show Post Spectrum", true));
    params.push_back(std::make_unique<juce::AudioParameterBool>("out_auto_gain", "Output Auto Gain", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>("phase_invert", "Phase Invert", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>("global_bypass", "Global Bypass", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "gain_scale", "Gain Scale",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "output_gain", "Output Gain",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>("spec_delta", "Spectral Delta", false));
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
        const juce::String prefix = "band_" + juce::String(i) + "_";
        const juce::String labelPrefix = "Band " + juce::String(i + 1) + " ";

        const bool defaultEnabled = (i < 3);
        const float defaultFreq = (i == 0) ? 120.0f : ((i == 1) ? 850.0f : ((i == 2) ? 4200.0f : 1000.0f));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            prefix + "enabled", labelPrefix + "Enabled", defaultEnabled));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            prefix + "shape", labelPrefix + "Shape",
            juce::StringArray { "Bell", "Low Shelf", "High Shelf", "Low Cut", "High Cut", "Notch", "Band Pass", "Tilt Shelf" }, 0));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            prefix + "route", labelPrefix + "Routing",
            juce::StringArray { "Stereo", "Mid", "Side", "Left", "Right" }, 0));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            prefix + "character", labelPrefix + "Character",
            juce::StringArray { "Clean", "Subtle", "Warm" }, 0));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            prefix + "dyn_enabled", labelPrefix + "Dynamic Enabled", false));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            prefix + "dyn_auto", labelPrefix + "Dynamic Auto Thresh", true));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            prefix + "spec_enabled", labelPrefix + "Spectral Enabled", false));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            prefix + "solo", labelPrefix + "Solo", false));

        juce::NormalisableRange<float> freqRange(20.0f, 20000.0f, 0.1f);
        freqRange.setSkewForCentre(1000.0f);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            prefix + "freq", labelPrefix + "Freq", freqRange, defaultFreq));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            prefix + "gain", labelPrefix + "Gain",
            juce::NormalisableRange<float>(-24.0f, 24.0f, 0.05f), 0.0f));

        juce::NormalisableRange<float> qRange(0.1f, 18.0f, 0.01f);
        qRange.setSkewForCentre(1.0f);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            prefix + "q", labelPrefix + "Q", qRange, 1.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            prefix + "slope", labelPrefix + "Slope",
            juce::NormalisableRange<float>(6.0f, 48.0f, 6.0f), 12.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            prefix + "dyn_range", labelPrefix + "Dynamic Range",
            juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), -6.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            prefix + "dyn_thresh", labelPrefix + "Dynamic Threshold",
            juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -24.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            prefix + "dyn_attack", labelPrefix + "Dynamic Attack",
            juce::NormalisableRange<float>(0.5f, 200.0f, 0.1f), 15.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            prefix + "dyn_release", labelPrefix + "Dynamic Release",
            juce::NormalisableRange<float>(10.0f, 1000.0f, 0.5f), 120.0f));
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
    }

    std::fill(fifoL.begin(), fifoL.end(), 0.0f);
    std::fill(fifoR.begin(), fifoR.end(), 0.0f);
    std::fill(olaBufferL.begin(), olaBufferL.end(), 0.0f);
    std::fill(olaBufferR.begin(), olaBufferR.end(), 0.0f);
    std::fill(olaNormBuffer.begin(), olaNormBuffer.end(), 0.0f);
    std::fill(binEnvelope.begin(), binEnvelope.end(), 0.0f);
    std::fill(binReductionDb.begin(), binReductionDb.end(), 0.0f);
    fifoIndex = 0;

    const bool bypassed = apvts.getRawParameterValue("global_bypass")->load() > 0.5f;
    bypassMixSmooth.setCurrentAndTargetValue(bypassed ? 0.0f : 1.0f);
    outputGainSmooth.setCurrentAndTargetValue(dbToGain(apvts.getRawParameterValue("output_gain")->load()));
    autoGainCompDb.store(0.0f);
}

void HybridResonanceEQProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = juce::jmax(8000.0, sampleRate);
    currentBlockSize = juce::jmax(1, samplesPerBlock);

    juce::dsp::ProcessSpec spec { currentSampleRate, static_cast<juce::uint32>(currentBlockSize), 1 };
    for (auto& st : bandStates)
    {
        st.filterL.prepare(spec);
        st.filterR.prepare(spec);
    }

    bypassMixSmooth.reset(currentSampleRate, 0.012);
    outputGainSmooth.reset(currentSampleRate, 0.015);

    resetDSPState();
    updateBandFilters(currentSampleRate);
}

void HybridResonanceEQProcessor::releaseResources()
{
}

bool HybridResonanceEQProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo() ||
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

float HybridResonanceEQProcessor::applyBandCharacterSample(float sample, int characterMode) const noexcept
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
        return std::tanh(asym * drive) / std::tanh(drive * 1.04f);
    }
    return sample;
}

void HybridResonanceEQProcessor::updateBandFilters(double sampleRate)
{
    const float gainScale = apvts.getRawParameterValue("gain_scale")->load();

    for (int i = 0; i < dspcore::ParametricEQEngine::maxBands; ++i)
    {
        const auto prefix = bandParamId(i, "");
        const auto shape = static_cast<dspcore::FilterShape>(
            static_cast<int>(apvts.getRawParameterValue(prefix + "shape")->load()));
        const float freq = apvts.getRawParameterValue(prefix + "freq")->load();
        const float baseGain = apvts.getRawParameterValue(prefix + "gain")->load();
        const float qVal = apvts.getRawParameterValue(prefix + "q")->load();
        const float slope = apvts.getRawParameterValue(prefix + "slope")->load();

        const float effectiveGain = dspcore::BandFilter::supportsGain(shape)
            ? (baseGain * gainScale + bandStates[static_cast<size_t>(i)].currentDynDb)
            : bandStates[static_cast<size_t>(i)].currentDynDb;

        auto coeffs = makeCoefficientsForShape(shape, sampleRate, freq, effectiveGain, qVal, slope);
        if (coeffs != nullptr)
        {
            *bandStates[static_cast<size_t>(i)].filterL.coefficients = *coeffs;
            *bandStates[static_cast<size_t>(i)].filterR.coefficients = *coeffs;
        }
    }
}

void HybridResonanceEQProcessor::processSpectralStage(juce::AudioBuffer<float>& buffer,
                                                     bool anySpecEnabled,
                                                     float depth,
                                                     float sharpness,
                                                     float smoothing,
                                                     float mix,
                                                     bool deltaMode)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    if (numSamples <= 0 || numChannels < 2)
        return;

    if (!anySpecEnabled || depth <= 0.0001f || mix <= 0.0001f)
    {
        for (int b = 0; b < numBins; ++b)
            binReductionDb[static_cast<size_t>(b)] = 0.0f;
        return;
    }

    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer, true);

    auto* leftPtr = buffer.getWritePointer(0);
    auto* rightPtr = buffer.getWritePointer(1);

    std::vector<float> wetL(static_cast<size_t>(numSamples), 0.0f);
    std::vector<float> wetR(static_cast<size_t>(numSamples), 0.0f);
    std::copy(leftPtr, leftPtr + numSamples, wetL.begin());
    std::copy(rightPtr, rightPtr + numSamples, wetR.begin());

    const float binHz = static_cast<float>(currentSampleRate) / static_cast<float>(fftSize);
    const float maxAttenDb = -2.0f - depth * 16.0f;

    std::array<bool, numBins> activeBinMask {};
    activeBinMask.fill(false);

    for (int i = 0; i < dspcore::ParametricEQEngine::maxBands; ++i)
    {
        const auto prefix = bandParamId(i, "");
        const bool enabled = apvts.getRawParameterValue(prefix + "enabled")->load() > 0.5f;
        const bool specOn = apvts.getRawParameterValue(prefix + "spec_enabled")->load() > 0.5f;
        if (!enabled || !specOn)
            continue;

        const float centerFreq = juce::jlimit(25.0f, 19500.0f, apvts.getRawParameterValue(prefix + "freq")->load());
        const float qVal = juce::jlimit(0.2f, 12.0f, apvts.getRawParameterValue(prefix + "q")->load());
        const float bandwidthOct = 1.0f / juce::jmax(0.25f, qVal);
        const float fLow = centerFreq * std::pow(2.0f, -bandwidthOct * 0.65f);
        const float fHigh = centerFreq * std::pow(2.0f, bandwidthOct * 0.65f);

        for (int b = 1; b < numBins; ++b)
        {
            const float f = static_cast<float>(b) * binHz;
            if (f >= fLow && f <= fHigh)
                activeBinMask[static_cast<size_t>(b)] = true;
        }
    }

    for (int n = 0; n < numSamples; ++n)
    {
        fifoL[static_cast<size_t>(fifoIndex)] = leftPtr[n];
        fifoR[static_cast<size_t>(fifoIndex)] = rightPtr[n];
        ++fifoIndex;

        if (fifoIndex >= hopSize)
        {
            fifoIndex = 0;
            std::memmove(fifoL.data(), fifoL.data() + hopSize, static_cast<size_t>(fftSize - hopSize) * sizeof(float));
            std::memmove(fifoR.data(), fifoR.data() + hopSize, static_cast<size_t>(fftSize - hopSize) * sizeof(float));

            std::fill(fftWorkspace.begin(), fftWorkspace.end(), 0.0f);
            for (int k = 0; k < fftSize; ++k)
            {
                const float mono = 0.5f * (fifoL[static_cast<size_t>(k)] + fifoR[static_cast<size_t>(k)]);
                fftWorkspace[static_cast<size_t>(k)] = mono;
            }
            hannWindow.multiplyWithWindowingTable(fftWorkspace.data(), static_cast<size_t>(fftSize));
            forwardFFT.performFrequencyOnlyForwardTransform(fftWorkspace.data());

            const int localRadius = juce::jlimit(2, 18, static_cast<int>(std::round(16.0f - sharpness * 12.0f)));
            const float envAlpha = juce::jlimit(0.05f, 0.85f, 0.65f - smoothing * 0.50f);

            for (int b = 1; b < numBins; ++b)
            {
                const float mag = fftWorkspace[static_cast<size_t>(b)] / static_cast<float>(fftSize * 0.5f);
                const float db = gainToDb(mag);

                float localSum = 0.0f;
                int count = 0;
                const int bStart = juce::jmax(1, b - localRadius);
                const int bEnd = juce::jmin(numBins - 1, b + localRadius);
                for (int m = bStart; m <= bEnd; ++m)
                {
                    localSum += gainToDb(fftWorkspace[static_cast<size_t>(m)] / static_cast<float>(fftSize * 0.5f));
                    ++count;
                }
                const float localAvgDb = (count > 0) ? (localSum / static_cast<float>(count)) : db;
                const float prominenceDb = juce::jmax(0.0f, db - localAvgDb - (3.0f - sharpness * 2.0f));

                float targetRedDb = 0.0f;
                if (activeBinMask[static_cast<size_t>(b)] && prominenceDb > 0.0f)
                {
                    const float normPeak = juce::jlimit(0.0f, 1.0f, prominenceDb / 14.0f);
                    const float shaped = std::pow(normPeak, 0.7f + sharpness * 0.8f);
                    targetRedDb = juce::jlimit(maxAttenDb, 0.0f, -shaped * std::abs(maxAttenDb));
                }

                binEnvelope[static_cast<size_t>(b)] = binEnvelope[static_cast<size_t>(b)] * (1.0f - envAlpha) + targetRedDb * envAlpha;
                binReductionDb[static_cast<size_t>(b)] = binEnvelope[static_cast<size_t>(b)];
            }

            float avgAttenLinear = 1.0f;
            {
                double sumLin = 0.0;
                int c = 0;
                for (int b = 1; b < numBins; ++b)
                {
                    if (activeBinMask[static_cast<size_t>(b)])
                    {
                        sumLin += static_cast<double>(dbToGain(binReductionDb[static_cast<size_t>(b)]));
                        ++c;
                    }
                }
                if (c > 0)
                    avgAttenLinear = juce::jlimit(0.15f, 1.0f, static_cast<float>(sumLin / static_cast<double>(c)));
            }

            const float effectiveAtten = 1.0f - (1.0f - avgAttenLinear) * depth * mix;
            for (int k = 0; k < hopSize; ++k)
            {
                const int idx = juce::jmin(numSamples - 1, n - hopSize + 1 + k);
                if (idx >= 0)
                {
                    wetL[static_cast<size_t>(idx)] *= effectiveAtten;
                    wetR[static_cast<size_t>(idx)] *= effectiveAtten;
                }
            }
        }
    }

    if (deltaMode)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* out = buffer.getWritePointer(ch);
            const auto* dry = dryBuffer.getReadPointer(ch);
            const auto* wet = (ch == 0) ? wetL.data() : wetR.data();
            for (int n = 0; n < numSamples; ++n)
                out[n] = (dry[n] - wet[n]) * 1.5f;
        }
    }
    else
    {
        for (int n = 0; n < numSamples; ++n)
        {
            leftPtr[n] = wetL[static_cast<size_t>(n)];
            rightPtr[n] = wetR[static_cast<size_t>(n)];
        }
    }
}

void HybridResonanceEQProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    if (numChannels <= 0 || numSamples <= 0)
        return;

    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer, true);

    const bool bypassed = apvts.getRawParameterValue("global_bypass")->load() > 0.5f;
    bypassMixSmooth.setTargetValue(bypassed ? 0.0f : 1.0f);

    const float outGainDb = apvts.getRawParameterValue("output_gain")->load();
    outputGainSmooth.setTargetValue(dbToGain(outGainDb));

    const bool phaseInvert = apvts.getRawParameterValue("phase_invert")->load() > 0.5f;
    const float phaseSign = phaseInvert ? -1.0f : 1.0f;

    bool anySoloActive = false;
    bool anySpecActive = false;
    for (int i = 0; i < dspcore::ParametricEQEngine::maxBands; ++i)
    {
        const auto prefix = bandParamId(i, "");
        const bool enabled = apvts.getRawParameterValue(prefix + "enabled")->load() > 0.5f;
        if (!enabled)
            continue;

        if (apvts.getRawParameterValue(prefix + "solo")->load() > 0.5f)
            anySoloActive = true;

        if (apvts.getRawParameterValue(prefix + "spec_enabled")->load() > 0.5f)
            anySpecActive = true;
    }

    const float gainScale = apvts.getRawParameterValue("gain_scale")->load();
    for (int i = 0; i < dspcore::ParametricEQEngine::maxBands; ++i)
    {
        auto& st = bandStates[static_cast<size_t>(i)];
        const auto prefix = bandParamId(i, "");
        const bool enabled = apvts.getRawParameterValue(prefix + "enabled")->load() > 0.5f;
        const bool dynOn = apvts.getRawParameterValue(prefix + "dyn_enabled")->load() > 0.5f;

        if (!enabled || !dynOn)
        {
            st.currentDynDb = 0.0f;
            continue;
        }

        const bool autoThresh = apvts.getRawParameterValue(prefix + "dyn_auto")->load() > 0.5f;
        const float dynRange = apvts.getRawParameterValue(prefix + "dyn_range")->load();
        const float threshDb = autoThresh ? -22.0f : apvts.getRawParameterValue(prefix + "dyn_thresh")->load();
        const float attackMs = juce::jmax(0.5f, apvts.getRawParameterValue(prefix + "dyn_attack")->load());
        const float releaseMs = juce::jmax(10.0f, apvts.getRawParameterValue(prefix + "dyn_release")->load());

        float sumSq = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const auto* ptr = buffer.getReadPointer(ch);
            for (int n = 0; n < numSamples; ++n)
                sumSq += ptr[n] * ptr[n];
        }
        const float rms = std::sqrt(sumSq / juce::jmax(1, numChannels * numSamples));
        const float inputDb = gainToDb(rms);

        const float coeffMs = (inputDb > st.envLevelDb) ? attackMs : releaseMs;
        const float alpha = 1.0f - std::exp(-static_cast<float>(numSamples) /
                                              static_cast<float>(currentSampleRate * (coeffMs * 0.001)));
        st.envLevelDb += (inputDb - st.envLevelDb) * alpha;

        const float overDb = juce::jmax(0.0f, st.envLevelDb - threshDb);
        const float ratio = juce::jlimit(0.0f, 1.0f, overDb / 18.0f);
        st.currentDynDb = dynRange * ratio;
    }

    updateBandFilters(currentSampleRate);

    juce::AudioBuffer<float> soloSumBuffer;
    soloSumBuffer.setSize(numChannels, numSamples);
    soloSumBuffer.clear();

    for (int i = 0; i < dspcore::ParametricEQEngine::maxBands; ++i)
    {
        auto& st = bandStates[static_cast<size_t>(i)];
        const auto prefix = bandParamId(i, "");
        const bool enabled = apvts.getRawParameterValue(prefix + "enabled")->load() > 0.5f;
        if (!enabled)
            continue;

        const bool soloOn = apvts.getRawParameterValue(prefix + "solo")->load() > 0.5f;
        const int charMode = static_cast<int>(apvts.getRawParameterValue(prefix + "character")->load());

        juce::AudioBuffer<float> bandBuf;
        bandBuf.makeCopyOf(buffer, true);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* ptr = bandBuf.getWritePointer(ch);
            for (int n = 0; n < numSamples; ++n)
            {
                float s = ptr[n];
                s = (ch == 0) ? st.filterL.processSample(s) : st.filterR.processSample(s);
                s = applyBandCharacterSample(s, charMode);
                ptr[n] = s;
            }
        }

        if (anySoloActive)
        {
            if (soloOn)
            {
                const auto shape = static_cast<dspcore::FilterShape>(
                    static_cast<int>(apvts.getRawParameterValue(prefix + "shape")->load()));
                const float bandGainDb = dspcore::BandFilter::supportsGain(shape)
                    ? (apvts.getRawParameterValue(prefix + "gain")->load() * gainScale)
                    : 0.0f;
                const float soloBandGain = dbToGain(bandGainDb);

                for (int ch = 0; ch < numChannels; ++ch)
                    soloSumBuffer.addFrom(ch, 0, bandBuf, ch, 0, numSamples, soloBandGain);
            }
        }
        else
        {
            buffer.makeCopyOf(bandBuf, true);
        }
    }

    if (anySoloActive)
        buffer.makeCopyOf(soloSumBuffer, true);

    const float specDepth = apvts.getRawParameterValue("spec_depth")->load();
    const float specSharp = apvts.getRawParameterValue("spec_sharpness")->load();
    const float specSmooth = apvts.getRawParameterValue("spec_smoothing")->load();
    const float specMix = apvts.getRawParameterValue("spec_mix")->load();
    const bool specDelta = apvts.getRawParameterValue("spec_delta")->load() > 0.5f;

    processSpectralStage(buffer, anySpecActive, specDepth, specSharp, specSmooth, specMix, specDelta);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* out = buffer.getWritePointer(ch);
        for (int n = 0; n < numSamples; ++n)
        {
            const float gOut = outputGainSmooth.getNextValue();
            out[n] = out[n] * phaseSign * gOut;
        }
    }

    for (int n = 0; n < numSamples; ++n)
    {
        const float wetAmt = bypassMixSmooth.getNextValue();
        const float dryAmt = 1.0f - wetAmt;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* out = buffer.getWritePointer(ch);
            const auto* dry = dryBuffer.getReadPointer(ch);
            out[n] = dry[n] * dryAmt + out[n] * wetAmt;
        }
    }

    updateSpectrumVisuals(dryBuffer, buffer);
}

void HybridResonanceEQProcessor::updateSpectrumVisuals(const juce::AudioBuffer<float>& preBuffer,
                                                      const juce::AudioBuffer<float>& postBuffer)
{
    const int numSamples = juce::jmin(preBuffer.getNumSamples(), postBuffer.getNumSamples());
    const int numChannels = juce::jmin(preBuffer.getNumChannels(), postBuffer.getNumChannels());
    if (numSamples <= 0 || numChannels <= 0)
        return;

    float preSq = 0.0f;
    float postSq = 0.0f;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        const auto* preP = preBuffer.getReadPointer(ch);
        const auto* postP = postBuffer.getReadPointer(ch);
        for (int n = 0; n < numSamples; ++n)
        {
            preSq += preP[n] * preP[n];
            postSq += postP[n] * postP[n];
        }
    }

    const float denom = static_cast<float>(juce::jmax(1, numChannels * numSamples));
    const float preDb = juce::jlimit(-100.0f, 6.0f, gainToDb(std::sqrt(preSq / denom)));
    const float postDb = juce::jlimit(-100.0f, 6.0f, gainToDb(std::sqrt(postSq / denom)));

    const int speedIdx = static_cast<int>(apvts.getRawParameterValue("ana_speed")->load());
    const float visAlpha = (speedIdx == 0) ? 0.08f : ((speedIdx == 1) ? 0.18f : 0.35f);

    for (int b = 0; b < numBins; ++b)
    {
        const size_t idx = static_cast<size_t>(b);
        preSpectrumSmooth[idx] += (preDb - preSpectrumSmooth[idx]) * visAlpha;
        postSpectrumSmooth[idx] += (postDb - postSpectrumSmooth[idx]) * visAlpha;
        reductionSmooth[idx] += (binReductionDb[idx] - reductionSmooth[idx]) * 0.25f;
    }
}

juce::AudioProcessorEditor* HybridResonanceEQProcessor::createEditor()
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

const juce::String HybridResonanceEQProcessor::getProgramName(int /*index*/)
{
    return {};
}

void HybridResonanceEQProcessor::changeProgramName(int /*index*/, const juce::String& /*newName*/)
{
}

void HybridResonanceEQProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void HybridResonanceEQProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}
