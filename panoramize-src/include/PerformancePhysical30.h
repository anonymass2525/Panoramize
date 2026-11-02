#pragma once
// Panoramize v3.0 - research-grade performance event tracking and effective
// coupled string/bridge/body system identification. NOT a general guitar
// technique classifier or reconstruction of unknown 3D geometry.
#include "GuitarVoice29.h"
#include <array>
#include <string>
#include <memory>
#include <vector>
namespace panorama::performance30 {
enum class EventKind { Pluck, PitchTransition, DampingChange, VibratoCandidate, LegatoCandidate };
struct Event { EventKind kind=EventKind::Pluck; double seconds=0., pitchHz=0., confidence=0.; };
struct Frame { double seconds=0., pitchHz=0., pitchConfidence=0., rms=0., flux=0.; };
struct Performance { std::vector<Frame> frames; std::vector<Event> events; double medianPitchHz=0.; bool valid=false; };
Performance analyze(const float* audio,int frames,int sampleRate);
const char* eventName(EventKind kind) noexcept;

// Each axis models an effective linear resonant direction, not a reconstructed
// geometric mesh. Bridge and body share a source from the 2.9 string model.
struct CoupledModel {
 guitar29::Model string;
 bool bridge=false,body=false;
 double bridgeHz=480., bodyHz=270.;
 double bridgeDecay=28.,bodyDecay=15.;
 double bridgeMix=.38,bodyMix=.33;
 // three mutually orthogonal effective mode axes, normalized coordinates.
 std::array<double,3> excitation{{.85,.35,.10}};
 std::array<double,3> pickup{{.70,.25,.35}};
};
std::vector<float> render(const CoupledModel& model,int sampleRate,double seconds,
 double onset=.035,double pitchFactor=1.);
struct Take {const float* audio=nullptr;int frames=0,sampleRate=16000;
 double knownPitchHz=0.;bool training=true;};
struct Proposal {CoupledModel model;double trainingLoss=0.,holdoutLoss=0.;
 std::string topology;};
struct Discovery {std::vector<Proposal> proposals;std::vector<Performance> performances;
 bool success=false;};
// train-only ranking; holdout not used to choose model or tune its parameters.
Discovery discover(const std::vector<Take>& recordings,const guitar29::Model& stringPrior,
 unsigned maxCandidates=70,unsigned maxProposals=4);
// Real-time MIDI source; all allocations take place in prepare(), never processOne().
class Instrument {
public:
 static std::unique_ptr<Instrument> prepare(const CoupledModel&,double sampleRate,std::string* error=nullptr);
 void noteOn(int note,float velocity) noexcept;
 void noteOff(int note) noexcept;
 void pitchBend(float semitones) noexcept;
 void sustain(bool down) noexcept;
 void allNotesOff() noexcept;
 float processOne() noexcept;
private:
 struct Filter {double a=0,b=0,c=0,d=0,scale=0;
  double tick(double in) noexcept {double y=c*a+d*b+scale*in;b=a;a=y;return y;}};
 struct Bank {std::array<Filter,3> filters;double amount=0.;};
 CoupledModel model_;
 Bank bridge_,body_;
 std::unique_ptr<guitar29::Instrument> string_;
 double tick(Bank&,double) noexcept;
};
std::string serialize(const CoupledModel& model);
bool deserialize(const std::string& text,CoupledModel& model);
} // namespace panorama::performance30
