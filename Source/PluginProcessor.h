#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <vector>

#if __has_include("dsp/ParametricEQEngine.h")
 #include "dsp/ParametricEQEngine.h"
#elif __has_include("ParametricEQEngine.h")
 #include "ParametricEQEngine.h"
#endif

#if __has_include("dsp/SpectralResonanceEngine.h")
 #include "dsp/SpectralResonanceEngine.h"
#elif __has_include("SpectralResonanceEngine.h")
 #include "SpectralResonanceEngine.h"
#endif

#ifndef HYBRID_RESONANCE_FALLBACK_DSP
#define HYBRID_RESONANCE_FALLBACK_DSP 1
#endif

namespace dspcore
{
#if !defined(PARAMETRIC_EQ_ENGINE_DEFINED)
enum class FilterShape
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

struct BandFilter
{
    static constexpr bool supportsSlope(FilterShape shape) noexcept
    {
        return shape == FilterShape::LowCut || shape == FilterShape::HighCut ||
               shape == FilterShape::LowShelf || shape == FilterShape::HighShelf ||
               shape == FilterShape::TiltShelf;
    }

    static constexpr bool supportsGain(FilterShape shape) noexcept
    {
        return shape != FilterShape::LowCut && shape != FilterShape::HighCut &&
               shape != FilterShape::BandPass;
    }
};

struct ParametricEQEngine
{
    static constexpr int maxBands = 16;
};
#endif

#if !defined(SPECTRAL_RESONANCE_ENGINE_DEFINED)
struct SpectralResonanceEngine
{
    static constexpr int fftSize = 2048;
    static constexpr int numBins = fftSize / 2 + 1;
};
#endif
} // namespace dspcore

class HybridResonanceEQProcessor : public juce::AudioProcessor
{
public:
    HybridResonanceEQProcessor();
    ~HybridResonanceEQProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    double getCurrentSampleRateHz() const noexcept;
    float getBandEQResponseDb(int bandIndex, double freqHz) const noexcept;
    float getCombinedEQResponseDb(double freqHz) const noexcept;
    float getBandDynamicGainDb(int bandIndex) const noexcept;
    float getAutoGainCompensationDb() const noexcept;

    void getVisualData(std::array<float, dspcore::SpectralResonanceEngine::numBins>& preOut,
                       std::array<float, dspcore::SpectralResonanceEngine::numBins>& postOut,
                       std::array<float, dspcore::SpectralResonanceEngine::numBins>& redOut) const noexcept;

    juce::AudioProcessorValueTreeState apvts;

private:
    struct BandRuntimeState
    {
        float envLevelDb = -120.0f;
        float currentDynDb = 0.0f;
        juce::dsp::IIR::Filter<float> filterL;
        juce::dsp::IIR::Filter<float> filterR;
    };

    double currentSampleRate = 48000.0;
    int currentBlockSize = 512;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> bypassMixSmooth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> outputGainSmooth;

    std::array<BandRuntimeState, dspcore::ParametricEQEngine::maxBands> bandStates;

    // Estado de FFT y reducción espectral
    static constexpr int fftOrder = 11; // 2048
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int hopSize = fftSize / 4; // 512 (75% overlap)
    static constexpr int numBins = fftSize / 2 + 1;

    juce::dsp::FFT forwardFFT { fftOrder };
    juce::dsp::WindowingFunction<float> hannWindow { static_cast<size_t>(fftSize),
                                                     juce::dsp::WindowingFunction<float>::hann,
                                                     false };

    std::vector<float> fifoL;
    std::vector<float> fifoR;
    std::vector<float> fftWorkspace;
    std::vector<float> olaBufferL;
    std::vector<float> olaBufferR;
    std::vector<float> olaNormBuffer;
    std::vector<float> binEnvelope;
    std::vector<float> binReductionDb;
    int fifoIndex = 0;

    std::array<float, numBins> preSpectrumSmooth;
    std::array<float, numBins> postSpectrumSmooth;
    std::array<float, numBins> reductionSmooth;

    std::atomic<float> autoGainCompDb { 0.0f };

    void resetDSPState();
    void updateBandFilters(double sampleRate);
    float computeSingleBandMagnitudeDb(int bandIndex, double freqHz) const noexcept;
    float applyBandCharacterSample(float sample, int characterMode) const noexcept;
    void processSpectralStage(juce::AudioBuffer<float>& buffer,
                              bool anySpecEnabled,
                              float depth,
                              float sharpness,
                              float smoothing,
                              float mix,
                              bool deltaMode);
    void updateSpectrumVisuals(const juce::AudioBuffer<float>& preBuffer,
                               const juce::AudioBuffer<float>& postBuffer);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HybridResonanceEQProcessor)
};
