#include "PluginEditor.h"
#include <cmath>

namespace
{
juce::String bandParam(int index, const juce::String& suffix)
{
    return "band_" + juce::String(index) + "_" + suffix;
}
} // namespace

ProLookAndFeel::ProLookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff121419));
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1c2028));
    setColour(juce::ComboBox::textColourId, juce::Colour(0xffe8edf5));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff313845));
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff191d24));
    setColour(juce::PopupMenu::textColourId, juce::Colour(0xffe8edf5));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff2c6bed));
    setColour(juce::Label::textColourId, juce::Colour(0xff9ea9bd));
    setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1f242d));
    setColour(juce::TextButton::textColourOffId, juce::Colour(0xffdfe6f2));
}

void ProLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                      float sliderPosProportional, float rotaryStartAngle,
                                      float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                         static_cast<float>(width), static_cast<float>(height)).reduced(3.0f);
    const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float arcRadius = juce::jmax(4.0f, radius - 2.5f);
    const float angle = rotaryStartAngle + juce::jlimit(0.0f, 1.0f, sliderPosProportional) * (rotaryEndAngle - rotaryStartAngle);

    const float alphaScale = slider.isEnabled() ? 1.0f : 0.35f;

    juce::Path bgArc;
    bgArc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(juce::Colour(0xff2b313d).withAlpha(alphaScale));
    g.strokePath(bgArc, juce::PathStrokeType(2.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path valArc;
    const bool isBipolar = (slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0);
    const float startArc = isBipolar ? (0.5f * (rotaryStartAngle + rotaryEndAngle)) : rotaryStartAngle;
    valArc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f, startArc, angle, true);
    g.setColour(juce::Colour(0xff4da3ff).withAlpha(alphaScale));
    g.strokePath(valArc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float bodyRadius = juce::jmax(3.0f, arcRadius - 4.0f);
    g.setColour(juce::Colour(0xff181c23).withAlpha(alphaScale));
    g.fillEllipse(centre.x - bodyRadius, centre.y - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f);
    g.setColour(juce::Colour(0xff475264).withAlpha(alphaScale));
    g.drawEllipse(centre.x - bodyRadius, centre.y - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f, 1.0f);

    juce::Path pointer;
    const float pointerLen = bodyRadius * 0.76f;
    pointer.startNewSubPath(centre.x, centre.y);
    pointer.lineTo(centre.x + pointerLen * std::sin(angle), centre.y - pointerLen * std::cos(angle));
    g.setColour(juce::Colour(0xfff5d042).withAlpha(alphaScale));
    g.strokePath(pointer, juce::PathStrokeType(1.9f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void ProLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                                  int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                                  juce::ComboBox& box)
{
    auto r = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)).reduced(0.5f);
    const float alphaScale = box.isEnabled() ? 1.0f : 0.4f;

    g.setColour(box.findColour(juce::ComboBox::backgroundColourId).withAlpha(alphaScale));
    g.fillRoundedRectangle(r, 4.0f);
    g.setColour(box.findColour(juce::ComboBox::outlineColourId).withAlpha(alphaScale));
    g.drawRoundedRectangle(r, 4.0f, 1.0f);

    const float arrowX = r.getRight() - 11.0f;
    const float arrowY = r.getCentreY();
    juce::Path arrow;
    arrow.addTriangle(arrowX - 3.2f, arrowY - 1.4f, arrowX + 3.2f, arrowY - 1.4f, arrowX, arrowY + 2.2f);
    g.setColour(juce::Colour(0xff9aa5b8).withAlpha(alphaScale));
    g.fillPath(arrow);
}

void ProLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                      bool shouldDrawButtonAsHighlighted, bool /*shouldDrawButtonAsDown*/)
{
    auto r = button.getLocalBounds().toFloat().reduced(1.0f);
    const bool active = button.getToggleState();
    const float alphaScale = button.isEnabled() ? 1.0f : 0.4f;

    juce::Colour fill = active ? juce::Colour(0xff234b7a) : juce::Colour(0xff1b1f27);
    if (shouldDrawButtonAsHighlighted && button.isEnabled())
        fill = fill.brighter(0.12f);

    juce::Colour border = active ? juce::Colour(0xff52a8ff) : juce::Colour(0xff313845);
    juce::Colour textCol = active ? juce::Colour(0xffffffff) : juce::Colour(0xffb5bfd0);

    g.setColour(fill.withAlpha(alphaScale));
    g.fillRoundedRectangle(r, 4.0f);
    g.setColour(border.withAlpha(alphaScale));
    g.drawRoundedRectangle(r, 4.0f, 1.0f);

    g.setColour(textCol.withAlpha(alphaScale));
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText(button.getButtonText(), r, juce::Justification::centred, true);
}

void ProLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                          const juce::Colour& /*backgroundColour*/,
                                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto r = button.getLocalBounds().toFloat().reduced(1.0f);
    const float alphaScale = button.isEnabled() ? 1.0f : 0.4f;

    juce::Colour col = juce::Colour(0xff20252e);
    if (shouldDrawButtonAsDown && button.isEnabled())
        col = juce::Colour(0xff355c8c);
    else if (shouldDrawButtonAsHighlighted && button.isEnabled())
        col = col.brighter(0.14f);

    g.setColour(col.withAlpha(alphaScale));
    g.fillRoundedRectangle(r, 4.0f);
    g.setColour(juce::Colour(0xff353d4b).withAlpha(alphaScale));
    g.drawRoundedRectangle(r, 4.0f, 1.0f);
}

HybridResonanceEQEditor::HybridResonanceEQEditor(HybridResonanceEQProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&proLookAndFeel);
    setResizable(true, true);
    setResizeLimits(1060, 620, 1920, 1150);
    setSize(1320, 740);

    preSpectrumDb.fill(-120.0f);
    postSpectrumDb.fill(-120.0f);
    reductionDb.fill(0.0f);

    for (int i = 0; i < dspcore::ParametricEQEngine::maxBands; ++i)
        bandSelectorBox.addItem("Band " + juce::String(i + 1), i + 1);
    bandSelectorBox.setSelectedId(1, juce::dontSendNotification);
    bandSelectorBox.onChange = [this]
    {
        selectBand(juce::jmax(0, bandSelectorBox.getSelectedId() - 1));
    };
    addAndMakeVisible(bandSelectorBox);

    addBandButton.onClick = [this] { addNewBandAt(1000.0f, 0.0f); };
    addAndMakeVisible(addBandButton);

    delBandButton.onClick = [this]
    {
        if (auto* param = audioProcessor.apvts.getParameter(bandParam(selectedBandIndex, "enabled")))
            param->setValueNotifyingHost(0.0f);
        updateContextualBandControls();
        repaint();
    };
    addAndMakeVisible(delBandButton);

    copyBandButton.onClick = [this] { copySelectedBand(); };
    addAndMakeVisible(copyBandButton);

    pasteBandButton.onClick = [this] { pasteToSelectedBand(); };
    addAndMakeVisible(pasteBandButton);

    abButton.onClick = [this] { toggleABState(); };
    addAndMakeVisible(abButton);

    resolutionBox.addItemList({ "Res: Low", "Res: Med", "Res: High", "Res: Max" }, 1);
    addAndMakeVisible(resolutionBox);
    resolutionAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.apvts, "ana_resolution", resolutionBox);

    speedBox.addItemList({ "Spd: Slow", "Spd: Med", "Spd: Fast" }, 1);
    addAndMakeVisible(speedBox);
    speedAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.apvts, "ana_speed", speedBox);

    tiltBox.addItemList({ "Tilt: 0 dB", "Tilt: 3 dB", "Tilt: 4.5 dB", "Tilt: 6 dB" }, 1);
    addAndMakeVisible(tiltBox);
    tiltAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.apvts, "ana_tilt", tiltBox);

    rangeBox.addItemList({ "+/- 6 dB", "+/- 12 dB", "+/- 30 dB" }, 1);
    addAndMakeVisible(rangeBox);
    rangeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.apvts, "display_range", rangeBox);

    characterBox.addItemList({ "Char: Clean", "Char: Subtle", "Char: Warm" }, 1);
    addAndMakeVisible(characterBox);
    characterAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.apvts, "character_mode", characterBox);

    addAndMakeVisible(showPreToggle);
    showPreAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, "ana_show_pre", showPreToggle);

    addAndMakeVisible(showPostToggle);
    showPostAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, "ana_show_post", showPostToggle);

    autoGainToggle.setButtonText("Out Auto");
    addAndMakeVisible(autoGainToggle);
    autoGainAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, "out_auto_gain", autoGainToggle);

    addAndMakeVisible(phaseInvertToggle);
    phaseInvertAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, "phase_invert", phaseInvertToggle);

    addAndMakeVisible(bypassToggle);
    bypassAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, "global_bypass", bypassToggle);

    auto configureRotary = [this](juce::Slider& s, juce::Label& lbl, const juce::String& text, const juce::String& suffix)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 15);
        s.setTextValueSuffix(suffix);
        s.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffe8edf5));
        s.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff171b22));
        s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff2c3340));
        addAndMakeVisible(s);

        lbl.setText(text, juce::dontSendNotification);
        lbl.setJustificationType(juce::Justification::centred);
        lbl.setFont(juce::FontOptions(10.0f));
        addAndMakeVisible(lbl);
    };

    configureRotary(gainScaleSlider, gainScaleLabel, "SCALE", "");
    gainScaleAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "gain_scale", gainScaleSlider);

    configureRotary(outputGainSlider, outputGainLabel, "OUT", " dB");
    outputGainAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "output_gain", outputGainSlider);

    addAndMakeVisible(bandEnabledToggle);
    bandShapeBox.addItemList({ "Bell", "Low Shelf", "High Shelf", "Low Cut", "High Cut", "Notch", "Band Pass", "Tilt Shelf" }, 1);
    bandShapeBox.onChange = [this] { updateContextualBandControls(); };
    addAndMakeVisible(bandShapeBox);

    bandRouteBox.addItemList({ "Stereo", "Mid", "Side", "Left", "Right" }, 1);
    addAndMakeVisible(bandRouteBox);

    addAndMakeVisible(bandDynToggle);
    bandDynToggle.onClick = [this] { updateContextualBandControls(); };

    addAndMakeVisible(bandAutoThreshToggle);
    bandAutoThreshToggle.onClick = [this] { updateContextualBandControls(); };

    addAndMakeVisible(bandSpecToggle);
    addAndMakeVisible(bandSoloToggle);

    configureRotary(bandFreqSlider, bandFreqLabel, "FREQ", " Hz");
    configureRotary(bandGainSlider, bandGainLabel, "GAIN", " dB");
    configureRotary(bandQSlider, bandQLabel, "Q", "");
    configureRotary(bandSlopeSlider, bandSlopeLabel, "SLOPE", " dB");
    configureRotary(bandDynRangeSlider, bandDynRangeLabel, "DYN RNG", " dB");
    configureRotary(bandDynThreshSlider, bandDynThreshLabel, "THRESH", " dB");
    configureRotary(bandDynAttackSlider, bandDynAttackLabel, "ATTACK", " ms");
    configureRotary(bandDynReleaseSlider, bandDynReleaseLabel, "RELEASE", " ms");

    addAndMakeVisible(specDeltaToggle);
    specDeltaAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, "spec_delta", specDeltaToggle);

    configureRotary(specDepthSlider, specDepthLabel, "SPEC DEPTH", "");
    specDepthAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "spec_depth", specDepthSlider);

    configureRotary(specSharpSlider, specSharpLabel, "SPEC SHARP", "");
    specSharpAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "spec_sharpness", specSharpSlider);

    configureRotary(specSmoothSlider, specSmoothLabel, "SPEC SMOOTH", "");
    specSmoothAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "spec_smoothing", specSmoothSlider);

    configureRotary(specMixSlider, specMixLabel, "SPEC MIX", "");
    specMixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "spec_mix", specMixSlider);

    selectBand(0);
    startTimerHz(30);
}

