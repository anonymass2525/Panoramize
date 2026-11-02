#pragma once
// PANORAMA 2.3: real-time playable bridge between all 72 legacy factory sounds
// and the 2.2 unified acoustic-state solver. Compiled off audio thread.
// A variable-length audio graph with typed nodes, validation and feedback delay.
// This is a migration BRIDGE, not yet a wholesale replacement of legacy DSP.
#include "PanoramaDSP.h"
#include "UnifiedGraph22.h"
#include "NativeState27.h"
#include "GuitarInverse29.h"
#include "PerformancePhysical30.h"
#include "ExpressivePhysical32.h"
#include "ArticulatedPhysical33.h"
#include "ContactPhysical34.h"
#include "CoupledGuitar35.h"
#include "UnifiedAcousticPhysical39.h"
#include "EnergyPorts40.h"
#include "BoundaryPorts41.h"
#include "SparseHamiltonian42.h"
#include "SparseHamiltonian43.h"
#include "ReducedHamiltonian44.h"
#include "NonlinearPassive45.h"
#include "GenuineNative47.h"
#include <memory>
#include <atomic>
#include <string>
#include <vector>
namespace panorama::universal23 {
enum class Kind:int { Legacy=0, UnifiedState=1, Mix=2, Gain=3, Fold=4, Delay=5, Output=6, StateDrive=7, StateReturn=8, NativeState=9, DirectState27=10, GuitarString29=11, PhysicalCoupled30=12, PhysicalExpressive32=13, PhysicalArticulated33=14, PhysicalContact34=15, PhysicalCoupled35=16, UnifiedPhysical39=17, EnergyGraph40=18, BoundaryGraph41=19, SparseHamiltonian42=20, SparseHamiltonian43=21, ReducedHamiltonian44=22, NonlinearPassive45=23, GenuineNative47=24 };
// StateDrive ports: A is a Legacy source; B is a UnifiedState target.
// Node.preset selects: 0 phase, 1 spectral energy, 2 physical displacement, 3 velocity.
// StateReturn: A is UnifiedState and B is Legacy; the port selects the typed
// LEGACY state to change. Delivers solver state after its current solve, with
// an explicit one-sample causal delay before the legacy synth processes again.
// This is an ACTIVE, metered state-to-force transducer, not an energy-conserving
// connection to a legacy oscillator; unsupported ports result in zero force.
struct Node {
 Kind kind=Kind::Legacy;
 int a=-1,b=-1;
 int preset=0;         // legacy 0..71; native state program 0..5
 float level=0.65f;  // normalized gain for legacy/processor; -1..1 for mixer
};
struct GlobalModRoute {genuine47::Source source=genuine47::Source::Wheel;int node=0;float depth=.2f;};
struct Graph {
 std::string name="Horizon Matter";
 std::vector<Node> nodes;
 std::array<float,4> globalMacros{{.5f,.5f,.5f,.5f}};
 // v5.1: optional nonstructural Native27 multipliers stored in universal .pgraph.
 // 0.5 means exactly 1x original native oscillator or mode readout.
 std::array<float,8> nativePickup51{{.5f,.5f,.5f,.5f,.5f,.5f,.5f,.5f}};
 std::array<float,8> nativeExcite51{{.5f,.5f,.5f,.5f,.5f,.5f,.5f,.5f}};
 std::vector<GlobalModRoute> globalRoutes; // PCM-level modulation for every legacy / native / physical source
 float globalLfoHz=4.f;
 native27::Graph instrument= native27::example();
 guitar29::Model guitar;
 performance30::CoupledModel physical30;
 contact34::Model contact34;
 coupled35::Model coupled35;
 unified39::Recipe unified39=unified39::example();
 energy40::Recipe energy40=energy40::example();
 boundary41::Recipe boundary41=boundary41::example();
 sparse42::Recipe sparse42=sparse42::example();
 sparse43::Recipe sparse43=sparse43::example();
 reduced44::Recipe reduced44=reduced44::example();
 passive45::Recipe passive45=passive45::example();
 genuine47::Recipe genuine47=genuine47::example();
 articulated33::Model articulated33; // physical contacts and fret articulation
 expressive32::Model expressive32; // 3.0 effective string-bridge-body model
 // physical string model, directly loadable from guitar inverse 2.9 // one editable joint state system shared by DirectState27 source nodes
};
Graph defaultGraph();
Graph directGraph();
Graph expressiveGraph();
Graph articulatedGraph();
Graph contactGraph();
Graph coupledGraph();
Graph unifiedPhysicalGraph();
Graph energyGraph();
Graph boundaryGraph();
Graph sparseHamiltonianGraph();
Graph sparseEnhancedGraph();
Graph reducedMultirateGraph();
Graph nonlinearPassiveGraph();
Graph genuineNativeGraph();
std::string serialize(const Graph&);
bool deserialize(const std::string&,Graph&,std::string* error=nullptr);
class Instrument {
public:
 static std::shared_ptr<Instrument> prepare(const Graph&,double sampleRate,std::string* error=nullptr);
 void noteOn(int note,float velocity,int channel=1) noexcept;
 void noteOff(int note,int channel=1) noexcept;
 void pitchBend(float semitones) noexcept;
 void channelPressure(float normalized) noexcept;
 void polyPressure(int midi,float normalized) noexcept;
 void sustain(bool down) noexcept;
 void modWheel(float amount) noexcept;
 bool setHostParameterNormalized(int index,double value) noexcept;
 void clearHostAutomation() noexcept; // all live host overrides
 void clearNativeHostAutomation() noexcept;
 void clearNativeModeAutomation() noexcept;
 void clearGlobalHostAutomation() noexcept;
 void articulationControl(int controller,float normalized) noexcept;
 void allNotesOff() noexcept;
 Sample processOne() noexcept;
 const Graph& graph() const noexcept {return graph_;}
 int nodeCount() const noexcept {return int(graph_.nodes.size());}
 bool tryUpdateEnergyGraph(const Graph& next) noexcept; // atomic numeric retune, no voice reset
 double activeStateWork() const noexcept;
 double returnResponseMagnitude() const noexcept; // accumulated applied solver->legacy input // physically accounted work inside unified22 targets
private:
 struct Runtime;
 Instrument()=default;
 Graph graph_;
 double sampleRate_=48000.;
 std::vector<int> order_;
 std::vector<std::unique_ptr<Runtime>> runtime_;
 int output_=-1;
 bool sustainDown_=false;
 float globalWheel_=0,globalPressure_=0,globalVelocity_=0,globalKey_=0;
 std::array<float,4> globalMacros_{{.5f,.5f,.5f,.5f}};
 std::array<std::atomic<float>,4> hostMacros_{}; // -1 is MIDI/preset fallback
 std::array<std::atomic<float>,64> hostNodeLevels_{}; // -1 uses saved graph level
 double globalPhase_=0.;
 double returnResponseMagnitude_=0.;
};
} // namespace panorama::universal23
