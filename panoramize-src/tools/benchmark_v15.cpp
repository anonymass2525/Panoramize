#include "PanoramaDSP.h"
#include <chrono>
#include <cmath>
#include <iostream>
using namespace panorama;
int main(){
 for(int preset=60;preset<64;preset++)for(int voices:{4,16}){
  Synth s;s.prepare(48000);s.setPatch(factoryPresets()[(size_t)preset]);
  for(int i=0;i<voices;i++)s.noteOn(48+i, .8f);
  constexpr int frames=4096;
  double checksum=0;
  const auto t0=std::chrono::steady_clock::now();
  for(int i=0;i<frames;i++){
   const auto x=s.processOne();checksum+=x.left*x.left;
  }
  const auto t1=std::chrono::steady_clock::now();
  const auto ms=std::chrono::duration<double,std::milli>(t1-t0).count();
  std::cout<<preset+1<<","<<voices<<","<<ms<<","<<100*ms/(1000.0*frames/48000)<<","<<checksum<<"\n";
 }
}
