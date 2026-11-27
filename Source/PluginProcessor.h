#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/ParametricEQEngine.h"
#include "dsp/SpectralResonanceEngine.h"

class HybridResonanceEQProcessor : public juce::AudioProcessor
{
public:
    HybridResonanceEQProcessor();
    ~HybridResonanceEQProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

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

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts;

    void getVisualData(std::array<float, dspcore::SpectralResonanceEngine::numBins>& outPreDb,
                       std::array<float, dspcore::SpectralResonanceEngine::numBins>& outPostDb,
                       std::array<float, dspcore::SpectralResonanceEngine::numBins>& outReductionDb) const noexcept
    {
        spectralEngine.getSpectra(outPreDb, outPostDb, outReductionDb);
    }

    float getGainScale() const noexcept
    {
        return apvts.getRawParameterValue("gain_scale")->load();
    }

    float getCombinedEQResponseDb(double freqHz) const noexcept
    {
        return eqEngine.getCombinedResponseDb(freqHz, getGainScale());
    }

    float getBandEQResponseDb(int bandIndex, double freqHz) const noexcept
    {
        return eqEngine.getBandResponseDb(bandIndex, freqHz, getGainScale());
    }

    float getBandDynamicGainDb(int bandIndex) const noexcept
    {
        return eqEngine.getBandDynamicGainDb(bandIndex);
    }

    float getAutoGainCompensationDb() const noexcept
    {
        return autoGainSmoothDb;
    }

    double getCurrentSampleRateHz() const noexcept { return currentSampleRate; }

private:
    enum class CharacterMode : int
    {
        Clean = 0,
        Subtle,
        Warm
    };

    void syncParametersToDSP();
    void applyCharacterAndOutput(const juce::AudioBuffer<float>& inputRef,
                                 juce::AudioBuffer<float>& outputBuffer,
                                 CharacterMode mode,
                                 float outGainDb,
                                 bool phaseInvert,
                                 bool autoGainEnabled) noexcept;

    dspcore::ParametricEQEngine eqEngine;
    dspcore::SpectralResonanceEngine spectralEngine;
    juce::AudioBuffer<float> inputReferenceBuffer;
    double currentSampleRate = 48000.0;
    float autoGainSmoothDb = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HybridResonanceEQProcessor)
};
