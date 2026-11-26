#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include <array>

class ProLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ProLookAndFeel();

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override;

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};

class HybridResonanceEQEditor : public juce::AudioProcessorEditor,
                                  private juce::Timer
{
public:
    explicit HybridResonanceEQEditor(HybridResonanceEQProcessor&);
    ~HybridResonanceEQEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

private:
    void timerCallback() override;
    void selectBand(int bandIndex);
    void syncBandControlsFromParams();
    void updateBandParamFromGraph(int bandIndex, float freqHz, float gainDb);
    void addNewBandAt(float freqHz, float gainDb);

    float xToFreqHz(float x) const noexcept;
    float freqHzToX(float freqHz) const noexcept;
    float yToGainDb(float y) const noexcept;
    float gainDbToY(float gainDb) const noexcept;
    float getRangeDb() const noexcept;
    int hitTestBandNode(juce::Point<float> pos) const noexcept;

    static juce::Colour getBandColour(int index) noexcept;

    HybridResonanceEQProcessor& audioProcessor;
    ProLookAndFeel proLookAndFeel;

    juce::Rectangle<int> topBarBounds;
    juce::Rectangle<int> graphBounds;
    juce::Rectangle<int> bandBarBounds;
    juce::Rectangle<int> bottomBarBounds;

    int selectedBandIndex = 0;
    int draggedBandIndex = -1;

    // Top bar & Analyzer controls
    juce::ComboBox bandSelectorBox;
    juce::TextButton addBandButton { "+ Band" };
    juce::TextButton delBandButton { "Del" };

    juce::ComboBox resolutionBox;
    juce::ComboBox speedBox;
    juce::ComboBox tiltBox;
    juce::ComboBox rangeBox;

    juce::ToggleButton showPreToggle { "Pre" };
    juce::ToggleButton showPostToggle { "Post" };
    juce::ToggleButton bypassToggle { "Bypass" };

    juce::Slider outputGainSlider;
    juce::Label outputGainLabel;

    // Selected Band controls
    juce::ToggleButton bandEnabledToggle { "On" };
    juce::ComboBox bandShapeBox;
    juce::ComboBox bandRouteBox;
    juce::ToggleButton bandDynToggle { "Dyn" };
    juce::ToggleButton bandSoloToggle { "Solo" };

    juce::Slider bandFreqSlider;
    juce::Slider bandGainSlider;
    juce::Slider bandQSlider;
    juce::Slider bandSlopeSlider;
    juce::Slider bandDynRangeSlider;
    juce::Slider bandDynThreshSlider;

    juce::Label bandFreqLabel;
    juce::Label bandGainLabel;
    juce::Label bandQLabel;
    juce::Label bandSlopeLabel;
    juce::Label bandDynRangeLabel;
    juce::Label bandDynThreshLabel;

    // Spectral Resonance controls
    juce::ToggleButton specEnabledToggle { "Spectral" };
    juce::ToggleButton specDeltaToggle { "Delta" };

    juce::Slider specDepthSlider;
    juce::Slider specSharpSlider;
    juce::Slider specSmoothSlider;
    juce::Slider specMixSlider;
    juce::Slider specMinFreqSlider;
    juce::Slider specMaxFreqSlider;

    juce::Label specDepthLabel;
    juce::Label specSharpLabel;
    juce::Label specSmoothLabel;
    juce::Label specMixLabel;
    juce::Label specMinFreqLabel;
    juce::Label specMaxFreqLabel;

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> resolutionAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> speedAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> tiltAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> rangeAttach;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> showPreAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> showPostAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputGainAttach;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> specEnabledAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> specDeltaAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> specDepthAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> specSharpAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> specSmoothAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> specMixAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> specMinFreqAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> specMaxFreqAttach;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bandEnabledAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> bandShapeAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> bandRouteAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bandDynAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bandSoloAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bandFreqAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bandGainAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bandQAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bandSlopeAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bandDynRangeAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bandDynThreshAttach;

    std::array<float, dspcore::SpectralResonanceEngine::numBins> preSpectrumDb {};
    std::array<float, dspcore::SpectralResonanceEngine::numBins> postSpectrumDb {};
    std::array<float, dspcore::SpectralResonanceEngine::numBins> reductionDb {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HybridResonanceEQEditor)
};
