#include "PluginProcessor.h"
#include "PluginEditor.h"

HybridResonanceEQAudioProcessor::HybridResonanceEQAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMS", createParameterLayout())
{
    for (auto& buf : spectrumBuffers)
        buf.fill(-120.0f);
    for (auto& buf : dynBuffers)
        buf.fill(0.0f);
}

juce::AudioProcessorValueTreeState::ParameterLayout HybridResonanceEQAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // 16 bandas estilo Pro-Q 4
    for (int b = 0; b < kNumBands; ++b)
    {
        const bool defaultEnabled = (b < 4);
        const float defaultFreq = (b == 0) ? 80.0f
                                : (b == 1) ? 350.0f
                                : (b == 2) ? 1800.0f
                                : (b == 3) ? 6500.0f
                                : 1000.0f;

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            bandEnabledId(b), "Band " + juce::String(b + 1) + " Enable", defaultEnabled));
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            bandSoloId(b), "Band " + juce::String(b + 1) + " Solo", false));
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            bandDynEnId(b), "Band " + juce::String(b + 1) + " Dynamic", false));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            bandTypeId(b), "Band " + juce::String(b + 1) + " Type",
            juce::StringArray { "Bell", "Low Shelf", "High Shelf", "Low Cut", "High Cut", "Notch" }, 0));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            bandRouteId(b), "Band " + juce::String(b + 1) + " Route",
            juce::StringArray { "Stereo", "Left", "Right", "Mid", "Side" }, 0));

        auto freqRange = juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.25f);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            bandFreqId(b), "Band " + juce::String(b + 1) + " Freq", freqRange, defaultFreq));

        auto gainRange = juce::NormalisableRange<float>(-24.0f, 24.0f, 0.05f);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            bandGainId(b), "Band " + juce::String(b + 1) + " Gain", gainRange, 0.0f));

        auto qRange = juce::NormalisableRange<float>(0.15f, 12.0f, 0.01f, 0.4f);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            bandQId(b), "Band " + juce::String(b + 1) + " Q", qRange, 1.0f));

        auto thrRange = juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            bandThreshId(b), "Band " + juce::String(b + 1) + " Threshold", thrRange, -18.0f));

        auto rngRange = juce::NormalisableRange<float>(-18.0f, 18.0f, 0.1f);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            bandRangeId(b), "Band " + juce::String(b + 1) + " Dyn Range", rngRange, -6.0f));
    }

    // Motor espectral de resonancias estilo Soothe 3
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "spec_en", "Spectral Enable", true));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "spec_delta", "Spectral Delta", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "spec_depth", "Spectral Depth", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.55f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "spec_sharp", "Spectral Sharpness", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.60f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "spec_smooth", "Spectral Smoothing", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.50f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "spec_mix", "Spectral Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "spec_low", "Spectral Low Hz", juce::NormalisableRange<float>(30.0f, 4000.0f, 1.0f, 0.35f), 120.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "spec_high", "Spectral High Hz", juce::NormalisableRange<float>(500.0f, 20000.0f, 1.0f, 0.35f), 16000.0f));

    return { params.begin(), params.end() };
}

bool HybridResonanceEQAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo() ||
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void HybridResonanceEQAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    eqEngine.prepare(sampleRate, samplesPerBlock);
    spectralEngine.prepare(sampleRate, samplesPerBlock);
    syncEnginesFromParameters();
    setLatencySamples(spectralEngine.getLatencySamples());
}

void HybridResonanceEQAudioProcessor::syncEnginesFromParameters() noexcept
{
    for (int b = 0; b < kNumBands; ++b)
    {
        dspcore::BandConfig cfg;
        cfg.enabled = apvts.getRawParameterValue(bandEnabledId(b))->load() > 0.5f;
        cfg.solo    = apvts.getRawParameterValue(bandSoloId(b))->load() > 0.5f;
        cfg.dynamicEnabled = apvts.getRawParameterValue(bandDynEnId(b))->load() > 0.5f;

        cfg.type = static_cast<dspcore::BandType>(
            static_cast<int>(apvts.getRawParameterValue(bandTypeId(b))->load()));
        cfg.route = static_cast<dspcore::ChannelRoute>(
            static_cast<int>(apvts.getRawParameterValue(bandRouteId(b))->load()));

        cfg.freqHz = apvts.getRawParameterValue(bandFreqId(b))->load();
        cfg.gainDb = apvts.getRawParameterValue(bandGainId(b))->load();
        cfg.q      = apvts.getRawParameterValue(bandQId(b))->load();
        cfg.thresholdDb = apvts.getRawParameterValue(bandThreshId(b))->load();
        cfg.dynamicRangeDb = apvts.getRawParameterValue(bandRangeId(b))->load();

        eqEngine.setBandConfig(b, cfg);
    }

    dspcore::SpectralConfig sCfg;
    sCfg.enabled     = apvts.getRawParameterValue("spec_en")->load() > 0.5f;
    sCfg.deltaListen = apvts.getRawParameterValue("spec_delta")->load() > 0.5f;
    sCfg.depth       = apvts.getRawParameterValue("spec_depth")->load();
    sCfg.sharpness   = apvts.getRawParameterValue("spec_sharp")->load();
    sCfg.smoothing   = apvts.getRawParameterValue("spec_smooth")->load();
    sCfg.mix         = apvts.getRawParameterValue("spec_mix")->load();
    sCfg.minFreqHz   = apvts.getRawParameterValue("spec_low")->load();
    sCfg.maxFreqHz   = apvts.getRawParameterValue("spec_high")->load();

    spectralEngine.setConfig(sCfg);
}

void HybridResonanceEQAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    syncEnginesFromParameters();

    const int expectedLatency = spectralEngine.getLatencySamples();
    if (getLatencySamples() != expectedLatency)
        setLatencySamples(expectedLatency);

    eqEngine.processBlock(buffer);
    spectralEngine.processBlock(buffer);

    // Publica espectro y offsets dinámicos en doble buffer para la GUI (30 FPS)
    spectralEngine.getMagnitudeSpectrumDb(spectrumBuffers[static_cast<size_t>(spectrumWriteIndex)]);
    activeSpectrumReadIndex.store(spectrumWriteIndex, std::memory_order_release);
    spectrumWriteIndex = (spectrumWriteIndex == 0) ? 1 : 0;

    for (int b = 0; b < kNumBands; ++b)
        dynBuffers[static_cast<size_t>(dynWriteIndex)][static_cast<size_t>(b)] = eqEngine.getBandDynamicGainDb(b);
    activeDynReadIndex.store(dynWriteIndex, std::memory_order_release);
    dynWriteIndex = (dynWriteIndex == 0) ? 1 : 0;
}

void HybridResonanceEQAudioProcessor::copyLatestSpectrumDb(std::array<float, kSpectrumBins>& dest) const noexcept
{
    const int idx = activeSpectrumReadIndex.load(std::memory_order_acquire);
    dest = spectrumBuffers[static_cast<size_t>(idx)];
}

void HybridResonanceEQAudioProcessor::copyLatestDynamicOffsets(std::array<float, kNumBands>& dest) const noexcept
{
    const int idx = activeDynReadIndex.load(std::memory_order_acquire);
    dest = dynBuffers[static_cast<size_t>(idx)];
}

juce::AudioProcessorEditor* HybridResonanceEQAudioProcessor::createEditor()
{
    return new HybridResonanceEQAudioProcessorEditor(*this);
}

void HybridResonanceEQAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void HybridResonanceEQAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HybridResonanceEQAudioProcessor();
}
