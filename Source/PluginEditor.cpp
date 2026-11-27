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
    setSize(1060, 620);
    setResizable(true, true);
    setResizeLimits(820, 480, 1920, 1200);

    // Título y Tilt en la barra superior
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

    bypassButton.setClickingTogglesState(true);
    bypassButton.setColour(juce::TextButton::buttonColourId, theme.panelElevated);
    bypassButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0x66f59e0b));
    bypassButton.setColour(juce::TextButton::textColourOffId, theme.textMuted);
    bypassButton.setColour(juce::TextButton::textColourOnId, theme.textPrimary);
    addAndMakeVisible(bypassButton);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.apvts, "global_bypass", bypassButton);

    // Controles de la franja dinámica inferior (por banda)
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
    characterCombo.setTooltip("Coloración / carácter de la banda");
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

    auto configureSlider = [this](juce::Slider& s, const juce::String& suffix)
    {
        s.setSliderStyle(juce::Slider::LinearHorizontal);
        s.setTextBoxStyle(juce::Slider::TextBoxRight, false, 64, 20);
        s.setTextValueSuffix(suffix);
        s.setColour(juce::Slider::trackColourId, theme.panelElevated);
        s.setColour(juce::Slider::thumbColourId, theme.textPrimary);
        s.setColour(juce::Slider::textBoxTextColourId, theme.textPrimary);
        s.setColour(juce::Slider::textBoxBackgroundColourId, theme.panelGlass);
        s.setColour(juce::Slider::textBoxOutlineColourId, theme.borderSubtle);
        addAndMakeVisible(s);
    };

    configureSlider(freqSlider, " Hz");
    configureSlider(gainSlider, " dB");
    configureSlider(qSlider, "");

    smoothMainCurve.fill(-90.0f);
    fastRedCurve.fill(0.0f);

    selectBand(0);
    startTimerHz(60);
}

HybridResonanceEQEditor::~HybridResonanceEQEditor()
{
    stopTimer();
}

void HybridResonanceEQEditor::selectBand(int bandIndex)
{
    selectedBand = juce::jlimit(0, dspcore::ParametricEQEngine::maxBands - 1, bandIndex);
    rebindSelectedBandControls();
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
    freqAttachment.reset();
    gainAttachment.reset();
    qAttachment.reset();

    const juce::String prefix = bandParamId(selectedBand, "");
    const juce::Colour col = getBandColour(selectedBand);

    selectedBandLabel.setText("BANDA " + juce::String(selectedBand + 1), juce::dontSendNotification);
    selectedBandLabel.setColour(juce::Label::textColourId, col);

    freqSlider.setColour(juce::Slider::thumbColourId, col);
    gainSlider.setColour(juce::Slider::thumbColourId, col);
    qSlider.setColour(juce::Slider::thumbColourId, col);

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
    freqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.apvts, prefix + "freq", freqSlider);
    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.apvts, prefix + "gain", gainSlider);
    qAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.apvts, prefix + "q", qSlider);
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

