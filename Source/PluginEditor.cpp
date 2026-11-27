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
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff101216));
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff181c23));
    setColour(juce::ComboBox::textColourId, juce::Colour(0xffedf2fa));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff2d3442));
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff161a21));
    setColour(juce::PopupMenu::textColourId, juce::Colour(0xffedf2fa));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff2c6bed));
    setColour(juce::Label::textColourId, juce::Colour(0xff9aa5b8));
    setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1c2029));
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
    g.setColour(juce::Colour(0xff262c38).withAlpha(alphaScale));
    g.strokePath(bgArc, juce::PathStrokeType(2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path valArc;
    const bool isBipolar = (slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0);
    const float startArc = isBipolar ? (0.5f * (rotaryStartAngle + rotaryEndAngle)) : rotaryStartAngle;
    valArc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f, startArc, angle, true);
    g.setColour(juce::Colour(0xff52a8ff).withAlpha(alphaScale));
    g.strokePath(valArc, juce::PathStrokeType(2.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float bodyRadius = juce::jmax(3.0f, arcRadius - 4.0f);
    g.setColour(juce::Colour(0xff151920).withAlpha(alphaScale));
    g.fillEllipse(centre.x - bodyRadius, centre.y - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f);
    g.setColour(juce::Colour(0xff424c5e).withAlpha(alphaScale));
    g.drawEllipse(centre.x - bodyRadius, centre.y - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f, 1.0f);

    juce::Path pointer;
    const float pointerLen = bodyRadius * 0.76f;
    pointer.startNewSubPath(centre.x, centre.y);
    pointer.lineTo(centre.x + pointerLen * std::sin(angle), centre.y - pointerLen * std::cos(angle));
    g.setColour(juce::Colour(0xffffd23f).withAlpha(alphaScale));
    g.strokePath(pointer, juce::PathStrokeType(1.9f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void ProLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                                  int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                                  juce::ComboBox& box)
{
    auto r = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)).reduced(0.5f);
    const float alphaScale = box.isEnabled() ? 1.0f : 0.4f;

    g.setColour(box.findColour(juce::ComboBox::backgroundColourId).withAlpha(alphaScale));
    g.fillRoundedRectangle(r, 3.5f);
    g.setColour(box.findColour(juce::ComboBox::outlineColourId).withAlpha(alphaScale));
    g.drawRoundedRectangle(r, 3.5f, 1.0f);

    const float arrowX = r.getRight() - 10.0f;
    const float arrowY = r.getCentreY();
    juce::Path arrow;
    arrow.addTriangle(arrowX - 3.0f, arrowY - 1.3f, arrowX + 3.0f, arrowY - 1.3f, arrowX, arrowY + 2.0f);
    g.setColour(juce::Colour(0xff9aa5b8).withAlpha(alphaScale));
    g.fillPath(arrow);
}

void ProLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                      bool shouldDrawButtonAsHighlighted, bool /*shouldDrawButtonAsDown*/)
{
    auto r = button.getLocalBounds().toFloat().reduced(1.0f);
    const bool active = button.getToggleState();
    const float alphaScale = button.isEnabled() ? 1.0f : 0.4f;

    juce::Colour fill = active ? juce::Colour(0xff234b7a) : juce::Colour(0xff171b22);
    if (shouldDrawButtonAsHighlighted && button.isEnabled())
        fill = fill.brighter(0.12f);

    juce::Colour border = active ? juce::Colour(0xff52a8ff) : juce::Colour(0xff2d3442);
    juce::Colour textCol = active ? juce::Colour(0xffffffff) : juce::Colour(0xffaeb8ca);

    g.setColour(fill.withAlpha(alphaScale));
    g.fillRoundedRectangle(r, 3.5f);
    g.setColour(border.withAlpha(alphaScale));
    g.drawRoundedRectangle(r, 3.5f, 1.0f);

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

    juce::Colour col = juce::Colour(0xff1b2029);
    if (shouldDrawButtonAsDown && button.isEnabled())
        col = juce::Colour(0xff355c8c);
    else if (shouldDrawButtonAsHighlighted && button.isEnabled())
        col = col.brighter(0.14f);

    g.setColour(col.withAlpha(alphaScale));
    g.fillRoundedRectangle(r, 3.5f);
    g.setColour(juce::Colour(0xff313948).withAlpha(alphaScale));
    g.drawRoundedRectangle(r, 3.5f, 1.0f);
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
        s.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffedf2fa));
        s.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff14181f));
        s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff29303d));
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
    specDepthLabel.setAlpha(specOn ? 1.0f : 0.35f);
    specSharpSlider.setEnabled(specOn);
    specSharpLabel.setAlpha(specOn ? 1.0f : 0.35f);
    specSmoothSlider.setEnabled(specOn);
    specSmoothLabel.setAlpha(specOn ? 1.0f : 0.35f);
    specMixSlider.setEnabled(specOn);
    specMixLabel.setAlpha(specOn ? 1.0f : 0.35f);
}

