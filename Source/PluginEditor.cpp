#include "PluginEditor.h"
#include <cmath>

namespace
{
constexpr float kMinFreq = 20.0f;
constexpr float kMaxFreq = 20000.0f;
constexpr float kMinGainDb = -18.0f;
constexpr float kMaxGainDb = 18.0f;
}

HybridResonanceEQAudioProcessorEditor::HybridResonanceEQAudioProcessorEditor(HybridResonanceEQAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    spectrumDb.fill(-120.0f);
    dynOffsetsDb.fill(0.0f);

    auto setupKnob = [this](juce::Slider& s, const juce::String& suffix)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 16);
        s.setTextValueSuffix(suffix);
        addAndMakeVisible(s);
    };

    setupKnob(freqSlider, " Hz");
    setupKnob(gainSlider, " dB");
    setupKnob(qSlider, "");

    bandTypeBox.addItemList({ "Bell", "Low Shelf", "High Shelf", "Low Cut", "High Cut", "Notch" }, 1);
    bandRouteBox.addItemList({ "Stereo", "Left", "Right", "Mid", "Side" }, 1);
    addAndMakeVisible(bandTypeBox);
    addAndMakeVisible(bandRouteBox);

    addAndMakeVisible(dynButton);
    addAndMakeVisible(soloButton);
    addAndMakeVisible(bandEnableButton);

    setupKnob(depthSlider, "");
    setupKnob(sharpSlider, "");
    setupKnob(smoothSlider, "");
    setupKnob(mixSlider, "");

    addAndMakeVisible(specEnableButton);
    addAndMakeVisible(specDeltaButton);

    syncControlsToSelectedBand();

    // Tasa fija de 30 FPS para equilibrio entre fluidez y bajo consumo de CPU
    startTimerHz(30);
    setSize(960, 560);
    setResizable(true, true);
    setResizeLimits(720, 420, 1600, 1000);
}

void HybridResonanceEQAudioProcessorEditor::syncControlsToSelectedBand()
{
    auto& apvts = audioProcessor.getAPVTS();
    const int b = selectedBandIndex;

    bandEnAtt.reset();
    bandSoloAtt.reset();
    bandDynAtt.reset();
    bandTypeAtt.reset();
    bandRouteAtt.reset();
    freqAtt.reset();
    gainAtt.reset();
    qAtt.reset();

    bandEnAtt    = std::make_unique<ButtonAttachment>(apvts, HybridResonanceEQAudioProcessor::bandEnabledId(b), bandEnableButton);
    bandSoloAtt  = std::make_unique<ButtonAttachment>(apvts, HybridResonanceEQAudioProcessor::bandSoloId(b), soloButton);
    bandDynAtt   = std::make_unique<ButtonAttachment>(apvts, HybridResonanceEQAudioProcessor::bandDynEnId(b), dynButton);
    bandTypeAtt  = std::make_unique<ComboAttachment>(apvts, HybridResonanceEQAudioProcessor::bandTypeId(b), bandTypeBox);
    bandRouteAtt = std::make_unique<ComboAttachment>(apvts, HybridResonanceEQAudioProcessor::bandRouteId(b), bandRouteBox);
    freqAtt      = std::make_unique<SliderAttachment>(apvts, HybridResonanceEQAudioProcessor::bandFreqId(b), freqSlider);
    gainAtt      = std::make_unique<SliderAttachment>(apvts, HybridResonanceEQAudioProcessor::bandGainId(b), gainSlider);
    qAtt         = std::make_unique<SliderAttachment>(apvts, HybridResonanceEQAudioProcessor::bandQId(b), qSlider);
}

void HybridResonanceEQAudioProcessorEditor::timerCallback()
{
    audioProcessor.copyLatestSpectrumDb(spectrumDb);
    audioProcessor.copyLatestDynamicOffsets(dynOffsetsDb);
    repaint();
}

