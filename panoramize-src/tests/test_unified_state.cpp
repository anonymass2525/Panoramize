#include "PanoramaDSP.h"
#include <array>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
using namespace panorama;
static double renderEnergy(Patch p,double sr=48000,int frames=6000){
 Synth s;s.prepare(sr);s.setPatch(p);assert(s.graphValid());s.noteOn(55,.75f);
 double energy=0;
 for(int i=0;i<frames;i++){
  auto x=s.processOne();assert(std::isfinite(x.left)&&std::isfinite(x.right));
  assert(std::abs(x.left)<1.1&&std::abs(x.right)<1.1);
  energy+=x.left*x.left+x.right*x.right;
 }
 return energy/frames;
}
int main(){
 std::array<int,kGraphNodes> order{};
 // All physical inputs must come from mechanical-state nodes, not generic PCM.
 for(int i=60;i<64;i++){
  auto p=factoryPresets()[(size_t)i];
  assert(p.engine==Engine::Modular && compileGraph(p.graph,order));
  assert(renderEnergy(p)>1.e-9);
  for(int rate:{44100,48000,96000})
   assert(renderEnergy(p,rate,3072)>1.e-10);
 }
 auto p=factoryPresets()[60];
 assert(p.graph.nodes[3].type==NodeType::UnifiedCoupler);
 assert(p.graph.nodes[2].type==NodeType::PhysicalBody);
 // Typed spectral and mechanics states must remain usable with PCM muted.
 p.graph.nodes[0].level=0;
 p.graph.nodes[2].level=0;
 p.graph.nodes[1].level=0;
 p.graph.nodes[8]={NodeType::Output,3,-1,.90f,0.f};
 assert(compileGraph(p.graph,order));
 const double sound=renderEnergy(p);
 assert(sound>1.e-9);
 auto alternative=p;
 alternative.graph.nodes[2].color=.97f;
 Synth a,b;a.prepare(48000);b.prepare(48000);
 a.setPatch(p);b.setPatch(alternative);a.noteOn(55,.75f);b.noteOn(55,.75f);
 double difference=0.;
 for(int i=0;i<4096;i++){
  const auto x=a.processOne(),y=b.processOne();
  difference+=double(x.left-y.left)*(x.left-y.left);
 }
 assert(difference/4096>1.e-10); // physical x,v,F affect the sound
 // Malformed indices must fail safely before any typed-bus read.
 alternative=p; alternative.graph.nodes[3].a=kGraphNodes;
 assert(!compileGraph(alternative.graph,order));
 alternative=p; alternative.graph.nodes[3].b=kGraphNodes;
 assert(!compileGraph(alternative.graph,order));
 // Replacing physical B with untyped oscillator must be rejected.
 alternative=p;
 alternative.graph.nodes[2].type=NodeType::Harmonic;
 assert(!compileGraph(alternative.graph,order));
 // Replacing spectral A with ordinary audio must also be rejected.
 alternative=p;alternative.graph.nodes[0].type=NodeType::VA;
 assert(!compileGraph(alternative.graph,order));
 // Pre-existing complex resonator states are published as equivalent
 // physical coordinates, so the bridge is not restricted to new nodes.
 alternative=p;
 alternative.graph.nodes[2]={NodeType::ResonantExchange,1,1,0.f,.5f};
 assert(compileGraph(alternative.graph,order));
 assert(renderEnergy(alternative)>1.e-9);
 // A state-to-state chain is legal, but an instantaneous DAG cycle is not.
 alternative=factoryPresets()[63];
 assert(compileGraph(alternative.graph,order));
 alternative.graph.nodes[0].a=4;
 assert(!compileGraph(alternative.graph,order));
 // Muted output of physical node still has active physics, but no audio.
 auto physicalOnly=factoryPresets()[60];
 physicalOnly.graph.nodes[8]={NodeType::Output,2,-1,.9f,0};
 physicalOnly.graph.nodes[2].level=0;
 assert(renderEnergy(physicalOnly)<1.e-12);
 physicalOnly.graph.nodes[2].level=1;
 assert(renderEnergy(physicalOnly)>1.e-10);
 // Long multi-voice stress for bounded output and numerical stability.
 Synth stress;stress.prepare(48000);stress.setPatch(factoryPresets()[63]);
 for(int note=48;note<56;note++)stress.noteOn(note,.8f);
 for(int i=0;i<8192;i++){
  if(i==2048)stress.pitchBend(2.0f);
  if(i==4096)stress.pitchBend(-1.5f);
  const auto s=stress.processOne();
  assert(std::isfinite(s.left)&&std::isfinite(s.right));
  assert(std::abs(s.left)<1.1f&&std::abs(s.right)<1.1f);
 }
 std::cout<<"Unified-state typed topology, PCM independence, kinematic influence, stability PASS\n";
}
