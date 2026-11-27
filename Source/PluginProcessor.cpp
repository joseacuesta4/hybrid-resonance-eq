void HybridResonanceEQProcessor::processSpectralStage(juce::AudioBuffer<float>& buffer,
                                                     bool anySpecEnabled,
                                                     float depth,
                                                     float sharpness,
                                                     float smoothing,
                                                     float mix,
                                                     bool deltaMode)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    if (numSamples <= 0 || numChannels < 2)
        return;

    if (!anySpecEnabled || depth <= 0.0001f || mix <= 0.0001f)
    {
        for (int b = 0; b < numBins; ++b)
        {
            const size_t idx = static_cast<size_t>(b);
            binEnvelope[idx] *= 0.85f;
            binReductionDb[idx] = 0.0f;
        }
        return;
    }

    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer, true);

    auto* leftPtr = buffer.getWritePointer(0);
    auto* rightPtr = buffer.getWritePointer(1);

    const float sr = static_cast<float>(juce::jmax(8000.0, currentSampleRate));
    const float binHz = sr / static_cast<float>(fftSize);
    const float maxAttenDb = -3.0f - depth * 18.0f;

    std::array<bool, numBins> activeBinMask {};
    activeBinMask.fill(false);

    for (int i = 0; i < dspcore::ParametricEQEngine::maxBands; ++i)
    {
        const auto prefix = bandParamId(i, "");
        const bool enabled = apvts.getRawParameterValue(prefix + "enabled")->load() > 0.5f;
        const bool specOn = apvts.getRawParameterValue(prefix + "spec_enabled")->load() > 0.5f;
        if (!enabled || !specOn)
            continue;

        const float centerFreq = juce::jlimit(25.0f, sr * 0.48f, apvts.getRawParameterValue(prefix + "freq")->load());
        const float qVal = juce::jlimit(0.2f, 12.0f, apvts.getRawParameterValue(prefix + "q")->load());
        const float bandwidthOct = 1.0f / juce::jmax(0.25f, qVal);
        const float fLow = centerFreq * std::pow(2.0f, -bandwidthOct * 0.75f);
        const float fHigh = centerFreq * std::pow(2.0f, bandwidthOct * 0.75f);

        for (int b = 1; b < numBins; ++b)
        {
            const float f = static_cast<float>(b) * binHz;
            if (f >= fLow && f <= fHigh)
                activeBinMask[static_cast<size_t>(b)] = true;
        }
    }

    const int localRadius = juce::jlimit(2, 16, static_cast<int>(std::round(14.0f - sharpness * 10.0f)));
    const float envAlpha = juce::jlimit(0.08f, 0.90f, 0.72f - smoothing * 0.52f);

    float frameAttenLinear = 1.0f;

    for (int n = 0; n < numSamples; ++n)
    {
        fifoL[static_cast<size_t>(fifoIndex)] = leftPtr[n];
        fifoR[static_cast<size_t>(fifoIndex)] = rightPtr[n];
        ++fifoIndex;

        if (fifoIndex >= hopSize)
        {
            fifoIndex = 0;
            std::memmove(fifoL.data(), fifoL.data() + hopSize, static_cast<size_t>(fftSize - hopSize) * sizeof(float));
            std::memmove(fifoR.data(), fifoR.data() + hopSize, static_cast<size_t>(fftSize - hopSize) * sizeof(float));

            std::fill(fftWorkspace.begin(), fftWorkspace.end(), 0.0f);
            for (int k = 0; k < fftSize; ++k)
            {
                const float mono = 0.5f * (fifoL[static_cast<size_t>(k)] + fifoR[static_cast<size_t>(k)]);
                fftWorkspace[static_cast<size_t>(k)] = mono;
            }
            hannWindow.multiplyWithWindowingTable(fftWorkspace.data(), static_cast<size_t>(fftSize));
            forwardFFT.performFrequencyOnlyForwardTransform(fftWorkspace.data());

            double sumActiveGain = 0.0;
            int activeCount = 0;

            for (int b = 1; b < numBins; ++b)
            {
                const size_t idx = static_cast<size_t>(b);
                const float mag = fftWorkspace[idx] / static_cast<float>(fftSize * 0.5f);
                const float db = gainToDb(mag);

                float localSum = 0.0f;
                int count = 0;
                const int bStart = juce::jmax(1, b - localRadius);
                const int bEnd = juce::jmin(numBins - 1, b + localRadius);
                for (int m = bStart; m <= bEnd; ++m)
                {
                    const float mMag = fftWorkspace[static_cast<size_t>(m)] / static_cast<float>(fftSize * 0.5f);
                    localSum += gainToDb(mMag);
                    ++count;
                }
                const float localAvgDb = (count > 0) ? (localSum / static_cast<float>(count)) : db;
                const float thresholdProminence = 2.2f - sharpness * 1.4f;
                const float prominenceDb = db - localAvgDb - thresholdProminence;

                float targetRedDb = 0.0f;
                if (activeBinMask[idx] && prominenceDb > 0.0f)
                {
                    const float normPeak = juce::jlimit(0.0f, 1.0f, prominenceDb / 10.0f);
                    const float shaped = std::pow(normPeak, 0.65f + sharpness * 0.75f);
                    targetRedDb = juce::jlimit(maxAttenDb, 0.0f, -shaped * std::abs(maxAttenDb));
                }

                binEnvelope[idx] += (targetRedDb - binEnvelope[idx]) * envAlpha;
                binReductionDb[idx] = binEnvelope[idx];

                if (activeBinMask[idx])
                {
                    sumActiveGain += static_cast<double>(dbToGain(binReductionDb[idx]));
                    ++activeCount;
                }
            }

            const float avgGain = (activeCount > 0)
                ? juce::jlimit(0.12f, 1.0f, static_cast<float>(sumActiveGain / static_cast<double>(activeCount)))
                : 1.0f;
            frameAttenLinear = 1.0f - (1.0f - avgGain) * depth * mix;
        }

        leftPtr[n] *= frameAttenLinear;
        rightPtr[n] *= frameAttenLinear;
    }

    if (deltaMode)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* out = buffer.getWritePointer(ch);
            const auto* dry = dryBuffer.getReadPointer(ch);
            for (int n = 0; n < numSamples; ++n)
                out[n] = (dry[n] - out[n]) * 1.5f;
        }
    }
}

