# Panoramize — Project specification

Status: working specification, 2026-11-02. This document distinguishes goals from verified behavior.

## Mission
Universal Sound Synthesis Environment: a modular instrument for connecting synthesis methods and acoustic phenomena through a Unified Acoustic State Graph. Prioritize sound quality, numerical stability, predictable real-time behavior, and accessible controls without sacrificing expert-level depth.

## Research and product goals (not all implemented)
- VA, wavetable, FM/PM, additive/subtractive, granular, spectral, nonlinear/chaotic, physical and vocal synthesis.
- Connect 1D/2D/3D physical systems with acoustic, spectral, phase and nonlinear state operators.
- Energy-aware boundary ports distinguishing passive exchange, dissipation and active excitation.
- Editable graph UI, safe parameter changes, low latency and stable coupled numerical solvers.
- Windows x64 VST3, tested in Studio One.

## Evidence policy
Do not mark a feature complete based only on a successful build or plugin scan. Track separately: implementation, unit test, VST3 validation, host load, audio correctness, listening evaluation and real-time performance.

## Current reported state
Panoramize 5.4 source ZIP is stored in this repository. User reported successful loading in Studio One. Previous Windows CI report: 40/46 CTest passed, 6 failures; these observations require reconfirmation against the latest workflow run. GUI/audio/long-term stability are not yet certified.