void HybridResonanceEQEditor::selectBand(int bandIndex)
{
    selectedBandIndex = juce::jlimit(0, dspcore::ParametricEQEngine::maxBands - 1, bandIndex);
    bandSelectorBox.setSelectedId(selectedBandIndex + 1, juce::dontSendNotification);

    bandEnabledAttach.reset();
    bandShapeAttach.reset();
    bandRouteAttach.reset();
    bandDynAttach.reset();
    bandAutoThreshAttach.reset();
    bandSpecAttach.reset();
    bandSoloAttach.reset();
    bandFreqAttach.reset();
    bandGainAttach.reset();
    bandQAttach.reset();
    bandSlopeAttach.reset();
    bandDynRangeAttach.reset();
    bandDynThreshAttach.reset();
    bandDynAttackAttach.reset();
    bandDynReleaseAttach.reset();

    bandEnabledAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, bandParam(selectedBandIndex, "enabled"), bandEnabledToggle);
    bandShapeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.apvts, bandParam(selectedBandIndex, "shape"), bandShapeBox);
    bandRouteAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.apvts, bandParam(selectedBandIndex, "route"), bandRouteBox);
    bandDynAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, bandParam(selectedBandIndex, "dyn_enabled"), bandDynToggle);
    bandAutoThreshAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, bandParam(selectedBandIndex, "dyn_auto"), bandAutoThreshToggle);
    bandSpecAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, bandParam(selectedBandIndex, "spec_enabled"), bandSpecToggle);
    bandSoloAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, bandParam(selectedBandIndex, "solo"), bandSoloToggle);

    bandFreqAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, bandParam(selectedBandIndex, "freq"), bandFreqSlider);
    bandGainAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, bandParam(selectedBandIndex, "gain"), bandGainSlider);
    bandQAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, bandParam(selectedBandIndex, "q"), bandQSlider);
    bandSlopeAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, bandParam(selectedBandIndex, "slope"), bandSlopeSlider);
    bandDynRangeAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, bandParam(selectedBandIndex, "dyn_range"), bandDynRangeSlider);
    bandDynThreshAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, bandParam(selectedBandIndex, "dyn_thresh"), bandDynThreshSlider);
    bandDynAttackAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, bandParam(selectedBandIndex, "dyn_attack"), bandDynAttackSlider);
    bandDynReleaseAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, bandParam(selectedBandIndex, "dyn_release"), bandDynReleaseSlider);

    updateContextualBandControls();
    repaint();
}

