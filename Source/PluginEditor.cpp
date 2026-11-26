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
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff14161a));
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1e2229));
    setColour(juce::ComboBox::textColourId, juce::Colour(0xffe6ebf2));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff343b46));
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff1b1f26));
    setColour(juce::PopupMenu::textColourId, juce::Colour(0xffe6ebf2));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff2c6bed));
    setColour(juce::Label::textColourId, juce::Colour(0xffa7b1c2));
    setColour(juce::TextButton::buttonColourId, juce::Colour(0xff20252d));
    setColour(juce::TextButton::textColourOffId, juce::Colour(0xffdce2ec));
}

void ProLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                      float sliderPosProportional, float rotaryStartAngle,
                                      float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                         static_cast<float>(width), static_cast<float>(height)).reduced(4.0f);
    const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float arcRadius = juce::jmax(4.0f, radius - 3.0f);
    const float angle = rotaryStartAngle + juce::jlimit(0.0f, 1.0f, sliderPosProportional) * (rotaryEndAngle - rotaryStartAngle);

    // Base track
    juce::Path bgArc;
    bgArc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(juce::Colour(0xff2a303a));
    g.strokePath(bgArc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Active arc
    juce::Path valArc;
    const bool isBipolar = (slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0);
    const float startArc = isBipolar ? (0.5f * (rotaryStartAngle + rotaryEndAngle)) : rotaryStartAngle;
    valArc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f, startArc, angle, true);
    g.setColour(juce::Colour(0xff4da3ff));
    g.strokePath(valArc, juce::PathStrokeType(3.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Knob body
    const float bodyRadius = juce::jmax(3.0f, arcRadius - 4.5f);
    g.setColour(juce::Colour(0xff1b1f27));
    g.fillEllipse(centre.x - bodyRadius, centre.y - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f);
    g.setColour(juce::Colour(0xff475161));
    g.drawEllipse(centre.x - bodyRadius, centre.y - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f, 1.0f);

    // Indicator line
    juce::Path pointer;
    const float pointerLen = bodyRadius * 0.78f;
    pointer.startNewSubPath(centre.x, centre.y);
    pointer.lineTo(centre.x + pointerLen * std::sin(angle), centre.y - pointerLen * std::cos(angle));
    g.setColour(juce::Colour(0xfff5d042));
    g.strokePath(pointer, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void ProLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                                  int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                                  juce::ComboBox& box)
{
    auto r = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)).reduced(0.5f);
    g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle(r, 4.0f);
    g.setColour(box.findColour(juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle(r, 4.0f, 1.0f);

    // Chevron
    const float arrowX = r.getRight() - 12.0f;
    const float arrowY = r.getCentreY();
    juce::Path arrow;
    arrow.addTriangle(arrowX - 3.5f, arrowY - 1.5f, arrowX + 3.5f, arrowY - 1.5f, arrowX, arrowY + 2.5f);
    g.setColour(juce::Colour(0xff9aa5b8));
    g.fillPath(arrow);
}

void ProLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                      bool shouldDrawButtonAsHighlighted, bool /*shouldDrawButtonAsDown*/)
{
    auto r = button.getLocalBounds().toFloat().reduced(1.0f);
    const bool active = button.getToggleState();

    juce::Colour fill = active ? juce::Colour(0xff234b7a) : juce::Colour(0xff1d2129);
    if (shouldDrawButtonAsHighlighted)
        fill = fill.brighter(0.12f);

    juce::Colour border = active ? juce::Colour(0xff52a8ff) : juce::Colour(0xff343b46);
    juce::Colour textCol = active ? juce::Colour(0xffffffff) : juce::Colour(0xffb5bfd0);

    g.setColour(fill);
    g.fillRoundedRectangle(r, 4.0f);
    g.setColour(border);
    g.drawRoundedRectangle(r, 4.0f, 1.0f);

    g.setColour(textCol);
    g.setFont(juce::FontOptions(11.5f, juce::Font::bold));
    g.drawText(button.getButtonText(), r, juce::Justification::centred, true);
}

void ProLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                          const juce::Colour& /*backgroundColour*/,
                                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto r = button.getLocalBounds().toFloat().reduced(1.0f);
    juce::Colour col = juce::Colour(0xff222730);
    if (shouldDrawButtonAsDown)
        col = juce::Colour(0xff355c8c);
    else if (shouldDrawButtonAsHighlighted)
        col = col.brighter(0.14f);

    g.setColour(col);
    g.fillRoundedRectangle(r, 4.0f);
    g.setColour(juce::Colour(0xff38404d));
    g.drawRoundedRectangle(r, 4.0f, 1.0f);
}

