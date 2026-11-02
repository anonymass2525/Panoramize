#include "PanoramaDSP.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
using namespace panorama;
int main(){
 // Test strictly conservative quadratic energy, and real physical cycles.
 for(int shape=0;shape<3;shape++){
  MaterialMesh mesh;
  const auto topo=makeMeshTopology(shape,180.,1.);
  assert(mesh.configure(topo,48000.));
  mesh.excite(0,6.);
  const double initial=mesh.energy();assert(initial>0);
  double maxDrift=0;
  for(int i=0;i<9600;i++){
   const double x=mesh.step();assert(std::isfinite(x));
   maxDrift=std::max(maxDrift,std::abs(mesh.energy()-initial)/initial);
  }
  std::cout<<"shape "<<shape<<" conservative drift="<<maxDrift<<"\n";
  assert(maxDrift<3.e-8);
 }
 // Energy of forced model equals initial energy + precisely accounted work.
 {
  MaterialMesh mesh;assert(mesh.configure(makeMeshTopology(1,220.),44100.));
  mesh.excite(0,2.);const double e0=mesh.energy();
  for(int i=0;i<4000;i++)mesh.step(100.*std::sin(.005*i));
  const double balance=std::abs(mesh.energy()-e0-mesh.cumulativeWork());
  std::cout<<"external work balance "<<balance<<"\n";
  assert(balance<1.e-6);
  auto invalid=makeMeshTopology(1,220.);
  invalid.links[0].a=100;
  assert(!mesh.configure(invalid,48000.));
 }
 // Arbitrary 3-D coordinates and irregular closed topology can be modeled.
 {
  MeshTopology custom{};custom.count=4;custom.edges=5;custom.anchorSpring=1000.;
  custom.points[0]={0,0,0,1};custom.points[1]={1,0,0,.7};
  custom.points[2]={0,1,.5,1.3};custom.points[3]={1,1,1.,1.1};
  custom.links[0]={0,1,50000};custom.links[1]={1,3,40000};
  custom.links[2]={3,2,30000};custom.links[3]={2,0,65000};custom.links[4]={0,3,25000};
  MaterialMesh m;assert(m.configure(custom,48000));m.excite(2,5.);
  const double e0=m.energy();for(int i=0;i<18000;i++)assert(std::isfinite(m.step()));
  assert(std::abs(m.energy()-e0)/e0<2.e-8);
 }
 // Feedback is invalid when algebraic, valid with an explicit z^-1 node.
 {
  GraphPatch g{};g.nodes[0]={NodeType::VA,-1,-1,.8f,.4f};
  g.nodes[1]={NodeType::FeedbackDelay,5,-1,.7f,.4f};
  g.nodes[2]={NodeType::Mix,0,1,.5f,.4f};
  g.nodes[3]={NodeType::MeshBody,2,-1,.7f,.5f};
  g.nodes[4]={NodeType::Modal,3,-1,.55f,.5f};
  g.nodes[5]={NodeType::Mix,3,4,.8f,.5f};
  g.nodes[8]={NodeType::Output,5,-1,.8f,.4f};
  std::array<int,kGraphNodes> order{};
  assert(compileGraph(g,order));
  auto invalid=g;invalid.nodes[1].type=NodeType::Mix;
  assert(!compileGraph(invalid,order));
  auto p=factoryPresets()[66];assert(compileGraph(p.graph,order));
  Synth synth;synth.prepare(48000);synth.setPatch(p);assert(synth.graphValid());
  synth.noteOn(60,.85f);double energy=0;
  for(int i=0;i<18000;i++){
   auto s=synth.processOne();assert(std::isfinite(s.left)&&std::isfinite(s.right));
   assert(std::abs(s.left)<=1&&std::abs(s.right)<=1);
   energy+=s.left*s.left+s.right*s.right;
  }
  assert(energy>1.e-5);
 }
 for(int i=64;i<kPresetCount;i++){
  auto p=factoryPresets()[(size_t)i];Synth s;s.prepare(48000);s.setPatch(p);
  assert(s.graphValid());s.noteOn(64,.85f);
  double energy=0;
  for(int n=0;n<4096;n++){
   auto x=s.processOne();assert(std::isfinite(x.left)&&std::isfinite(x.right));
   energy+=x.left*x.left;
  }
  std::cout<<p.name<<" energy="<<energy<<"\n";
  assert(energy>1.e-7);
 }
 // Restricted inverse recovers an in-dictionary synthetic reference.
 {
  constexpr int frames=8192;
  MaterialMesh source;assert(source.configure(makeMeshTopology(1,220.,1.),48000.));
  source.excite(0,.8);
  std::vector<float> pcm(frames);
  for(auto& s:pcm)s=float(std::max(-1.,std::min(1.,source.step()*200.)));
  const auto result=estimateMeshFromAudio(pcm.data(),int(pcm.size()),48000.);
  std::cout<<"inverse candidates="<<result.evaluated<<" best shape="<<result.best[0].shape
           <<" pitch="<<result.best[0].fundamental<<" error="<<result.best[0].error<<"\n";
  assert(result.evaluated==171);
  assert(result.best[0].error<1.e-7);
 }
 std::cout<<"PANORAMA_AdvancedState PASSED\n";
}
