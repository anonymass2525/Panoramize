#pragma once
// Panoramize 2.7: editable, MIDI-playable *shared* typed-state instrument.
// VA and FM are explicitly powered transducers into the joint state solver;
// they are NOT mislabeled conservative states or legacy patch adapters.
#include "NativeState26.h"
#include <array>
#include <atomic>
#include <memory>
#include <string>
#include <vector>
namespace panorama::native27 {
enum class Engine:int { VA, FM, Spectral, Phase, Resonance, Physical, Vocal, Nonlinear };
struct Mode {
 Engine engine=Engine::Resonance;
 double ratio=1., damping=6., nonlinear=0., pickup=0.2;
 double excitation=0.; // active generalized effort injected into this mode
};
using Edge = native26::Edge;
struct Graph {
 std::string name="Horizon State";
 std::vector<Mode> modes;
 std::vector<Edge> edges;
 int strikeMode=0;
};
Graph example();
std::string serialize(const Graph&);
bool deserialize(const std::string&,Graph&,std::string* error=nullptr);
class Instrument {
public:
 static std::unique_ptr<Instrument> prepare(const Graph&,double sampleRate,std::string* error=nullptr);
 void noteOn(int midi,float velocity) noexcept;
 void noteOff(int midi) noexcept;
 bool addExternalEffort(int midi,int mode,double effort) noexcept; // active, metered by native26
 double modeFlow(int midi,int mode) const noexcept; // last integrated normalized flow
 void allNotesOff() noexcept;
 void pitchBend(float semitones) noexcept;
 void modWheel(float value) noexcept;
 void sustain(bool down) noexcept;
 // Live Native27 controls: port readout and active oscillator excitation.
 // Normalized 0..1 maps to 0..2x original amplitude; .5 is exact bypass.
 bool setLiveModeNormalized(int mode,bool excitation,double normalized) noexcept;
 void clearLiveModeControls() noexcept;
 float processMono() noexcept;
 double energyResidual() const noexcept;
 double activeWork() const noexcept;
 const Graph& graph() const noexcept {return graph_;}
private:
 Instrument()=default;
 struct Note {float level=0.f;bool held=false;double phase=0.,subphase=0.;};
 Graph graph_;
 std::unique_ptr<native26::Instrument> modal_;
 std::array<std::array<Note,64>,128> notes_{};
 std::array<double,128> noteHz_{};
 std::array<std::atomic<double>,64> excitationTargets_{};
 std::array<double,64> excitationSmooth_{};
 std::array<bool,128> active_{},keyHeld_{};
 bool pedalDown_=false;
 double sampleRate_=48000.,bendRatio_=1.;float mod_=0.f;
};
} // namespace panorama::native27