HybridResonanceEQEditor::HybridResonanceEQEditor(HybridResonanceEQProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&proLookAndFeel);
    setResizable(true, true);
    setResizeLimits(980, 580, 1920, 1150);
    setSize(1240, 720);

    preSpectrumDb.fill(-120.0f);
    postSpectrumDb.fill(-120.0f);
    reductionDb.fill(0.0f);

    // Band selector & quick add/remove
    for (int i = 0; i < dspcore::ParametricEQEngine::maxBands; ++i)
        bandSelectorBox.addItem("Band " + juce::String(i + 1), i + 1);
    bandSelectorBox.setSelectedId(1, juce::dontSendNotification);
    bandSelectorBox.onChange = [this]
    {
        selectBand(juce::jmax(0, bandSelectorBox.getSelectedId() - 1));
    };
    addAndMakeVisible(bandSelectorBox);

    addBandButton.onClick = [this]
    {
        addNewBandAt(1000.0f, 0.0f);
    };
    addAndMakeVisible(addBandButton);

    delBandButton.onClick = [this]
    {
        if (auto* param = audioProcessor.apvts.getParameter(bandParam(selectedBandIndex, "enabled")))
            param->setValueNotifyingHost(0.0f);
        repaint();
    };
    addAndMakeVisible(delBandButton);

    // Analyzer options
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

    addAndMakeVisible(showPreToggle);
    showPreAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, "ana_show_pre", showPreToggle);

    addAndMakeVisible(showPostToggle);
    showPostAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, "ana_show_post", showPostToggle);

    addAndMakeVisible(bypassToggle);
    bypassAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, "global_bypass", bypassToggle);

    auto configureRotary = [this](juce::Slider& s, juce::Label& lbl, const juce::String& text, const juce::String& suffix)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 16);
        s.setTextValueSuffix(suffix);
        s.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffe6ebf2));
        s.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff191d24));
        s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff2f3642));
        addAndMakeVisible(s);

        lbl.setText(text, juce::dontSendNotification);
        lbl.setJustificationType(juce::Justification::centred);
        lbl.setFont(juce::FontOptions(10.5f));
        addAndMakeVisible(lbl);
    };

    configureRotary(outputGainSlider, outputGainLabel, "OUT", " dB");
    outputGainAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "output_gain", outputGainSlider);

    // Band bar controls
    addAndMakeVisible(bandEnabledToggle);
    bandShapeBox.addItemList({ "Bell", "Low Shelf", "High Shelf", "Low Cut", "High Cut", "Notch", "Band Pass", "Tilt Shelf" }, 1);
    addAndMakeVisible(bandShapeBox);
    bandRouteBox.addItemList({ "Stereo", "Mid", "Side", "Left", "Right" }, 1);
    addAndMakeVisible(bandRouteBox);
    addAndMakeVisible(bandDynToggle);
    addAndMakeVisible(bandSoloToggle);

    configureRotary(bandFreqSlider, bandFreqLabel, "FREQ", " Hz");
    configureRotary(bandGainSlider, bandGainLabel, "GAIN", " dB");
    configureRotary(bandQSlider, bandQLabel, "Q", "");
    configureRotary(bandSlopeSlider, bandSlopeLabel, "SLOPE", " dB");
    configureRotary(bandDynRangeSlider, bandDynRangeLabel, "DYN RNG", " dB");
    configureRotary(bandDynThreshSlider, bandDynThreshLabel, "THRESH", " dB");

    // Spectral bar controls
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

