# Panoramize 5.5 — Bell refinement laboratory

## Purpose
Evolve the factory preset `05 / Algorithmic Bell` beyond a solely PM/fold carrier. The new Fractal Digital branch can now mix a stylized **8-mode inharmonic resonator** with the original dry carrier. Do not confuse this with a calibrated thin-plate FEM or plate reverb, which are separate R&D targets.

## New sound controls
- `metalMix` / Bell Metal Body: 0.0 = exact original Fractal Digital PCM path, >0 = parallel resonator added; fully live.
- `metalColor` / Metallic Color: changes inharmonic partial stretching on note-on and tilts their output energy while playing.
- `metalDecay` / Bell Modal Tail: sets physically plausible exponential modal T60 at note-on, with higher modes fading more quickly; existing sustained notes do not retune/retime when this knob moves.

The eight modal frequencies are *artistic* inharmonic ratios, not empirically measured spectral modes of any particular bell. All feedback pole radii stay below 1 to ensure damped passive response. Current nonlinear saturation remains bounded.

## Factory preset #05
- Enhanced default body `metalMix=.57`, color `.66`, decay `.75`, subtle stereo mode width `.24`.
- A/B original: set `metalMix=0`, which returns the legacy Fractal Digital carrier output unchanged (regression tested).
- No new sample libraries. This is algorithmic synthesis.
- Other factory sounds retain `metalMix=0` by default. Non-Fractal engines ignore the new controls.

## Verified results — 2026-11-02

- **Windows x64 MSVC Release**: PASS.
- **CTest**: **47/47 passed**, including new `PANORAMA_Bell55` test.
- **pluginval strictness 5**: PASS, including editor/automation checks.
- **Three 48kHz dry preview WAVs**: successfully rendered on Windows and Linux.
- **Studio One actual playback / preset restoration**: **not yet manually verified** for this build.
- Windows CI artifact: https://github.com/anonymass2525/Panoramize/actions/runs/41387560668
- DSP-only Linux run and dry previews: https://github.com/anonymass2525/Panoramize/actions/runs/41387963679
- Product bundle retains legacy name `Panoramize v5.4.0.vst3` from JUCE `PRODUCT_NAME`; the project now compiles version 5.5.0 and includes enhanced sound. Keep only one installed VST3 with the original identifier.

## Workflow and comparison materials
- `tools/render_bell55.cpp`: offline deterministic dry stereo WAV renders at 48 kHz (C5, velocity .82, 1 s note-on, 2.5 s tail).
- `bell05_original_dry.wav` vs `bell05_evolved55_dry.wav` vs `bell05_shimmer55_dry.wav` are produced by the Windows GitHub Actions workflow and bundled with the VST3.
- `tests/test_bell55.cpp`: disabled-state identity, render finite/peak, stereo differences, parameter influence and 44.1/48/96 kHz MIDI tests.
- CI: https://github.com/anonymass2525/Panoramize/actions/workflows/bell55-ci.yml

## Product promises and limitations
No claims of superiority without level-matched listening. Stronger resonances may create subjectively heavier or harsher timbres. Plate reverb, actual 2D/3D plate modal extraction, GPU acceleration, full oversampling and spectral alias suppression remain future separate work. External DAW/host measurements and Studio One live acceptance are still required.

## Safety and reversibility
- Code isolated on `feature/bell-55` based on the GUI branch. No merge to main.
- Factory preset names/order and VST3 plugin identifiers unchanged.
- Existing host sessions lacking the new APVTS parameters should receive default `metalMix=0`, retaining old recorded sound; actual Studio One restoration must still be checked.
- To compare current factory sound with old reference in the same version, set `metalMix=0`.
