#pragma once
// Panoramize 2.6: native, typed, jointly-solved acoustic state graph.
// Modal representations are deliberately distinct from the old 72-preset DSP.
// All inter-modal conservative and dissipative edges are compiled into ONE
// matrix system. An active force is an externally powered, metered input.
#include "UnifiedGraph22.h"
#include <array>
#include <atomic>
#include <memory>
#include <string>
#include <vector>
namespace panorama::native26 {
using unified22::Domain;
enum class Coupling { Conservative=0, Dissipative=1 };
struct Mode {
 Domain domain=Domain::Physical;
 double harmonicRatio=1., damping=2., nonlinear=0., pickupGain=.2;
};
struct Edge {
 int a=-1,b=-1;
 Coupling kind=Coupling::Conservative;
 double strength=0., ratio=1.;
 // Each endpoint is an energy-normalized generalized coordinate q.
 // Uncalibrated physical SI sensors must NOT be connected directly.
 unified22::Quantity quantity=unified22::Quantity::NormalizedCoordinate;
 unified22::Unit unit=unified22::Unit::RootJouleSeconds;
};
struct Graph {
 std::string name="Native Acoustic Graph";
 std::vector<Mode> modes;
 std::vector<Edge> edges;
 int strikeMode=0;
};
Graph example(int index=0);
std::string serialize(const Graph&);
bool deserialize(const std::string&, Graph&, std::string* error=nullptr);
class Instrument {
public:
 static std::unique_ptr<Instrument> prepare(const Graph&, double sampleRate, std::string* error=nullptr);
 void noteOn(int midi,float velocity) noexcept;
 void noteOff(int midi) noexcept;
 // Metered active effort injected into the jointly-solved native modes.
 bool addActiveEffort(int midi,int mode,double effort) noexcept;
 double normalizedFlow(int midi,int mode) const noexcept; // energy-normalized p [sqrt(J)]
 void allNotesOff() noexcept;
 // Audio-thread safe bounded per-mode readout gain. Does not alter port energy.
 bool setLivePickupScale(int mode,double scale) noexcept;
 float processMono() noexcept;
 double energyResidual() const noexcept;
 double dissipated() const noexcept;
 double activeWork() const noexcept;
 const Graph& graph() const noexcept {return graph_;}
private:
 Instrument()=default;
 struct VoiceSlot {
  std::unique_ptr<unified22::Voice> voice;
  float envelope=0.f;
  bool held=false;
 };
 Graph graph_;
 std::array<VoiceSlot,128> voices_{};
 double sampleRate_=48000., lastMaxResidual_=0.;
 std::array<std::atomic<double>,64> pickupTarget_{};
 std::array<double,64> pickupSmooth_{};
};
} // namespace panorama::native26
