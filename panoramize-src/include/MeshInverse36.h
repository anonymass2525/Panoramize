#pragma once
#include "GeneralizedFEM36.h"
#include <cstddef>
#include <string>
#include <vector>
namespace panorama::inverse36 {
struct Take {
 std::vector<float> audio; // mono, onset aligned, identical sample rate as the renderer
 int midi=40;
 float velocity=.7f;
 bool training=true;
};
struct Proposal {
 fem36::Model model;
 double trainingLoss=1e20;
 double validationLoss=1e20;
 int meshCandidate=0;
};
struct Result {
 std::vector<Proposal> proposals;
 int evaluations=0;
 std::string error;
};
// Deliberately bounded inverse over a supplied library of tetra mesh hypotheses.
// This is NOT unconstrained inverse geometry/material identification.
Result infer(const std::vector<Take>& takes,const std::vector<fem36::Model>& seeds,
             int sampleRate,int maxEvaluations=80,int maxProposals=3);
std::vector<float> render(const fem36::Model&,int sampleRate,int midi,float velocity,size_t count);
}
