# Bell 5.5 — dry WAV comparison metrics (first Linux reference)

Generated with `panorama_bell55_preview` on Linux x64, 2026-11-02, 48 kHz / stereo 16-bit, MIDI C5 (72), velocity 0.82, 1 s held note, 3.5 s total. No reverb, limiter or loudness normalization. Windows and Studio One host results pending.

| Measurement | Original | Evolved 5.5 | Shimmer 5.5 |
|---|---:|---:|---:|
| Peak (linear) | 0.2086 | 0.2364 | 0.2455 |
| Total stereo RMS (linear) | 0.06714 | 0.06429 | 0.06587 |
| RMS vs original (dB) | 0.00 | -0.38 | -0.17 |
| Stereo L-R RMS difference | 0 | 0.00329 | 0.00429 |
| Early spectral centroid Hz (0-0.8s; Hann, magnitude) | 7452 | 5812 | 5279 |
| Tail RMS 2.5–3.5s (dBFS) | -52.5 | -52.9 | -52.6 |

## Interpretation
- Overall loudness is closely matched (roughly 0.4 dB RMS difference), making direct A/B sensible.
- Both altered patches produce nonzero stereo width, unlike the dry monophonic original.
- The added modal body *lowers*, rather than raises, the measured early spectral centroid; the name "Shimmer" is an artistic variant label, **not a claim of higher spectral brightness**.
- The late full-band envelope is not yet measurably longer. The new per-mode damping creates different internal decays, but factory ADSR and mixed carrier still dominate overall tail amplitude.
- A lower centroid does not establish quality, weight or metallic realism. Auditory evaluation and several notes/velocities are required.
- No proof of antialiasing or calibrated plate physics is made.

## Priority for next iteration
Perform level-matched listening in Studio One. If the user wants a longer audible bell tail, add separate resonator release articulation and verify its decay without destroying the punchy FM transient; then test energy, aliasing, mono fold-down and CPU.

Source CI artifact:
https://github.com/anonymass2525/Panoramize/actions/runs/41387963679
