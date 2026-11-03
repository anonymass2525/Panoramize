# Panoramize 5.6 — coupled Bell resonance, spectral evolution and clean decay

## Design intent
Advance the user's preferred `05 / Algorithmic Bell` **Evolved 5.5** timbre without losing its direct digital tone or the 5.5 modal body. This version adds a second, strictly bounded resonator family and concentrates on four musical goals:

1. Natural metallic density: slightly split, inharmonic companion partials rather than a louder reverb-like wash.
2. Attack-to-tail spectral movement: decaying strike excitation, mode-specific T60 and independently attenuated direct/body output.
3. Real mode interaction: pairwise orthogonal state rotation exchanges normalized resonator state between adjacent modes, without adding power on its own.
4. Smooth and clear tails: high-partial damping and a softer tail envelope applied only to the new companion body.

## User parameters
- `Bell Evolution` / `bellDynamics`: 0 restores the Evolved 5.5 signal path exactly. Higher values add time-evolving resonances; changing this parameter uses ~12ms smoothing.
- `Mode Coupling` / `bellCoupling`: controls adjacent-mode energy exchange, sampled at note-on.
- `Tail Clarity` / `bellClarity`: controls faster high-mode decay, sampled at note-on.

New settings are exposed on the **ADVANCED** global parameter page; existing STANDARD controls retain Bell Metal Body / Color / Tail. New parameters default to 0 for legacy saved DAW states.

Default factory 05 5.6 audition settings: evolution 0.58, coupling 0.34, clarity 0.62. This is an *experimental listening choice*, not proof of higher quality. Bypass evolution to recover Evolved 5.5; set `metalMix=0` for the original Fractal Digital Bell.

## Engineering constraints and limitations
- No heap allocations, locks or sin/cos evaluations in the **per-sample** companion modal update; cached rotation and pole coefficients computed at note-on.
- No direct positive feedback from old 5.5 modes to the new companion bank; coupling is an orthogonal transformation of normalized `x,v` variables. Norm preservation holds for **the coupling transform alone**, not the full driven/damped sound generation path.
- Companion resonances above 0.4 of sample rate are omitted rather than clamped to a pile-up frequency.
- Independent high-mode T60 and longer release attenuation act on the companion body. **No claimed physical reconstruction of a particular bell**, and no synthetic plate reverb.
- Pitch and material coefficient tuning are evaluated on note-on; audio-rate automation of Mode Coupling or Tail Clarity changes only subsequently struck notes.
- VST3 plugin ID and factory slot numbering remain unchanged.
- Existing full-engine quality checks, oversampling, click-free automation of all other legacy controls, host-state restoration, cross-platform GPU scheduling and actual FEM metal-plate synthesis remain future tasks.

## Validation
- `tests/test_bell56.cpp`: exact 5.5 bypass, test of mode coupling, different clarity, output stability at 44.1/48/96kHz and high MIDI notes, orthogonal rotation energy invariant, other engines unchanged.
- `tools/render_bell56.cpp`: 4 dry stereo WAV references at C5, 48kHz, one-second held note plus 2.5-second tail:
  * original Fractal Digital
  * Evolved 5.5
  * Evolved 5.6
  * Coupled 5.6 experimental
- Linux targeted CI and Windows Release + full CTest + pluginval strictness 5, manual Studio One listening still required.

Do not merge this experimental branch until CI and level-matched audio audition succeed.
