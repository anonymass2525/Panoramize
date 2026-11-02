#include "DeepInverse46.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>
namespace d=panorama::deep46;
namespace g=panorama::gen37;
namespace f=panorama::fem36;
int main(){
 // 8 s time-evolving tone with a smooth 4-semitone legato and vibrato (6 Hz)
 constexpr int sr=16000;constexpr int N=8*sr;const double pi=3.14159265358979323846;
 std::vector<float> audio(N);double phase=0;
 for(int i=0;i<N;i++){
  const double t=double(i)/sr,legato=std::clamp((t-3.)/.25,0.,1.);
  const double f0=220*std::pow(2.,4*legato/12.)*std::pow(2.,(.4*std::sin(2*pi*6*t))/12.);
  phase+=2*pi*f0/sr;
  const double harmonics=(t<5.2?.13:.6);
  audio[size_t(i)]=float(.22*(std::sin(phase)+harmonics*std::sin(2*phase)));
 }
 const auto analysis=d::analyze(audio,sr);
 if(!analysis.error.empty()){std::cerr<<analysis.error<<'\n';return 1;}
 assert(analysis.sections.size()>=14&&analysis.frames.size()>=200);
 assert(analysis.validPitchFraction>.3);
 assert(std::isfinite(analysis.centroidChange));
 bool hasPitch=false,hasLegato=false,hasVibrato=false;
 for(auto&e:analysis.gestures){
  if(e.kind==d::Gesture::Kind::PitchChange)hasPitch=true;
  if(e.kind==d::Gesture::Kind::LegatoCandidate)hasLegato=true;
  if(e.kind==d::Gesture::Kind::VibratoCandidate&&e.rateHz>4.&&e.rateHz<8.)hasVibrato=true;
 }
 std::cerr<<"pitch="<<hasPitch<<" legato="<<hasLegato<<" vibrato="<<hasVibrato<<"\n";
 assert(hasPitch&&hasLegato&&hasVibrato);
 assert(analysis.centroidChange>40.);
 std::cout<<"long-analysis sections="<<analysis.sections.size()<<" frames="<<analysis.frames.size()<<" pitchValid="<<analysis.validPitchFraction<<" events="<<analysis.gestures.size()<<" brightness-change="<<analysis.centroidChange<<" pitch-change="<<hasPitch<<'\n';
 auto invalid=audio;invalid[0]=std::numeric_limits<float>::quiet_NaN();assert(!d::analyze(invalid,sr).error.empty());
 // Source uses known pickup and an exciter within the parameterized search family.
 for(auto family:{g::Family::String1D,g::Family::Membrane2D}){
  g::WaveRecipe source;source.family=family;source.nx=24;source.ny=16;
  source.fundamentalHz=196;source.exciter=.28;source.pickup=.64;source.damping=2.;
  auto training=g::renderWave(source,22050,3072);
  auto holdout=g::renderWave(source,22050,3072);
  for(size_t i=0;i<holdout.size();i++)holdout[i]=.96f*holdout[i]+float(.00003*std::sin(2*pi*i/113.));
  assert(training.size()==3072);
  auto fitted=d::fitWave(training,holdout,22050,family,source.pickup,48);
  if(!fitted.error.empty()){std::cerr<<fitted.error<<'\n';return 2;}
  assert(fitted.evaluations==48&&!fitted.proposals.empty());
  auto changedHoldout=holdout;for(auto&z:changedHoldout)z*=.55f;
  auto noLeak=d::fitWave(training,changedHoldout,22050,family,source.pickup,48);
  assert(noLeak.proposals.size()==fitted.proposals.size());
  for(size_t i=0;i<fitted.proposals.size();i++){
   assert(fitted.proposals[i].trainingLoss==noLeak.proposals[i].trainingLoss);
   assert(g::encodeWaveRecipe(fitted.proposals[i].model)==g::encodeWaveRecipe(noLeak.proposals[i].model));
  }
  for(auto&x:fitted.proposals)assert(std::isfinite(x.trainingLoss)&&std::isfinite(x.holdoutLoss));
  std::cout<<"wave fit="<<int(family)<<" loss="<<fitted.proposals.front().trainingLoss<<" exciter="<<fitted.proposals.front().model.exciter<<" nx="<<fitted.proposals.front().model.nx<<" ny="<<fitted.proposals.front().model.ny<<'\n';
 }
 f::Model model;g::Shape shape;shape.xCells=2;shape.yCells=1;shape.zCells=1;shape.camber=.1;shape.taper=-.1;
 assert(g::buildTetraMesh(shape,model.mesh));model.meshYoung=5.2e8;model.meshDensity=1.3e5;
 model.pluckPosition=.72;
 std::vector<panorama::inverse36::Take> takes;
 for(int j=0;j<3;j++){
  panorama::inverse36::Take x;x.midi=40+5*j;x.velocity=.6f+.1f*j;x.training=j!=2;
  x.audio=panorama::inverse36::render(model,22050,x.midi,x.velocity,1152);assert(x.audio.size()==1152);
  takes.push_back(std::move(x));
 }
 auto fitted=d::fit3D(takes,22050,20);if(!fitted.error.empty()){std::cerr<<fitted.error<<'\n';return 3;}
 assert(!fitted.proposals.empty()&&fitted.evaluations<=20);
 for(auto&p:fitted.proposals){assert(std::isfinite(p.trainingLoss)&&std::isfinite(p.validationLoss));}
 std::cout<<"3d fit generated="<<fitted.proposals.size()<<" loss="<<fitted.proposals.front().trainingLoss<<" density="<<fitted.proposals.front().model.meshDensity<<" vertices="<<fitted.proposals.front().model.mesh.vertices.size()<<"\n";
 auto invalidTakes=takes;invalidTakes[0].audio[0]=std::numeric_limits<float>::quiet_NaN();assert(!d::fit3D(invalidTakes,22050,20).error.empty());
 std::cout<<"DEEP46_PASS\n";
}
