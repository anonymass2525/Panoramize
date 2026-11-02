#pragma once
// Panoramize 4.8: bounded long-form recording QC, freeform FEM shape search,
// trained-on-train/held-out validation. No claim of material identifiability.
#include "DeepInverse46.h"
#include "UniversalInstrument23.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>
namespace panorama::production48 {
struct Quality {double duration=0, rms=0, peak=0, clippedFraction=0, silentFraction=0;
 double noiseFloorRms=0, estimatedDynamicRangeDb=0;bool questionable=false;};
struct LongAnalysis {Quality quality;std::vector<deep46::Section> sections;
 std::vector<deep46::Gesture> gestures;double validPitchFraction=0;std::string error;};
Quality assess(const std::vector<float>&audio,int rate);
LongAnalysis analyzeLong(const std::vector<float>&audio,int rate,int hop=512);
struct Freeform {gen37::Shape base;std::array<double,4> controls{{0,0,0,0}};};
bool buildFreeform(const Freeform&,fem36::Mesh&,std::string*error=nullptr);
struct ShapeFit {Freeform surface;fem36::Model material;
 double trainError=1e20,holdoutError=1e20;};
struct Inversion {std::vector<ShapeFit> ranked;int evaluations=0;bool uniqueIdentification=false;
 double ambiguityRatio=0;std::string error;};
Inversion fitSurfaceMaterial(const std::vector<inverse36::Take>&takes,int sr,
                            const fem36::Model&seed,int budget=16);
// Annotated continuous performance -> independent training/holdout FEM takes.
// Note onsets and pitches are user-provided; automatic note transcription is NOT solved.
struct AnnotatedNote {double onsetSeconds=0;int midi=60;float velocity=.8f;bool training=true;};
Inversion fitAnnotatedPerformance(const std::vector<float>&continuous,int sr,
          const std::vector<AnnotatedNote>&notes,const fem36::Model&seed,int budget=16);
struct Parameter {std::string id,name,unit;double min=0,max=1;bool live=false;bool requiresRebuild=true;};
std::vector<Parameter> parameterRegistry(const universal23::Graph&);
} // namespace panorama::production48
