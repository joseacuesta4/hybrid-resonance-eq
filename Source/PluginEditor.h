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
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    HybridResonanceEQProcessor& processorRef;

    struct ThemeColors
    {
        juce::Colour bgDeep         { 0xff0a0d12 };
        juce::Colour panelGlass     { 0xcc131822 };
        juce::Colour panelElevated  { 0xe01a2130 };
        juce::Colour borderSubtle   { 0x24ffffff };
        juce::Colour borderAccent   { 0x5560a5fa };
        juce::Colour textPrimary    { 0xfff1f5f9 };
        juce::Colour textMuted      { 0x9994a3b8 };
        juce::Colour gridMajor      { 0x1cffffff };
        juce::Colour gridMinor      { 0x0bffffff };
        juce::Colour specMainStroke { 0xcc7dd3fc };
        juce::Colour specMainFill   { 0x2238bdf8 };
        juce::Colour redStroke      { 0xf0fb7185 };
        juce::Colour redFill        { 0x38f43f5e };
    } theme;

    juce::Rectangle<int> topBarBounds;
    juce::Rectangle<int> graphBounds;
    juce::Rectangle<int> bottomStripBounds;

    // Top bar: Title, Tilt, Bypass (all English)
    juce::Label titleLabel;
    juce::Label tiltLabel;
    juce::ComboBox tiltCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> tiltAttachment;
    juce::Label specResolutionLabel;
    juce::ComboBox specResolutionCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> specResolutionAttachment;

    juce::TextButton bypassButton { "Bypass" };
    juce::ToggleButton preAnalyzerButton { "Pre" };
    juce::ToggleButton postAnalyzerButton { "Post" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> preAnalyzerAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> postAnalyzerAttachment;

    // Bottom band strip (Pro-Q 4 style per-band controls in English, without manual Freq/Q sliders)
    juce::Label selectedBandLabel;
    juce::ComboBox shapeCombo;
    juce::ComboBox characterCombo;
    juce::ToggleButton dynToggle { "Dynamic" };
    juce::ToggleButton specToggle { "Spectral" };
    juce::ToggleButton soloToggle { "Solo" };
    juce::ToggleButton bandBypassToggle { "Mute Band" };
    juce::TextButton removeBandButton { "Remove" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> characterAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> dynAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> specAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> soloAttachment;

    int selectedBand = 0;
    bool bandStripVisible = true;
    int draggingBand = -1;

    static constexpr int renderPoints = 512;
    std::array<float, HybridResonanceEQProcessor::analyzerNumBins> rawPreBins {};
    std::array<float, HybridResonanceEQProcessor::analyzerNumBins> rawPostBins {};
    std::array<float, HybridResonanceEQProcessor::analyzerNumBins> rawRedBins {};

    std::array<float, renderPoints> smoothMainCurve {};
    std::array<float, renderPoints> smoothPreCurve {};
    std::array<float, renderPoints> fastRedCurve {};

    void timerCallback() override;
    void selectBand(int bandIndex, bool showStrip = true);
    void rebindSelectedBandControls();
    void syncBandBypassToggle();
    void createBandAt(juce::Point<float> pos, bool spectral = false, bool dynamic = false);
    void showBandContextMenu(int bandIndex, juce::Point<float> pos);
    void showGraphContextMenu(juce::Point<float> pos);

    float freqToX(float freqHz) const noexcept;
    float xToFreq(float x) const noexcept;
    float dbToY(float db, float maxDb) const noexcept;
    float yToDb(float y, float maxDb) const noexcept;
    float getDisplayRangeDb() const noexcept;
    juce::Colour getBandColour(int bandIndex) const noexcept;
    int hitTestBandNode(juce::Point<float> pos) const noexcept;
    int findFreeBandSlot() const noexcept;

    void drawModernBackground(juce::Graphics& g) const;
    void drawGraphGridAndCurves(juce::Graphics& g);
    void drawBandNodes(juce::Graphics& g) const;
    void drawBottomBandStrip(juce::Graphics& g);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HybridResonanceEQEditor)
};
