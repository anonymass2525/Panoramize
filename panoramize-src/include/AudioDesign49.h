#pragma once
// 4.9: audio -> *ranked editable* existing Panoramize graphs.
// Restricted resynthesis, NOT unique structure recognition or a trained ML inverse.
#include "UniversalInstrument23.h"
#include "ProductionInverse48.h"
#include <string>
#include <vector>
namespace panorama::design49 {
enum class Family { NativeVA_FM, NativeState, PhysicalString, NativePhysicalHybrid, MultidimensionalFEM };
const char* name(Family) noexcept;
struct Features {double rms=0, pitchHz=0, pitchConfidence=0, centroidHz=0, flatness=0;
 double highHarmonicRatio=0, attackSeconds=0, durationSeconds=0;};
struct Candidate {Family family=Family::NativeVA_FM;double distance=1.e10;
 double spectralDistance=0, temporalDistance=0;int midi=60;
 universal23::Graph graph;};
struct Result {Features observation;std::vector<Candidate> ranked;
 bool ambiguous=true;std::string warning,error;};
Features extract(const std::vector<float>&,int sampleRate);
std::vector<production48::Parameter> unifiedParameterRegistry(const universal23::Graph&);
Result infer(const std::vector<float>&,int sampleRate,int evaluations=6);
universal23::Graph editableGraph(Family, double fundamentalHz, double pickPosition=.24);
std::vector<float> renderGraph(const universal23::Graph&,int midi,int rate,double seconds,std::string*error=nullptr);
} // namespace panorama::design49
