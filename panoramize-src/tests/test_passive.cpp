#include "PanoramaDSP.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <algorithm>
#include <cmath>
#include <iostream>
using namespace panorama;
static void check(bool damped,bool forced){
 PassiveMeshSettings config{};
 config.topology=makeMeshTopology(1,270.,1.);
 for(int i=0;i<config.topology.count;i++)config.groundDamping[(size_t)i]=damped?.35:0.;
 for(int i=0;i<config.topology.edges;i++)config.edgeDamping[(size_t)i]=damped?1.8:0.;
 PassiveMesh model;
 assert(model.configure(config,48000.));
 model.excite(0,5.);
 const double initial=model.energy();assert(initial>0);
 double maxRelError=0.;
 double previousEnergy=initial;
 for(int n=0;n<9000;n++){
  const double drive=forced?120.*std::sin(.006*n):0.;
  const double out=model.step(drive);
  assert(std::isfinite(out));
  const double e=model.energy();
  assert(std::isfinite(e)&&e>=0.);
  assert(model.dissipatedEnergy()>=0.);
  if(damped&&!forced)assert(e<=previousEnergy+1e-9);
  previousEnergy=e;
  const double balance=e+model.dissipatedEnergy()-model.externalWork();
  maxRelError=std::max(maxRelError,std::abs(balance-initial)/initial);
 }
 std::cout<<"damped="<<damped<<" forced="<<forced
          <<" ledger relative error="<<maxRelError
          <<" dissipation="<<model.dissipatedEnergy()<<"\n";
 assert(maxRelError<1.e-8);
 if(damped)assert(model.dissipatedEnergy()>0.);
 if(!damped)assert(model.dissipatedEnergy()==0.);
}
int main(){
 check(false,false);check(true,false);check(true,true);check(false,true);
 PassiveMeshSettings custom{};
 custom.topology.count=3;custom.topology.edges=2;
 custom.topology.points[0]={0,0,0,1};custom.topology.points[1]={1,1,1,2};custom.topology.points[2]={2,0,1,.5};
 custom.topology.links[0]={0,1,10000};custom.topology.links[1]={1,2,7000};
 custom.groundDamping[0]=.3;custom.edgeDamping[1]=3.;
 PassiveMesh s;assert(s.configure(custom,48000));s.excite(0,1.);
 const double initial=s.energy();
 for(int i=0;i<10000;i++)s.step();
 assert(std::abs(s.energy()+s.dissipatedEnergy()-initial)<1.e-8);
 custom.groundDamping[0]=-1.;assert(!s.configure(custom,48000));
 custom.groundDamping[0]=.2;custom.edgeDamping[1]=NAN;
 assert(!s.configure(custom,48000));
 for(int index=68;index<kPresetCount;index++){
  auto patch=factoryPresets()[(size_t)index];
  std::array<int,kGraphNodes> order{};
  assert(compileGraph(patch.graph,order));
  Synth voice;voice.prepare(48000);voice.setPatch(patch);voice.noteOn(60,.8f);
  double energy=0.;
  for(int i=0;i<4096;i++){
   const auto x=voice.processOne();
   assert(std::isfinite(x.left)&&std::isfinite(x.right));
   energy+=x.left*x.left+x.right*x.right;
  }
  std::cout<<patch.name<<" signal energy="<<energy<<"\n";
  assert(energy>1.e-8);
 }
 std::cout<<"PANORAMA_PassiveCoupling PASSED\n";
}
