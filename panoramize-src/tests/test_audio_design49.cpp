#include "AudioDesign49.h"
#include "ProductionInverse48.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <vector>
using namespace panorama;
int main(){
 constexpr int sr=48000;
 auto g=design49::editableGraph(design49::Family::PhysicalString,220.,.28);
 std::string err;
 const auto patch=universal23::serialize(g);
 universal23::Graph loaded;assert(universal23::deserialize(patch,loaded,&err));
 auto a=design49::renderGraph(loaded,57,sr,.7,&err);assert(!a.empty());
 for(auto x:a)assert(std::isfinite(x));
 std::vector<float> degraded(a.size()+3);
 for(size_t i=0;i<a.size();i++)degraded[i+3]=float(.72*a[i]+.000008*std::sin(double(i)*.177));
 const auto features=design49::extract(degraded,sr);
 std::cout<<"pitch="<<features.pitchHz<<" confidence="<<features.pitchConfidence<<" rms="<<features.rms<<'\n';
 assert(features.rms>1e-5);
 auto fitted=design49::infer(degraded,sr,5);
 if(!fitted.error.empty())std::cerr<<fitted.error<<'\n';
 assert(fitted.error.empty()&&fitted.ranked.size()>=3);
 for(const auto&candidate:fitted.ranked){
  assert(std::isfinite(candidate.distance));
  std::string why;auto s=universal23::serialize(candidate.graph);
  universal23::Graph reloaded;assert(universal23::deserialize(s,reloaded,&why));
  assert(universal23::serialize(reloaded)==s);
  auto v=design49::renderGraph(reloaded,candidate.midi,sr,.1,&why);assert(!v.empty());
  // Modulation and editing remain available after import.
  assert(!reloaded.globalRoutes.empty());
 }
 // Manual edit the generated physical instrument (not just display a thumbnail).
 auto edited=design49::editableGraph(design49::Family::PhysicalString,features.pitchHz,.22);
 edited.guitar.pickPosition=.33;
 auto playback=design49::renderGraph(edited,57,sr,.15,&err);assert(!playback.empty());
 auto h=design49::editableGraph(design49::Family::NativePhysicalHybrid,220.,.27);
 auto hybrid=universal23::Instrument::prepare(h,sr,&err);
 if(!hybrid)std::cerr<<err<<'\n';assert(hybrid);
 hybrid->noteOn(57,.7f);double energy=0;
 for(int i=0;i<3000;i++){auto s=hybrid->processOne();assert(std::isfinite(s.left));energy+=s.left*s.left;}
 assert(energy>1e-10);
 std::vector<float> silence(sr/2);assert(!design49::infer(silence,sr).error.empty());
 silence[10]=std::numeric_limits<float>::quiet_NaN();assert(!design49::infer(silence,sr).error.empty());
 // Common stable host macro and node-level parameter routing must affect
 // any source family without rebuilding its voices; disabling restores the exact saved graph.
 auto base=universal23::Instrument::prepare(edited,sr,&err);
 auto changed=universal23::Instrument::prepare(edited,sr,&err);
 auto restored=universal23::Instrument::prepare(edited,sr,&err);
 assert(base&&changed&&restored);
 base->noteOn(57,.8f);changed->noteOn(57,.8f);restored->noteOn(57,.8f);
 assert(!changed->setHostParameterNormalized(16,-.5));
 assert(changed->setHostParameterNormalized(16,.13));
 assert(changed->setHostParameterNormalized(9,1.));
 assert(!changed->setHostParameterNormalized(99,.5));
 restored->setHostParameterNormalized(16,.13);
 restored->setHostParameterNormalized(9,1.);
 restored->clearGlobalHostAutomation();
 double delta=0,resetError=0;
 for(int i=0;i<1024;i++){
  auto a=base->processOne(),b=changed->processOne(),c=restored->processOne();
  delta+=std::abs(a.left-b.left);resetError+=std::abs(a.left-c.left);
 }
 assert(delta>1.e-5&&resetError==0.);
 // Original Native47 bypass remains unmodified by candidate builder.
 auto original=universal23::genuineNativeGraph();auto preserved=universal23::serialize(original);
 assert(preserved==universal23::serialize(original));
 auto registry=design49::unifiedParameterRegistry(h);assert(registry.size()>9);
 std::cout<<"PANORAMIZE49_PASS candidates="<<fitted.ranked.size()<<" hybridEnergy="<<energy<<'\n';
}
