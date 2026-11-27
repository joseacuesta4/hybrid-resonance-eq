#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include "PluginProcessor.h"

class HybridResonanceEQEditor : public juce::AudioProcessorEditor,
                                private juce::Timer
{
public:
    explicit HybridResonanceEQEditor(HybridResonanceEQProcessor&);
    ~HybridResonanceEQEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    HybridResonanceEQProcessor& processorRef;

    // Paleta moderna oscura con transparencias y acentos por banda
    struct ThemeColors
    {
        juce::Colour bgDeep        { 0xff0b0d12 };
        juce::Colour panelGlass    { 0xcc141821 };
        juce::Colour panelElevated { 0xd91a202c };
        juce::Colour borderSubtle  { 0x26ffffff };
        juce::Colour borderAccent  { 0x5958a6ff };
        juce::Colour textPrimary   { 0xfff0f4f8 };
        juce::Colour textMuted     { 0x999aa5b1 };
        juce::Colour gridMajor     { 0x1cffffff };
        juce::Colour gridMinor     { 0x0cffffff };
        juce::Colour specMainStroke{ 0xd97dd3fc };
        juce::Colour specMainFill  { 0x2638bdf8 };
        juce::Colour redStroke     { 0xe6fb7185 };
        juce::Colour redFill       { 0x33f43f5e };
    } theme;

    // Áreas calculadas en resized()
    juce::Rectangle<int> topBarBounds;
    juce::Rectangle<int> graphBounds;
    juce::Rectangle<int> bottomStripBounds;

    // Controles superiores (se mantiene Tilt; sin Band X / +Band / Del / Copy / Paste)
    juce::Label titleLabel;
    juce::Label tiltLabel;
    juce::ComboBox tiltCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> tiltAttachment;

    juce::TextButton bypassButton { "Bypass" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    // Controles de la franja dinámica inferior (visible cuando hay banda seleccionada)
    juce::Label selectedBandLabel;
    juce::ComboBox shapeCombo;
    juce::ComboBox characterCombo;
    juce::ToggleButton dynToggle { "Dyn" };
    juce::ToggleButton specToggle { "Spec" };
    juce::ToggleButton soloToggle { "Solo" };
    juce::Slider freqSlider;
    juce::Slider gainSlider;
    juce::Slider qSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> characterAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> dynAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> specAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> soloAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> freqAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> qAttachment;

    int selectedBand = 0;
    int draggingBand = -1;

    // Buffers visuales de alta resolución y suavizado diferenciado
    static constexpr int renderPoints = 512;
    std::array<float, dspcore::SpectralResonanceEngine::numBins> rawPreBins {};
    std::array<float, dspcore::SpectralResonanceEngine::numBins> rawPostBins {};
    std::array<float, dspcore::SpectralResonanceEngine::numBins> rawRedBins {};

    std::array<float, renderPoints> smoothMainCurve {};
    std::array<float, renderPoints> fastRedCurve {};

    void timerCallback() override;
    void selectBand(int bandIndex);
    void rebindSelectedBandControls();
    void showGraphContextMenu(juce::Point<float> pos);

    float freqToX(float freqHz) const noexcept;
    float xToFreq(float x) const noexcept;
    float dbToY(float db, float maxDb) const noexcept;
    float yToDb(float y, float maxDb) const noexcept;
    float getDisplayRangeDb() const noexcept;
    juce::Colour getBandColour(int bandIndex) const noexcept;
    int hitTestBandNode(juce::Point<float> pos) const noexcept;

    void drawModernBackground(juce::Graphics& g) const;
    void drawGraphGridAndCurves(juce::Graphics& g);
    void drawBandNodes(juce::Graphics& g) const;
    void drawBottomBandStrip(juce::Graphics& g);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HybridResonanceEQEditor)
};
