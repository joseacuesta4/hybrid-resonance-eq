#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

namespace
{
juce::String bandParamId(int index, const juce::String& suffix)
{
    return "band_" + juce::String(index) + "_" + suffix;
}

constexpr float kMinFreq = 20.0f;
constexpr float kMaxFreq = 20000.0f;
} // namespace

HybridResonanceEQEditor::HybridResonanceEQEditor(HybridResonanceEQProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setSize(1080, 620);
    setResizable(true, true);
    setResizeLimits(840, 480, 1920, 1200);

    titleLabel.setText("HYBRID RESONANCE EQ", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, theme.textPrimary);
    addAndMakeVisible(titleLabel);

    tiltLabel.setText("Tilt", juce::dontSendNotification);
    tiltLabel.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    tiltLabel.setColour(juce::Label::textColourId, theme.textMuted);
    addAndMakeVisible(tiltLabel);

    tiltCombo.addItemList({ "0 dB", "3 dB", "4.5 dB", "6 dB" }, 1);
    tiltCombo.setColour(juce::ComboBox::backgroundColourId, theme.panelElevated);
    tiltCombo.setColour(juce::ComboBox::textColourId, theme.textPrimary);
    tiltCombo.setColour(juce::ComboBox::outlineColourId, theme.borderSubtle);
    addAndMakeVisible(tiltCombo);
    tiltAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processorRef.apvts, "ana_tilt", tiltCombo);

    specResolutionLabel.setText("Spec FFT", juce::dontSendNotification);
    specResolutionLabel.setFont(juce::FontOptions(11.5f, juce::Font::bold));
    specResolutionLabel.setColour(juce::Label::textColourId, theme.textMuted);
    addAndMakeVisible(specResolutionLabel);

    specResolutionCombo.addItemList({ "2048 · Low Latency", "4096 · High Quality" }, 1);
    specResolutionCombo.setColour(juce::ComboBox::backgroundColourId, theme.panelElevated);
    specResolutionCombo.setColour(juce::ComboBox::textColourId, theme.textPrimary);
    specResolutionCombo.setColour(juce::ComboBox::outlineColourId, theme.borderSubtle);
    specResolutionCombo.setTooltip("Spectral processing FFT size. 4096 improves frequency selectivity and increases latency.");
    addAndMakeVisible(specResolutionCombo);
    specResolutionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processorRef.apvts, "spec_resolution", specResolutionCombo);

    bypassButton.setClickingTogglesState(true);
    bypassButton.setColour(juce::TextButton::buttonColourId, theme.panelElevated);
    bypassButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0x66f59e0b));
    bypassButton.setColour(juce::TextButton::textColourOffId, theme.textMuted);
    bypassButton.setColour(juce::TextButton::textColourOnId, theme.textPrimary);
    addAndMakeVisible(bypassButton);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.apvts, "global_bypass", bypassButton);

    auto configureAnalyzerToggle = [this](juce::ToggleButton& button, juce::Colour active)
    {
        button.setColour(juce::ToggleButton::textColourId, theme.textMuted);
        button.setColour(juce::ToggleButton::tickColourId, active);
        button.setColour(juce::ToggleButton::tickDisabledColourId, theme.textMuted);
        addAndMakeVisible(button);
    };
    configureAnalyzerToggle(preAnalyzerButton, juce::Colour(0xff94a3b8));
    configureAnalyzerToggle(postAnalyzerButton, juce::Colour(0xff7dd3fc));
    preAnalyzerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.apvts, "ana_show_pre", preAnalyzerButton);
    postAnalyzerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.apvts, "ana_show_post", postAnalyzerButton);

    selectedBandLabel.setFont(juce::FontOptions(12.5f, juce::Font::bold));
    addAndMakeVisible(selectedBandLabel);

    shapeCombo.addItemList({ "Bell", "Low Shelf", "High Shelf", "Low Cut", "High Cut", "Notch", "Band Pass", "Tilt Shelf" }, 1);
    shapeCombo.setColour(juce::ComboBox::backgroundColourId, theme.panelElevated);
    shapeCombo.setColour(juce::ComboBox::textColourId, theme.textPrimary);
    shapeCombo.setColour(juce::ComboBox::outlineColourId, theme.borderSubtle);
    addAndMakeVisible(shapeCombo);

    characterCombo.addItemList({ "Clean", "Subtle", "Warm" }, 1);
    characterCombo.setColour(juce::ComboBox::backgroundColourId, theme.panelElevated);
    characterCombo.setColour(juce::ComboBox::textColourId, theme.textPrimary);
    characterCombo.setColour(juce::ComboBox::outlineColourId, theme.borderSubtle);
    characterCombo.setTooltip("Band character");
    addAndMakeVisible(characterCombo);

    auto configureToggle = [this](juce::ToggleButton& btn, juce::Colour activeCol)
    {
        btn.setColour(juce::ToggleButton::textColourId, theme.textMuted);
        btn.setColour(juce::ToggleButton::tickColourId, activeCol);
        btn.setColour(juce::ToggleButton::tickDisabledColourId, theme.textMuted);
        addAndMakeVisible(btn);
    };

    configureToggle(dynToggle, juce::Colour(0xff38bdf8));
    configureToggle(specToggle, juce::Colour(0xfff43f5e));
    configureToggle(soloToggle, juce::Colour(0xfffbbf24));

    bandBypassToggle.setClickingTogglesState(true);
    bandBypassToggle.setColour(juce::TextButton::buttonColourId, theme.panelElevated);
    bandBypassToggle.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0x66ef4444));
    bandBypassToggle.setColour(juce::TextButton::textColourOffId, theme.textMuted);
    bandBypassToggle.setColour(juce::TextButton::textColourOnId, theme.textPrimary);
    bandBypassToggle.onClick = [this]()
    {
        const auto prefix = bandParamId(selectedBand, "");
        if (auto* param = processorRef.apvts.getParameter(prefix + "enabled"))
            param->setValueNotifyingHost(bandBypassToggle.getToggleState() ? 0.0f : 1.0f);
        repaint();
    };
    addAndMakeVisible(bandBypassToggle);

    removeBandButton.setColour(juce::TextButton::buttonColourId, theme.panelElevated);
    removeBandButton.setColour(juce::TextButton::textColourOffId, theme.textPrimary);
    removeBandButton.onClick = [this]()
    {
        const auto prefix = bandParamId(selectedBand, "");
        if (auto* param = processorRef.apvts.getParameter(prefix + "enabled"))
            param->setValueNotifyingHost(0.0f);
        bandStripVisible = false;
        resized();
        repaint();
    };
    addAndMakeVisible(removeBandButton);

    smoothMainCurve.fill(-9.0f);
    smoothPreCurve.fill(-9.0f);
    fastRedCurve.fill(0.0f);

    selectBand(0, true);
    startTimerHz(60);
}

