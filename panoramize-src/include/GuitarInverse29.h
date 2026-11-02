#pragma once
// Panoramize 2.9: limited monophonic plucked-string physical inverse model.
// NOT a general inverse of 3-D guitar geometry or full performance recognition.
#include <string>
#include <vector>
namespace panorama::guitar29 {
struct Model {
 double frequencyHz=196., pickPosition=.24, pickupPosition=.78;
 double decayBase=2.4, decayHigh=.015, inharmonicity=.00012;
 double spectralTilt=.02, gain=.45;
 int harmonics=16;
};
struct Take {
 const float* samples=nullptr;
 int frames=0, sampleRate=16000;
 double knownFrequencyHz=0.; // optional prior; if 0, estimate F0 from the audio
 bool training=true;
};
struct Analysis {
 double frequencyHz=0., onsetSeconds=0., attackSeconds=0.;
 double estimatedDecayBase=0., estimatedDecayHigh=0.;
 double pickPosition=0., spectralTilt=0., inharmonicity=0.;
 double rms=0., loss=0., confidence=0.;
 int analyzedHarmonics=0;
 bool success=false;
};
struct InferenceResult {
 Model model;
 std::vector<Analysis> takes;
 double trainingLoss=0., holdoutLoss=0.;
 bool success=false;
};
// A real-time-safe modal pluck source: no allocations in the render sample loop.
std::vector<float> render(const Model& model, int sampleRate, double seconds,
                          double onsetSeconds=.04, double pitchFactor=1.);
Analysis analyze(const Take& take, double pickupPosition=.78);
InferenceResult infer(const std::vector<Take>& takes,double pickupPosition=.78);
std::string serialize(const Model&);
bool deserialize(const std::string&, Model&);
} // namespace panorama::guitar29