void HybridResonanceEQEditor::copySelectedBand()
{
    copiedBand.valid = true;
    copiedBand.enabled = audioProcessor.apvts.getRawParameterValue(bandParam(selectedBandIndex, "enabled"))->load() > 0.5f;
    copiedBand.solo = audioProcessor.apvts.getRawParameterValue(bandParam(selectedBandIndex, "solo"))->load() > 0.5f;
    copiedBand.dynamicEnabled = audioProcessor.apvts.getRawParameterValue(bandParam(selectedBandIndex, "dyn_enabled"))->load() > 0.5f;
    copiedBand.autoThreshold = audioProcessor.apvts.getRawParameterValue(bandParam(selectedBandIndex, "dyn_auto"))->load() > 0.5f;
    copiedBand.spectralEnabled = audioProcessor.apvts.getRawParameterValue(bandParam(selectedBandIndex, "spec_enabled"))->load() > 0.5f;
    copiedBand.shape = static_cast<int>(audioProcessor.apvts.getRawParameterValue(bandParam(selectedBandIndex, "shape"))->load());
    copiedBand.route = static_cast<int>(audioProcessor.apvts.getRawParameterValue(bandParam(selectedBandIndex, "route"))->load());
    copiedBand.freq = audioProcessor.apvts.getRawParameterValue(bandParam(selectedBandIndex, "freq"))->load();
    copiedBand.gain = audioProcessor.apvts.getRawParameterValue(bandParam(selectedBandIndex, "gain"))->load();
    copiedBand.q = audioProcessor.apvts.getRawParameterValue(bandParam(selectedBandIndex, "q"))->load();
    copiedBand.slope = audioProcessor.apvts.getRawParameterValue(bandParam(selectedBandIndex, "slope"))->load();
    copiedBand.dynRange = audioProcessor.apvts.getRawParameterValue(bandParam(selectedBandIndex, "dyn_range"))->load();
    copiedBand.dynThresh = audioProcessor.apvts.getRawParameterValue(bandParam(selectedBandIndex, "dyn_thresh"))->load();
    copiedBand.dynAttack = audioProcessor.apvts.getRawParameterValue(bandParam(selectedBandIndex, "dyn_attack"))->load();
    copiedBand.dynRelease = audioProcessor.apvts.getRawParameterValue(bandParam(selectedBandIndex, "dyn_release"))->load();
}

void HybridResonanceEQEditor::pasteToSelectedBand()
{
    if (!copiedBand.valid)
        return;

    auto setBool = [this](const juce::String& id, bool v)
    {
        if (auto* p = audioProcessor.apvts.getParameter(id))
            p->setValueNotifyingHost(v ? 1.0f : 0.0f);
    };
    auto setFloat = [this](const juce::String& id, float v)
    {
        if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(audioProcessor.apvts.getParameter(id)))
            p->setValueNotifyingHost(p->convertTo0to1(v));
    };

    setBool(bandParam(selectedBandIndex, "enabled"), copiedBand.enabled);
    setBool(bandParam(selectedBandIndex, "solo"), copiedBand.solo);
    setBool(bandParam(selectedBandIndex, "dyn_enabled"), copiedBand.dynamicEnabled);
    setBool(bandParam(selectedBandIndex, "dyn_auto"), copiedBand.autoThreshold);
    setBool(bandParam(selectedBandIndex, "spec_enabled"), copiedBand.spectralEnabled);
    setFloat(bandParam(selectedBandIndex, "shape"), static_cast<float>(copiedBand.shape));
    setFloat(bandParam(selectedBandIndex, "route"), static_cast<float>(copiedBand.route));
    setFloat(bandParam(selectedBandIndex, "freq"), copiedBand.freq);
    setFloat(bandParam(selectedBandIndex, "gain"), copiedBand.gain);
    setFloat(bandParam(selectedBandIndex, "q"), copiedBand.q);
    setFloat(bandParam(selectedBandIndex, "slope"), copiedBand.slope);
    setFloat(bandParam(selectedBandIndex, "dyn_range"), copiedBand.dynRange);
    setFloat(bandParam(selectedBandIndex, "dyn_thresh"), copiedBand.dynThresh);
    setFloat(bandParam(selectedBandIndex, "dyn_attack"), copiedBand.dynAttack);
    setFloat(bandParam(selectedBandIndex, "dyn_release"), copiedBand.dynRelease);

    updateContextualBandControls();
    repaint();
}