HybridResonanceEQEditor::~HybridResonanceEQEditor()
{
    stopTimer();
}

void HybridResonanceEQEditor::syncBandBypassToggle()
{
    const auto prefix = bandParamId(selectedBand, "");
    const bool enabled = processorRef.apvts.getRawParameterValue(prefix + "enabled")->load() > 0.5f;
    bandBypassToggle.setToggleState(!enabled, juce::dontSendNotification);
}

void HybridResonanceEQEditor::selectBand(int bandIndex, bool showStrip)
{
    selectedBand = juce::jlimit(0, dspcore::ParametricEQEngine::maxBands - 1, bandIndex);
    bandStripVisible = showStrip;
    rebindSelectedBandControls();
    syncBandBypassToggle();
    resized();
    repaint();
}

void HybridResonanceEQEditor::rebindSelectedBandControls()
{
    shapeAttachment.reset();
    characterAttachment.reset();
    dynAttachment.reset();
    specAttachment.reset();
    soloAttachment.reset();

    const juce::String prefix = bandParamId(selectedBand, "");
    const juce::Colour col = getBandColour(selectedBand);

    selectedBandLabel.setText("BAND " + juce::String(selectedBand + 1), juce::dontSendNotification);
    selectedBandLabel.setColour(juce::Label::textColourId, col);

    shapeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processorRef.apvts, prefix + "shape", shapeCombo);
    characterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processorRef.apvts, prefix + "character", characterCombo);
    dynAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.apvts, prefix + "dyn_enabled", dynToggle);
    specAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.apvts, prefix + "spec_enabled", specToggle);
    soloAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.apvts, prefix + "solo", soloToggle);
}

int HybridResonanceEQEditor::findFreeBandSlot() const noexcept
{
    for (int i = 0; i < dspcore::ParametricEQEngine::maxBands; ++i)
    {
        const auto prefix = bandParamId(i, "");
        if (processorRef.apvts.getRawParameterValue(prefix + "enabled")->load() <= 0.5f)
            return i;
    }
    return selectedBand;
}