void HybridResonanceEQProcessor::updateSpectrumVisuals(const juce::AudioBuffer<float>& preBuffer,
                                                      const juce::AudioBuffer<float>& postBuffer)
{
    const int numSamples = juce::jmin(preBuffer.getNumSamples(), postBuffer.getNumSamples());
    const int numChannels = juce::jmin(preBuffer.getNumChannels(), postBuffer.getNumChannels());
    if (numSamples <= 0 || numChannels <= 0)
        return;

    const int speedIdx = static_cast<int>(apvts.getRawParameterValue("ana_speed")->load());
    const float mainAlpha = (speedIdx == 0) ? 0.03f : ((speedIdx == 1) ? 0.075f : 0.14f);
    const float redAlpha = 0.50f;

    float preSq = 0.0f;
    float postSq = 0.0f;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        const auto* preP = preBuffer.getReadPointer(ch);
        const auto* postP = postBuffer.getReadPointer(ch);
        for (int n = 0; n < numSamples; ++n)
        {
            preSq += preP[n] * preP[n];
            postSq += postP[n] * postP[n];
        }
    }

    const float denom = static_cast<float>(juce::jmax(1, numChannels * numSamples));
    const float preRmsDb = juce::jlimit(-100.0f, 6.0f, gainToDb(std::sqrt(preSq / denom)));
    const float postRmsDb = juce::jlimit(-100.0f, 6.0f, gainToDb(std::sqrt(postSq / denom)));

    for (int b = 0; b < numBins; ++b)
    {
        const size_t idx = static_cast<size_t>(b);
        const float targetPre = juce::jlimit(-100.0f, 6.0f, preRmsDb);
        const float targetPost = juce::jlimit(-100.0f, 6.0f, postRmsDb);

        preSpectrumSmooth[idx] += (targetPre - preSpectrumSmooth[idx]) * mainAlpha;
        postSpectrumSmooth[idx] += (targetPost - postSpectrumSmooth[idx]) * mainAlpha;
        reductionSmooth[idx] += (binReductionDb[idx] - reductionSmooth[idx]) * redAlpha;
    }
}
