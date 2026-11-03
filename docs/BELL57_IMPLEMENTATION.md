# Panoramize Bell 5.7 — Original attack / coupled resonance bloom

## Listener-led hypothesis
The user ranked **Original** highest and **Coupled 5.6** second in the last listening test. Our independent new 5.7 experiment combines an Original-led early transient with the 5.6 family of coupled metal modes emerging afterward.

## Engine design
- Factory preset 05 remains **Fractal Digital**, not a separate physical-FEM instrument.
- `bell57Morph` (0..1): *0* reproduces the exact existing 5.6 signal path for the same other settings; increasing it introduces a time-evolving Original/physical body blend and fades in the coupled output after the strike.
- `bell57Bloom` (0..1): sampled at note-on, changes how quickly coupled metal is introduced; smaller = faster. Uses a one-pole time constant from 18 to 188 ms.
- For Bell57-enabled new notes only, mode coupling receives a small additional note-on sampled increment. Passive energy-neutral rotation and independent modal damping remain part of the 5.6 network.
- Parameters are appended to APVTS; Factory preset 05 enables Bell57. Other engines are not changed.
- Explicit `bellDynamics=0` reverts to 5.5 even if Morph57 is nonzero. Explicit `metalMix=0` preserves Original dry.

## Test + listening pack
- Bell57 standalone DSP regression: previous 5.6 bypass, Original/5.5 bypass, attack closeness, bloom effect, preset cache, 44.1/48/96 kHz, pitch bounds, other engine isolation.
- Six dry 48 kHz stereo WAV previews: Original, Evolved 5.5, Evolved 5.6, Coupled 5.6, Bell57 default and fast-bloom Bell57. All render note C5, velocity 0.82, 1s hold and 2.5s release. Same settings as prior comparison.
- Compare at matched loudness; no claims of subjective improvement until listening.

## Limits
- This is a **stylized time-varying modal bell design**, not calibrated 2D/3D FEM and not a plate reverb.
- Morph does not mean continuously changing the actual voice solver between two separate synthesis engines. It is a continuous blend of the Fractal carrier, 5.5 body and 5.6 companion network.
- Hold off on merging with main until Studio One acceptance, preset restore and real-world automation is checked.