void HybridResonanceEQEditor::createBandAt(juce::Point<float> pos, bool spectral, bool dynamic)
{
    const int slot = findFreeBandSlot();
    const auto prefix = bandParamId(slot, "");
    const float freq = xToFreq(pos.x);
    const float gain = juce::jlimit(-24.0f, 24.0f, yToDb(pos.y, getDisplayRangeDb()));

    int shapeChoice = 0; // Bell
    if (freq < 65.0f && std::abs(gain) > 2.5f)
        shapeChoice = 3; // Low Cut
    else if (freq > 11000.0f && std::abs(gain) > 2.5f)
        shapeChoice = 4; // High Cut

    if (auto* p = processorRef.apvts.getParameter(prefix + "enabled"))
        p->setValueNotifyingHost(1.0f);
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*>(processorRef.apvts.getParameter(prefix + "shape")))
        p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(shapeChoice)));
    if (auto* p = processorRef.apvts.getParameter(prefix + "freq"))
        p->setValueNotifyingHost(p->convertTo0to1(freq));
    if (auto* p = processorRef.apvts.getParameter(prefix + "gain"))
        p->setValueNotifyingHost(p->convertTo0to1(gain));
    if (auto* p = processorRef.apvts.getParameter(prefix + "spec_enabled"))
        p->setValueNotifyingHost(spectral ? 1.0f : 0.0f);
    if (auto* p = processorRef.apvts.getParameter(prefix + "dyn_enabled"))
        p->setValueNotifyingHost(dynamic ? 1.0f : 0.0f);

    selectBand(slot, true);
    draggingBand = slot;
}

float HybridResonanceEQEditor::getDisplayRangeDb() const noexcept
{
    const int idx = static_cast<int>(processorRef.apvts.getRawParameterValue("display_range")->load());
    if (idx == 0) return 6.0f;
    if (idx == 2) return 30.0f;
    return 12.0f;
}

float HybridResonanceEQEditor::freqToX(float freqHz) const noexcept
{
    const float clamped = juce::jlimit(kMinFreq, kMaxFreq, freqHz);
    const float norm = std::log(clamped / kMinFreq) / std::log(kMaxFreq / kMinFreq);
    return static_cast<float>(graphBounds.getX()) + norm * static_cast<float>(graphBounds.getWidth());
}

float HybridResonanceEQEditor::xToFreq(float x) const noexcept
{
    const float w = juce::jmax(1.0f, static_cast<float>(graphBounds.getWidth()));
    const float norm = juce::jlimit(0.0f, 1.0f, (x - static_cast<float>(graphBounds.getX())) / w);
    return kMinFreq * std::pow(kMaxFreq / kMinFreq, norm);
}

float HybridResonanceEQEditor::dbToY(float db, float maxDb) const noexcept
{
    const float clamped = juce::jlimit(-maxDb, maxDb, db);
    const float norm = (maxDb - clamped) / (2.0f * maxDb);
    return static_cast<float>(graphBounds.getY()) + norm * static_cast<float>(graphBounds.getHeight());
}

float HybridResonanceEQEditor::yToDb(float y, float maxDb) const noexcept
{
    const float h = juce::jmax(1.0f, static_cast<float>(graphBounds.getHeight()));
    const float norm = juce::jlimit(0.0f, 1.0f, (y - static_cast<float>(graphBounds.getY())) / h);
    return maxDb - norm * (2.0f * maxDb);
}

juce::Colour HybridResonanceEQEditor::getBandColour(int bandIndex) const noexcept
{
    static constexpr juce::uint32 palette[] = {
        0xff38bdf8, 0xff34d399, 0xfffbbf24, 0xfff472b6,
        0xffa78bfa, 0xff2dd4bf, 0xfffb923c, 0xff60a5fa,
        0xff4ade80, 0xffe879f9, 0xff22d3ee, 0xfffacc15,
        0xff818cf8, 0xfff87171, 0xffa3e635, 0xffc084fc
    };
    const int idx = juce::jlimit(0, 15, bandIndex);
    return juce::Colour(palette[idx]);
}

int HybridResonanceEQEditor::hitTestBandNode(juce::Point<float> pos) const noexcept
{
    const float rangeDb = getDisplayRangeDb();
    int bestIdx = -1;
    float bestDist = 18.0f;

    for (int i = 0; i < dspcore::ParametricEQEngine::maxBands; ++i)
    {
        const auto prefix = bandParamId(i, "");
        if (processorRef.apvts.getRawParameterValue(prefix + "enabled")->load() <= 0.5f)
            continue;

        const float freq = processorRef.apvts.getRawParameterValue(prefix + "freq")->load();
        const float gain = processorRef.apvts.getRawParameterValue(prefix + "gain")->load();
        const float nx = freqToX(freq);
        const float ny = dbToY(gain, rangeDb);
        const float d = pos.getDistanceFrom({ nx, ny });

        if (d < bestDist)
        {
            bestDist = d;
            bestIdx = i;
        }
    }
    return bestIdx;
}