void HybridResonanceEQEditor::toggleABState()
{
    juce::MemoryBlock currentBlock;
    audioProcessor.getStateInformation(currentBlock);

    if (!activeSlotB)
    {
        slotAState = currentBlock;
        activeSlotB = true;
        abButton.setButtonText("B");
        if (slotBState.getSize() > 0)
            audioProcessor.setStateInformation(slotBState.getData(), static_cast<int>(slotBState.getSize()));
    }
    else
    {
        slotBState = currentBlock;
        activeSlotB = false;
        abButton.setButtonText("A");
        if (slotAState.getSize() > 0)
            audioProcessor.setStateInformation(slotAState.getData(), static_cast<int>(slotAState.getSize()));
    }

    updateContextualBandControls();
    repaint();
}

void HybridResonanceEQEditor::showContextMenuForBand(int bandIndex, juce::Point<float> screenPos)
{
    selectBand(bandIndex);

    const bool dynOn = audioProcessor.apvts.getRawParameterValue(bandParam(bandIndex, "dyn_enabled"))->load() > 0.5f;
    const bool specOn = audioProcessor.apvts.getRawParameterValue(bandParam(bandIndex, "spec_enabled"))->load() > 0.5f;
    const bool soloOn = audioProcessor.apvts.getRawParameterValue(bandParam(bandIndex, "solo"))->load() > 0.5f;
    const int curShape = static_cast<int>(audioProcessor.apvts.getRawParameterValue(bandParam(bandIndex, "shape"))->load());
    const int curRoute = static_cast<int>(audioProcessor.apvts.getRawParameterValue(bandParam(bandIndex, "route"))->load());

    juce::PopupMenu menu;
    menu.addSectionHeader("Band " + juce::String(bandIndex + 1));
    menu.addItem(1, "Dynamic EQ", true, dynOn);
    menu.addItem(2, "Spectral Suppression", true, specOn);
    menu.addItem(3, "Solo Band", true, soloOn);
    menu.addSeparator();

    juce::PopupMenu shapeMenu;
    const juce::StringArray shapes { "Bell", "Low Shelf", "High Shelf", "Low Cut", "High Cut", "Notch", "Band Pass", "Tilt Shelf" };
    for (int s = 0; s < shapes.size(); ++s)
        shapeMenu.addItem(100 + s, shapes[s], true, s == curShape);
    menu.addSubMenu("Shape", shapeMenu);

    juce::PopupMenu routeMenu;
    const juce::StringArray routes { "Stereo", "Mid", "Side", "Left", "Right" };
    for (int r = 0; r < routes.size(); ++r)
        routeMenu.addItem(200 + r, routes[r], true, r == curRoute);
    menu.addSubMenu("Stereo / Mid-Side", routeMenu);

    menu.addSeparator();
    menu.addItem(4, "Reset Gain (0.0 dB)");
    menu.addItem(5, "Disable Band");

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({ static_cast<int>(screenPos.x), static_cast<int>(screenPos.y), 2, 2 }),
        [this, bandIndex](int result)
        {
            if (result == 0)
                return;

            auto toggleBool = [this, bandIndex](const juce::String& suffix)
            {
                if (auto* p = audioProcessor.apvts.getParameter(bandParam(bandIndex, suffix)))
                    p->setValueNotifyingHost(p->getValue() > 0.5f ? 0.0f : 1.0f);
            };

            if (result == 1) toggleBool("dyn_enabled");
            else if (result == 2) toggleBool("spec_enabled");
            else if (result == 3) toggleBool("solo");
            else if (result >= 100 && result < 200)
            {
                if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(audioProcessor.apvts.getParameter(bandParam(bandIndex, "shape"))))
                    p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(result - 100)));
            }
            else if (result >= 200 && result < 300)
            {
                if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(audioProcessor.apvts.getParameter(bandParam(bandIndex, "route"))))
                    p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(result - 200)));
            }
            else if (result == 4)
            {
                if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(audioProcessor.apvts.getParameter(bandParam(bandIndex, "gain"))))
                    p->setValueNotifyingHost(p->convertTo0to1(0.0f));
            }
            else if (result == 5)
            {
                if (auto* p = audioProcessor.apvts.getParameter(bandParam(bandIndex, "enabled")))
                    p->setValueNotifyingHost(0.0f);
            }

            updateContextualBandControls();
            repaint();
        });
}

