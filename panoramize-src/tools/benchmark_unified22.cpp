#include "UnifiedGraph22.h"
#include <chrono>
#include <iostream>
#include <vector>
using namespace panorama::unified22;
int main(int argc,char**){
 (void)argc;
 Builder b;
 const Domain d[]={Domain::Physical,Domain::Spectral,Domain::Phase,Domain::Resonance,Domain::Nonlinear,Domain::VocalFormant,Domain::VocalFormant};
 for(int i=0;i<7;i++){Mode m;m.name="n"+std::to_string(i);m.domain=d[i];m.frequencyHz=120.+i*150.;m.dampingPerSecond=1.+i*.5;m.quartic=(i==4?8.e11:0.);b.addMode(m);}
 for(int i=0;i<7;i++)b.connect({i,(i+1)%7,LinkKind::Conservative,2.e5,.7});
 auto g=b.compile(48000.);if(!g)return 1;
 for(int poly:{1,4,16}){
  std::vector<Voice> voices;voices.reserve(size_t(poly));
  for(int i=0;i<poly;i++){voices.push_back(g->makeVoice());voices.back().excite(i%7,.5);}
  const auto t=std::chrono::steady_clock::now();double checksum=0.;
  for(int j=0;j<4096;j++)for(auto& v:voices){if(!v.processSample())return 2;checksum+=v.audioSample(0);}
  const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t).count();
  std::cout<<"poly="<<poly<<" modes=7 frames=4096 ms="<<ms<<" realtime_budget_ms=85.3333 checksum="<<checksum<<"\n";
 }
 return 0;
}