void HybridResonanceEQEditor::showBandContextMenu(int bandIndex, juce::Point<float> pos)
{
    selectBand(bandIndex, true);
    const auto prefix = bandParamId(bandIndex, "");

    const bool enabled = processorRef.apvts.getRawParameterValue(prefix + "enabled")->load() > 0.5f;
    const bool soloOn = processorRef.apvts.getRawParameterValue(prefix + "solo")->load() > 0.5f;
    const bool dynOn = processorRef.apvts.getRawParameterValue(prefix + "dyn_enabled")->load() > 0.5f;
    const bool specOn = processorRef.apvts.getRawParameterValue(prefix + "spec_enabled")->load() > 0.5f;
    const int curShape = static_cast<int>(processorRef.apvts.getRawParameterValue(prefix + "shape")->load());
    const int curChar = static_cast<int>(processorRef.apvts.getRawParameterValue(prefix + "character")->load());
    const float curSlope = processorRef.apvts.getRawParameterValue(prefix + "slope")->load();

    juce::PopupMenu menu;
    menu.addSectionHeader("Band " + juce::String(bandIndex + 1));

    juce::PopupMenu shapeMenu;
    const juce::StringArray shapes { "Bell", "Low Shelf", "High Shelf", "Low Cut", "High Cut", "Notch", "Band Pass", "Tilt Shelf" };
    for (int s = 0; s < shapes.size(); ++s)
        shapeMenu.addItem(100 + s, shapes[s], true, s == curShape);
    menu.addSubMenu("Shape", shapeMenu);

    juce::PopupMenu slopeMenu;
    static constexpr float slopes[] { 6.0f, 12.0f, 18.0f, 24.0f, 36.0f, 48.0f };
    for (int i = 0; i < 6; ++i)
        slopeMenu.addItem(300 + i, juce::String(static_cast<int>(slopes[i])) + " dB/oct", true, std::abs(curSlope - slopes[i]) < 0.1f);
    menu.addSubMenu("Slope", slopeMenu);

    juce::PopupMenu charMenu;
    const juce::StringArray chars { "Clean", "Subtle", "Warm" };
    for (int c = 0; c < chars.size(); ++c)
        charMenu.addItem(200 + c, chars[c], true, c == curChar);
    menu.addSubMenu("Character", charMenu);

    menu.addSeparator();
    menu.addItem(2, "Solo", true, soloOn);
    menu.addItem(3, "Dynamic", true, dynOn);
    menu.addItem(4, "Spectral", true, specOn);
    menu.addItem(1, "Bypass Band", true, !enabled);
    menu.addSeparator();
    menu.addItem(10, "Reset Gain & Q");
    menu.addItem(11, "Delete Band");

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
        [this, bandIndex, prefix, enabled, soloOn, dynOn, specOn](int result)
        {
            if (result == 0)
                return;

            auto setBool = [this](const juce::String& id, bool v)
            {
                if (auto* p = processorRef.apvts.getParameter(id))
                    p->setValueNotifyingHost(v ? 1.0f : 0.0f);
            };
            auto setFloat = [this](const juce::String& id, float v)
            {
                if (auto* p = processorRef.apvts.getParameter(id))
                    p->setValueNotifyingHost(p->convertTo0to1(v));
            };
            auto setChoice = [this](const juce::String& id, int idx)
            {
                if (auto* p = dynamic_cast<juce::AudioParameterChoice*>(processorRef.apvts.getParameter(id)))
                    p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(idx)));
            };

            static constexpr float slopes[] { 6.0f, 12.0f, 18.0f, 24.0f, 36.0f, 48.0f };

            if (result == 1) setBool(prefix + "enabled", !enabled);
            else if (result == 2) setBool(prefix + "solo", !soloOn);
            else if (result == 3) setBool(prefix + "dyn_enabled", !dynOn);
            else if (result == 4) setBool(prefix + "spec_enabled", !specOn);
            else if (result == 10) { setFloat(prefix + "gain", 0.0f); setFloat(prefix + "q", 1.0f); }
            else if (result == 11) { setBool(prefix + "enabled", false); bandStripVisible = false; }
            else if (result >= 100 && result < 200) setChoice(prefix + "shape", result - 100);
            else if (result >= 200 && result < 300) setChoice(prefix + "character", result - 200);
            else if (result >= 300 && result < 306) setFloat(prefix + "slope", slopes[result - 300]);

            syncBandBypassToggle();
            selectBand(bandIndex, result != 11);
            repaint();
        });

    juce::ignoreUnused(pos);
}

void HybridResonanceEQEditor::showGraphContextMenu(juce::Point<float> pos)
{
    const int hit = hitTestBandNode(pos);
    if (hit >= 0)
    {
        showBandContextMenu(hit, pos);
        return;
    }

    const int curRangeIdx = static_cast<int>(processorRef.apvts.getRawParameterValue("display_range")->load());

    juce::PopupMenu menu;
    menu.addSectionHeader("EQ Display");
    menu.addItem(1, "Add Band Here (Double-Click)");
    menu.addItem(2, "Add Spectral Band Here");
    menu.addSeparator();

    juce::PopupMenu rangeMenu;
    rangeMenu.addItem(20, "+/- 6 dB", true, curRangeIdx == 0);
    rangeMenu.addItem(21, "+/- 12 dB", true, curRangeIdx == 1);
    rangeMenu.addItem(22, "+/- 30 dB", true, curRangeIdx == 2);
    menu.addSubMenu("Display Range", rangeMenu);

    menu.addItem(30, "Deselect Band");

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
        [this, pos](int result)
        {
            if (result == 0)
                return;

            if (result == 1)
                createBandAt(pos, false, false);
            else if (result == 2)
                createBandAt(pos, true, false);
            else if (result >= 20 && result <= 22)
            {
                if (auto* p = dynamic_cast<juce::AudioParameterChoice*>(processorRef.apvts.getParameter("display_range")))
                    p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(result - 20)));
            }
            else if (result == 30)
            {
                bandStripVisible = false;
                resized();
            }
            repaint();
        });
}

