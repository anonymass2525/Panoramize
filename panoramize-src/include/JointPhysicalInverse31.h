#pragma once
// Panoramize 3.1: shared effective physical structure + per-performance parameter
// block-coordinate search. Research prototype, not 3D inverse geometry.
#include "PerformancePhysical30.h"
#include <string>
#include <vector>
namespace panorama::joint31 {
struct Take {
 const float* audio=nullptr; int frames=0, sampleRate=16000;
 double knownPitchHz=0.; bool training=true;
};
struct PerformanceParams {
 double pitchHz=196., onsetSeconds=.035, pickPosition=.24;
 double decayBase=2.4, gain=.45;
};
struct Candidate {
 performance30::CoupledModel structure;
 std::vector<PerformanceParams> performances; // one per input take
 std::string topology;
 double initialTrainingLoss=0., trainingLoss=0., holdoutLoss=0.;
};
struct Result {
 std::vector<Candidate> proposals;
 unsigned evaluatedModels=0, trainingTakes=0, holdoutTakes=0;
 bool success=false;
};
// Identical physical instrument parameters across recordings, with distinct
// onset, fundamental, pluck position, damping and excitation gain per take.
// Holdout recordings are NEVER used to select/update shared parameters.
Result infer(const std::vector<Take>& takes,
 const performance30::CoupledModel& prior,
 unsigned evaluations=160, unsigned alternatives=4);
performance30::CoupledModel forTake(const Candidate& candidate,size_t takeIndex);
std::string serialize(const Candidate& candidate);
bool deserialize(const std::string& text,Candidate& candidate);
} // namespace panorama::joint31
