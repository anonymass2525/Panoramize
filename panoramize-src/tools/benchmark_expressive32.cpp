#include "ExpressivePhysical32.h"
#include <chrono>
#include <iostream>
#include <cmath>
int main(){
 constexpr int rate=48000,frames=4096;volatile double checksum=0;
 for(int voices:{1,4,8,16}){
  panorama::expressive32::Model m;m.monoLegato=voices==1;m.physical.bridge=true;m.physical.body=true;
  auto synth=panorama::expressive32::Instrument::prepare(m,rate);
  for(int i=0;i<voices;i++)synth->noteOn(45+i*2,.78f);
  double shortest=1e9;
  for(int repeat=0;repeat<3;repeat++){
   const auto start=std::chrono::steady_clock::now();
   for(int i=0;i<frames;i++)checksum+=synth->processOne();
   const double elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
   shortest=std::min(shortest,elapsed);
  }
  std::cout<<voices<<" voices: best-of-three "<<shortest<<" ms / "<<1000.*frames/rate<<" ms buffer budget\n";
 }
 std::cout<<"checksum "<<checksum<<'\n';
}