void HybridResonanceEQEditor::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (!graphBounds.contains(e.getPosition()))
        return;

    const int hit = hitTestBandNode(e.position);
    if (hit >= 0)
    {
        selectBand(hit, true);
        return;
    }

    createBandAt(e.position, e.mods.isShiftDown() && e.mods.isAltDown(), e.mods.isAltDown());
}

void HybridResonanceEQEditor::mouseDown(const juce::MouseEvent& e)
{
    if (!graphBounds.contains(e.getPosition()))
        return;

    if (e.mods.isPopupMenu() || e.mods.isRightButtonDown())
    {
        showGraphContextMenu(e.position);
        return;
    }

    const int hit = hitTestBandNode(e.position);
    if (hit >= 0)
    {
        selectBand(hit, true);
        draggingBand = hit;
    }
    else
    {
        bandStripVisible = false;
        draggingBand = -1;
        resized();
        repaint();
    }
}

void HybridResonanceEQEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (draggingBand < 0 || draggingBand >= dspcore::ParametricEQEngine::maxBands)
        return;

    const auto prefix = bandParamId(draggingBand, "");
    const float rangeDb = getDisplayRangeDb();

    if (auto* fParam = processorRef.apvts.getParameter(prefix + "freq"))
        fParam->setValueNotifyingHost(fParam->convertTo0to1(xToFreq(e.position.x)));

    if (auto* gParam = processorRef.apvts.getParameter(prefix + "gain"))
        gParam->setValueNotifyingHost(gParam->convertTo0to1(juce::jlimit(-24.0f, 24.0f, yToDb(e.position.y, rangeDb))));

    repaint();
}

void HybridResonanceEQEditor::mouseUp(const juce::MouseEvent&)
{
    draggingBand = -1;
}

void HybridResonanceEQEditor::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (!graphBounds.contains(e.getPosition()))
        return;

    const int hit = hitTestBandNode(e.position);
    const int target = (hit >= 0) ? hit : selectedBand;
    selectBand(target, true);

    const auto prefix = bandParamId(target, "");
    if (auto* qParam = processorRef.apvts.getParameter(prefix + "q"))
    {
        const float curQ = processorRef.apvts.getRawParameterValue(prefix + "q")->load();
        const float factor = (wheel.deltaY >= 0.0f) ? 1.12f : (1.0f / 1.12f);
        const float nextQ = juce::jlimit(0.1f, 18.0f, curQ * factor);
        qParam->setValueNotifyingHost(qParam->convertTo0to1(nextQ));
        repaint();
    }
}

