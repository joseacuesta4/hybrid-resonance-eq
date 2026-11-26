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

    // Band selector & quick actions
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

    // Analyzer & Character options
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

    // Band controls
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

    addAndMakeVisible(bandSoloToggle);

    configureRotary(bandFreqSlider, bandFreqLabel, "FREQ", " Hz");
    configureRotary(bandGainSlider, bandGainLabel, "GAIN", " dB");
    configureRotary(bandQSlider, bandQLabel, "Q", "");
    configureRotary(bandSlopeSlider, bandSlopeLabel, "SLOPE", " dB");
    configureRotary(bandDynRangeSlider, bandDynRangeLabel, "DYN RNG", " dB");
    configureRotary(bandDynThreshSlider, bandDynThreshLabel, "THRESH", " dB");
    configureRotary(bandDynAttackSlider, bandDynAttackLabel, "ATTACK", " ms");
    configureRotary(bandDynReleaseSlider, bandDynReleaseLabel, "RELEASE", " ms");

    // Spectral controls
    addAndMakeVisible(specEnabledToggle);
    specEnabledAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, "spec_enabled", specEnabledToggle);

    addAndMakeVisible(specDeltaToggle);
    specDeltaAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, "spec_delta", specDeltaToggle);

    configureRotary(specDepthSlider, specDepthLabel, "DEPTH", "");
    specDepthAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "spec_depth", specDepthSlider);

    configureRotary(specSharpSlider, specSharpLabel, "SHARP", "");
    specSharpAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "spec_sharpness", specSharpSlider);

    configureRotary(specSmoothSlider, specSmoothLabel, "SMOOTH", "");
    specSmoothAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "spec_smoothing", specSmoothSlider);

    configureRotary(specMixSlider, specMixLabel, "MIX", "");
    specMixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "spec_mix", specMixSlider);

    configureRotary(specMinFreqSlider, specMinFreqLabel, "LOW LIM", " Hz");
    specMinFreqAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "spec_min_freq", specMinFreqSlider);

    configureRotary(specMaxFreqSlider, specMaxFreqLabel, "HIGH LIM", " Hz");
    specMaxFreqAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "spec_max_freq", specMaxFreqSlider);

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
    g.fillAll(juce::Colour(0xff111318));

    g.setColour(juce::Colour(0xff171a21));
    g.fillRect(topBarBounds);
    g.fillRect(bandBarBounds);
    g.fillRect(bottomBarBounds);

    g.setColour(juce::Colour(0xff282e39));
    g.drawLine(0.0f, static_cast<float>(topBarBounds.getBottom()), static_cast<float>(getWidth()), static_cast<float>(topBarBounds.getBottom()), 1.0f);
    g.drawLine(0.0f, static_cast<float>(bandBarBounds.getY()), static_cast<float>(getWidth()), static_cast<float>(bandBarBounds.getY()), 1.0f);
    g.drawLine(0.0f, static_cast<float>(bottomBarBounds.getY()), static_cast<float>(getWidth()), static_cast<float>(bottomBarBounds.getY()), 1.0f);

    g.setColour(juce::Colour(0xffedf2fa));
    g.setFont(juce::FontOptions(12.5f, juce::Font::bold));
    g.drawText("HYBRID RESONANCE EQ", 14, topBarBounds.getY(), 200, topBarBounds.getHeight(), juce::Justification::centredLeft);

    const auto gb = graphBounds.toFloat();
    g.setColour(juce::Colour(0xff0d0f13));
    g.fillRect(gb);

    // Frequency grid
    const std::array<float, 10> gridFreqs { 20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f };
    g.setFont(juce::FontOptions(10.5f));
    for (float f : gridFreqs)
    {
        const float x = freqHzToX(f);
        g.setColour(juce::Colour(0xff1b2029));
        g.drawVerticalLine(static_cast<int>(std::round(x)), gb.getY(), gb.getBottom());

        juce::String label = (f >= 1000.0f) ? (juce::String(f / 1000.0f, (f == 1000.0f || f == 2000.0f || f == 5000.0f || f == 10000.0f || f == 20000.0f) ? 0 : 1) + "k")
                                          : juce::String(static_cast<int>(f));
        g.setColour(juce::Colour(0xff7d8799));
        g.drawText(label, static_cast<int>(x) - 18, static_cast<int>(gb.getBottom()) - 18, 36, 16, juce::Justification::centred);
    }

    // Gain grid
    const float rangeDb = getRangeDb();
    const float stepDb = (rangeDb <= 6.0f) ? 2.0f : (rangeDb <= 12.0f ? 6.0f : 10.0f);
    for (float db = -rangeDb; db <= rangeDb + 0.001f; db += stepDb)
    {
        const float y = gainDbToY(db);
        const bool isZero = std::abs(db) < 0.01f;
        g.setColour(isZero ? juce::Colour(0xff424b5c) : juce::Colour(0xff181c24));
        g.drawHorizontalLine(static_cast<int>(std::round(y)), gb.getX(), gb.getRight());

        if (!isZero)
        {
            g.setColour(juce::Colour(0xff6f798c));
            const juce::String txt = (db > 0.0f ? "+" : "") + juce::String(static_cast<int>(db));
            g.drawText(txt, static_cast<int>(gb.getRight()) - 38, static_cast<int>(y) - 8, 34, 16, juce::Justification::right);
        }
    }

    // Pre & Post spectrum + localized reduction bands only
    const bool showPre = audioProcessor.apvts.getRawParameterValue("ana_show_pre")->load() > 0.5f;
    const bool showPost = audioProcessor.apvts.getRawParameterValue("ana_show_post")->load() > 0.5f;
    const double sr = audioProcessor.getCurrentSampleRateHz();
    const int numBins = dspcore::SpectralResonanceEngine::numBins;
    const float binHzStep = static_cast<float>(sr) / static_cast<float>(dspcore::SpectralResonanceEngine::fftSize);

    auto mapSpectrumDbToY = [this, &gb](float db) noexcept -> float
    {
        const float norm = juce::jlimit(0.0f, 1.0f, (db - (-85.0f)) / 91.0f);
        return gb.getBottom() - norm * gb.getHeight();
    };

    juce::Path prePath, postPath, postFill;
    bool started = false;
    const float zeroY = gainDbToY(0.0f);

    for (float px = gb.getX(); px <= gb.getRight(); px += 2.0f)
    {
        const float freq = xToFreqHz(px);
        const int bin = juce::jlimit(0, numBins - 1, static_cast<int>(std::round(freq / juce::jmax(1.0f, binHzStep))));

        const float preY = mapSpectrumDbToY(preSpectrumDb[static_cast<size_t>(bin)]);
        const float postY = mapSpectrumDbToY(postSpectrumDb[static_cast<size_t>(bin)]);

        if (!started)
        {
            prePath.startNewSubPath(px, preY);
            postPath.startNewSubPath(px, postY);
            postFill.startNewSubPath(px, gb.getBottom());
            postFill.lineTo(px, postY);
            started = true;
        }
        else
        {
            prePath.lineTo(px, preY);
            postPath.lineTo(px, postY);
            postFill.lineTo(px, postY);
        }

        // Draw localized reduction only where reductionDb is significantly negative
        const float redDb = reductionDb[static_cast<size_t>(bin)];
        if (redDb < -0.35f)
        {
            const float intensity = juce::jlimit(0.0f, 1.0f, (-redDb) / 14.0f);
            const float barH = intensity * (gb.getHeight() * 0.36f);
            g.setColour(juce::Colour(0xffff5252).withAlpha(0.14f + 0.36f * intensity));
            g.fillRect(px - 1.0f, zeroY, 2.0f, barH);
        }
    }

    if (started)
    {
        postFill.lineTo(gb.getRight(), gb.getBottom());
        postFill.closeSubPath();

        if (showPost)
        {
            g.setColour(juce::Colour(0x244da3ff));
            g.fillPath(postFill);

            g.setColour(juce::Colour(0x8a64b5ff));
            g.strokePath(postPath, juce::PathStrokeType(1.35f));
        }

        if (showPre)
        {
            g.setColour(juce::Colour(0x4d9aa7bd));
            g.strokePath(prePath, juce::PathStrokeType(1.05f));
        }
    }

    // Selected individual band curve
    {
        juce::Path selCurve;
        bool selStarted = false;
        for (float px = gb.getX(); px <= gb.getRight(); px += 2.0f)
        {
            const float db = audioProcessor.getBandEQResponseDb(selectedBandIndex, static_cast<double>(xToFreqHz(px)));
            const float y = gainDbToY(db);
            if (!selStarted) { selCurve.startNewSubPath(px, y); selStarted = true; }
            else selCurve.lineTo(px, y);
        }
        if (selStarted)
        {
            g.setColour(getBandColour(selectedBandIndex).withAlpha(0.55f));
            g.strokePath(selCurve, juce::PathStrokeType(1.45f));
        }
    }

    // Combined EQ curve
    {
        juce::Path sumCurve;
        juce::Path sumFill;
        bool sumStarted = false;

        for (float px = gb.getX(); px <= gb.getRight(); px += 2.0f)
        {
            const float db = audioProcessor.getCombinedEQResponseDb(static_cast<double>(xToFreqHz(px)));
            const float y = gainDbToY(db);
            if (!sumStarted)
            {
                sumCurve.startNewSubPath(px, y);
                sumFill.startNewSubPath(px, zeroY);
                sumFill.lineTo(px, y);
                sumStarted = true;
            }
            else
            {
                sumCurve.lineTo(px, y);
                sumFill.lineTo(px, y);
            }
        }
        if (sumStarted)
        {
            sumFill.lineTo(gb.getRight(), zeroY);
            sumFill.closeSubPath();

            g.setColour(juce::Colour(0x18ffd23f));
            g.fillPath(sumFill);

            g.setColour(juce::Colour(0xffffd23f));
            g.strokePath(sumCurve, juce::PathStrokeType(2.15f));
        }
    }

    // Band nodes + dynamic reduction ring
    for (int i = 0; i < dspcore::ParametricEQEngine::maxBands; ++i)
    {
        const bool enabled = audioProcessor.apvts.getRawParameterValue(bandParam(i, "enabled"))->load() > 0.5f;
        if (!enabled)
            continue;

        const float freq = audioProcessor.apvts.getRawParameterValue(bandParam(i, "freq"))->load();
        const float gain = audioProcessor.apvts.getRawParameterValue(bandParam(i, "gain"))->load();
        const float dynDb = audioProcessor.getBandDynamicGainDb(i);

        const float nx = freqHzToX(freq);
        const float ny = gainDbToY(gain);
        const auto col = getBandColour(i);
        const bool isSelected = (i == selectedBandIndex);

        const float r = isSelected ? 10.5f : 8.0f;

        if (std::abs(dynDb) > 0.25f)
        {
            const float dynRadius = r + juce::jlimit(2.0f, 12.0f, std::abs(dynDb) * 0.8f);
            g.setColour(juce::Colour(0xffff5252).withAlpha(0.55f));
            g.drawEllipse(nx - dynRadius, ny - dynRadius, dynRadius * 2.0f, dynRadius * 2.0f, 1.8f);
        }

        g.setColour(col.withAlpha(isSelected ? 0.35f : 0.16f));
        g.fillEllipse(nx - r * 1.4f, ny - r * 1.4f, r * 2.8f, r * 2.8f);

        g.setColour(col);
        g.fillEllipse(nx - r, ny - r, r * 2.0f, r * 2.0f);

        g.setColour(juce::Colour(0xff101216));
        g.drawEllipse(nx - r, ny - r, r * 2.0f, r * 2.0f, 1.4f);

        g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
        g.drawText(juce::String(i + 1), static_cast<int>(nx) - 10, static_cast<int>(ny) - 8, 20, 16, juce::Justification::centred);
    }
}

