# Repair pass 1 — Hybrid Resonance EQ

## Changes in this pass
- Replaced the analyzer's flat per-bin RMS display with a real 2048-point Hann-windowed FFT analysis of pre- and post-EQ audio.
- Corrected the analyzer/editor bin-count mismatch (1025 bins for a 2048-point real FFT). Previously, the editor treated a 1025-bin spectrum as 2049 bins, compressing the frequency axis and reading invalid bins.
- Added independent Pre and Post analyzer visibility controls.
- Published analyzer data through atomic values so the audio thread and GUI timer do not directly race over the same floating-point arrays.
- Reworked Solo to audition a band-pass monitor centered on the selected band's frequency/Q, based on the original input. Solo no longer sums the full EQ output from selected bands or accidentally chains the whole EQ path.
- Disabled spectral processing while Solo is active so the audition signal remains intelligible.

## Important remaining work
The current `processSpectralStage()` is still an approximation, not a true per-bin spectral dynamics processor: it detects spectral peaks with an FFT but converts the reductions into one scalar gain applied to the entire signal for each hop. It therefore cannot selectively attenuate only the offending frequency bins like Pro-Q 4. This stage needs a proper STFT -> per-bin gain -> inverse STFT overlap-add implementation with explicit latency reporting and a delta-monitor path.

Other areas that still need a dedicated pass:
- Realtime-safety: `processBlock()` currently allocates/copies several `AudioBuffer` objects per block. Move scratch buffers to `prepareToPlay()` and reuse them.
- Dynamic EQ: detector is currently broadband RMS rather than a per-band filtered detector; attack/release and threshold behavior need verification.
- Analyzer resolution choice (`ana_resolution`) is parameterized but the current analyzer implementation remains fixed at 2048 points.
- Band solo for Low Cut/High Cut should audition the rejected side of the filter, as Pro-Q 4 does; this pass uses a band-pass monitor for all shapes.
- Build and host validation are still required.

## Official Pro-Q 4 references
- Online help: https://www.fabfilter.com/help/pro-q/welcome/welcome
- PDF manual: https://www.fabfilter.com/downloads/pdf/help/ffproq4-manual.pdf
- Spectrum analyzer: https://www.fabfilter.com/help/pro-q/using/analyzer
- Solo: https://www.fabfilter.com/help/pro-q/using/solo
- Spectral dynamics: https://www.fabfilter.com/help/pro-q/using/spectral-dynamics

This is an independent implementation. It does not include FabFilter source code, assets, or proprietary UI resources.
