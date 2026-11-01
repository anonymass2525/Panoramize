#undef NDEBUG
#include <cassert>
#include "NativeState26.h"
#include <cmath>
#include <iostream>
int main(){
 for(int i=0;i<6;i++){
  auto g=panorama::native26::example(i);
  auto serialized=panorama::native26::serialize(g);
  panorama::native26::Graph roundtrip;
  assert(panorama::native26::deserialize(serialized,roundtrip));
  auto synth=panorama::native26::Instrument::prepare(roundtrip,48000.);assert(synth);
  for(int note=48;note<64;note++)synth->noteOn(note,.4f);
  for(int n=0;n<1024;n++){
   if(n==256)assert(synth->addActiveEffort(57,2,2.));
   auto value=synth->processMono();assert(std::isfinite(value));
  }
  assert(std::abs(synth->energyResidual())<1e-5);
 }
 std::cout<<"ASAN_NATIVE26_SUCCESS\n";
}
