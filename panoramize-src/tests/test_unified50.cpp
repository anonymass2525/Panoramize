#include "JointInverse50.h"
#include <cassert>
#include <algorithm>
#include <cmath>
#include <iostream>
using namespace panorama;
int main(){
 std::string err;
 auto src=universal23::genuineNativeGraph();
 auto list=controls50::catalog(src);
 std::cout<<"Native control count="<<list.size()<<"\n";
 assert(list.size()>=12);
 double d=0;assert(controls50::read(src,"genuine47.tone.cutoff",d));
 assert(controls50::change(src,"genuine47.tone.cutoff",8500,&err));
 assert(controls50::read(src,"genuine47.tone.cutoff",d)&&d==8500);
 auto original=universal23::serialize(src);
 assert(!controls50::change(src,"genuine47.tone.cutoff",-55,&err));
 assert(original==universal23::serialize(src));
 assert(!controls50::change(src,"genuine47.tone.cutoff",std::nan(""),&err));
 assert(!controls50::change(src,"bad.parameter",1,&err));
 assert(controls50::transaction(src,{{"genuine47.tone.cutoff",7400},{"genuine47.tone.output",.7}},48000,&err));
 auto good=universal23::serialize(src);
 assert(!controls50::transaction(src,{{"genuine47.tone.cutoff",7000},{"genuine47.tone.resonance",-1}},48000,&err));
 assert(good==universal23::serialize(src));
 universal23::Graph roundtrip;assert(universal23::deserialize(good,roundtrip,&err));
 assert(roundtrip.genuine47.tone.cutoff==7400);
 auto physical=universal23::sparseEnhancedGraph();
 auto cp=controls50::catalog(physical);assert(cp.size()>=12);
 const auto name="sparse43.region.0.young";
 assert(controls50::read(physical,name,d));
 assert(controls50::change(physical,name,5e7,&err));
 assert(controls50::read(physical,name,d)&&d==5e7);
 // Build an annotated 2-take input from true 2.9 string, not a substitute synth.
 auto str=design49::editableGraph(design49::Family::PhysicalString,220,.28);
 std::vector<joint50::Take>takes;
 for(int i=0;i<2;i++){
  const int midi=i?60:57;
  auto a=design49::renderGraph(str,midi,24000,.26,&err);assert(!a.empty());
  for(size_t k=0;k<a.size();k++)a[k]=float(.75*a[k]+.00002*std::sin(double(k)*.1));
  takes.push_back({a,midi,i==0});
 }
 auto inv=joint50::identify(takes,24000,12);
 std::cout<<"Joint candidates="<<inv.ranked.size()<<" evals="<<inv.totalEvaluations<<" top="<<(inv.ranked.empty()?"none":inv.ranked[0].family)<<"\n";
 assert(inv.error.empty());assert(inv.ranked.size()>=2);assert(!inv.identifiable);
 assert(inv.totalEvaluations>0);
 for(auto&c:inv.ranked){
  const std::string text=universal23::serialize(c.editable);
  universal23::Graph reread;assert(universal23::deserialize(text,reread,&err));
  auto sound=design49::renderGraph(reread,takes[0].midi,24000,.20,&err);
  assert(!sound.empty());for(auto x:sound)assert(std::isfinite(x));
 }
 std::cout<<"Panoramize 5.0 controls and uncertainty-aware inverse PASS\n";
}
