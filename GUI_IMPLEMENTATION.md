# Panoramize Panoramic GUI — implementation and acceptance

Status: GUI implementation committed on `feature/panoramic-gui`; validation is reported by [Windows GUI CI](https://github.com/anonymass2525/Panoramize/actions/workflows/panoramic-gui-ci.yml). No claim of Studio One acceptance until a human runs it.

## Three modes (UI navigation only)
- **Standard:** vector-rendered panoramic sound landscape, schematic state graph, factory DSP parameter knobs and factory engine selector.
- **Advanced:** preserved v5.4 factory patchbay with 9 audio nodes, 2 modulation lanes, and parameter editors.
- **Graph:** preserved Universal Graph editor with deeper physical and acoustic tools.

Switching between Standard, Advanced and Graph **does not change the active audio engine**.
The explicitly labeled **ENGINE** button switches factory vs unified DSP. This is intentionally distinct from navigation.

## Functional vs decorative
Functional:
- Standard macro knobs and mid-panel filter/ADSR/body/cross settings: existing JUCE AudioProcessorValueTreeState (APVTS) attachments.
- Standard factory engine selection: existing `engine` parameter.
- Factory preset selector, existing Advanced editor and existing Graph editor.
- Factory controls are disabled when Unified DSP is active, to avoid implying they shape the unified engine.
- Engine switch is explicit.

Illustrative / **not functional DSP visualizers yet**:
- The panoramic ridgeline and node network are a design motif, **not an FFT analyzer**.
- State graph miniature is a schematic, not a live topology display.
- FX tiles preview future UX; no new DSP effect modules are created by this GUI.
- Real-time graph meters, direct manipulation of native/unified engine nodes and interactive piano are not part of this first iteration.

## Source and rollback
- Original ZIP on `main` is unchanged.
- GUI source is versioned at `panoramize-src/juce/PanoramicDashboard.h` and `panoramize-src/juce/PluginEditor.{h,cpp}`.
- Windows GUI workflow builds `panoramize-src`, not the original ZIP.
- Revert by running the existing main-branch VST3 artifact or reverting the GUI branch; do not overwrite the installed working plugin without a backup.

## Acceptance checklist
- [ ] Clean Windows x64 MSVC Release build (see CI run)
- [ ] CTest and pluginval strictness 5 (see CI run)
- [ ] Studio One loads the new VST3 and saves/reloads a session
- [ ] Standard/Advanced/Graph switches without dropping held notes unexpectedly
- [ ] Factory knob changes and automation audibly affect Factory DSP; Unified knob disable is evident
- [ ] Explicit DSP engine switch restores state correctly
- [ ] GUI resizes from minimum to maximum without clipping or controls disappearing
- [ ] Compare old and new versions at the same presets, levels and sample rates to detect DSP changes

## Further quality goals
Custom vector rotary knob theming, scalable typography, accessibility/keyboard focus, colorblind-safe distinction, live FFT visualizer with bounded snapshot exchange, and real topology preview that reads the current Unified Acoustic State Graph.