void HybridResonanceEQEditor::resized()
{
    auto area = getLocalBounds();

    topBarBounds = area.removeFromTop(42);
    bottomBarBounds = area.removeFromBottom(102);
    bandBarBounds = area.removeFromBottom(94);
    graphBounds = area;

    {
        auto r = topBarBounds.reduced(8, 7);
        r.removeFromLeft(195);

        bandSelectorBox.setBounds(r.removeFromLeft(88));
        r.removeFromLeft(4);
        addBandButton.setBounds(r.removeFromLeft(56));
        r.removeFromLeft(3);
        delBandButton.setBounds(r.removeFromLeft(38));
        r.removeFromLeft(6);
        copyBandButton.setBounds(r.removeFromLeft(46));
        r.removeFromLeft(3);
        pasteBandButton.setBounds(r.removeFromLeft(48));
        r.removeFromLeft(3);
        abButton.setBounds(r.removeFromLeft(32));
        r.removeFromLeft(8);

        resolutionBox.setBounds(r.removeFromLeft(84));
        r.removeFromLeft(3);
        speedBox.setBounds(r.removeFromLeft(82));
        r.removeFromLeft(3);
        tiltBox.setBounds(r.removeFromLeft(84));
        r.removeFromLeft(3);
        rangeBox.setBounds(r.removeFromLeft(78));
        r.removeFromLeft(3);
        characterBox.setBounds(r.removeFromLeft(94));
        r.removeFromLeft(6);

        showPreToggle.setBounds(r.removeFromLeft(40));
        r.removeFromLeft(3);
        showPostToggle.setBounds(r.removeFromLeft(44));
        r.removeFromLeft(3);
        phaseInvertToggle.setBounds(r.removeFromLeft(30));
        r.removeFromLeft(6);
        bypassToggle.setBounds(r.removeFromLeft(58));
    }

    {
        auto r = bandBarBounds.reduced(8, 4);
        auto leftCol = r.removeFromLeft(275);
        auto topBtns = leftCol.removeFromTop(30);
        bandEnabledToggle.setBounds(topBtns.removeFromLeft(44));
        topBtns.removeFromLeft(4);
        bandShapeBox.setBounds(topBtns.removeFromLeft(96));
        topBtns.removeFromLeft(4);
        bandRouteBox.setBounds(topBtns.removeFromLeft(82));

        auto botBtns = leftCol.removeFromTop(28).translated(0, 4);
        bandDynToggle.setBounds(botBtns.removeFromLeft(52));
        botBtns.removeFromLeft(4);
        bandAutoThreshToggle.setBounds(botBtns.removeFromLeft(52));
        botBtns.removeFromLeft(4);
        bandSoloToggle.setBounds(botBtns.removeFromLeft(52));

        const int knobW = juce::jmax(58, r.getWidth() / 8);
        auto placeKnob = [&r, knobW](juce::Slider& s, juce::Label& l)
        {
            auto slot = r.removeFromLeft(knobW);
            l.setBounds(slot.removeFromTop(14));
            s.setBounds(slot);
        };

        placeKnob(bandFreqSlider, bandFreqLabel);
        placeKnob(bandGainSlider, bandGainLabel);
        placeKnob(bandQSlider, bandQLabel);
        placeKnob(bandSlopeSlider, bandSlopeLabel);
        placeKnob(bandDynRangeSlider, bandDynRangeLabel);
        placeKnob(bandDynThreshSlider, bandDynThreshLabel);
        placeKnob(bandDynAttackSlider, bandDynAttackLabel);
        placeKnob(bandDynReleaseSlider, bandDynReleaseLabel);
    }

    {
        auto r = bottomBarBounds.reduced(8, 4);
        auto leftCol = r.removeFromLeft(155);
        specEnabledToggle.setBounds(leftCol.removeFromTop(34).reduced(0, 2));
        specDeltaToggle.setBounds(leftCol.removeFromTop(34).reduced(0, 2));

        const int totalKnobs = 8; // 6 spectral + scale + output
        const int knobW = juce::jmax(58, r.getWidth() / totalKnobs);
        auto placeKnob = [&r, knobW](juce::Slider& s, juce::Label& l)
        {
            auto slot = r.removeFromLeft(knobW);
            l.setBounds(slot.removeFromTop(14));
            s.setBounds(slot);
        };

        placeKnob(specDepthSlider, specDepthLabel);
        placeKnob(specSharpSlider, specSharpLabel);
        placeKnob(specSmoothSlider, specSmoothLabel);
        placeKnob(specMixSlider, specMixLabel);
        placeKnob(specMinFreqSlider, specMinFreqLabel);
        placeKnob(specMaxFreqSlider, specMaxFreqLabel);
        placeKnob(gainScaleSlider, gainScaleLabel);
        placeKnob(outputGainSlider, outputGainLabel);
    }
}
