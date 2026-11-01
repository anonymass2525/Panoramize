#include "NativeState27.h"
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
using namespace panorama;
int main(){
 constexpr int frames=4096;const double sr=48000.;
 for(int voices:{1,4,8,16,32}){
  std::string error;
  auto inst=native27::Instrument::prepare(native27::example(),sr,&error);
  if(!inst){std::cerr<<error<<'\n';return 1;}
  for(int i=0;i<voices;i++)inst->noteOn(48+i,.85f);
  volatile double prevent=0;
  for(int i=0;i<256;i++)prevent+=inst->processMono();
  const auto before=std::chrono::steady_clock::now();
  for(int i=0;i<frames;i++)prevent+=inst->processMono();
  const auto end=std::chrono::steady_clock::now();
  const double ms=std::chrono::duration<double,std::milli>(end-before).count();
  std::cout<<voices<<" voices: "<<ms<<" ms / 85.333ms budget, checks="<<prevent<<'\n';
 }
}
