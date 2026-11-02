#include "PerformancePhysical30.h"
#include "UniversalInstrument23.h"
#undef NDEBUG
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
using namespace panorama::performance30;
int main(){
 constexpr int sr=16000;constexpr double pi=3.141592653589793;
 std::vector<float> phrase(size_t(sr*23/10),0.f);
 double phase=0.;
 for(size_t i=0;i<phrase.size();i++){
  const double sec=double(i)/sr;
  if(sec<.1)continue;
  double hz=sec<1.15?196.:246.94;
  if(sec>1.6)hz*=std::pow(2.,.65*std::sin(2*pi*5*(sec-1.6))/12.);
  phase+=2*pi*hz/sr;
  const double level=sec<1.8?.35:.035;
  phrase[i]=float(level*std::sin(phase));
 }
 auto analyzed=analyze(phrase.data(),int(phrase.size()),sr);
 assert(analyzed.valid && analyzed.frames.size()>100);
 bool pluck=false,transition=false,mute=false,vibrato=false,legato=false;
 for(const auto&e:analyzed.events){
  if(e.kind==EventKind::Pluck)pluck=true;
  if(e.kind==EventKind::PitchTransition)transition=true;
  if(e.kind==EventKind::DampingChange)mute=true;
  if(e.kind==EventKind::VibratoCandidate)vibrato=true;
  if(e.kind==EventKind::LegatoCandidate)legato=true;
 }
 for(const auto&e:analyzed.events)std::cout<<eventName(e.kind)<<" at "<<e.seconds<<" Hz "<<e.pitchHz<<" conf="<<e.confidence<<"\n";
 assert(pluck);assert(transition);assert(mute);assert(vibrato);assert(legato);
 std::vector<float> zero(phrase.size(),0.f);
 assert(!analyze(zero.data(),int(zero.size()),sr).valid);
 assert(!analyze(nullptr,1000,sr).valid);
 panorama::guitar29::Model string;
 string.frequencyHz=196.;string.pickPosition=.26;string.decayBase=2.6;string.decayHigh=.026;
 CoupledModel ref;ref.string=string;ref.bridge=true;ref.body=true;
 ref.bridgeHz=480.;ref.bodyHz=270.;
 auto restored=ref;const auto txt=serialize(ref);
 assert(deserialize(txt,restored));assert(serialize(restored)==txt);
 assert(!deserialize(txt+"EXTRA",restored));
 assert(!deserialize("bogus",restored));
 auto noBody=ref;noBody.body=false;noBody.bridge=false;
 const auto full=render(ref,sr,1.65,.035,1.);
 const auto bare=render(noBody,sr,1.65,.035,1.);
 double delta=0.;for(size_t i=0;i<full.size();i++){
  assert(std::isfinite(full[i]));delta+=std::abs(full[i]-bare[i]);
 }assert(delta>1.e-3);
 std::vector<std::vector<float>> recordings;
 std::vector<Take> takes;
 for(int j=0;j<4;j++){
  const double factor=std::pow(2.,(j==0?0:j==1?5:j==2?-5:7)/12.);
  recordings.push_back(render(ref,sr,1.65,.035,factor));
 }
 for(int j=0;j<4;j++){
  const double factor=std::pow(2.,(j==0?0:j==1?5:j==2?-5:7)/12.);
  takes.push_back({recordings[j].data(),int(recordings[j].size()),sr,196.*factor,j!=3});
 }
 const auto inferred=discover(takes,string,70,4);
 assert(inferred.success&&inferred.performances.size()==4&&inferred.proposals.size()==4);
 assert(inferred.proposals[0].trainingLoss>=0&&std::isfinite(inferred.proposals[0].holdoutLoss));
 for(size_t j=1;j<inferred.proposals.size();j++)
  assert(inferred.proposals[j].topology!=inferred.proposals[0].topology);
 const auto again=discover(takes,string,70,4);
 assert(again.success&&again.proposals[0].topology==inferred.proposals[0].topology);
 assert(std::abs(again.proposals[0].trainingLoss-inferred.proposals[0].trainingLoss)<1.e-12);
 assert(!discover({},string).success);
 // Verify the 3.0 model can be stored in a normal Studio One .pgraph source
 // and played through the existing JUCE processor's universal DSP route.
 panorama::universal23::Graph g;
 g.name="Physical30 MIDI source";
 g.nodes={{panorama::universal23::Kind::PhysicalCoupled30,-1,-1,0,.9f},
          {panorama::universal23::Kind::Output,0,-1,0,.8f}};
 g.physical30=inferred.proposals[0].model;
 const auto graphData=panorama::universal23::serialize(g);
 panorama::universal23::Graph parsed;std::string error;
 assert(panorama::universal23::deserialize(graphData,parsed,&error));
 assert(graphData==panorama::universal23::serialize(parsed));
 auto playable=panorama::universal23::Instrument::prepare(parsed,sr,&error);
 assert(playable);
 playable->noteOn(60,.78f);
 double sum=0.;for(int n=0;n<12000;n++){
  const auto v=playable->processOne();assert(std::isfinite(v.left));sum+=v.left*v.left;
 }assert(sum>1.e-8);
 playable->noteOff(60);
 auto corrupt=graphData;const auto ix=corrupt.find("PHYSICAL30_END");assert(ix!=std::string::npos);
 corrupt.replace(ix,14,"PHYSICAL30_BAD");
 assert(!panorama::universal23::deserialize(corrupt,parsed,&error));
 std::cout<<"PERFORMANCE30_OK events="<<analyzed.events.size()<<" proposals="<<inferred.proposals.size()
          <<" best="<<inferred.proposals[0].topology<<" score="<<inferred.proposals[0].trainingLoss
          <<" hold="<<inferred.proposals[0].holdoutLoss<<'\n';
}