void HybridResonanceEQEditor::timerCallback()
{
    processorRef.getVisualData(rawPreBins, rawPostBins, rawRedBins);

    const float rangeDb = getDisplayRangeDb();
    const float nyquist = juce::jmax(1000.0f, static_cast<float>(processorRef.getCurrentSampleRateHz()) * 0.5f);
    const int maxBin = juce::jmax(1, HybridResonanceEQProcessor::analyzerNumBins - 1);

    // Main spectrum: slower, organic smoothed waves across log frequencies
    constexpr float mainInertia = 0.045f;
    for (int i = 0; i < renderPoints; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(renderPoints - 1);
        const float freq = kMinFreq * std::pow(kMaxFreq / kMinFreq, t);
        const float binPos = juce::jlimit(0.0f, static_cast<float>(maxBin),
                                          (freq / nyquist) * static_cast<float>(maxBin));
        const int centerBin = static_cast<int>(std::round(binPos));

        float weightedSum = 0.0f;
        float weightTotal = 0.0f;
        for (int k = -5; k <= 5; ++k)
        {
            const int b = juce::jlimit(0, maxBin, centerBin + k);
            const float w = 1.0f - 0.15f * std::abs(k);
            weightedSum += rawPostBins[static_cast<size_t>(b)] * w;
            weightTotal += w;
        }
        const float dbVal = (weightTotal > 0.0f) ? (weightedSum / weightTotal) : -90.0f;
        const float normLevel = juce::jlimit(0.0f, 1.0f, (dbVal + 72.0f) / 66.0f);
        const float targetDb = -rangeDb + normLevel * (rangeDb * 1.15f);

        smoothMainCurve[static_cast<size_t>(i)] +=
            (juce::jlimit(-rangeDb, rangeDb, targetDb) - smoothMainCurve[static_cast<size_t>(i)]) * mainInertia;

        float preWeightedSum = 0.0f;
        float preWeightTotal = 0.0f;
        for (int k = -5; k <= 5; ++k)
        {
            const int b = juce::jlimit(0, maxBin, centerBin + k);
            const float w = 1.0f - 0.15f * std::abs(k);
            preWeightedSum += rawPreBins[static_cast<size_t>(b)] * w;
            preWeightTotal += w;
        }
        const float preDb = (preWeightTotal > 0.0f) ? (preWeightedSum / preWeightTotal) : -90.0f;
        const float preNorm = juce::jlimit(0.0f, 1.0f, (preDb + 72.0f) / 66.0f);
        const float preTarget = -rangeDb + preNorm * (rangeDb * 1.15f);
        smoothPreCurve[static_cast<size_t>(i)] +=
            (juce::jlimit(-rangeDb, rangeDb, preTarget) - smoothPreCurve[static_cast<size_t>(i)]) * mainInertia;
    }

    // Reduction spectrum: faster response and higher detail along log frequencies
    constexpr float fastRedResponse = 0.55f;
    for (int i = 0; i < renderPoints; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(renderPoints - 1);
        const float freq = kMinFreq * std::pow(kMaxFreq / kMinFreq, t);
        const float binPos = juce::jlimit(0.0f, static_cast<float>(maxBin),
                                          (freq / nyquist) * static_cast<float>(maxBin));
        const int b0 = static_cast<int>(binPos);
        const int b1 = juce::jmin(maxBin, b0 + 1);
        const float frac = binPos - static_cast<float>(b0);

        const float interpRed = rawRedBins[static_cast<size_t>(b0)] * (1.0f - frac)
                              + rawRedBins[static_cast<size_t>(b1)] * frac;
        const float targetRed = juce::jlimit(-rangeDb, 0.0f, interpRed);

        fastRedCurve[static_cast<size_t>(i)] +=
            (targetRed - fastRedCurve[static_cast<size_t>(i)]) * fastRedResponse;
    }

    repaint();
}

void HybridResonanceEQEditor::resized()
{
    auto area = getLocalBounds().reduced(12);

    topBarBounds = area.removeFromTop(44);
    area.removeFromTop(10);

    const auto prefix = bandParamId(selectedBand, "");
    const bool showStrip = bandStripVisible
        && (processorRef.apvts.getRawParameterValue(prefix + "enabled")->load() > 0.5f);

    if (showStrip)
    {
        bottomStripBounds = area.removeFromBottom(62);
        area.removeFromBottom(10);
    }
    else
    {
        bottomStripBounds = {};
    }

    graphBounds = area;

    auto topRow = topBarBounds.reduced(14, 6);
    titleLabel.setBounds(topRow.removeFromLeft(220));

    bypassButton.setBounds(topRow.removeFromRight(84));
    topRow.removeFromRight(10);
    postAnalyzerButton.setBounds(topRow.removeFromRight(58));
    topRow.removeFromRight(4);
    preAnalyzerButton.setBounds(topRow.removeFromRight(52));
    topRow.removeFromRight(12);
    tiltCombo.setBounds(topRow.removeFromRight(86));
    topRow.removeFromRight(6);
    tiltLabel.setBounds(topRow.removeFromRight(30));
    topRow.removeFromRight(10);
    specResolutionCombo.setBounds(topRow.removeFromRight(142));
    topRow.removeFromRight(5);
    specResolutionLabel.setBounds(topRow.removeFromRight(54));

    const bool stripActive = !bottomStripBounds.isEmpty();
    selectedBandLabel.setVisible(stripActive);
    shapeCombo.setVisible(stripActive);
    characterCombo.setVisible(stripActive);
    dynToggle.setVisible(stripActive);
    specToggle.setVisible(stripActive);
    soloToggle.setVisible(stripActive);
    bandBypassToggle.setVisible(stripActive);
    removeBandButton.setVisible(stripActive);

    if (stripActive)
    {
        auto row = bottomStripBounds.reduced(14, 10);
        selectedBandLabel.setBounds(row.removeFromLeft(84));
        shapeCombo.setBounds(row.removeFromLeft(116));
        row.removeFromLeft(8);
        characterCombo.setBounds(row.removeFromLeft(96));
        row.removeFromLeft(14);

        dynToggle.setBounds(row.removeFromLeft(82));
        specToggle.setBounds(row.removeFromLeft(86));
        soloToggle.setBounds(row.removeFromLeft(66));
        row.removeFromLeft(8);

        removeBandButton.setBounds(row.removeFromRight(78));
        row.removeFromRight(8);
        bandBypassToggle.setBounds(row.removeFromRight(94));
    }
}

