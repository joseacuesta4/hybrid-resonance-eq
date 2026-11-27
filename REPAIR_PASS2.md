# Hybrid Resonance EQ — Repair Pass 2

## What changed

### Spectral Dynamics engine
- Replaced the previous frame-wide scalar attenuation with per-frequency-bin gain reduction.
- Uses linked-stereo detection: one detector gain per bin is applied to L/R to preserve stereo image.
- Uses an STFT analysis/synthesis path with weighted overlap-add and a matching delayed dry path.
- Uses the selected band's center frequency, Q, dynamic range, threshold/auto-threshold, attack and release.
- Adds selectable spectral FFT resolution: 2048 (lower latency) or 4096 (higher frequency selectivity).
- Reports the corresponding algorithmic latency to the host while spectral processing is active.
- Delta listen is derived from the time-aligned delayed dry and processed signals.

### Dynamic EQ
- Replaced the shared full-band RMS detector with an independent band-pass detector for each band.
- Detector filters track the band's frequency and Q, so unrelated frequency content no longer drives every dynamic band equally.

### Processor/runtime
- Reused preallocated scratch buffers for the dry and per-band paths instead of constructing temporary AudioBuffers for every band in every block.
- Corrected output-gain smoothing so it advances once per sample, not once per channel.
- Spectral processing is disabled while global bypass or band Solo is active, avoiding a delayed/wet signal being crossfaded against an undelayed dry signal.

### UI
- Added a `Spec FFT` selector with 2048 Low Latency and 4096 High Quality choices.

## Important engineering notes

- The spectral path is a custom STFT spectral-dynamics implementation, not a reproduction of FabFilter's proprietary algorithm.
- The selected FFT size is also the current reported latency in samples (2048 or 4096). Host behavior when changing latency during playback varies; validate in Ableton Live and consider stopping playback before changing the setting if the host does not update plug-in delay compensation immediately.
- The base parametric EQ is still IIR/minimum-phase. The new selector changes Spectral Dynamics resolution; it does not yet switch the entire EQ between Zero Latency, Natural Phase and global Linear Phase modes.
- Analyzer display remains a separate 2048-point Pre/Post analyzer. It is intentionally independent from the spectral processor; the Pre/Post displays are not sample-aligned during spectral mode's latency warm-up.
- The spectral algorithm needs listening and null-test validation at 44.1/48/88.2/96 kHz, multiple block sizes, mono/stereo routing, rapid enable/disable, and automation. No successful JUCE build is claimed in this pass because JUCE is fetched from GitHub by CMake and the environment could not verify that dependency.

## Build

The project still uses CMake FetchContent to retrieve JUCE 8.0.4. From a machine with network access:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target HybridResonanceEQ_VST3
```