void HybridResonanceEQEditor::showContextMenuForGraph(juce::Point<float> graphPos)
{
    const float freq = xToFreqHz(graphPos.x);
    const float gain = yToGainDb(graphPos.y);
    const auto screenPos = localPointToGlobal(graphPos);

    juce::PopupMenu menu;
    menu.addSectionHeader("Graph (" + juce::String(freq, 0) + " Hz)");
    menu.addItem(1, "Add Band Here");
    menu.addItem(2, "Add Dynamic Band Here");
    menu.addItem(3, "Add Spectral Band Here");
    menu.addSeparator();
    menu.addItem(4, "Clear All Solo");

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({ static_cast<int>(screenPos.x), static_cast<int>(screenPos.y), 2, 2 }),
        [this, freq, gain](int result)
        {
            if (result == 1 || result == 2 || result == 3)
            {
                addNewBandAt(freq, gain);
                if (result == 2)
                {
                    if (auto* p = audioProcessor.apvts.getParameter(bandParam(selectedBandIndex, "dyn_enabled")))
                        p->setValueNotifyingHost(1.0f);
                }
                else if (result == 3)
                {
                    if (auto* p = audioProcessor.apvts.getParameter(bandParam(selectedBandIndex, "spec_enabled")))
                        p->setValueNotifyingHost(1.0f);
                }
            }
            else if (result == 4)
            {
                for (int i = 0; i < dspcore::ParametricEQEngine::maxBands; ++i)
                {
                    if (auto* p = audioProcessor.apvts.getParameter(bandParam(i, "solo")))
                        p->setValueNotifyingHost(0.0f);
                }
            }

            updateContextualBandControls();
            repaint();
        });
}

float HybridResonanceEQEditor::getRangeDb() const noexcept
{
    const int idx = static_cast<int>(audioProcessor.apvts.getRawParameterValue("display_range")->load());
    if (idx == 0) return 6.0f;
    if (idx == 2) return 30.0f;
    return 12.0f;
}

float HybridResonanceEQEditor::xToFreqHz(float x) const noexcept
{
    const float left = static_cast<float>(graphBounds.getX());
    const float width = juce::jmax(1.0f, static_cast<float>(graphBounds.getWidth()));
    const float norm = juce::jlimit(0.0f, 1.0f, (x - left) / width);
    constexpr float minF = 20.0f;
    constexpr float maxF = 20000.0f;
    return minF * std::pow(maxF / minF, norm);
}

float HybridResonanceEQEditor::freqHzToX(float freqHz) const noexcept
{
    const float left = static_cast<float>(graphBounds.getX());
    const float width = juce::jmax(1.0f, static_cast<float>(graphBounds.getWidth()));
    constexpr float minF = 20.0f;
    constexpr float maxF = 20000.0f;
    const float clamped = juce::jlimit(minF, maxF, freqHz);
    const float norm = std::log(clamped / minF) / std::log(maxF / minF);
    return left + norm * width;
}

float HybridResonanceEQEditor::yToGainDb(float y) const noexcept
{
    const float top = static_cast<float>(graphBounds.getY());
    const float height = juce::jmax(1.0f, static_cast<float>(graphBounds.getHeight()));
    const float rangeDb = getRangeDb();
    const float norm = juce::jlimit(0.0f, 1.0f, (y - top) / height);
    return rangeDb - norm * (2.0f * rangeDb);
}