void HybridResonanceEQEditor::drawModernBackground(juce::Graphics& g) const
{
    juce::ColourGradient bgGrad(
        juce::Colour(0xff0d1017), 0.0f, 0.0f,
        juce::Colour(0xff07090d), 0.0f, static_cast<float>(getHeight()), false);
    g.setGradientFill(bgGrad);
    g.fillAll();

    auto drawGlassPanel = [&g, this](juce::Rectangle<int> rect)
    {
        if (rect.isEmpty())
            return;
        juce::Path p;
        p.addRoundedRectangle(rect.toFloat(), 14.0f);
        g.setColour(theme.panelGlass);
        g.fillPath(p);
        g.setColour(theme.borderSubtle);
        g.strokePath(p, juce::PathStrokeType(1.0f));
    };

    drawGlassPanel(topBarBounds);
    drawGlassPanel(graphBounds);
    if (!bottomStripBounds.isEmpty())
        drawGlassPanel(bottomStripBounds);
}

void HybridResonanceEQEditor::drawGraphGridAndCurves(juce::Graphics& g)
{
    if (graphBounds.isEmpty())
        return;

    juce::Graphics::ScopedSaveState clipState(g);
    g.reduceClipRegion(graphBounds.reduced(2));

    const float rangeDb = getDisplayRangeDb();
    const float zeroY = dbToY(0.0f, rangeDb);
    const float bottomY = static_cast<float>(graphBounds.getBottom());

    static constexpr float gridFreqs[] { 30.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f };
    g.setFont(juce::FontOptions(10.5f));

    for (float f : gridFreqs)
    {
        const float x = freqToX(f);
        g.setColour(theme.gridMinor);
        g.drawVerticalLine(static_cast<int>(std::round(x)), static_cast<float>(graphBounds.getY() + 6),
                           static_cast<float>(graphBounds.getBottom() - 6));

        g.setColour(theme.textMuted);
        const juce::String lbl = (f >= 1000.0f) ? (juce::String(f / 1000.0f, 0) + "k") : juce::String(static_cast<int>(f));
        g.drawText(lbl, static_cast<int>(x) - 16, graphBounds.getBottom() - 20, 32, 16, juce::Justification::centred);
    }

    const int dbSteps = (rangeDb <= 6.0f) ? 3 : ((rangeDb <= 12.0f) ? 6 : 15);
    for (int d = -static_cast<int>(rangeDb); d <= static_cast<int>(rangeDb); d += dbSteps)
    {
        const float y = dbToY(static_cast<float>(d), rangeDb);
        g.setColour(d == 0 ? theme.borderAccent : theme.gridMajor);
        g.drawHorizontalLine(static_cast<int>(std::round(y)), static_cast<float>(graphBounds.getX() + 8),
                             static_cast<float>(graphBounds.getRight() - 8));

        g.setColour(theme.textMuted);
        g.drawText(juce::String(d) + " dB", graphBounds.getX() + 8, static_cast<int>(y) - 9, 44, 18,
                   juce::Justification::left);
    }

    // 1. Pre-EQ spectrum (thin, subdued trace)
    const bool showPre = processorRef.apvts.getRawParameterValue("ana_show_pre")->load() > 0.5f;
    const bool showPost = processorRef.apvts.getRawParameterValue("ana_show_post")->load() > 0.5f;
    if (showPre)
    {
        juce::Path preStroke;
        for (int i = 0; i < renderPoints; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(renderPoints - 1);
            const float x = static_cast<float>(graphBounds.getX()) + t * static_cast<float>(graphBounds.getWidth());
            const float y = juce::jlimit(static_cast<float>(graphBounds.getY()), bottomY,
                dbToY(smoothPreCurve[static_cast<size_t>(i)], rangeDb));
            if (i == 0) preStroke.startNewSubPath(x, y);
            else preStroke.lineTo(x, y);
        }
        g.setColour(juce::Colour(0x887c8798));
        g.strokePath(preStroke, juce::PathStrokeType(1.0f));
    }

    // 2. Post-EQ spectrum
    juce::Path specStroke;
    juce::Path specFill;
    for (int i = 0; i < renderPoints; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(renderPoints - 1);
        const float x = static_cast<float>(graphBounds.getX()) + t * static_cast<float>(graphBounds.getWidth());
        const float y = juce::jlimit(static_cast<float>(graphBounds.getY()), bottomY,
                                     dbToY(smoothMainCurve[static_cast<size_t>(i)], rangeDb));

        if (i == 0)
        {
            specStroke.startNewSubPath(x, y);
            specFill.startNewSubPath(x, bottomY);
            specFill.lineTo(x, y);
        }
        else
        {
            specStroke.lineTo(x, y);
            specFill.lineTo(x, y);
        }
    }
    specFill.lineTo(static_cast<float>(graphBounds.getRight()), bottomY);
    specFill.closeSubPath();

    if (showPost)
    {
        g.setColour(theme.specMainFill);
        g.fillPath(specFill);
        g.setColour(theme.specMainStroke);
        g.strokePath(specStroke, juce::PathStrokeType(1.7f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // 3. Spectral reduction curve (fast, high-resolution continuous curve)
    juce::Path redStroke;
    juce::Path redFill;
    bool hasReduction = false;
    for (int i = 0; i < renderPoints; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(renderPoints - 1);
        const float x = static_cast<float>(graphBounds.getX()) + t * static_cast<float>(graphBounds.getWidth());
        const float redDb = juce::jmin(0.0f, fastRedCurve[static_cast<size_t>(i)]);
        if (redDb < -0.05f)
            hasReduction = true;
        const float y = juce::jlimit(static_cast<float>(graphBounds.getY()), bottomY, dbToY(redDb, rangeDb));

        if (i == 0)
        {
            redStroke.startNewSubPath(x, y);
            redFill.startNewSubPath(x, zeroY);
            redFill.lineTo(x, y);
        }
        else
        {
            redStroke.lineTo(x, y);
            redFill.lineTo(x, y);
        }
    }
    redFill.lineTo(static_cast<float>(graphBounds.getRight()), zeroY);
    redFill.closeSubPath();

    if (hasReduction)
    {
        g.setColour(theme.redFill);
        g.fillPath(redFill);
        g.setColour(theme.redStroke);
        g.strokePath(redStroke, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // 4. Combined EQ curve
    juce::Path eqCurve;
    const int steps = 256;
    for (int s = 0; s <= steps; ++s)
    {
        const float t = static_cast<float>(s) / static_cast<float>(steps);
        const double freq = static_cast<double>(kMinFreq * std::pow(kMaxFreq / kMinFreq, t));
        const float x = static_cast<float>(graphBounds.getX()) + t * static_cast<float>(graphBounds.getWidth());
        const float totalDb = processorRef.getCombinedEQResponseDb(freq);
        const float y = juce::jlimit(static_cast<float>(graphBounds.getY()), bottomY, dbToY(totalDb, rangeDb));

        if (s == 0)
            eqCurve.startNewSubPath(x, y);
        else
            eqCurve.lineTo(x, y);
    }

    g.setColour(getBandColour(selectedBand));
    g.strokePath(eqCurve, juce::PathStrokeType(2.3f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void HybridResonanceEQEditor::drawBandNodes(juce::Graphics& g) const
{
    const float rangeDb = getDisplayRangeDb();

    for (int i = 0; i < dspcore::ParametricEQEngine::maxBands; ++i)
    {
        const auto prefix = bandParamId(i, "");
        if (processorRef.apvts.getRawParameterValue(prefix + "enabled")->load() <= 0.5f)
            continue;

        const float freq = processorRef.apvts.getRawParameterValue(prefix + "freq")->load();
        const float gain = processorRef.apvts.getRawParameterValue(prefix + "gain")->load();
        const float x = freqToX(freq);
        const float y = dbToY(gain, rangeDb);
        const juce::Colour col = getBandColour(i);
        const bool isSel = (i == selectedBand);

        if (isSel)
        {
            g.setColour(col.withAlpha(0.24f));
            g.fillEllipse(x - 15.0f, y - 15.0f, 30.0f, 30.0f);
        }

        g.setColour(col);
        g.fillEllipse(x - 7.5f, y - 7.5f, 15.0f, 15.0f);

        g.setColour(isSel ? theme.textPrimary : theme.bgDeep);
        g.drawEllipse(x - 7.5f, y - 7.5f, 15.0f, 15.0f, 1.5f);

        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.setColour(theme.textPrimary);
        g.drawText(juce::String(i + 1), static_cast<int>(x) - 8, static_cast<int>(y) - 8, 16, 16,
                   juce::Justification::centred);
    }
}

void HybridResonanceEQEditor::drawBottomBandStrip(juce::Graphics& g)
{
    if (bottomStripBounds.isEmpty())
        return;

    const juce::Colour col = getBandColour(selectedBand);
    juce::Path accentBar;
    accentBar.addRoundedRectangle(static_cast<float>(bottomStripBounds.getX() + 4),
                                  static_cast<float>(bottomStripBounds.getY() + 10),
                                  4.0f,
                                  static_cast<float>(bottomStripBounds.getHeight() - 20),
                                  2.0f);
    g.setColour(col);
    g.fillPath(accentBar);
}

void HybridResonanceEQEditor::paint(juce::Graphics& g)
{
    drawModernBackground(g);
    drawGraphGridAndCurves(g);
    drawBandNodes(g);
    drawBottomBandStrip(g);
}