HybridResonanceEQEditor::~HybridResonanceEQEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

juce::Colour HybridResonanceEQEditor::getBandColour(int index) noexcept
{
    static constexpr std::array<juce::uint32, 16> palette {
        0xfff5c242, 0xff4da3ff, 0xff45d483, 0xffff6b6b,
        0xffb97aff, 0xffff9f43, 0xff2ed5d5, 0xffff78b5,
        0xff8be9fd, 0xffbd93f9, 0xffffb86c, 0xff50fa7b,
        0xffff5555, 0xff6272a4, 0xfff1fa8c, 0xff80ffea
    };
    return juce::Colour(palette[static_cast<size_t>(juce::jlimit(0, 15, index))]);
}

void HybridResonanceEQEditor::updateContextualBandControls()
{
    const auto shape = static_cast<dspcore::FilterShape>(juce::jmax(0, bandShapeBox.getSelectedItemIndex()));
    const bool hasSlope = dspcore::BandFilter::supportsSlope(shape);
    const bool hasGain = dspcore::BandFilter::supportsGain(shape);
    const bool dynOn = bandDynToggle.getToggleState();
    const bool autoThresh = bandAutoThreshToggle.getToggleState();
    const bool specOn = bandSpecToggle.getToggleState();

    bandSlopeSlider.setEnabled(hasSlope);
    bandSlopeLabel.setAlpha(hasSlope ? 1.0f : 0.35f);

    bandGainSlider.setEnabled(hasGain);
    bandGainLabel.setAlpha(hasGain ? 1.0f : 0.35f);

    bandAutoThreshToggle.setEnabled(dynOn);
    bandDynRangeSlider.setEnabled(dynOn);
    bandDynRangeLabel.setAlpha(dynOn ? 1.0f : 0.35f);

    bandDynThreshSlider.setEnabled(dynOn && !autoThresh);
    bandDynThreshLabel.setAlpha((dynOn && !autoThresh) ? 1.0f : 0.35f);

    bandDynAttackSlider.setEnabled(dynOn);
    bandDynAttackLabel.setAlpha(dynOn ? 1.0f : 0.35f);

    bandDynReleaseSlider.setEnabled(dynOn);
    bandDynReleaseLabel.setAlpha(dynOn ? 1.0f : 0.35f);

    specDeltaToggle.setEnabled(specOn);
    specDepthSlider.setEnabled(specOn);
    specDepthLabel.setAlpha(specOn ? 1.0f :