void HybridResonanceEQEditor::selectBand(int bandIndex)
{
    selectedBandIndex = juce::jlimit(0, dspcore::ParametricEQEngine::maxBands - 1, bandIndex);
    bandSelectorBox.setSelectedId(selectedBandIndex + 1, juce::dontSendNotification);

    bandEnabledAttach.reset();
    bandShapeAttach.reset();
    bandRouteAttach.reset();
    bandDynAttach.reset();
    bandSoloAttach.reset();
    bandFreqAttach.reset();
    bandGainAttach.reset();
    bandQAttach.reset();
    bandSlopeAttach.reset();
    bandDynRangeAttach.reset();
    bandDynThreshAttach.reset();

    bandEnabledAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, bandParam(selectedBandIndex, "enabled"), bandEnabledToggle);
    bandShapeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.apvts, bandParam(selectedBandIndex, "shape"), bandShapeBox);
    bandRouteAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.apvts, bandParam(selectedBandIndex, "route"), bandRouteBox);
    bandDynAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, bandParam(selectedBandIndex, "dyn_enabled"), bandDynToggle);
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

    repaint();
}

void HybridResonanceEQEditor::syncBandControlsFromParams()
{
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
    repaint();
}

void HybridResonanceEQEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff121418));

    // Top & bottom panels
    g.setColour(juce::Colour(0xff181b22));
    g.fillRect(topBarBounds);
    g.fillRect(bandBarBounds);
    g.fillRect(bottomBarBounds);

    g.setColour(juce::Colour(0xff292f3a));
    g.drawLine(0.0f, static_cast<float>(topBarBounds.getBottom()), static_cast<float>(getWidth()), static_cast<float>(topBarBounds.getBottom()), 1.0f);
    g.drawLine(0.0f, static_cast<float>(bandBarBounds.getY()), static_cast<float>(getWidth()), static_cast<float>(bandBarBounds.getY()), 1.0f);
    g.drawLine(0.0f, static_cast<float>(bottomBarBounds.getY()), static_cast<float>(getWidth()), static_cast<float>(bottomBarBounds.getY()), 1.0f);

    // Title
    g.setColour(juce::Colour(0xffe9eef7));
    g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    g.drawText("HYBRID RESONANCE EQ  |  16-BAND PRO-STYLE + 4096 FFT SPECTRAL",
               14, topBarBounds.getY(), 430, topBarBounds.getHeight(), juce::Justification::centredLeft);

    // Graph background
    const auto gb = graphBounds.toFloat();
    g.setColour(juce::Colour(0xff0e1014));
    g.fillRect(gb);

    // Frequency grid lines
    const std::array<float, 10> gridFreqs { 20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f };
    g.setFont(juce::FontOptions(10.5f));
    for (float f : gridFreqs)
    {
        const float x = freqHzToX(f);
        g.setColour(juce::Colour(0xff1d212a));
        g.drawVerticalLine(static_cast<int>(std::round(x)), gb.getY(), gb.getBottom());

        juce::String label = (f >= 1000.0f) ? (juce::String(f / 1000.0f, (f == 1000.0f || f == 2000.0f || f == 5000.0f || f == 10000.0f || f == 20000.0f) ? 0 : 1) + "k")
                                          : juce::String(static_cast<int>(f));
        g.setColour(juce::Colour(0xff7d8799));
        g.drawText(label, static_cast<int>(x) - 18, static_cast<int>(gb.getBottom()) - 18, 36, 16, juce::Justification::centred);
    }

    // Gain grid lines
    const float rangeDb = getRangeDb();
    const float stepDb = (rangeDb <= 6.0f) ? 2.0f : (rangeDb <= 12.0f ? 6.0f : 10.0f);
    for (float db = -rangeDb; db <= rangeDb + 0.001f; db += stepDb)
    {
        const float y = gainDbToY(db);
        const bool isZero = std::abs(db) < 0.01f;
        g.setColour(isZero ? juce::Colour(0xff424b5c) : juce::Colour(0xff191d26));
        g.drawHorizontalLine(static_cast<int>(std::round(y)), gb.getX(), gb.getRight());

        if (!isZero)
        {
            g.setColour(juce::Colour(0xff6f798c));
            const juce::String txt = (db > 0.0f ? "+" : "") + juce::String(static_cast<int>(db));
            g.drawText(txt, static_cast<int>(gb.getRight()) - 38, static_cast<int>(y) - 8, 34, 16, juce::Justification::right);
        }
    }

    // Spectrum Pre & Post + Reduction overlay
    const bool showPre = audioProcessor.apvts.getRawParameterValue("ana_show_pre")->load() > 0.5f;
    const bool showPost = audioProcessor.apvts.getRawParameterValue("ana_show_post")->load() > 0.5f;
    const double sr = audioProcessor.getCurrentSampleRateHz();
    const int numBins = dspcore::SpectralResonanceEngine::numBins;
    const float binHzStep = static_cast<float>(sr) / static_cast<float>(dspcore::SpectralResonanceEngine::fftSize);

    auto mapSpectrumDbToY = [this, &gb](float db) noexcept -> float
    {
        // Map [-85 dB .. +6 dB] to bottom..top of graph
        const float norm = juce::jlimit(0.0f, 1.0f, (db - (-85.0f)) / 91.0f);
        return gb.getBottom() - norm * gb.getHeight();
    };

    juce::Path prePath, postPath, postFill, redFill;
    bool started = false;

    const int stepPx = 2;
    for (float px = gb.getX(); px <= gb.getRight(); px += static_cast<float>(stepPx))
    {
        const float freq = xToFreqHz(px);
        const int bin = juce::jlimit(0, numBins - 1, static_cast<int>(std::round(freq / juce::jmax(1.0f, binHzStep))));

        const float preY = mapSpectrumDbToY(preSpectrumDb[static_cast<size_t>(bin)]);
        const float postY = mapSpectrumDbToY(postSpectrumDb[static_cast<size_t>(bin)]);

        // Reduction in dB (negative values) mapped onto graph height
        const float redDb = juce::jlimit(-24.0f, 0.0f, reductionDb[static_cast<size_t>(bin)]);
        const float redHeight = (-redDb / 24.0f) * (gb.getHeight() * 0.45f);
        const float zeroY = gainDbToY(0.0f);

        if (!started)
        {
            prePath.startNewSubPath(px, preY);
            postPath.startNewSubPath(px, postY);
            postFill.startNewSubPath(px, gb.getBottom());
            postFill.lineTo(px, postY);
            redFill.startNewSubPath(px, zeroY);
            redFill.lineTo(px, zeroY + redHeight);
            started = true;
        }
        else
        {
            prePath.lineTo(px, preY);
            postPath.lineTo(px, postY);
            postFill.lineTo(px, postY);
            redFill.lineTo(px, zeroY + redHeight);
        }
    }

    if (started)
    {
        postFill.lineTo(gb.getRight(), gb.getBottom());
        postFill.closeSubPath();

        redFill.lineTo(gb.getRight(), gainDbToY(0.0f));
        redFill.closeSubPath();

        if (showPost)
        {
            g.setColour(juce::Colour(0x284da3ff));
            g.fillPath(postFill);

            g.setColour(juce::Colour(0x8864b5ff));
            g.strokePath(postPath, juce::PathStrokeType(1.4f));
        }

        if (showPre)
        {
            g.setColour(juce::Colour(0x559aa7bd));
            g.strokePath(prePath, juce::PathStrokeType(1.1f));
        }

        // Visible reduction shading where spectral/dynamic attenuation occurs
        g.setColour(juce::Colour(0x52ff5252));
        g.fillPath(redFill);
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
            g.strokePath(selCurve, juce::PathStrokeType(1.5f));
        }
    }

    // Combined EQ curve (yellow)
    {
        juce::Path sumCurve;
        juce::Path sumFill;
        bool sumStarted = false;
        const float zeroY = gainDbToY(0.0f);

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

            g.setColour(juce::Colour(0x1affd23f));
            g.fillPath(sumFill);

            g.setColour(juce::Colour(0xffffd23f));
            g.strokePath(sumCurve, juce::PathStrokeType(2.2f));
        }
    }

    // Band nodes
    for (int i = 0; i < dspcore::ParametricEQEngine::maxBands; ++i)
    {
        const bool enabled = audioProcessor.apvts.getRawParameterValue(bandParam(i, "enabled"))->load() > 0.5f;
        if (!enabled)
            continue;

        const float freq = audioProcessor.apvts.getRawParameterValue(bandParam(i, "freq"))->load();
        const float gain = audioProcessor.apvts.getRawParameterValue(bandParam(i, "gain"))->load();
        const float nx = freqHzToX(freq);
        const float ny = gainDbToY(gain);
        const auto col = getBandColour(i);
        const bool isSelected = (i == selectedBandIndex);

        const float r = isSelected ? 11.0f : 8.5f;
        g.setColour(col.withAlpha(isSelected ? 0.35f : 0.18f));
        g.fillEllipse(nx - r * 1.45f, ny - r * 1.45f, r * 2.9f, r * 2.9f);

        g.setColour(col);
        g.fillEllipse(nx - r, ny - r, r * 2.0f, r * 2.0f);

        g.setColour(juce::Colour(0xff101216));
        g.drawEllipse(nx - r, ny - r, r * 2.0f, r * 2.0f, 1.5f);

        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawText(juce::String(i + 1), static_cast<int>(nx) - 10, static_cast<int>(ny) - 8, 20, 16, juce::Justification::centred);
    }
}

