#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
juce::String bandParamId(int bandIndex, const juce::String& suffix)
{
    return "band_" + juce::String(bandIndex) + "_" + suffix;
}
} // namespace

HybridResonanceEQProcessor::HybridResonanceEQProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout HybridResonanceEQProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    const std::array<float, 16> defaultFreqs {
        60.0f, 120.0f, 250.0f, 500.0f, 1000.0f, 2000.0f, 4000.0f, 8000.0f,
        12000.0f, 90.0f, 350.0f, 750.0f, 1500.0f, 3000.0f, 6000.0f, 10000.0f
    };

    for (int i = 0; i < dspcore::ParametricEQEngine::maxBands; ++i)
    {
        const juce::String prefix = "Band " + juce::String(i + 1) + " ";

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            bandParamId(i, "enabled"), prefix + "Enabled", i == 0));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            bandParamId(i, "solo"), prefix + "Solo", false));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            bandParamId(i, "dyn_enabled"), prefix + "Dynamic", false));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            bandParamId(i, "dyn_auto"), prefix + "Dyn Auto Thresh", true));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            bandParamId(i, "spec_enabled"), prefix + "Spectral", false));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            bandParamId(i, "shape"), prefix + "Shape",
            juce::StringArray { "Bell", "Low Shelf", "High Shelf", "Low Cut", "High Cut", "Notch", "Band Pass", "Tilt Shelf" }, 0));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            bandParamId(i, "route"), prefix + "Route",
            juce::StringArray { "Stereo", "Mid", "Side", "Left", "Right" }, 0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            bandParamId(i, "freq"), prefix + "Freq",
            juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.25f),
            defaultFreqs[static_cast<size_t>(i)]));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            bandParamId(i, "gain"), prefix + "Gain",
            juce::NormalisableRange<float>(-30.0f, 30.0f, 0.05f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            bandParamId(i, "q"), prefix + "Q",
            juce::NormalisableRange<float>(0.15f, 20.0f, 0.01f, 0.35f), 1.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            bandParamId(i, "slope"), prefix + "Slope",
            juce::NormalisableRange<float>(6.0f, 48.0f, 6.0f), 12.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            bandParamId(i, "dyn_range"), prefix + "Dyn Range",
            juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), -6.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            bandParamId(i, "dyn_thresh"), prefix + "Dyn Thresh",
            juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -24.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            bandParamId(i, "dyn_attack"), prefix + "Dyn Attack",
            juce::NormalisableRange<float>(0.5f, 150.0f, 0.1f, 0.4f), 15.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            bandParamId(i, "dyn_release"), prefix + "Dyn Release",
            juce::NormalisableRange<float>(10.0f, 600.0f, 1.0f, 0.4f), 120.0f));
    }

    params.push_back(std::make_unique<juce::AudioParameterBool>("spec_delta", "Spectral Delta", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "spec_depth", "Spectral Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.45f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "spec_sharpness", "Spectral Sharpness",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.65f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "spec_smoothing", "Spectral Smoothing",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.55f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "spec_mix", "Spectral Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "ana_resolution", "Analyzer Resolution",
        juce::StringArray { "Low", "Medium", "High", "Max" }, 2));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "ana_speed", "Analyzer Speed",
        juce::StringArray { "Slow", "Medium", "Fast" }, 1));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "ana_tilt", "Analyzer Tilt",
        juce::StringArray { "0 dB/oct", "3 dB/oct", "4.5 dB/oct", "6 dB/oct" }, 2));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "ana_show_pre", "Show Pre Spectrum", true));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "ana_show_post", "Show Post Spectrum", true));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "display_range", "Display Range",
        juce::StringArray { "+/- 6 dB", "+/- 12 dB", "+/- 30 dB" }, 1));

    // Character y Auto Gain independientes
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "character_mode", "Character",
        juce::StringArray { "Clean", "Subtle", "Warm" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "out_auto_gain", "Out Auto Gain", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "gain_scale", "Gain Scale",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "phase_invert", "Phase Invert", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "output_gain", "Output Gain",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.05f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "global_bypass", "Bypass", false));

    return { params.begin(), params.end() };
}

void HybridResonanceEQProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    eqEngine.prepare(sampleRate, samplesPerBlock);
    spectralEngine.prepare(sampleRate, samplesPerBlock);
    inputReferenceBuffer.setSize(2, juce::jmax(1, samplesPerBlock), false, false, true);
    autoGainSmoothDb = 0.0f;
    syncParametersToDSP();
    setLatencySamples(spectralEngine.getLatencySamples());
}

void HybridResonanceEQProcessor::releaseResources()
{
    eqEngine.reset();
}

bool HybridResonanceEQProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void HybridResonanceEQProcessor::syncParametersToDSP()
{
    dspcore::SpectralConfig sc;
    sc.enabled = true;
    sc.deltaListen = apvts.getRawParameterValue("spec_delta")->load() > 0.5f;
    sc.depth = apvts.getRawParameterValue("spec_depth")->load();
    sc.sharpness = apvts.getRawParameterValue("spec_sharpness")->load();
    sc.smoothing = apvts.getRawParameterValue("spec_smoothing")->load();
    sc.mix = apvts.getRawParameterValue("spec_mix")->load();
    sc.resolution = static_cast<dspcore::AnalyzerResolution>(static_cast<int>(apvts.getRawParameterValue("ana_resolution")->load()));
    sc.speed = static_cast<dspcore::AnalyzerSpeed>(static_cast<int>(apvts.getRawParameterValue("ana_speed")->load()));

    const int tiltIdx = static_cast<int>(apvts.getRawParameterValue("ana_tilt")->load());
    constexpr std::array<float, 4> tiltValues { 0.0f, 3.0f, 4.5f, 6.0f };
    sc.tiltDbPerOct = tiltValues[static_cast<size_t>(juce::jlimit(0, 3, tiltIdx))];

    for (int i = 0; i < dspcore::ParametricEQEngine::maxBands; ++i)
    {
        dspcore::BandParams p;
        p.enabled = apvts.getRawParameterValue(bandParamId(i, "enabled"))->load() > 0.5f;
        p.solo = apvts.getRawParameterValue(bandParamId(i, "solo"))->load() > 0.5f;
        p.dynamicEnabled = apvts.getRawParameterValue(bandParamId(i, "dyn_enabled"))->load() > 0.5f;
        p.autoThreshold = apvts.getRawParameterValue(bandParamId(i, "dyn_auto"))->load() > 0.5f;
        p.spectralEnabled = apvts.getRawParameterValue(bandParamId(i, "spec_enabled"))->load() > 0.5f;
        p.shape = static_cast<dspcore::FilterShape>(static_cast<int>(apvts.getRawParameterValue(bandParamId(i, "shape"))->load()));
        p.route = static_cast<dspcore::ChannelRoute>(static_cast<int>(apvts.getRawParameterValue(bandParamId(i, "route"))->load()));
        p.freqHz = apvts.getRawParameterValue(bandParamId(i, "freq"))->load();
        p.gainDb = apvts.getRawParameterValue(bandParamId(i, "gain"))->load();
        p.q = apvts.getRawParameterValue(bandParamId(i, "q"))->load();
        p.slopeDbOct = apvts.getRawParameterValue(bandParamId(i, "slope"))->load();
        p.dynRangeDb = apvts.getRawParameterValue(bandParamId(i, "dyn_range"))->load();
        p.dynThresholdDb = apvts.getRawParameterValue(bandParamId(i, "dyn_thresh"))->load();
        p.dynAttackMs = apvts.getRawParameterValue(bandParamId(i, "dyn_attack"))->load();
        p.dynReleaseMs = apvts.getRawParameterValue(bandParamId(i, "dyn_release"))->load();

        eqEngine.setBandParameters(i, p);

        sc.bandWindows[static_cast<size_t>(i)] = {
            p.enabled && p.spectralEnabled,
            p.freqHz,
            p.q
        };
    }

    spectralEngine.setConfig(sc);
}

