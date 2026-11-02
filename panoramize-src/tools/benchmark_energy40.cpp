#include "EnergyPorts40.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <numeric>
#include <vector>
using namespace panorama;
int main(){
 constexpr double sr=48000.;constexpr int block=64;constexpr int cycles=500;
 std::cout<<"nodes,links,block_frames,block_budget_us,measured_block_us,rt_ratio\n";
 double checksum=0;
 for(int n:{3,6,9,12}){
  auto r=energy40::example();r.nodes.resize(size_t(n));
  for(int i=3;i<n;i++){r.nodes[size_t(i)]=r.nodes[size_t(i%3)];r.nodes[size_t(i)].x=std::min(.9,.10+i*.06);}
  r.links.clear();for(int i=1;i<n;i++){
   energy40::Link l;l.from=i-1;l.to=i;l.stiffness=60+i*12;l.damping=.015;r.links.push_back(l);
  }
  for(int i=0;i<n/2;i++){
   energy40::Link l;l.from=i;l.to=(n-i-1);if(l.from==l.to)continue;l.stiffness=20;l.damping=.007;r.links.push_back(l);
  }
  auto x=energy40::Instrument::prepare(r,sr);if(!x)return 2;
  x->noteOn(57,.8f);for(int i=0;i<5000;i++)checksum+=x->processOne();
  std::vector<double> us;us.reserve(cycles);
  for(int i=0;i<cycles;i++){
   auto start=std::chrono::steady_clock::now();
   for(int j=0;j<block;j++)checksum+=x->processOne();
   auto end=std::chrono::steady_clock::now();
   us.push_back(std::chrono::duration<double,std::micro>(end-start).count());
  }
  std::sort(us.begin(),us.end());double median=us[cycles/2];
  double budget=1e6*block/sr;
  std::cout<<n<<","<<r.links.size()<<","<<block<<","<<budget<<","<<median<<","<<median/budget<<"\n";
 }
 std::cerr<<"checksum="<<checksum<<"\n";
}