void HybridResonanceEQEditor::showGraphContextMenu(juce::Point<float> pos)
{
    const int hitBand = hitTestBandNode(pos);
    const int targetBand = (hitBand >= 0) ? hitBand : selectedBand;
    selectBand(targetBand);

    const auto prefix = bandParamId(targetBand, "");
    const bool enabled = processorRef.apvts.getRawParameterValue(prefix + "enabled")->load() > 0.5f;
    const bool soloOn = processorRef.apvts.getRawParameterValue(prefix + "solo")->load() > 0.5f;
    const bool dynOn = processorRef.apvts.getRawParameterValue(prefix + "dyn_enabled")->load() > 0.5f;
    const bool specOn = processorRef.apvts.getRawParameterValue(prefix + "spec_enabled")->load() > 0.5f;

    juce::PopupMenu menu;
    menu.addSectionHeader("Banda " + juce::String(targetBand + 1));

    menu.addItem(1, enabled ? "Desactivar banda" : "Activar / colocar banda aquí", true, enabled);
    menu.addItem(2, "Solo", true, soloOn);
    menu.addItem(3, "Dinámica (Dyn)", true, dynOn);
    menu.addItem(4, "Espectral (Spec)", true, specOn);

    juce::PopupMenu shapeMenu;
    const juce::StringArray shapes { "Bell", "Low Shelf", "High Shelf", "Low Cut", "High Cut", "Notch", "Band Pass", "Tilt Shelf" };
    const int curShape = static_cast<int>(processorRef.apvts.getRawParameterValue(prefix + "shape")->load());
    for (int s = 0; s < shapes.size(); ++s)
        shapeMenu.addItem(100 + s, shapes[s], true, s == curShape);
    menu.addSubMenu("Forma de filtro", shapeMenu);

    juce::PopupMenu charMenu;
    const juce::StringArray chars { "Clean", "Subtle", "Warm" };
    const int curChar = static_cast<int>(processorRef.apvts.getRawParameterValue(prefix + "character")->load());
    for (int c = 0; c < chars.size(); ++c)
        charMenu.addItem(200 + c, chars[c], true, c == curChar);
    menu.addSubMenu("Carácter / coloración", charMenu);

    menu.addSeparator();
    menu.addItem(10, "Resetear ganancia y Q");

    const float clickFreq = xToFreq(pos.x);
    const float clickGain = juce::jlimit(-24.0f, 24.0f, yToDb(pos.y, getDisplayRangeDb()));

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
        [this, targetBand, prefix, enabled, soloOn, dynOn, specOn, clickFreq, clickGain](int result)
        {
            if (result == 0)
                return;

            auto setBoolParam = [this](const juce::String& id, bool val)
            {
                if (auto* param = processorRef.apvts.getParameter(id))
                    param->setValueNotifyingHost(val ? 1.0f : 0.0f);
            };

            auto setFloatParam = [this](const juce::String& id, float realVal)
            {
                if (auto* param = processorRef.apvts.getParameter(id))
                    param->setValueNotifyingHost(param->convertTo0to1(realVal));
            };

            auto setChoiceParam = [this](const juce::String& id, int choiceIdx)
            {
                if (auto* param = dynamic_cast<juce::AudioParameterChoice*>(processorRef.apvts.getParameter(id)))
                    param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(choiceIdx)));
            };

            if (result == 1)
            {
                const bool nextEn = !enabled;
                setBoolParam(prefix + "enabled", nextEn);
                if (nextEn)
                {
                    setFloatParam(prefix + "freq", clickFreq);
                    setFloatParam(prefix + "gain", clickGain);
                }
            }
            else if (result == 2)
            {
                setBoolParam(prefix + "solo", !soloOn);
            }
            else if (result == 3)
            {
                setBoolParam(prefix + "dyn_enabled", !dynOn);
            }
            else if (result == 4)
            {
                setBoolParam(prefix + "spec_enabled", !specOn);
            }
            else if (result >= 100 && result < 200)
            {
                setChoiceParam(prefix + "shape", result - 100);
            }
            else if (result >= 200 && result < 300)
            {
                setChoiceParam(prefix + "character", result - 200);
            }
            else if (result == 10)
            {
                setFloatParam(prefix + "gain", 0.0f);
                setFloatParam(prefix + "q", 1.0f);
            }

            selectBand(targetBand);
            repaint();
        });
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
        selectBand(hit);
        draggingBand = hit;
    }
    else
    {
        // Activar o mover la banda seleccionada hacia el punto clicado
        const auto prefix = bandParamId(selectedBand, "");
        if (auto* enParam = processorRef.apvts.getParameter(prefix + "enabled"))
            enParam->setValueNotifyingHost(1.0f);

        if (auto* fParam = processorRef.apvts.getParameter(prefix + "freq"))
            fParam->setValueNotifyingHost(fParam->convertTo0to1(xToFreq(e.position.x)));

        const float rangeDb = getDisplayRangeDb();
        if (auto* gParam = processorRef.apvts.getParameter(prefix + "gain"))
            gParam->setValueNotifyingHost(gParam->convertTo0to1(juce::jlimit(-24.0f, 24.0f, yToDb(e.position.y, rangeDb))));

        draggingBand = selectedBand;
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
    selectBand(target);

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

    // Espectro general: movimiento lento y forma ondulada continua
    constexpr float mainInertia = 0.045f;
    for (int i = 0; i < renderPoints; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(renderPoints - 1);
        const float freq = kMinFreq * std::pow(kMaxFreq / kMinFreq, t);
        const int binIdx = juce::jlimit(0, dspcore::SpectralResonanceEngine::numBins - 1,
            static_cast<int>(freq / juce::jmax(100.0f, static_cast<float>(processorRef.getCurrentSampleRateHz()) * 0.5f)
                             * static_cast<float>(dspcore::SpectralResonanceEngine::numBins - 1)));

        // Suavizado en frecuencia alrededor del bin para ondas orgánicas
        float sumDb = 0.0f;
        int wCount = 0;
        for (int k = -4; k <= 4; ++k)
        {
            const int b = juce::jlimit(0, dspcore::SpectralResonanceEngine::numBins - 1, binIdx + k);
            const float weight = 1.0f - 0.16f * std::abs(k);
            sumDb += rawPostBins[static_cast<size_t>(b)] * weight;
            wCount += 1;
        }
        const float avgDb = (wCount > 0) ? (sumDb / 5.4f) : rawPostBins[static_cast<size_t>(binIdx)];
        const float mappedDb = juce::jlimit(-rangeDb, rangeDb, (avgDb + 48.0f) * (rangeDb / 42.0f) - rangeDb * 0.35f);
        smoothMainCurve[static_cast<size_t>(i)] += (mappedDb - smoothMainCurve[static_cast<size_t>(i)]) * mainInertia;
    }

    // Espectro de reducción: respuesta rápida y mayor resolución (512 puntos continuos)
    constexpr float fastRedResponse = 0.55f;
    for (int i = 0; i < renderPoints; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(renderPoints - 1);
        const float freq = kMinFreq * std::pow(kMaxFreq / kMinFreq, t);
        const int binIdx = juce::jlimit(0, dspcore::SpectralResonanceEngine::numBins - 1,
            static_cast<int>(freq / juce::jmax(100.0f, static_cast<float>(processorRef.getCurrentSampleRateHz()) * 0.5f)
                             * static_cast<float>(dspcore::SpectralResonanceEngine::numBins - 1)));

        const float targetRed = juce::jlimit(-rangeDb, 0.0f, rawRedBins[static_cast<size_t>(binIdx)]);
        fastRedCurve[static_cast<size_t>(i)] += (targetRed - fastRedCurve[static_cast<size_t>(i)]) * fastRedResponse;
    }

    repaint();
}

