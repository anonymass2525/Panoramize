cmake_minimum_required(VERSION 3.21)
project(Panoramize VERSION 5.6.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(CTest)
add_library(panorama_dsp STATIC src/PanoramaDSP.cpp src/Presets.cpp src/AdvancedAcoustics.cpp src/GraphInverse.cpp src/AcousticGraph21.cpp src/UnifiedGraph22.cpp src/UniversalInstrument23.cpp src/NativeState26.cpp src/NativeState27.cpp src/TemporalInverse28.cpp src/GuitarInverse29.cpp src/GuitarVoice29.cpp src/PerformancePhysical30.cpp src/JointPhysicalInverse31.cpp src/ExpressivePhysical32.cpp src/ArticulatedPhysical33.cpp src/ContactPhysical34.cpp src/CoupledGuitar35.cpp src/GeneralizedFEM36.cpp src/MeshInverse36.cpp src/GenerativePhysics37.cpp src/ShapeInverse37.cpp src/CoupledPhysics38.cpp src/UnifiedAcousticPhysical39.cpp src/EnergyPorts40.cpp src/BoundaryPorts41.cpp src/SparseHamiltonian42.cpp src/SparseHamiltonian43.cpp src/ReducedHamiltonian44.cpp src/NonlinearPassive45.cpp src/GenuineNative47.cpp src/ProductionInverse48.cpp src/AudioDesign49.cpp src/UnifiedControls50.cpp src/JointInverse50.cpp src/RecordedValidation51.cpp src/RealtimeSafety52.cpp src/SmoothGraphSwap52.cpp src/ExperimentalObservability53.cpp src/InverseDecision54.cpp)
target_include_directories(panorama_dsp PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
if(MSVC)
  target_compile_options(panorama_dsp PRIVATE /W4 /permissive-)
else()
  target_compile_options(panorama_dsp PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_executable(panorama_render tools/render_demos.cpp)
target_link_libraries(panorama_render PRIVATE panorama_dsp)
add_executable(panorama_test tests/test_panorama.cpp)
target_link_libraries(panorama_test PRIVATE panorama_dsp)
add_test(NAME PANORAMA_DSP COMMAND panorama_test)
add_executable(panorama_bell55_test tests/test_bell55.cpp)
target_link_libraries(panorama_bell55_test PRIVATE panorama_dsp)
add_test(NAME PANORAMA_Bell55 COMMAND panorama_bell55_test)
add_executable(panorama_bell56_test tests/test_bell56.cpp)
target_link_libraries(panorama_bell56_test PRIVATE panorama_dsp)
add_test(NAME PANORAMA_Bell56 COMMAND panorama_bell56_test)
add_executable(panorama_bell56_preview tools/render_bell56.cpp)
target_link_libraries(panorama_bell56_preview PRIVATE panorama_dsp)
add_executable(panorama_bell55_preview tools/render_bell55.cpp)
target_link_libraries(panorama_bell55_preview PRIVATE panorama_dsp)
add_executable(panorama_fastmath_test tests/test_fastmath.cpp)
add_test(NAME PANORAMA_FastMath COMMAND panorama_fastmath_test)
add_executable(panorama_benchmark_v15 tools/benchmark_v15.cpp)
target_link_libraries(panorama_benchmark_v15 PRIVATE panorama_dsp)
add_executable(panorama_render_v15 tools/render_v15_preview.cpp)
target_link_libraries(panorama_render_v15 PRIVATE panorama_dsp)
add_executable(panorama_unified_state_test tests/test_unified_state.cpp)
target_link_libraries(panorama_unified_state_test PRIVATE panorama_dsp)
add_test(NAME PANORAMA_UnifiedState COMMAND panorama_unified_state_test)
add_executable(panorama_advanced_test tests/test_advanced.cpp)
target_link_libraries(panorama_advanced_test PRIVATE panorama_dsp)
add_test(NAME PANORAMA_AdvancedState COMMAND panorama_advanced_test)
add_executable(panorama_inverse tools/inverse_mesh.cpp)
target_link_libraries(panorama_inverse PRIVATE panorama_dsp)
add_executable(panorama_passive_test tests/test_passive.cpp)
target_link_libraries(panorama_passive_test PRIVATE panorama_dsp)
add_test(NAME PANORAMA_PassiveCoupling COMMAND panorama_passive_test)
add_executable(panorama_inverse_graph tools/inverse_graph.cpp)
target_link_libraries(panorama_inverse_graph PRIVATE panorama_dsp)
add_executable(panorama_graph_inverse_test tests/test_graph_inverse.cpp)
target_link_libraries(panorama_graph_inverse_test PRIVATE panorama_dsp)
add_test(NAME PANORAMA_GraphInverse COMMAND panorama_graph_inverse_test)
add_executable(panorama_playable_inverse_test tests/test_playable_inverse.cpp)
target_link_libraries(panorama_playable_inverse_test PRIVATE panorama_dsp)
add_test(NAME PANORAMA_PlayableInstrument COMMAND panorama_playable_inverse_test)
add_executable(panorama_inverse_instrument tools/inverse_instrument.cpp)
target_link_libraries(panorama_inverse_instrument PRIVATE panorama_dsp)
add_executable(panorama_render_v16 tools/render_v16_preview.cpp)
target_link_libraries(panorama_render_v16 PRIVATE panorama_dsp)
option(PANORAMA_BUILD_VST3 "Build the JUCE VST3 if JUCE is installed" OFF)
set(PANORAMA_JUCE_PATH "" CACHE PATH "Path to local JUCE source (JUCE/CMakeLists.txt)")
if(PANORAMA_BUILD_VST3)
 if(PANORAMA_JUCE_PATH)
   add_subdirectory("${PANORAMA_JUCE_PATH}" "${CMAKE_CURRENT_BINARY_DIR}/juce")
 else()
   find_package(JUCE CONFIG REQUIRED)
 endif()
 juce_add_plugin(Panoramize_Plugin
  COMPANY_NAME "Independent Audio Lab"
  BUNDLE_ID "com.independent.panoramize.typegpt.v52"
  PLUGIN_MANUFACTURER_CODE PnGP
  PLUGIN_CODE Pz52
  FORMATS VST3
  PRODUCT_NAME "Panoramize v5.4.0"
  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE)
 juce_generate_juce_header(Panoramize_Plugin)
 target_sources(Panoramize_Plugin PRIVATE juce/PluginProcessor.cpp juce/PluginEditor.cpp juce/UniversalGraphEditor23.cpp juce/NativeGraphEditor27.cpp)
 target_include_directories(Panoramize_Plugin PRIVATE include juce)
 target_link_libraries(Panoramize_Plugin PRIVATE panorama_dsp juce::juce_audio_utils juce::juce_dsp juce::juce_audio_processors juce::juce_gui_extra)
 target_compile_definitions(Panoramize_Plugin PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
 # v2.0 is an independent VST3 identity to protect earlier saved sessions.
 option(PANORAMA_BUILD_JUCE_SMOKE "Build host-independent JUCE processor/editor integration checks" ON)
 if(PANORAMA_BUILD_JUCE_SMOKE)
  juce_add_console_app(panorama_juce_smoke PRODUCT_NAME "Panoramize JUCE Smoke")
  juce_generate_juce_header(panorama_juce_smoke)
  target_sources(panorama_juce_smoke PRIVATE
   tests/test_juce_smoke.cpp juce/PluginProcessor.cpp juce/PluginEditor.cpp juce/UniversalGraphEditor23.cpp juce/NativeGraphEditor27.cpp)
  target_include_directories(panorama_juce_smoke PRIVATE include juce)
  target_link_libraries(panorama_juce_smoke PRIVATE panorama_dsp
   juce::juce_audio_utils juce::juce_dsp juce::juce_audio_processors juce::juce_gui_extra)
  target_compile_definitions(panorama_juce_smoke PRIVATE
   JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
  add_test(NAME PANORAMA_JUCE_Integration COMMAND panorama_juce_smoke)
 endif()
endif()
# v2.1 experimental generalized state-graph kernel and tests
add_executable(panorama_acoustic21_test tests/test_acoustic21.cpp)
target_link_libraries(panorama_acoustic21_test PRIVATE panorama_dsp)
add_test(NAME PANORAMA_Acoustic21 COMMAND panorama_acoustic21_test)
add_executable(panorama_acoustic21_demo tools/render_acoustic21.cpp)
target_link_libraries(panorama_acoustic21_demo PRIVATE panorama_dsp)
add_executable(panorama_acoustic21_benchmark tools/benchmark_acoustic21.cpp)
target_link_libraries(panorama_acoustic21_benchmark PRIVATE panorama_dsp)
add_executable(panorama_state_graph_csv tools/state_graph_csv.cpp)
target_link_libraries(panorama_state_graph_csv PRIVATE panorama_dsp)
add_test(NAME PANORAMA_StateGraphCSV
 COMMAND panorama_state_graph_csv
 "${CMAKE_CURRENT_SOURCE_DIR}/examples/tetra_instrument.csv"
 "${CMAKE_CURRENT_BINARY_DIR}/test_tetra_instrument.wav" 0.2)

# v2.2 research: generic power-normalized multimodal graph.
add_executable(panorama_unified22_test tests/test_unified22.cpp)
target_link_libraries(panorama_unified22_test PRIVATE panorama_dsp)
add_test(NAME PANORAMA_Unified22 COMMAND panorama_unified22_test)
add_executable(panorama_unified22_render tools/render_unified22.cpp)
target_link_libraries(panorama_unified22_render PRIVATE panorama_dsp)
add_executable(panorama_unified22_csv tools/render_unified22_csv.cpp)
target_link_libraries(panorama_unified22_csv PRIVATE panorama_dsp)
add_test(NAME PANORAMA_Unified22CSV
 COMMAND panorama_unified22_csv "${CMAKE_CURRENT_SOURCE_DIR}/examples/unified22_choir.csv"
 "${CMAKE_CURRENT_BINARY_DIR}/test_unified22_choir.wav")
add_executable(panorama_unified22_benchmark tools/benchmark_unified22.cpp)
target_link_libraries(panorama_unified22_benchmark PRIVATE panorama_dsp)

# v2.3: real-time playable generalized graph + legacy and state-core bridge.
add_executable(panorama_universal23_test tests/test_universal23.cpp)
target_link_libraries(panorama_universal23_test PRIVATE panorama_dsp)
add_test(NAME PANORAMA_Universal23 COMMAND panorama_universal23_test)

add_executable(panorama_universal23_render tools/render_universal23.cpp)
target_link_libraries(panorama_universal23_render PRIVATE panorama_dsp)

add_executable(panorama_universal23_benchmark tools/benchmark_universal23.cpp)
target_link_libraries(panorama_universal23_benchmark PRIVATE panorama_dsp)

# Panoramize v2.4: state-port readout and active source-to-state excitation.
add_executable(panoramaize_state_ports24_test tests/test_state_ports24.cpp)
target_link_libraries(panoramaize_state_ports24_test PRIVATE panorama_dsp)
add_test(NAME Panoramize_StatePorts24 COMMAND panoramaize_state_ports24_test)

# v2.5: reciprocal causally delayed state-port exchange regression.
add_executable(panoramize_bidirectional25_test tests/test_bidirectional25.cpp)
target_link_libraries(panoramize_bidirectional25_test PRIVATE panorama_dsp)
add_test(NAME Panoramize_Bidirectional25 COMMAND panoramize_bidirectional25_test)

# v2.6 native energy-state graph and playable VST3 source.
add_executable(panoramize_native26_test tests/test_native26.cpp)
target_link_libraries(panoramize_native26_test PRIVATE panorama_dsp)
add_test(NAME Panoramize_Native26 COMMAND panoramize_native26_test)
add_executable(panoramize_native26_render tools/render_native26.cpp)
target_link_libraries(panoramize_native26_render PRIVATE panorama_dsp)

add_executable(panoramize_native26_benchmark tools/benchmark_native26.cpp)
target_link_libraries(panoramize_native26_benchmark PRIVATE panorama_dsp)

# v2.7 editable, native shared state instrument
add_executable(panoramize_native27_test tests/test_native27.cpp)
target_link_libraries(panoramize_native27_test PRIVATE panorama_dsp)
add_test(NAME Panoramize_Native27 COMMAND panoramize_native27_test)
add_executable(panoramize_render_native27 tools/render_native27.cpp)
target_link_libraries(panoramize_render_native27 PRIVATE panorama_dsp)
add_executable(panoramize_benchmark_native27 tools/benchmark_native27.cpp)
target_link_libraries(panoramize_benchmark_native27 PRIVATE panorama_dsp)

# 2.8: offline, performance-conditioned temporal inverse search of editable native27 graphs
add_executable(panoramize_temporal28_test tests/test_temporal28.cpp)
target_link_libraries(panoramize_temporal28_test PRIVATE panorama_dsp)
add_test(NAME Panoramize_Temporal28 COMMAND panoramize_temporal28_test)
add_executable(panoramize_temporal28 tools/temporal_inverse28.cpp)
target_link_libraries(panoramize_temporal28 PRIVATE panorama_dsp)

# 2.9: monophonic guitar physical inversion research model.
add_executable(panoramize_guitar29_test tests/test_guitar29.cpp)
target_link_libraries(panoramize_guitar29_test PRIVATE panorama_dsp)
add_test(NAME Panoramize_Guitar29 COMMAND panoramize_guitar29_test)
add_executable(panoramize_guitar29 tools/guitar_inverse29.cpp)
target_link_libraries(panoramize_guitar29 PRIVATE panorama_dsp)

# 3.0: performance event analysis & effective 3-axis bridge/body inverse research.
add_executable(panoramize_performance30_test tests/test_performance30.cpp)
target_link_libraries(panoramize_performance30_test PRIVATE panorama_dsp)
add_test(NAME Panoramize_Performance30 COMMAND panoramize_performance30_test)
add_executable(panoramize_performance30 tools/performance_inverse30.cpp)
target_link_libraries(panoramize_performance30 PRIVATE panorama_dsp)

# 3.1 - shared physical structure and take-specific articulation co-optimization
add_executable(panoramize_joint31_test tests/test_joint31.cpp)
target_link_libraries(panoramize_joint31_test PRIVATE panorama_dsp)
add_test(NAME Panoramize_Joint31 COMMAND panoramize_joint31_test)
add_executable(panoramize_joint31 tools/joint_inverse31.cpp)
target_link_libraries(panoramize_joint31 PRIVATE panorama_dsp)

# 3.2: expressive physical guitar with smoothed tension, pressure damping, legato.
add_executable(panoramize_expressive32_test tests/test_expressive32.cpp)
target_link_libraries(panoramize_expressive32_test PRIVATE panorama_dsp)
add_test(NAME Panoramize_Expressive32 COMMAND panoramize_expressive32_test)
add_executable(panoramize_expressive32_demo tools/expressive32_demo.cpp)
target_link_libraries(panoramize_expressive32_demo PRIVATE panorama_dsp)
add_executable(panoramize_expressive32_benchmark tools/benchmark_expressive32.cpp)
target_link_libraries(panoramize_expressive32_benchmark PRIVATE panorama_dsp)
add_executable(panoramize_upgrade32 tools/upgrade_expressive32.cpp)
target_link_libraries(panoramize_upgrade32 PRIVATE panorama_dsp)

# 3.3: physically-motivated fretting, sliding, hammering and bridge-side damping
add_executable(panoramize_articulated33_test tests/test_articulated33.cpp)
target_link_libraries(panoramize_articulated33_test PRIVATE panorama_dsp)
add_test(NAME Panoramize_Articulated33 COMMAND panoramize_articulated33_test)
add_executable(panoramize_articulated33_demo tools/articulated33_demo.cpp)
target_link_libraries(panoramize_articulated33_demo PRIVATE panorama_dsp)
add_executable(panoramize_articulated33_benchmark tools/benchmark_articulated33.cpp)
target_link_libraries(panoramize_articulated33_benchmark PRIVATE panorama_dsp)

add_executable(panoramize_contact34_test tests/test_contact34.cpp)
target_link_libraries(panoramize_contact34_test PRIVATE panorama_dsp)
if(NOT MSVC)
  target_compile_options(panoramize_contact34_test PRIVATE -UNDEBUG)
endif()
add_test(NAME Panoramize_Contact34 COMMAND panoramize_contact34_test)
add_executable(panoramize_contact34_demo tools/contact34_demo.cpp)
target_link_libraries(panoramize_contact34_demo PRIVATE panorama_dsp)

# Panoramize 3.5: six-string transverse/longitudinal/torsional mesh FEM tests.
add_executable(panoramize_coupled35_test tests/test_coupled35.cpp)
target_link_libraries(panoramize_coupled35_test PRIVATE panorama_dsp)
if(NOT MSVC)
 target_compile_options(panoramize_coupled35_test PRIVATE -UNDEBUG)
endif()
add_test(NAME Panoramize_Coupled35 COMMAND panoramize_coupled35_test)
add_executable(panoramize_coupled35_demo tools/coupled35_demo.cpp)
target_link_libraries(panoramize_coupled35_demo PRIVATE panorama_dsp)
add_executable(panoramize_coupled35_benchmark tools/coupled35_benchmark.cpp)
target_link_libraries(panoramize_coupled35_benchmark PRIVATE panorama_dsp)

# 3.6: topology-configurable anisotropic tetrahedral mesh coupled to six strings.
add_executable(panoramize_fem36_test tests/test_fem36.cpp)
target_link_libraries(panoramize_fem36_test PRIVATE panorama_dsp)
if(NOT MSVC)
 target_compile_options(panoramize_fem36_test PRIVATE -UNDEBUG)
endif()
add_test(NAME Panoramize_FEM36 COMMAND panoramize_fem36_test)

add_executable(panoramize_inverse36_test tests/test_inverse36.cpp)
target_link_libraries(panoramize_inverse36_test PRIVATE panorama_dsp)
if(NOT MSVC)
 target_compile_options(panoramize_inverse36_test PRIVATE -UNDEBUG)
endif()
add_test(NAME Panoramize_MeshInverse36 COMMAND panoramize_inverse36_test)
add_executable(panoramize_inverse36 tools/mesh_inverse36.cpp)
target_link_libraries(panoramize_inverse36 PRIVATE panorama_dsp)
add_executable(panoramize_render_fem36 tools/render_fem36.cpp)
target_link_libraries(panoramize_render_fem36 PRIVATE panorama_dsp)
add_executable(panoramize_benchmark_fem36 tools/benchmark_fem36.cpp)
target_link_libraries(panoramize_benchmark_fem36 PRIVATE panorama_dsp)

# 3.7: procedural topology/shape search and 1D/2D distributed physical families.
add_executable(panoramize_generative37_test tests/test_generative37.cpp)
target_link_libraries(panoramize_generative37_test PRIVATE panorama_dsp)
if(NOT MSVC)
 target_compile_options(panoramize_generative37_test PRIVATE -UNDEBUG)
endif()
add_test(NAME Panoramize_Generative37 COMMAND panoramize_generative37_test)
add_executable(panoramize_shape37_test tests/test_shape37.cpp)
target_link_libraries(panoramize_shape37_test PRIVATE panorama_dsp)
if(NOT MSVC)
 target_compile_options(panoramize_shape37_test PRIVATE -UNDEBUG)
endif()
add_test(NAME Panoramize_ShapeInverse37 COMMAND panoramize_shape37_test)
add_executable(panoramize_render37 tools/render_generative37.cpp)
target_link_libraries(panoramize_render37 PRIVATE panorama_dsp)

# 3.8 multidimensional spatial coupling: reciprocal mechanical boundary ports.
add_executable(panoramize_coupled38_test tests/test_coupled38.cpp)
target_link_libraries(panoramize_coupled38_test PRIVATE panorama_dsp)
if(NOT MSVC)
 target_compile_options(panoramize_coupled38_test PRIVATE -UNDEBUG)
endif()
add_test(NAME Panoramize_Coupled38 COMMAND panoramize_coupled38_test)
add_executable(panoramize_coupled38 tools/render_coupled38.cpp)
target_link_libraries(panoramize_coupled38 PRIVATE panorama_dsp)
add_test(NAME Panoramize_Coupled38CLI COMMAND panoramize_coupled38
 "${CMAKE_CURRENT_SOURCE_DIR}/examples/physical38/all_five_reciprocal.phys38"
 "${CMAKE_CURRENT_BINARY_DIR}/test_coupled38.wav" 0.2 48000)

# 3.9: native acoustic state <-> 1D/2D/3D physical transducer bridge
add_executable(panoramize_unified39_test tests/test_unified39.cpp)
target_link_libraries(panoramize_unified39_test PRIVATE panorama_dsp)
if(NOT MSVC)
 target_compile_options(panoramize_unified39_test PRIVATE -UNDEBUG)
endif()
add_test(NAME Panoramize_Unified39 COMMAND panoramize_unified39_test)
add_executable(panoramize_unified39 tools/render_unified39.cpp)
target_link_libraries(panoramize_unified39 PRIVATE panorama_dsp)
add_test(NAME Panoramize_Unified39CLI COMMAND panoramize_unified39 --self-test)

# 4.0 simultaneous implicit generalized power-port oscillator graph
add_executable(panoramize_energy40_test tests/test_energy40.cpp)
target_link_libraries(panoramize_energy40_test PRIVATE panorama_dsp)
if(NOT MSVC)
 target_compile_options(panoramize_energy40_test PRIVATE -UNDEBUG)
endif()
add_test(NAME Panoramize_Energy40 COMMAND panoramize_energy40_test)
add_executable(panoramize_energy40_demo tools/render_energy40.cpp)
target_link_libraries(panoramize_energy40_demo PRIVATE panorama_dsp)
add_executable(panoramize_energy40_benchmark tools/benchmark_energy40.cpp)
target_link_libraries(panoramize_energy40_benchmark PRIVATE panorama_dsp)

# v4.1: spatial FEM, distributed boundary ports, SI acoustic cavity and typed active native bridge
add_executable(panoramize_boundary41_test tests/test_boundary41.cpp)
target_link_libraries(panoramize_boundary41_test PRIVATE panorama_dsp)
add_test(NAME Panoramize_Boundary41 COMMAND panoramize_boundary41_test)
add_executable(panoramize_boundary41_demo tools/render_boundary41.cpp)
target_link_libraries(panoramize_boundary41_demo PRIVATE panorama_dsp)
add_executable(panoramize_boundary41_benchmark tools/benchmark_boundary41.cpp)
target_link_libraries(panoramize_boundary41_benchmark PRIVATE panorama_dsp)

# 4.2 sparse port-Hamiltonian dynamic mesh assembly and matrix-free-style CSR PCG
add_executable(panoramize_sparse42_test tests/test_sparse42.cpp)
target_link_libraries(panoramize_sparse42_test PRIVATE panorama_dsp)
add_test(NAME Panoramize_Sparse42 COMMAND panoramize_sparse42_test)
add_executable(panoramize_sparse42_demo tools/render_sparse42.cpp)
target_link_libraries(panoramize_sparse42_demo PRIVATE panorama_dsp)
add_executable(panoramize_sparse42_benchmark tools/benchmark_sparse42.cpp)
target_link_libraries(panoramize_sparse42_benchmark PRIVATE panorama_dsp)

# v4.3: optional incomplete Cholesky preconditioning and typed spectral modal boundary ports
add_executable(panoramize_sparse43_test tests/test_sparse43.cpp)
target_link_libraries(panoramize_sparse43_test PRIVATE panorama_dsp)
if(NOT MSVC)
 target_compile_options(panoramize_sparse43_test PRIVATE -UNDEBUG)
endif()
add_test(NAME Panoramize_Sparse43 COMMAND panoramize_sparse43_test)
add_executable(panoramize_sparse43_demo tools/render_sparse43.cpp)
target_link_libraries(panoramize_sparse43_demo PRIVATE panorama_dsp)
add_executable(panoramize_sparse43_benchmark tools/benchmark_sparse43.cpp)
target_link_libraries(panoramize_sparse43_benchmark PRIVATE panorama_dsp)

# 4.4 offline port-congruent reduced model and multirate energy-consistent splitting.
add_executable(panoramize_reduced44_test tests/test_reduced44.cpp)
target_link_libraries(panoramize_reduced44_test PRIVATE panorama_dsp)
if(NOT MSVC)
 target_compile_options(panoramize_reduced44_test PRIVATE -UNDEBUG)
endif()
add_test(NAME Panoramize_Reduced44 COMMAND panoramize_reduced44_test)
add_executable(panoramize_reduced44_demo tools/render_reduced44.cpp)
target_link_libraries(panoramize_reduced44_demo PRIVATE panorama_dsp)
add_executable(panoramize_reduced44_benchmark tools/benchmark_reduced44.cpp)
target_link_libraries(panoramize_reduced44_benchmark PRIVATE panorama_dsp)

# 4.5 nonlinear conservative contact, monotone slip friction, geometric tetra edges,
# and an explicitly classified passive/active Native27 modal adapter.
add_executable(panoramize_passive45_test tests/test_passive45.cpp)
target_link_libraries(panoramize_passive45_test PRIVATE panorama_dsp)
if(NOT MSVC)
 target_compile_options(panoramize_passive45_test PRIVATE -UNDEBUG)
endif()
add_test(NAME Panoramize_Passive45 COMMAND panoramize_passive45_test)
add_executable(panoramize_passive45_demo tools/render_passive45.cpp)
target_link_libraries(panoramize_passive45_demo PRIVATE panorama_dsp)
add_executable(panoramize_passive45_benchmark tools/benchmark_passive45.cpp)
target_link_libraries(panoramize_passive45_benchmark PRIVATE panorama_dsp)

# 4.6: offline long-form timbre/technique descriptors + constrained inverse design.
target_sources(panorama_dsp PRIVATE src/DeepInverse46.cpp)
add_executable(panoramize_deep46_test tests/test_deep46.cpp)
target_link_libraries(panoramize_deep46_test PRIVATE panorama_dsp)
if(NOT MSVC)
 target_compile_options(panoramize_deep46_test PRIVATE -UNDEBUG)
endif()
add_test(NAME Panoramize_DeepInverse46 COMMAND panoramize_deep46_test)
add_executable(panoramize_deep46 tools/deep_inverse46_cli.cpp)
target_link_libraries(panoramize_deep46 PRIVATE panorama_dsp)
add_executable(panoramize_deep46_smoke tests/test_deep46_smoke.cpp)
target_link_libraries(panoramize_deep46_smoke PRIVATE panorama_dsp)
if(NOT MSVC)
 target_compile_options(panoramize_deep46_smoke PRIVATE -UNDEBUG)
endif()
add_test(NAME Panoramize_DeepInverse46_Smoke COMMAND panoramize_deep46_smoke)

# 4.7 original Native27 + physical39 fidelity and unified macro/tone performance
add_executable(panorama_genuine47_test tests/test_genuine47.cpp)
target_link_libraries(panorama_genuine47_test PRIVATE panorama_dsp)
if(NOT MSVC)
 target_compile_options(panorama_genuine47_test PRIVATE -UNDEBUG)
endif()
add_test(NAME PANORAMA_GenuineNative47 COMMAND panorama_genuine47_test)
add_executable(panorama_genuine47_demo tools/render_genuine47.cpp)
target_link_libraries(panorama_genuine47_demo PRIVATE panorama_dsp)
add_executable(panorama_genuine47_benchmark tools/benchmark_genuine47.cpp)
target_link_libraries(panorama_genuine47_benchmark PRIVATE panorama_dsp)

# 4.8: long-form QC, constrained freeform FEM inverse, unified parameter registry
add_executable(panoramize_production48_test tests/test_production48.cpp)
target_link_libraries(panoramize_production48_test PRIVATE panorama_dsp)
if(NOT MSVC)
 target_compile_options(panoramize_production48_test PRIVATE -UNDEBUG)
endif()
add_test(NAME PANORAMA_Production48 COMMAND panoramize_production48_test)
add_executable(panoramize_production48 tools/production48_cli.cpp)
target_link_libraries(panoramize_production48 PRIVATE panorama_dsp)
add_executable(panoramize_automation48_demo tools/render_automation48.cpp)
target_link_libraries(panoramize_automation48_demo PRIVATE panorama_dsp)

# 4.9 forward-model candidate inference and editable universal instrument handoff
add_executable(panorama_audio_design49_test tests/test_audio_design49.cpp)
target_link_libraries(panorama_audio_design49_test PRIVATE panorama_dsp)
if(NOT MSVC)
 target_compile_options(panorama_audio_design49_test PRIVATE -UNDEBUG)
endif()
add_test(NAME PANORAMA_AudioDesign49 COMMAND panorama_audio_design49_test)
add_executable(panorama_audio_design49 tools/audio_design49_cli.cpp)
target_link_libraries(panorama_audio_design49 PRIVATE panorama_dsp)

# 5.0: shared typed internal-property catalog and annotated multi-take inverse research.
add_executable(panoramize_unified50_test tests/test_unified50.cpp)
target_link_libraries(panoramize_unified50_test PRIVATE panorama_dsp)
if(NOT MSVC)
 target_compile_options(panoramize_unified50_test PRIVATE -UNDEBUG)
endif()
add_test(NAME Panoramize_Unified50 COMMAND panoramize_unified50_test)
add_executable(panoramize_joint50 tools/joint_inverse50_cli.cpp)
target_link_libraries(panoramize_joint50 PRIVATE panorama_dsp)

# v5.1: realtime Native27 internal control; no proxy graph rebuild.
add_executable(panoramize_live51_test tests/test_live51.cpp)
target_link_libraries(panoramize_live51_test PRIVATE panorama_dsp)
if(NOT MSVC)
 target_compile_options(panoramize_live51_test PRIVATE -UNDEBUG)
endif()
add_test(NAME Panoramize_LiveNative51 COMMAND panoramize_live51_test)

add_executable(panoramize_recorded51_test tests/test_recorded51.cpp)
target_link_libraries(panoramize_recorded51_test PRIVATE panorama_dsp)
if(NOT MSVC)
 target_compile_options(panoramize_recorded51_test PRIVATE -UNDEBUG)
endif()
add_test(NAME Panoramize_Recorded51 COMMAND panoramize_recorded51_test)
add_executable(panoramize_live51_demo tools/live51_demo.cpp)
target_link_libraries(panoramize_live51_demo PRIVATE panorama_dsp)

# v5.2: validated real-time controls, note-aware seamless DSP graph transitions.
add_executable(panoramize_safety52_test tests/test_safety52.cpp)
target_link_libraries(panoramize_safety52_test PRIVATE panorama_dsp)
if(NOT MSVC)
 target_compile_options(panoramize_safety52_test PRIVATE -UNDEBUG)
endif()
add_test(NAME Panoramize_Safety52 COMMAND panoramize_safety52_test)
add_executable(panoramize_swap52_audio tools/render_safety52.cpp)
target_link_libraries(panoramize_swap52_audio PRIVATE panorama_dsp)

# v5.3: real inverse limitation diagnostics and additional-measurement planner.
add_executable(panoramize_observability53_test tests/test_observability53.cpp)
target_link_libraries(panoramize_observability53_test PRIVATE panorama_dsp)
add_test(NAME Panoramize_Observability53 COMMAND panoramize_observability53_test)
add_executable(panoramize_observability53_demo tools/observability53_demo.cpp)
target_link_libraries(panoramize_observability53_demo PRIVATE panorama_dsp)

# 5.4: reproducible candidate uncertainty and physical measurement suggestions
add_executable(panoramize_decision54_test tests/test_decision54.cpp)
target_link_libraries(panoramize_decision54_test PRIVATE panorama_dsp)
add_test(NAME Panoramize_Decision54 COMMAND panoramize_decision54_test)
add_executable(panoramize_measurements54 tools/measurements54.cpp)
target_link_libraries(panoramize_measurements54 PRIVATE panorama_dsp)
