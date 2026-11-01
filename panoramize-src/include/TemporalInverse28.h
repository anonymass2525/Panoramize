#pragma once
// Panoramize 2.8: OFFLINE temporal, performance-conditioned inverse search over
// the *editable native27 state graph*. No work in the DAW audio callback.
#include "NativeState27.h"
#include <cstdint>
#include <string>
#include <vector>
namespace panorama::temporal28 {
enum class EventType : int { NoteOn=0, NoteOff=1, PitchBend=2, ModWheel=3 };
struct Event { int frame=0; EventType type=EventType::NoteOn; int note=60; float value=.75f; };
struct Example {
 const float* audio=nullptr;
 int frames=0, sampleRate=12000;
 const Event* events=nullptr;
 int eventCount=0;
};
struct Settings {
 int evaluations=24;   // 1..128, each evaluation re-renders every training example
 int proposals=3;      // 1..5 distinct graph designs
 std::uint32_t seed=0x2850ACEDu;
};
struct Proposal {
 native27::Graph graph;
 double trainingLoss=0., unseenLoss=0., novelty=0.;
 std::uint64_t topology=0;
};
struct Result {
 std::vector<Proposal> proposals;
 int evaluated=0,distinctTopologies=0,trainCount=0,holdoutCount=0;
 bool success=false;
};
// Event timeline MUST be supplied: this version scores legato/vibrato under
// known note and pitch-bend trajectories; it does NOT transcribe these gestures.
bool validate(const Example&,std::string* error=nullptr) noexcept;
bool render(const native27::Graph&,const Example&,std::vector<float>& output,
            std::string* error=nullptr);
Result infer(const Example* training,int trainingCount,
             const Example* holdout,int holdoutCount,
             Settings settings={});
// Reusable time-dependent descriptor; all of onset, release and sustained
// time windows influence search, with explicit level and timbral subfeatures.
std::vector<float> describe(const float* samples,int frames,int sampleRate);
double distance(const std::vector<float>& reference,const std::vector<float>& candidate);
} // panorama::temporal28
