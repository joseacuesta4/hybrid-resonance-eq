#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/ParametricEQEngine.h"
#include "dsp/SpectralResonanceEngine.h"
#include <array>
#include <atomic>

class HybridResonanceEQAudioProcessor : public juce::AudioProcessor
{
public:
    static constexpr int kNumBands = dspcore::ParametricEQEngine::kNumBands;
    static constexpr int kSpectrumBins = dspcore::SpectralResonanceEngine::numBins;

    HybridResonanceEQAudioProcessor();
    ~HybridResonanceEQAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }
    const dspcore::ParametricEQEngine& getEQEngine() const noexcept { return eqEngine; }

    // Lectura segura desde el hilo de GUI a 30 FPS
    void copyLatestSpectrumDb(std::array<float, kSpectrumBins>& dest) const noexcept;
    void copyLatestDynamicOffsets(std::array<float, kNumBands>& dest) const noexcept;

    // IDs de parámetros
    static juce::String bandEnabledId(int b) { return "b" + juce::String(b) + "_en"; }
    static juce::String bandSoloId(int b)    { return "b" + juce::String(b) + "_solo"; }
    static juce::String bandDynEnId(int b)   { return "b" + juce::String(b) + "_dyn"; }
    static juce::String bandTypeId(int b)    { return "b" + juce::String(b) + "_type"; }
    static juce::String bandRouteId(int b)   { return "b" + juce::String(b) + "_route"; }
    static juce::String bandFreqId(int b)    { return "b" + juce::String(b) + "_freq"; }
    static juce::String bandGainId(int b)    { return "b" + juce::String(b) + "_gain"; }
    static juce::String bandQId(int b)       { return "b" + juce::String(b) + "_q"; }
    static juce::String bandThreshId(int b)  { return "b" + juce::String(b) + "_thr"; }
    static juce::String bandRangeId(int b)   { return "b" + juce::String(b) + "_rng"; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void syncEnginesFromParameters() noexcept;

    juce::AudioProcessorValueTreeState apvts;
    dspcore::ParametricEQEngine eqEngine;
    dspcore::SpectralResonanceEngine spectralEngine;

    // Doble buffer lock-free para espectro y dinámica hacia la GUI
    std::array<std::array<float, kSpectrumBins>, 2> spectrumBuffers {};
    std::atomic<int> activeSpectrumReadIndex { 0 };
    int spectrumWriteIndex { 1 };

    std::array<std::array<float, kNumBands>, 2> dynBuffers {};
    std::atomic<int> activeDynReadIndex { 0 };
    int dynWriteIndex { 1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HybridResonanceEQAudioProcessor)
};
