# PANORAMA typeGPT v0.4.3 — Windows Hotfix 1 (C2660)

## Cause
GitHub Actions Windows build reported `error C2660` at `juce/PluginProcessor.cpp` starting at line 22.
The project had an anonymous-namespace helper named `addParameter` while
`PanoramaProcessor` inherits `juce::AudioProcessor::addParameter`. In
`PanoramaProcessor::createLayout()` the inherited member hides the helper
under unqualified name lookup.

## Change
Renamed the local helper and all its 21 invocations from `addParameter`
to `addFloatLayoutParameter`. No synthesis DSP, preset data, parameter IDs,
plugin identity, or audio processing algorithm was changed.

## Apply on GitHub without uploading everything
1. Open the existing GitHub repository, `juce/PluginProcessor.cpp` on `main`.
2. Use the edit (pencil) action and replace EVERY occurrence of
   `addParameter(` with `addFloatLayoutParameter(` in this file (22 occurrences
   including the helper definition). Preserve everything else.
3. Commit directly to `main`; the configured `push` action rebuilds.
4. Watch Actions > Windows VST3. If a different compiler error appears,
   share the *first* new `error` line and surrounding output.

## Alternative
Replace ONLY the existing `juce/PluginProcessor.cpp` on GitHub with the
file from this ZIP. Keep `.github/workflows/windows-vst3.yml` at the exact
root path; do not re-upload the directory tree or alter `.github` paths.

## Verification scope
- Static count of renamed helper + calls: 22/22.
- Original `src/PanoramaDSP.cpp` and `src/Presets.cpp` bytes unchanged.
- Linux C++ DSP CTest can be run separately.
- Windows VST3 / pluginval / Studio One NOT yet verified for this hotfix.

Do not mark VST3 verified until Windows build + tests + pluginval complete.
