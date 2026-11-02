#include "BoundaryPorts41.h"
#include "UniversalInstrument23.h"
#include <algorithm>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
using namespace panorama;
int main(){
 std::string err;
 auto model=boundary41::example();
 assert(boundary41::valid(model,&err));
 auto blob=boundary41::serialize(model);boundary41::Recipe loaded;
 assert(boundary41::deserialize(blob,loaded,&err));assert(boundary41::serialize(loaded)==blob);
 {auto invalid=model;invalid.pistonArea=0;assert(!boundary41::valid(invalid));
  invalid=model;invalid.bodyShape.zCells=20;assert(!boundary41::valid(invalid));
  assert(!boundary41::deserialize(blob+"TRAIL\n",loaded));}
 auto inst=boundary41::Instrument::prepare(model,48000,&err);
 assert(inst&&inst->dofCount()>30);
 inst->noteOn(57,.8f);
 double absSum=0,bmax=0,cmax=0;
 for(int i=0;i<14000;i++){
  double x=inst->processOne();assert(std::isfinite(x));absSum+=std::abs(x);
  bmax=std::max(bmax,std::abs(inst->boundaryVelocity(2)));
  cmax=std::max(cmax,std::abs(inst->cavityVolumeFlow()));
 }
 auto L=inst->ledger();
 std::cout<<"dofs "<<inst->dofCount()<<" output_sum "<<absSum<<" 3Dvelocity "<<bmax<<" cavity_flow "<<cmax
          <<" work "<<L.active<<" loss "<<L.dissipated<<" E "<<L.finalEnergy<<" residual "<<L.residual<<"\n";
 assert(absSum>1e-5);assert(bmax>1e-12);assert(cmax>1e-13);
 assert(L.finalEnergy>=0&&L.dissipated>=0&&L.active>=0);
 assert(std::abs(L.residual)<std::max(1e-11,1e-7*L.active));
 auto isolated=model;isolated.membraneBodyK=0;isolated.membraneBodyD=0;
 isolated.bodyCavityK=0;isolated.bodyCavityD=0;
 auto noLink=boundary41::Instrument::prepare(isolated,48000,&err);assert(noLink);
 noLink->noteOn(57,.8f);
 double disconnectedBody=0,disconnectedCavity=0;
 for(int i=0;i<10000;i++){
  noLink->processOne();disconnectedBody=std::max(disconnectedBody,std::abs(noLink->boundaryVelocity(2)));
  disconnectedCavity=std::max(disconnectedCavity,std::abs(noLink->cavityVolumeFlow()));
 }
 assert(disconnectedBody<1e-14&&disconnectedCavity<1e-14);
 // 4.0's native state is not SI calibrated; permitting it does not convert the
 // active bridge into a passive connection. Its work is in a separate ledger.
 auto active=model;active.nativeEnabled=true;active.velocityToEffort=3.;active.flowToForce=.05;
 auto withNative=boundary41::Instrument::prepare(active,48000,&err);assert(withNative);
 withNative->noteOn(57,.6f);
 for(int i=0;i<1000;i++)assert(std::isfinite(withNative->processOne()));
 assert(std::isfinite(withNative->ledger().nativeActiveWork));
 // When all linear damping is zero, the midpoint solve conserves the exact
 // quadratic energy after the single active impulse (within roundoff).
 auto ideal=model;ideal.stringDamping=0;ideal.membraneDamping=0;ideal.bodyDamping=0;
 ideal.stringMembraneD=0;ideal.membraneBodyD=0;ideal.bodyCavityD=0;
 auto lossless=boundary41::Instrument::prepare(ideal,48000,&err);assert(lossless);
 lossless->noteOn(57,.5f);
 for(int i=0;i<8000;i++)lossless->processOne();
 const auto exact=lossless->ledger();
 assert(std::abs(exact.residual)<std::max(1e-12,1e-7*exact.active));
 assert(std::abs(exact.finalEnergy-exact.active)<std::max(1e-12,1e-7*exact.active));
 // The 8 native modes may all be addressed through an ACTIVE adapter, but
 // are not misrepresented as calibrated SI passive connections.
 for(int mode=0;mode<8;mode++){
  active.nativeMode=mode;
  auto bridge=boundary41::Instrument::prepare(active,48000,&err);assert(bridge);
  bridge->noteOn(60,.5f);
  for(int i=0;i<32;i++)assert(std::isfinite(bridge->processOne()));
 }
 auto graph=universal23::boundaryGraph();
 auto patch=universal23::serialize(graph);universal23::Graph round;
 assert(universal23::deserialize(patch,round,&err));
 assert(universal23::serialize(round)==patch);
 auto player=universal23::Instrument::prepare(round,48000,&err);assert(player);
 player->noteOn(57,.8f);
 double sum=0;for(int i=0;i<5000;i++){
  auto v=player->processOne();assert(std::isfinite(v.left));sum+=std::abs(v.left);
 }
 assert(sum>1e-6);
 std::cout<<"Panoramize 4.1 boundary FEM and acoustic port PASS\n";
}
