# Panoramize panoramic UI: unified color theme

## Target
The same graphite / cyan / emerald / warm gold visual identity now applies to the STANDARD, ADVANCED and GRAPH tabs. All DSP behavior and host preset formats remain unchanged.

## Core palette
- Canvas background `0xFF061316`
- Row/surface background `0xFF102B30`
- Borders `0xFF2F655E`
- Sonic cyan `0xFF32D3BE` (active node connections and markers)
- Warm gold `0xFFF1C079` (output and energy-related emphasis)
- Primary text `0xFFE4FFF7`
- Secondary text `0xFFA4CDC4`

## Coverage
- ADVANCED: Factory modular patchbay rows, output node marking, modulation matrix, sound controls, common widgets (knobs, text fields, sliders, combo boxes, popup menus, toggles, scrollbars).
- GRAPH: Universal Patchbay background, graph list, title accents, controls and status.
- GRAPH inspectors: Direct State, Energy Graph, Unified Parameter, Expressive, Articulation, Contact, Coupled, Physical String and Genuine Native editors.

## Scope guard
Visual-only source changes. No edits under `src/`, `include/`, `PluginProcessor.*` or `PanoramicDashboard.h`. Graph recipes, note handling and audio DSP are intended to be identical.

## Acceptance
1. Windows x64 Release compile
2. Existing CTest and pluginval strictness 5
3. Manual Studio One check of all tabs and nested inspectors: readable labels, selection colors, scrollbars, window resizing, parameter editing and saved state
4. A/B old vs new audio render at the same patch, MIDI and host settings

Implementation branch: `feature/panoramic-gui`. Do not merge into main before checking GUI in Studio One.
