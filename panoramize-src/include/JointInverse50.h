#pragma once
// 5.0: bounded joint multi-take inverse selection. Not unique shape/material
// inference and NOT inverse reconstruction of arbitrary vocal tract geometry.
// Uses actual 4.9 resynthesis graphs and a reproducible multiresolution descriptor.
#include "AudioDesign49.h"
#include "UnifiedControls50.h"
#include <string>
#include <vector>
namespace panorama::joint50 {
struct Take {std::vector<float> audio;int midi=57;bool training=true;};
struct Candidate {
 std::string family;
 universal23::Graph editable;
 double trainError=0,holdoutError=0;
 int evaluations=0;
};
struct Result {
 std::vector<Candidate> ranked;
 double uncertaintyGap=0.;bool identifiable=false;
 int totalEvaluations=0;std::string warning,error;
};
Result identify(const std::vector<Take>&takes,int sampleRate,int budget=16);
}
