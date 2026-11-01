#include "ArticulatedPhysical33.h"
#include <chrono>
#include <cmath>
#include <iostream>
using namespace panorama;
int main(){
 using clock=std::chrono::steady_clock;
 constexpr int rate=48000, frames=4096, repeats=8;
 for(int voices:{1,4,8,16}){
  articulated33::Model m;m.monoLegato=false;m.physical.body=true;m.physical.bridge=true;
  auto instrument=articulated33::Instrument::prepare(m,rate);
  for(int i=0;i<voices;i++)instrument->noteOn(48+i,.72f);
  instrument->palmMute(.33f);
  volatile double sink=0.;
  const auto t0=clock::now();
  for(int r=0;r<repeats;r++)for(int i=0;i<frames;i++)sink+=instrument->processOne();
  const auto t1=clock::now();
  std::cout<<voices<<" voices "<<std::chrono::duration<double,std::milli>(t1-t0).count()/repeats
   <<" ms per "<<frames<<" frames at "<<rate<<" Hz (budget "<<1000.*frames/rate<<" ms)\n";
  if(!std::isfinite(sink))return 1;
 }
}
