# Panoramize — Verification register

## Historical baseline
- Windows x64 VST3 compile: previously succeeded.
- Previous CI CTest: 40 passed / 46 total, 6 failed (recheck against current HEAD).
- Previously reported failing tests: PANORAMA_UnifiedState, PANORAMA_GraphInverse, PANORAMA_PlayableInstrument, PANORAMA_JUCE_Integration, Panoramize_StatePorts24, Panoramize_Bidirectional25.
- Studio One: user confirmed plugin loads. This does NOT establish correct sound output or stability.
- pluginval: not certified from this evidence.

## Acceptance matrix for future releases
1. Clean Windows Release and Debug builds.
2. CTest 100%, with assertions and setup side effects handled correctly.
3. pluginval at documented strictness, including GUI and parameter automation where available.
4. Deterministic offline render comparisons and nonfinite/denormal checks.
5. Sample rates 44.1/48/96 kHz; varying buffer sizes and host transport/automation.
6. CPU, latency and peak callback measurements; long-running DAW session and preset save/reload.
7. Blind or level-matched listening comparisons for subjective quality claims.

Each test entry should include date, commit SHA, environment, command, result, artifact/log URL and known limitations.