float HybridResonanceEQAudioProcessorEditor::freqToX(float freqHz) const noexcept
{
    const float clamped = juce::jlimit(kMinFreq, kMaxFreq, freqHz);
    const float norm = std::log10(clamped / kMinFreq) / std::log10(kMaxFreq / kMinFreq);
    return plotBounds.getX() + norm * plotBounds.getWidth();
}

float HybridResonanceEQAudioProcessorEditor::xToFreq(float x) const noexcept
{
    const float norm = juce::jlimit(0.0f, 1.0f, (x - plotBounds.getX()) / juce::jmax(1.0f, plotBounds.getWidth()));
    return kMinFreq * std::pow(kMaxFreq / kMinFreq, norm);
}

float HybridResonanceEQAudioProcessorEditor::dbToY(float db) const noexcept
{
    const float clamped = juce::jlimit(kMinGainDb, kMaxGainDb, db);
    const float norm = (kMaxGainDb - clamped) / (kMaxGainDb - kMinGainDb);
    return plotBounds.getY() + norm * plotBounds.getHeight();
}

float HybridResonanceEQAudioProcessorEditor::yToDb(float y) const noexcept
{
    const float norm = juce::jlimit(0.0f, 1.0f, (y - plotBounds.getY()) / juce::jmax(1.0f, plotBounds.getHeight()));
    return kMaxGainDb - norm * (kMaxGainDb - kMinGainDb);
}

float HybridResonanceEQAudioProcessorEditor::specDbToY(float specDb) const noexcept
{
    const float clamped = juce::jlimit(-120.0f, 0.0f, specDb);
    const float norm = (0.0f - clamped) / 120.0f;
    return plotBounds.getY() + norm * plotBounds.getHeight();
}

int HybridResonanceEQAudioProcessorEditor::hitTestBandNode(juce::Point<float> pos) const noexcept
{
    for (int b = 0; b < HybridResonanceEQAudioProcessor::kNumBands; ++b)
    {
        const auto& cfg = audioProcessor.getEQEngine().getBandConfig(b);
        if (!cfg.enabled)
            continue;

        const float nx = freqToX(cfg.freqHz);
        const float ny = dbToY(cfg.gainDb + dynOffsetsDb[static_cast<size_t>(b)]);
        if (pos.getDistanceFrom({ nx, ny }) <= 11.0f)
            return b;
    }
    return -1;
}

void HybridResonanceEQAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    const auto pt = e.position;
    const int hit = hitTestBandNode(pt);

    if (hit >= 0)
    {
        selectedBandIndex = hit;
        draggingBandIndex = hit;
        syncControlsToSelectedBand();
        return;
    }

    if (plotBounds.contains(pt))
    {
        int targetBand = selectedBandIndex;
        const auto& curCfg = audioProcessor.getEQEngine().getBandConfig(targetBand);
        if (!curCfg.enabled)
        {
            auto* enParam = dynamic_cast<juce::AudioParameterBool*>(
                audioProcessor.getAPVTS().getParameter(HybridResonanceEQAudioProcessor::bandEnabledId(targetBand)));
            if (enParam != nullptr)
                *enParam = true;
        }

        if (auto* freqParam = audioProcessor.getAPVTS().getParameter(HybridResonanceEQAudioProcessor::bandFreqId(targetBand)))
            freqParam->setValueNotifyingHost(freqParam->getNormalisableRange().convertTo0to1(xToFreq(pt.x)));

        if (auto* gainParam = audioProcessor.getAPVTS().getParameter(HybridResonanceEQAudioProcessor::bandGainId(targetBand)))
            gainParam->setValueNotifyingHost(gainParam->getNormalisableRange().convertTo0to1(juce::jlimit(-24.0f, 24.0f, yToDb(pt.y))));

        draggingBandIndex = targetBand;
    }
}

void HybridResonanceEQAudioProcessorEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (draggingBandIndex < 0 || draggingBandIndex >= HybridResonanceEQAudioProcessor::kNumBands)
        return;

    const auto pt = e.position;
    if (auto* freqParam = audioProcessor.getAPVTS().getParameter(HybridResonanceEQAudioProcessor::bandFreqId(draggingBandIndex)))
        freqParam->setValueNotifyingHost(freqParam->getNormalisableRange().convertTo0to1(xToFreq(pt.x)));

    if (auto* gainParam = audioProcessor.getAPVTS().getParameter(HybridResonanceEQAudioProcessor::bandGainId(draggingBandIndex)))
        gainParam->setValueNotifyingHost(gainParam->getNormalisableRange().convertTo0to1(juce::jlimit(-24.0f, 24.0f, yToDb(pt.y))));
}

void HybridResonanceEQAudioProcessorEditor::mouseUp(const juce::MouseEvent&)
{
    draggingBandIndex = -1;
}

void HybridResonanceEQAudioProcessorEditor::mouseDoubleClick(const juce::MouseEvent& e)
{
    const int hit = hitTestBandNode(e.position);
    if (hit >= 0)
    {
        selectedBandIndex = hit;
        syncControlsToSelectedBand();
        if (auto* enParam = dynamic_cast<juce::AudioParameterBool*>(
                audioProcessor.getAPVTS().getParameter(HybridResonanceEQAudioProcessor::bandEnabledId(hit))))
        {
            *enParam = !enParam->get();
        }
    }
}

void HybridResonanceEQAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().toFloat();

    const float topBarH = 34.0f;
    const float specBarH = 68.0f;

    plotBounds = juce::Rectangle<float>(
        bounds.getX() + 8.0f,
        bounds.getY() + topBarH,
        juce::jmax(100.0f, bounds.getWidth() - 16.0f),
        juce::jmax(100.0f, bounds.getHeight() - topBarH - specBarH - 8.0f));

    const float panelW = juce::jmin(620.0f, plotBounds.getWidth() - 40.0f);
    const float panelH = 66.0f;
    floatingBarBounds = juce::Rectangle<float>(
        plotBounds.getCentreX() - panelW * 0.5f,
        plotBounds.getBottom() - panelH - 12.0f,
        panelW,
        panelH);

    bandEnableButton.setBounds(juce::roundToInt(floatingBarBounds.getX() + 10.0f), juce::roundToInt(floatingBarBounds.getY() + 18.0f), 42, 26);
    bandTypeBox.setBounds(juce::roundToInt(floatingBarBounds.getX() + 58.0f), juce::roundToInt(floatingBarBounds.getY() + 18.0f), 92, 26);
    freqSlider.setBounds(juce::roundToInt(floatingBarBounds.getX() + 158.0f), juce::roundToInt(floatingBarBounds.getY() + 6.0f), 84, 56);
    gainSlider.setBounds(juce::roundToInt(floatingBarBounds.getX() + 246.0f), juce::roundToInt(floatingBarBounds.getY() + 6.0f), 84, 56);
    qSlider.setBounds(juce::roundToInt(floatingBarBounds.getX() + 334.0f), juce::roundToInt(floatingBarBounds.getY() + 6.0f), 84, 56);
    bandRouteBox.setBounds(juce::roundToInt(floatingBarBounds.getX() + 426.0f), juce::roundToInt(floatingBarBounds.getY() + 18.0f), 80, 26);
    dynButton.setBounds(juce::roundToInt(floatingBarBounds.getX() + 512.0f), juce::roundToInt(floatingBarBounds.getY() + 18.0f), 46, 26);
    soloButton.setBounds(juce::roundToInt(floatingBarBounds.getX() + 562.0f), juce::roundToInt(floatingBarBounds.getY() + 18.0f), 48, 26);

    bottomSpecBarBounds = juce::Rectangle<float>(
        bounds.getX() + 8.0f,
        plotBounds.getBottom() + 4.0f,
        bounds.getWidth() - 16.0f,
        specBarH);

    const float sy = bottomSpecBarBounds.getY();
    specEnableButton.setBounds(juce::roundToInt(bottomSpecBarBounds.getX() + 12.0f), juce::roundToInt(sy + 20.0f), 86, 28);
    specDeltaButton.setBounds(juce::roundToInt(bottomSpecBarBounds.getX() + 104.0f), juce::roundToInt(sy + 20.0f), 72, 28);
    depthSlider.setBounds(juce::roundToInt(bottomSpecBarBounds.getX() + 195.0f), juce::roundToInt(sy + 4.0f), 86, 60);
    sharpSlider.setBounds(juce::roundToInt(bottomSpecBarBounds.getX() + 286.0f), juce::roundToInt(sy + 4.0f), 86, 60);
    smoothSlider.setBounds(juce::roundToInt(bottomSpecBarBounds.getX() + 377.0f), juce::roundToInt(sy + 4.0f), 86, 60);
    mixSlider.setBounds(juce::roundToInt(bottomSpecBarBounds.getX() + 468.0f), juce::roundToInt(sy + 4.0f), 86, 60);
}

void HybridResonanceEQAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff17181b));

    g.setColour(juce::Colour(0xff202227));
    g.fillRect(0, 0, getWidth(), 34);
    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.setFont(juce::Font(juce::FontOptions(15.0f).withStyle("Bold")));
    g.drawText("HYBRID RESONANCE EQ  |  16-BAND PRO-STYLE + 2048 FFT SPECTRAL", 14, 0, 560, 34, juce::Justification::centredLeft);

    g.setColour(juce::Colour(0xff1c1e23));
    g.fillRect(plotBounds);

    const std::array<float, 10> freqTicks { 20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f };
    g.setFont(juce::Font(juce::FontOptions(11.0f)));

    for (float f : freqTicks)
    {
        const float x = freqToX(f);
        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.drawVerticalLine(static_cast<int>(std::round(x)), plotBounds.getY(), plotBounds.getBottom());

        g.setColour(juce::Colours::white.withAlpha(0.55f));
        const juce::String label = (f >= 1000.0f) ? (juce::String(juce::roundToInt(f / 1000.0f)) + "k") : juce::String(juce::roundToInt(f));
        g.drawText(label, static_cast<int>(x) - 18, static_cast<int>(plotBounds.getBottom()) - 18, 36, 16, juce::Justification::centred);
    }

    const std::array<float, 5> dbTicks { -12.0f, -6.0f, 0.0f, 6.0f, 12.0f };
    for (float db : dbTicks)
    {
        const float y = dbToY(db);
        g.setColour(std::abs(db) < 0.01f ? juce::Colours::white.withAlpha(0.22f) : juce::Colours::white.withAlpha(0.07f));
        g.drawHorizontalLine(static_cast<int>(std::round(y)), plotBounds.getX(), plotBounds.getRight());

        g.setColour(juce::Colours::white.withAlpha(0.55f));
        const juce::String txt = (db > 0.0f ? "+" : "") + juce::String(static_cast<int>(db));
        g.drawText(txt, static_cast<int>(plotBounds.getRight()) - 38, static_cast<int>(y) - 9, 34, 18, juce::Justification::right);
    }

    juce::Path specFill;
    bool startedSpec = false;
    const double sr = audioProcessor.getSampleRate() > 0.0 ? audioProcessor.getSampleRate() : 48000.0;
    const float binStepHz = static_cast<float>(sr) / static_cast<float>(dspcore::SpectralResonanceEngine::fftSize);

    for (int b = 1; b < HybridResonanceEQAudioProcessor::kSpectrumBins; ++b)
    {
        const float fHz = static_cast<float>(b) * binStepHz;
        if (fHz < kMinFreq || fHz > kMaxFreq)
            continue;

        const float x = freqToX(fHz);
        const float y = specDbToY(spectrumDb[static_cast<size_t>(b)]);

        if (!startedSpec)
        {
            specFill.startNewSubPath(x, plotBounds.getBottom());
            specFill.lineTo(x, y);
            startedSpec = true;
        }
        else
        {
            specFill.lineTo(x, y);
        }
    }

    if (startedSpec)
    {
        specFill.lineTo(plotBounds.getRight(), plotBounds.getBottom());
        specFill.closeSubPath();
        g.setColour(juce::Colours::white.withAlpha(0.11f));
        g.fillPath(specFill);
    }

    juce::Path curvePath;
    juce::Path fillPath;
    const float zeroY = dbToY(0.0f);
    const int steps = juce::jmax(120, static_cast<int>(plotBounds.getWidth()));

    for (int i = 0; i <= steps; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(steps);
        const float x = plotBounds.getX() + t * plotBounds.getWidth();
        const float fHz = xToFreq(x);
        const float totalDb = audioProcessor.getEQEngine().getTotalMagnitudeDb(fHz);
        const float y = dbToY(totalDb);

        if (i == 0)
        {
            curvePath.startNewSubPath(x, y);
            fillPath.startNewSubPath(x, zeroY);
            fillPath.lineTo(x, y);
        }
        else
        {
            curvePath.lineTo(x, y);
            fillPath.lineTo(x, y);
        }
    }

    fillPath.lineTo(plotBounds.getRight(), zeroY);
    fillPath.closeSubPath();

    g.setColour(juce::Colour(0xff9b4dff).withAlpha(0.16f));
    g.fillPath(fillPath);

    g.setColour(juce::Colour(0xfff5d328));
    g.strokePath(curvePath, juce::PathStrokeType(2.2f));

    for (int b = 0; b < HybridResonanceEQAudioProcessor::kNumBands; ++b)
    {
        const auto& cfg = audioProcessor.getEQEngine().getBandConfig(b);
        if (!cfg.enabled)
            continue;

        const float nx = freqToX(cfg.freqHz);
        const float ny = dbToY(cfg.gainDb + dynOffsetsDb[static_cast<size_t>(b)]);
        const bool isSelected = (b == selectedBandIndex);

        g.setColour(isSelected ? juce::Colours::white : juce::Colour(0xff47d7ff));
        g.fillEllipse(nx - 7.0f, ny - 7.0f, 14.0f, 14.0f);

        g.setColour(juce::Colour(0xff17181b));
        g.drawEllipse(nx - 7.0f, ny - 7.0f, 14.0f, 14.0f, 1.5f);

        g.setFont(juce::Font(juce::FontOptions(10.0f).withStyle("Bold")));
        g.drawText(juce::String(b + 1), static_cast<int>(nx) - 7, static_cast<int>(ny) - 7, 14, 14, juce::Justification::centred);
    }

    g.setColour(juce::Colour(0xff23262d).withAlpha(0.94f));
    g.fillRoundedRectangle(floatingBarBounds, 8.0f);
    g.setColour(juce::Colours::white.withAlpha(0.18f));
    g.drawRoundedRectangle(floatingBarBounds, 8.0f, 1.0f);

    g.setColour(juce::Colour(0xff202227));
    g.fillRoundedRectangle(bottomSpecBarBounds, 6.0f);

    g.setColour(juce::Colours::white.withAlpha(0.65f));
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText("Depth", depthSlider.getX(), juce::roundToInt(bottomSpecBarBounds.getY()) + 2, depthSlider.getWidth(), 14, juce::Justification::centred);
    g.drawText("Sharp", sharpSlider.getX(), juce::roundToInt(bottomSpecBarBounds.getY()) + 2, sharpSlider.getWidth(), 14, juce::Justification::centred);
    g.drawText("Smooth", smoothSlider.getX(), juce::roundToInt(bottomSpecBarBounds.getY()) + 2, smoothSlider.getWidth(), 14, juce::Justification::centred);
    g.drawText("Mix", mixSlider.getX(), juce::roundToInt(bottomSpecBarBounds.getY()) + 2, mixSlider.getWidth(), 14, juce::Justification::centred);
}
