#include "BoundaryPorts41.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <numeric>
#include <vector>
int main(){
 constexpr int block=64,blocks=1600;volatile double sink=0;
 for(int native=0;native<2;native++){
  auto r=panorama::boundary41::example();r.nativeEnabled=native!=0;
  r.velocityToEffort=3.;r.flowToForce=.01;
  std::string error;auto p=panorama::boundary41::Instrument::prepare(r,48000,&error);
  if(!p){std::cerr<<error<<'\n';return 1;}
  p->noteOn(57,.8f);
  std::vector<double>us;us.reserve(blocks);
  for(int j=0;j<blocks;j++){
   auto before=std::chrono::steady_clock::now();
   for(int i=0;i<block;i++)sink+=p->processOne();
   auto after=std::chrono::steady_clock::now();
   if(j>=100)us.push_back(std::chrono::duration<double,std::micro>(after-before).count());
  }
  std::sort(us.begin(),us.end());
  const double mean=std::accumulate(us.begin(),us.end(),0.)/us.size();
  std::cout<<"native="<<native<<",dofs="<<p->dofCount()<<",frames="<<block
   <<",median_us="<<us[us.size()/2]<<",p95_us="<<us[us.size()*95/100]
   <<",mean_us="<<mean<<",realtime_budget_us="<<1e6*block/48000.<<",sink="<<sink<<'\n';
 }
}
