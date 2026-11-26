#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include <array>

class HybridResonanceEQAudioProcessorEditor : public juce::AudioProcessorEditor,
                                             private juce::Timer
{
public:
    explicit HybridResonanceEQAudioProcessorEditor(HybridResonanceEQAudioProcessor&);
    ~HybridResonanceEQAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    void syncControlsToSelectedBand();

    float freqToX(float freqHz) const noexcept;
    float xToFreq(float x) const noexcept;
    float dbToY(float db) const noexcept;
    float yToDb(float y) const noexcept;
    float specDbToY(float specDb) const noexcept;

    int hitTestBandNode(juce::Point<float> pos) const noexcept;

    HybridResonanceEQAudioProcessor& audioProcessor;

    juce::Rectangle<float> plotBounds;
    juce::Rectangle<float> floatingBarBounds;
    juce::Rectangle<float> bottomSpecBarBounds;

    int selectedBandIndex = 0;
    int draggingBandIndex = -1;

    std::array<float, HybridResonanceEQAudioProcessor::kSpectrumBins> spectrumDb {};
    std::array<float, HybridResonanceEQAudioProcessor::kNumBands> dynOffsetsDb {};

    // Controles del panel flotante inferior (banda activa)
    juce::ComboBox bandTypeBox;
    juce::ComboBox bandRouteBox;
    juce::Slider freqSlider;
    juce::Slider gainSlider;
    juce::Slider qSlider;
    juce::ToggleButton dynButton { "Dyn" };
    juce::ToggleButton soloButton { "Solo" };
    juce::ToggleButton bandEnableButton { "On" };

    // Controles inferiores del motor espectral (estilo Soothe 3)
    juce::ToggleButton specEnableButton { "Spectral" };
    juce::ToggleButton specDeltaButton { "Delta" };
    juce::Slider depthSlider;
    juce::Slider sharpSlider;
    juce::Slider smoothSlider;
    juce::Slider mixSlider;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<ButtonAttachment> bandEnAtt, bandSoloAtt, bandDynAtt;
    std::unique_ptr<ComboAttachment> bandTypeAtt, bandRouteAtt;
    std::unique_ptr<SliderAttachment> freqAtt, gainAtt, qAtt;

    ButtonAttachment specEnAtt { audioProcessor.getAPVTS(), "spec_en", specEnableButton };
    ButtonAttachment specDeltaAtt { audioProcessor.getAPVTS(), "spec_delta", specDeltaButton };
    SliderAttachment depthAtt { audioProcessor.getAPVTS(), "spec_depth", depthSlider };
    SliderAttachment sharpAtt { audioProcessor.getAPVTS(), "spec_sharp", sharpSlider };
    SliderAttachment smoothAtt { audioProcessor.getAPVTS(), "spec_smooth", smoothSlider };
    SliderAttachment mixAtt { audioProcessor.getAPVTS(), "spec_mix", mixSlider };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HybridResonanceEQAudioProcessorEditor)
};