float HybridResonanceEQEditor::gainDbToY(float gainDb) const noexcept
{
    const float top = static_cast<float>(graphBounds.getY());
    const float height = juce::jmax(1.0f, static_cast<float>(graphBounds.getHeight()));
    const float rangeDb = getRangeDb();
    const float clamped = juce::jlimit(-rangeDb, rangeDb, gainDb);
    const float norm = (rangeDb - clamped) / (2.0f * rangeDb);
    return top + norm * height;
}

int HybridResonanceEQEditor::hitTestBandNode(juce::Point<float> pos) const noexcept
{
    for (int i = 0; i < dspcore::ParametricEQEngine::maxBands; ++i)
    {
        const bool enabled = audioProcessor.apvts.getRawParameterValue(bandParam(i, "enabled"))->load() > 0.5f;
        if (!enabled)
            continue;

        const float freq = audioProcessor.apvts.getRawParameterValue(bandParam(i, "freq"))->load();
        const float gain = audioProcessor.apvts.getRawParameterValue(bandParam(i, "gain"))->load();
        const float nx = freqHzToX(freq);
        const float ny = gainDbToY(gain);

        if (pos.getDistanceFrom({ nx, ny }) <= 12.0f)
            return i;
    }
    return -1;
}

void HybridResonanceEQEditor::updateBandParamFromGraph(int bandIndex, float freqHz, float gainDb)
{
    if (auto* fParam = dynamic_cast<juce::RangedAudioParameter*>(audioProcessor.apvts.getParameter(bandParam(bandIndex, "freq"))))
        fParam->setValueNotifyingHost(fParam->convertTo0to1(juce::jlimit(20.0f, 20000.0f, freqHz)));

    if (auto* gParam = dynamic_cast<juce::RangedAudioParameter*>(audioProcessor.apvts.getParameter(bandParam(bandIndex, "gain"))))
        gParam->setValueNotifyingHost(gParam->convertTo0to1(juce::jlimit(-30.0f, 30.0f, gainDb)));
}

void HybridResonanceEQEditor::addNewBandAt(float freqHz, float gainDb)
{
    for (int i = 0; i < dspcore::ParametricEQEngine::maxBands; ++i)
    {
        const bool enabled = audioProcessor.apvts.getRawParameterValue(bandParam(i, "enabled"))->load() > 0.5f;
        if (!enabled)
        {
            if (auto* enParam = audioProcessor.apvts.getParameter(bandParam(i, "enabled")))
                enParam->setValueNotifyingHost(1.0f);

            selectBand(i);
            updateBandParamFromGraph(i, freqHz, gainDb);
            return;
        }
    }
}

void HybridResonanceEQEditor::mouseDown(const juce::MouseEvent& e)
{
    const auto pos = e.position;
    if (!graphBounds.toFloat().contains(pos))
        return;

    const int hit = hitTestBandNode(pos);

    if (e.mods.isPopupMenu())
    {
        if (hit >= 0)
            showContextMenuForBand(hit, e.getScreenPosition().toFloat());
        else
            showContextMenuForGraph(pos);
        return;
    }

    if (hit >= 0)
    {
        selectBand(hit);
        draggedBandIndex = hit;
        updateBandParamFromGraph(hit, xToFreqHz(pos.x), yToGainDb(pos.y));
    }
}

void HybridResonanceEQEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (draggedBandIndex < 0)
        return;

    const auto pos = e.position;
    updateBandParamFromGraph(draggedBandIndex, xToFreqHz(pos.x), yToGainDb(pos.y));
}

void HybridResonanceEQEditor::mouseUp(const juce::MouseEvent&)
{
    draggedBandIndex = -1;
}