void HybridResonanceEQEditor::resized()
{
    auto area = getLocalBounds().reduced(12);

    topBarBounds = area.removeFromTop(44);
    area.removeFromTop(10);

    const auto prefix = bandParamId(selectedBand, "");
    const bool bandSelectedActive = processorRef.apvts.getRawParameterValue(prefix + "enabled")->load() > 0.5f;

    if (bandSelectedActive)
    {
        bottomStripBounds = area.removeFromBottom(68);
        area.removeFromBottom(10);
    }
    else
    {
        bottomStripBounds = {};
    }

    graphBounds = area;

    // Layout de barra superior (solo título, Tilt y Bypass)
    auto topRow = topBarBounds.reduced(14, 6);
    titleLabel.setBounds(topRow.removeFromLeft(210));

    bypassButton.setBounds(topRow.removeFromRight(84));
    topRow.removeFromRight(12);
    tiltCombo.setBounds(topRow.removeFromRight(86));
    topRow.removeFromRight(6);
    tiltLabel.setBounds(topRow.removeFromRight(32));

    // Layout de la franja dinámica inferior
    if (!bottomStripBounds.isEmpty())
    {
        auto row = bottomStripBounds.reduced(14, 10);
        selectedBandLabel.setBounds(row.removeFromLeft(90));

        shapeCombo.setBounds(row.removeFromLeft(118));
        row.removeFromLeft(8);
        characterCombo.setBounds(row.removeFromLeft(96));
        row.removeFromLeft(12);

        dynToggle.setBounds(row.removeFromLeft(56));
        specToggle.setBounds(row.removeFromLeft(64));
        soloToggle.setBounds(row.removeFromLeft(62));
        row.removeFromLeft(10);

        const int sliderW = juce::jmax(110, row.getWidth() / 3 - 8);
        freqSlider.setBounds(row.removeFromLeft(sliderW));
        row.removeFromLeft(8);
        gainSlider.setBounds(row.removeFromLeft(sliderW));
        row.removeFromLeft(8);
        qSlider.setBounds(row);

        shapeCombo.setVisible(true);
        characterCombo.setVisible(true);
        dynToggle.setVisible(true);
        specToggle.setVisible(true);
        soloToggle.setVisible(true);
        freqSlider.setVisible(true);
        gainSlider.setVisible(true);
        qSlider.setVisible(true);
        selectedBandLabel.setVisible(true);
    }
    else
    {
        selectedBandLabel.setVisible(false);
        shapeCombo.setVisible(false);
        characterCombo.setVisible(false);
        dynToggle.setVisible(false);
        specToggle.setVisible(false);
        soloToggle.setVisible(false);
        freqSlider.setVisible(false);
        gainSlider.setVisible(false);
        qSlider.setVisible(false);
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

    const float rangeDb = getDisplayRangeDb();
    const float zeroY = dbToY(0.0f, rangeDb);

    // Cuadrícula logarítmica de frecuencias y líneas de dB
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

    // 1. Espectro general (curva continua lenta y ondulada)
    juce::Path specStroke;
    juce::Path specFill;
    for (int i = 0; i < renderPoints; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(renderPoints - 1);
        const float x = static_cast<float>(graphBounds.getX()) + t * static_cast<float>(graphBounds.getWidth());
        const float y = dbToY(smoothMainCurve[static_cast<size_t>(i)], rangeDb);

        if (i == 0)
        {
            specStroke.startNewSubPath(x, y);
            specFill.startNewSubPath(x, zeroY);
            specFill.lineTo(x, y);
        }
        else
        {
            specStroke.lineTo(x, y);
            specFill.lineTo(x, y);
        }
    }
    specFill.lineTo(static_cast<float>(graphBounds.getRight()), zeroY);
    specFill.closeSubPath();

    g.setColour(theme.specMainFill);
    g.fillPath(specFill);
    g.setColour(theme.specMainStroke);
    g.strokePath(specStroke, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // 2. Espectro de reducción (curva continua rápida y de alta resolución, sin bloques)
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
        const float y = dbToY(redDb, rangeDb);

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

    // 3. Curva combinada de EQ (suave y continua)
    juce::Path eqCurve;
    const int steps = 256;
    for (int s = 0; s <= steps; ++s)
    {
        const float t = static_cast<float>(s) / static_cast<float>(steps);
        const double freq = static_cast<double>(kMinFreq * std::pow(kMaxFreq / kMinFreq, t));
        const float x = static_cast<float>(graphBounds.getX()) + t * static_cast<float>(graphBounds.getWidth());
        const float totalDb = processorRef.getCombinedEQResponseDb(freq);
        const float y = dbToY(totalDb, rangeDb);

        if (s == 0)
            eqCurve.startNewSubPath(x, y);
        else
            eqCurve.lineTo(x, y);
    }

    g.setColour(getBandColour(selectedBand));
    g.strokePath(eqCurve, juce::PathStrokeType(2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
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
