#pragma once
#include "PanoramaDSP.h"
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace panorama {
// OFFLINE ONLY: explores actual PANORAMA DSP graphs, including different
// node types, edge rewiring, serial/parallel branches and continuous controls.
// This is a budget-limited heuristic search, not identification of a unique
// physical instrument, and not suitable for the real-time audio callback.
struct GraphInverseSettings {
 int maxEvaluations=180;    // includes the 54 backwards-compatible warm starts
 int maxActiveNodes=6;     // excludes the fixed Output node; 1..8
 std::uint32_t seed=0x1A8E1234u; // deterministic reproducibility
};
struct GraphInverseResult {
 Patch patch{};
 int evaluated=0;
 int mutatedEvaluated=0;
 int novelTopologiesEvaluated=0;
 int midiNote=60;
 std::uint32_t seed=0;
 double distance=std::numeric_limits<double>::infinity();
 bool success=false;
};
// Isolated single note; known MIDI note, 4096+ samples. Returns a validated
// renderable GraphPatch with a phase-insensitive spectral/envelope fit.
GraphInverseResult inferGraphFromAudio(const float* samples,int count,
                                      double sampleRate,int midiNote) noexcept;
GraphInverseResult inferGraphFromAudioEx(const float* samples,int count,
                                        double sampleRate,int midiNote,
                                        GraphInverseSettings settings) noexcept;
// An archive of acoustically plausible but structurally different instruments.
// Novelty is structural difference from previously selected alternatives, NOT
// a claim that the sound or instrument has never existed in human history.
struct GraphInverseProposal {
 Patch patch{};
 double distance=std::numeric_limits<double>::infinity();
 double structuralNovelty=0.; // normalized 0..1 compared with already selected graphs
 std::uint64_t topologySignature=0;
 int activeNodes=0;
};
struct GraphInversePortfolioResult {
 GraphInverseResult best{};
 std::vector<GraphInverseProposal> proposals;
 int distinctAcceptedStructures=0;
};
// Searches ONCE and extracts multiple distinct graph recipes from the same
// evaluated candidate archive. maxProposals 1..8, maxRelativeError >=1.
GraphInversePortfolioResult inferGraphPortfolioFromAudioEx(
 const float* samples,int count,double sampleRate,int midiNote,
 GraphInverseSettings settings,int maxProposals=4,
 double maxRelativeError=2.0) noexcept;
// Prototype 2.0: an instrument is trained on MULTIPLE performances of the
// SAME physical/virtual source. Every example is an isolated, dry mono capture
// with documented pitch, velocity and MIDI NoteOff instant. Arrays remain
// caller-owned; no lifetime is retained by the engine.
struct InstrumentExample {
 const float* samples=nullptr;
 int count=0;
 double sampleRate=48000.;
 int midiNote=60;
 float velocity=.8f;
 int noteOffFrame=2048; // 1..4096; 4096 means sustained throughout
};
struct InstrumentInverseSettings {
 int maxEvaluations=96; // 1..512; no work on the realtime audio callback
 int maxActiveNodes=6;
 std::uint32_t seed=0x20192026u;
 int maxProposals=3;
};
struct PlayableInstrumentProposal {
 Patch patch{};
 double trainingError=std::numeric_limits<double>::infinity();
 double holdoutError=std::numeric_limits<double>::infinity();
 double structuralNovelty=0.;
 std::uint64_t topologySignature=0;
 int activeNodes=0;
};
struct PlayableInstrumentResult {
 std::vector<PlayableInstrumentProposal> proposals;
 int evaluated=0,mutatedEvaluated=0,distinctTopologies=0;
 std::uint32_t seed=0;
 int trainingExamples=0,holdoutExamples=0;
 bool success=false;
};
// TRAIN examples drive optimization, HOLDOUT examples are NEVER accessed during
// candidate selection. Held-out metrics are computed only AFTER search ends.
// Reported values are multi-window spectral+level fitting scores, NOT perceptual
// similarity, proof of source identity, or guaranteed cross-note generalization.
PlayableInstrumentResult inferPlayableInstruments(
 const InstrumentExample* training,int trainingCount,
 const InstrumentExample* holdout,int holdoutCount,
 InstrumentInverseSettings settings={}) noexcept;
// Controlled performance renderer, also used by tests and preview generation.
bool renderInstrumentExample(const Patch& patch,const InstrumentExample& gesture,
                             float* output,int frames=4096) noexcept;

// JSON recipe accepted by the JUCE inverse-graph importer (format v1).
std::string graphInverseJson(const GraphInverseResult& result);
} // namespace panorama