void HybridResonanceEQEditor::mouseDoubleClick(const juce::MouseEvent& e)
{
    const auto pos = e.position;
    if (!graphBounds.toFloat().contains(pos))
        return;

    const int hit = hitTestBandNode(pos);
    if (hit >= 0)
    {
        selectBand(hit);
        updateBandParamFromGraph(hit, xToFreqHz(pos.x), 0.0f);
    }
    else
    {
        addNewBandAt(xToFreqHz(pos.x), yToGainDb(pos.y));
    }
}

void HybridResonanceEQEditor::timerCallback()
{
    audioProcessor.getVisualData(preSpectrumDb, postSpectrumDb, reductionDb);
    updateContextualBandControls();
    repaint();
}

void HybridResonanceEQEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff101216));

    // Barras superior e inferior de bajo perfil estilo Pro-Q 4
    g.setColour(juce::Colour(0xff151820));
    g.fillRect(topBarBounds);
    g.fillRect(bottomBarBounds);

    // Panel de banda integrado sutilmente sin caja pesada
    g.setColour(juce::Colour(0xff13161d));
    g.fillRect(bandBarBounds);

    g.setColour(juce::Colour(0xff252b36));
    g.drawLine(0.0f, static_cast<float>(topBarBounds.getBottom()), static_cast<float>(getWidth()), static_cast<float>(topBarBounds.getBottom()), 1.0f);
    g.drawLine(0.0f, static_cast<float>(bandBarBounds.getY()), static_cast<float>(getWidth()), static_cast<float>(bandBarBounds.getY()), 1.0f);
    g.drawLine(0.0f, static_cast<float>(bottomBarBounds.getY()), static_cast<float>(getWidth()), static_cast<float>(bottomBarBounds.getY()), 1.0f);

    g.setColour(juce::Colour(0xffedf2fa));
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.drawText("PRO RESONANCE EQ", 14, topBarBounds.getY(), 170, topBarBounds.getHeight(), juce::Justification::centredLeft);

    const auto gb = graphBounds.toFloat();
    g.setColour(juce::Colour(0xff0b0d11));
    g.fillRect(gb);

    // Rejilla de frecuencias estilo Pro-Q 4
    const std::array<float, 10> gridFreqs { 20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f };
    g.setFont(juce::FontOptions(10.0f));
    for (float f : gridFreqs)
    {
        const float x = freqHzToX(f);
        g.setColour(juce::Colour(0xff181d26));
        g.drawVerticalLine(static_cast<int>(std::round(x)), gb.getY(), gb.getBottom());

        juce::String label = (f >= 1000.0f) ? (juce::String(f / 1000.0f, (f == 1000.0f || f == 2000.0f || f == 5000.0f || f == 10000.0f || f == 20000.0f) ? 0 : 1) + "k")
                                          : juce::String(static_cast<int>(f));
        g.setColour(juce::Colour(0xff788396));
        g.drawText(label, static_cast<int>(x) - 18, static_cast<int>(gb.getBottom()) - 18, 36, 16, juce::Justification::centred);
    }

    // Rejilla de dB y línea central 0 dB amarilla
    const float rangeDb = getRangeDb();
    const float stepDb = (rangeDb <= 6.0f) ? 2.0f : (rangeDb <= 12.0f ? 6.0f : 10.0f);
    for (float db = -rangeDb; db <= rangeDb + 0.001f; db += stepDb)
    {
        const float y = gainDbToY(db);
        const bool isZero = std::abs(db) < 0.01f;
        g.setColour(isZero ? juce::Colour(0x88ffd23f) : juce::Colour(0xff161a22));
        g.drawHorizontalLine(static_cast<int>(std::round(y)), gb.getX(), gb.getRight());

        if (!isZero)
        {
            g.setColour(juce::Colour(0xff6b7588));
            const juce::String txt = (db > 0.0f ? "+" : "") + juce::String(static_cast<int>(db));
            g.drawText(txt, static_cast<int>(gb.getRight()) - 38, static_cast<int>(y) - 8, 34, 16, juce::Justification::right);
        }
    }

    const bool showPre = audioProcessor.apv