void HybridResonanceEQProcessor::applyCharacterAndOutput(const juce::AudioBuffer<float>& inputRef,
                                                       juce::AudioBuffer<float>& outputBuffer,
                                                       CharacterMode mode,
                                                       float outGainDb,
                                                       bool phaseInvert,
                                                       bool autoGainEnabled) noexcept
{
    const int numSamples = outputBuffer.getNumSamples();
    const int numChannels = outputBuffer.getNumChannels();
    if (numSamples <= 0 || numChannels < 2)
        return;

    float drive = 1.0f;
    if (mode == CharacterMode::Subtle)
        drive = 1.25f;
    else if (mode == CharacterMode::Warm)
        drive = 1.85f;

    if (mode != CharacterMode::Clean)
    {
        const float norm = 1.0f / std::tanh(drive);
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = outputBuffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i)
                data[i] = std::tanh(data[i] * drive) * norm;
        }
    }

    float compDb = 0.0f;
    if (autoGainEnabled && inputRef.getNumChannels() >= 2 && inputRef.getNumSamples() >= numSamples)
    {
        double inSq = 0.0;
        double outSq = 0.0;
        for (int ch = 0; ch < 2; ++ch)
        {
            const auto* inPtr = inputRef.getReadPointer(ch);
            const auto* outPtr = outputBuffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i)
            {
                inSq += static_cast<double>(inPtr[i]) * static_cast<double>(inPtr[i]);
                outSq += static_cast<double>(outPtr[i]) * static_cast<double>(outPtr[i]);
            }
        }

        const double inRms = std::sqrt(inSq / static_cast<double>(numSamples * 2));
        const double outRms = std::sqrt(outSq / static_cast<double>(numSamples * 2));

        if (inRms > 1.0e-5 && outRms > 1.0e-5)
        {
            const float inDb = juce::Decibels::gainToDecibels(static_cast<float>(inRms));
            const float outDb = juce::Decibels::gainToDecibels(static_cast<float>(outRms));
            const float targetCompDb = juce::jlimit(-18.0f, 18.0f, inDb - outDb);
            autoGainSmoothDb = 0.88f * autoGainSmoothDb + 0.12f * targetCompDb;
        }
        else
        {
            autoGainSmoothDb = 0.94f * autoGainSmoothDb;
        }
        compDb = autoGainSmoothDb;
    }
    else
    {
        autoGainSmoothDb = 0.90f * autoGainSmoothDb;
    }

    const float polarity = phaseInvert ? -1.0f : 1.0f;
    const float gainLinear = juce::Decibels::decibelsToGain(outGainDb + compDb) * polarity;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = outputBuffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
            data[i] *= gainLinear;
    }
}

void HybridResonanceEQProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    syncParametersToDSP();
    setLatencySamples(spectralEngine.getLatencySamples());

    const bool bypassed = apvts.getRawParameterValue("global_bypass")->load() > 0.5f;
    if (bypassed)
        return;

    const int numSamples = buffer.getNumSamples();
    if (inputReferenceBuffer.getNumChannels() < 2 || inputReferenceBuffer.getNumSamples() < numSamples)
        inputReferenceBuffer.setSize(2, juce::jmax(1, numSamples), false, false, true);

    inputReferenceBuffer.copyFrom(0, 0, buffer, 0, 0, numSamples);
    inputReferenceBuffer.copyFrom(1, 0, buffer, 1, 0, numSamples);

    const float scale = apvts.getRawParameterValue("gain_scale")->load();
    eqEngine.processBlock(buffer, scale);
    spectralEngine.processBlock(buffer);

    const auto mode = static_cast<CharacterMode>(static_cast<int>(apvts.getRawParameterValue("character_mode")->load()));
    const float outGainDb = apvts.getRawParameterValue("output_gain")->load();
    const bool phaseInv = apvts.getRawParameterValue("phase_invert")->load() > 0.5f;
    const bool outAutoGain = apvts.getRawParameterValue("out_auto_gain")->load() > 0.5f;

    applyCharacterAndOutput(inputReferenceBuffer, buffer, mode, outGainDb, phaseInv, outAutoGain);
}

juce::AudioProcessorEditor* HybridResonanceEQProcessor::createEditor()
{
    return new HybridResonanceEQEditor(*this);
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
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HybridResonanceEQProcessor();
}
