#include "GenuineNative47.h"
#include <chrono>
#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>
using namespace panorama;
int main(){
 constexpr int rate=48000,frames=64,trials=250;
 auto recipe=genuine47::example();
 std::cout<<"variant,frames,block_budget_us,median_us,p95_us,rt_ratio\n";
 double checksum=0.;
 for(int mode=0;mode<3;mode++){
  auto r=recipe;
  if(mode==0){r.routes.clear();r.tone.wet=0;r.tone.output=1;}
  if(mode==1){r.routes.clear();}
  auto synth=genuine47::Instrument::prepare(r,rate);if(!synth)return 2;
  synth->noteOn(60,.8f);
  for(int i=0;i<6000;i++)checksum+=synth->processOne();
  std::vector<double>times;times.reserve(trials);
  for(int i=0;i<trials;i++){
   auto begin=std::chrono::steady_clock::now();
   for(int j=0;j<frames;j++)checksum+=synth->processOne();
   auto end=std::chrono::steady_clock::now();
   times.push_back(std::chrono::duration<double,std::micro>(end-begin).count());
  }
  std::sort(times.begin(),times.end());double budget=1.e6*frames/rate;
  std::cout<<(mode==0?"original_bypass":mode==1?"tpt_filter":"tpt_filter_modulation")<<","<<frames<<","<<budget<<","<<times[trials/2]<<","<<times[int(trials*.95)]<<","<<times[trials/2]/budget<<"\n";
 }
 std::cerr<<"checksum="<<checksum<<"\n";
}
