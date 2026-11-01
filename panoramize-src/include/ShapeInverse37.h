#pragma once
#include "GenerativePhysics37.h"
#include "MeshInverse36.h"
#include <string>
#include <vector>
namespace panorama::gen37 {
struct Candidate {
 Shape shape;
 fem36::Model model;
 double trainingLoss=1e20,validationLoss=1e20;
 int candidateIndex=-1;
};
struct SearchResult {
 std::vector<Candidate> proposals;
 int evaluations=0;
 int generatedTopologies=0;
 std::string error;
};
// Automatically proposes parameterized mesh topologies. No explicit mesh library is
// required. Then alternating local geometric/material coordinate search operates
// ONLY on training takes. Validation is computed after the candidate ranking.
SearchResult discover(const std::vector<inverse36::Take>&takes,int sampleRate,
                      int evaluations=60,int maxProposals=3);
}
