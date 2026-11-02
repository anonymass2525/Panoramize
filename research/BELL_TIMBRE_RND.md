# Panoramize: Bell timbre, weight and physical plausibility — R&D brief

Status: 2026-11-02 research direction; NO DSP modifications. Preserve existing timbre and verify active engine before identifying sonic causes.

## Listener observation
The user reports strong positive listening response in Studio One, especially the fifth preset, describing **body / weight / realism**, a **metallic quality**, and potential compatibility with a **plate reverb**. This is subjective listening evidence, not proof of which algorithm is active.

## Source-backed audit
- `src/Presets.cpp` factory #05 is **"05 / Algorithmic Bell"**, engine `FractalDigital` (enum 1), with very fast attack, decay ≈1.15, sustain 0, release ≈1.20, high fold (0.78), low FM-index parameter (0.06), cutoff 0.97.
- `src/PanoramaDSP.cpp`, `Synth::Voice::process`: FM/phase modulation + iterative nonlinear sinusoidal wavefolding produce `fmOut`. The algorithm also updates eight damped resonator modes with inharmonic ratios `1, 1.499, 2.14, 2.997, 4.01, 5.53, 7.21, 10.9`, but `Engine::FractalDigital` explicitly routes `fmOut` to the audible output, *not* the modal/physical result. The modal result is used for `PhysicalResonance` or `Hybrid` engine routing.
- `juce/PluginProcessor.h` defaults `universalEnabled{true}`, and its default `directGraph()` uses a DirectState27 node, not factory program #05. `loadPreset` changes factory APVTS program/parameters but does not on its own toggle `universalEnabled`. Therefore confirm **ENGINE: FACTORY** before associating user listening with factory #05's implementation. GUI Standard disables factory control when Unified mode is active.
- Generalization: physical/modal engines really do exist in the codebase, but this does **not** establish that the acclaimed bell listening sample was produced by physical modeling.

## Objective: preserve the good sound *first*
1. Obtain a versioned *reference render* for factory #05 with explicitly recorded active engine mode, exact source commit, sample rate, note pitch/velocity, note lengths, and effects chain.
2. Record stereo WAV **dry**, not processed through a DAW room/plate reverb or loudness maximizer. Capture a second independent **wet** reverb example.
3. Freeze original factory patch and DSP output as a golden regression; no silent retuning of preset #05.
4. Measure spectral peak trajectories, inharmonicity, high-frequency decay vs low-frequency decay, transient attack, RMS, inter-channel correlation, peak, alias components and CPU. Do not assume that perceived "weight" equals any one measurement.
5. Use matched level and blind A/B where practical; compare on speakers/headphones and multiple octaves and velocities.

## Research paths (hypotheses, not claims)
A. **Algorithmic bell 1.0 / reference**: retain current phase-modulation and fold character.
B. **Resonance-enhanced bell**: parallel optional 8+ inharmonic modal resonators with excitation and per-mode damping; dry-mix slider; compare to baseline.
C. **Material plate bell**: rectangular/circular 2D thin plate or modal model with stiffness, anisotropy, thickness, exciters, struck position and pickup position; physically consistent boundary conditions and decays.
D. **Coupled plate + acoustic body**: plate resonator to cavity/body modes and passive energy-based coupling, calibrated active excitation and stability constraints.
E. **Plate reverb effect**: a *separate post-instrument effect*; investigate high-density diffusion, frequency-dependent damping, pre-delay, modulation, stereo decorrelation, wet/dry and latency. Do not confuse synthetic plate reverberation with physical struck-plate generation.

## Engineering gates
- Keep algorithmic baseline and new modes simultaneously selectable (A/B).
- Parameter change must avoid clicks, runaway energy and invalid graphs.
- Verify note-on/off, automation, 44.1/48/96 kHz, 64/128/256 frames, offline/realtime render, mono compatibility, CPU/latency.
- Avoid sample-rate-dependent resonance pitch/damping. For long tails, watch denormals.
- All new plate models must pass conservation/dissipation tests for passive-only configurations and demonstrate intentional energy injection for active exciters.
- Only promote algorithms if they improve sound through repeatable listening/measurement without regression.

## Decision pending
Which engine mode (Factory vs Unified) was active when the user listened to #05? The current UI includes an explicit DSP engine toggle. Ask only if necessary for attributing the sound, not for continuing general research.

## Readable implementation references
- [Preset definition](https://github.com/anonymass2525/Panoramize/blob/main/Panoramize_v5.4.0_MSVC_Fix1_Source.zip) (ZIP primary source)
- [Expanded source, feature branch](https://github.com/anonymass2525/Panoramize/blob/feature/panoramic-gui/panoramize-src/src/Presets.cpp)
- [Expanded DSP source](https://github.com/anonymass2525/Panoramize/blob/feature/panoramic-gui/panoramize-src/src/PanoramaDSP.cpp)
- [Processor routing](https://github.com/anonymass2525/Panoramize/blob/feature/panoramic-gui/panoramize-src/juce/PluginProcessor.cpp)