void HybridResonanceEQEditor::resized()
{
    auto area = getLocalBounds();

    topBarBounds = area.removeFromTop(42);
    bottomBarBounds = area.removeFromBottom(102);
    bandBarBounds = area.removeFromBottom(92);
    graphBounds = area;

    // Top bar layout
    {
        auto r = topBarBounds.reduced(8, 7);
        r.removeFromLeft(420); // Title space

        bandSelectorBox.setBounds(r.removeFromLeft(92));
        r.removeFromLeft(6);
        addBandButton.setBounds(r.removeFromLeft(58));
        r.removeFromLeft(4);
        delBandButton.setBounds(r.removeFromLeft(42));
        r.removeFromLeft(12);

        resolutionBox.setBounds(r.removeFromLeft(90));
        r.removeFromLeft(4);
        speedBox.setBounds(r.removeFromLeft(88));
        r.removeFromLeft(4);
        tiltBox.setBounds(r.removeFromLeft(90));
        r.removeFromLeft(4);
        rangeBox.setBounds(r.removeFromLeft(82));
        r.removeFromLeft(8);

        showPreToggle.setBounds(r.removeFromLeft(44));
        r.removeFromLeft(4);
        showPostToggle.setBounds(r.removeFromLeft(48));
        r.removeFromLeft(8);
        bypassToggle.setBounds(r.removeFromLeft(64));
    }

    // Band bar layout
    {
        auto r = bandBarBounds.reduced(8, 4);
        auto leftCol = r.removeFromLeft(250);
        auto topBtns = leftCol.removeFromTop(30);
        bandEnabledToggle.setBounds(topBtns.removeFromLeft(46));
        topBtns.removeFromLeft(4);
        bandShapeBox.setBounds(topBtns.removeFromLeft(98));
        topBtns.removeFromLeft(4);
        bandRouteBox.setBounds(topBtns.removeFromLeft(86));

        auto botBtns = leftCol.removeFromTop(28).translated(0, 4);
        bandDynToggle.setBounds(botBtns.removeFromLeft(58));
        botBtns.removeFromLeft(6);
        bandSoloToggle.setBounds(botBtns.removeFromLeft(58));

        const int knobW = juce::jmax(64, r.getWidth() / 6);
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
    }

    // Bottom bar layout (Spectral + Output)
    {
        auto r = bottomBarBounds.reduced(8, 4);
        auto leftCol = r.removeFromLeft(165);
        specEnabledToggle.setBounds(leftCol.removeFromTop(34).reduced(0, 2));
        specDeltaToggle.setBounds(leftCol.removeFromTop(34).reduced(0, 2));

        const int totalKnobs = 7; // 6 spectral + 1 output
        const int knobW = juce::jmax(62, r.getWidth() / totalKnobs);
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
        placeKnob(outputGainSlider, outputGainLabel);
    }
}
