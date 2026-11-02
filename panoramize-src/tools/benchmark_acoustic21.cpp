#include "AcousticGraph21.h"
#include <chrono>
#include <cmath>
#include <iostream>
#include <vector>
using namespace panorama::acoustic21;
static double measure(int bodies,int voices,int frames){
 GraphBuilder b;
 for(int j=0;j<bodies;j++){
  Body body;body.mass=.8;body.rest={std::cos(j*.4),std::sin(j*.4),std::sin(j*.14)};
  body.anchor.stiffness=Tensor3::isotropic(1.e5);body.anchor.damping=Tensor3::isotropic(2.);
  b.addBody(body);
 }
 for(int j=0;j<bodies;j++){
  Link l;l.a=j;l.b=(j+1)%bodies;l.coupling.stiffness=Tensor3::isotropic(2.e4);
  l.coupling.damping=Tensor3::isotropic(10.);b.addLink(l);
 }
 auto graph=b.compile(48000.);if(!graph)return -1.;
 std::vector<StateVoice> instances;instances.reserve(voices);
 for(int i=0;i<voices;i++){
  instances.emplace_back(graph->makeVoice());instances.back().excite(i%bodies,{.01,.02,.03});
 }
 const auto start=std::chrono::steady_clock::now();
 for(int i=0;i<frames;i++)for(auto& instance:instances)instance.processSample();
 const auto elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
 return elapsed;
}
int main(){
 const int frames=2048;
 std::cout<<"48kHz, "<<frames<<" frames, real time budget="<<(1000.*frames/48000.)<<"ms\n";
 for(const int bodies:{4,8,16,24})for(const int voices:{1,4,16}){
  double best=1.e12;
  for(int i=0;i<3;i++)best=std::min(best,measure(bodies,voices,frames));
  std::cout<<"bodies="<<bodies<<", dof="<<3*bodies<<", voices="<<voices<<", best_ms="<<best<<"\n";
 }
}
