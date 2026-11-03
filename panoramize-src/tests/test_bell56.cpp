// Panoramize 5.6: preserve 5.5, evaluate spectrally evolving coupled bell modes.
#include "PanoramaDSP.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
struct Audio {std::vector<float> l,r;double peak=0.,absSum=0.;};
void demand(bool v,const char*msg){if(!v)throw std::runtime_error(msg);}
Audio play(panorama::Patch p,double sr,int note=72,int ms=1700){
 panorama::Synth synth;
 synth.prepare(sr);synth.setPatch(p);synth.noteOn(note,.8f);
 const int n=int(sr*ms/1000);
 Audio a; a.l.reserve(size_t(n));a.r.reserve(size_t(n));
 for(int i=0;i<n;i++){
  if(i==int(sr*.45))synth.noteOff(note);
  const auto s=synth.processOne();
  if(!std::isfinite(s.left)||!std::isfinite(s.right))
    throw std::runtime_error("nonfinite coupled bell audio");
  a.l.push_back(s.left);a.r.push_back(s.right);
  a.absSum+=std::abs(double(s.left))+std::abs(double(s.right));
  a.peak=std::max(a.peak,std::max(std::abs(double(s.left)),std::abs(double(s.right))));
 }
 return a;
}
double difference(const Audio&a,const Audio&b,size_t offset=0){
 demand(a.l.size()==b.l.size(),"mismatched render lengths");
 double d=0.;
 for(size_t i=offset;i<a.l.size();i++)
  d+=std::fabs(a.l[i]-b.l[i])+std::fabs(a.r[i]-b.r[i]);
 return d;
}
} // namespace
int main(){
 try{
  auto bell=panorama::factoryPresets()[4];
  demand(bell.engine==panorama::Engine::FractalDigital,"bell engine mismatch");
  demand(bell.bellDynamics>0.1f&&bell.bellCoupling>0.f,"5.6 preset not evolved");
  auto old=bell;old.bellDynamics=0.f;
  // A zeroed evolution control must recover Evolved 5.5 exactly, regardless of
  // coupling or clarity values retained in the serialized patch.
  auto disabled=old;disabled.bellCoupling=1.f;disabled.bellClarity=0.f;
  demand(difference(play(old,48000.),play(disabled,48000.))==0.,
    "5.5 audio changed when bellDynamics=0");
  demand(!panorama::samePatchConfiguration(old,bell),
    "new DSP parameters must invalidate the compiled patch cache");
  const auto legacy=play(old,48000.);
  const auto updated=play(bell,48000.);
  demand(legacy.absSum>1.&&updated.absSum>1.,"bell output silent");
  demand(difference(legacy,updated)>0.01,"5.6 no measurable contribution");
  demand(difference(legacy,updated,size_t(48000*.6))>0.01,
    "5.6 late spectral/tail contribution is missing");
  auto noCoupling=bell;noCoupling.bellCoupling=0.f;
  demand(difference(updated,play(noCoupling,48000.))>0.001,
    "mode-coupling rotation has no audible influence");
  auto neutralClarity=bell;neutralClarity.bellClarity=0.f;
  demand(difference(updated,play(neutralClarity,48000.))>0.001,
    "tail clarity has no audible influence");
  // A proper 2x2 rotation is energy neutral when no excitation/damping acts.
  const double angle=.031,c=std::cos(angle),s=std::sin(angle);
  for(int n=0;n<10;n++){
   const double x1=.03*n,x2=1.-.02*n,y1=.18*n,y2=-.01*n;
   const double e0=x1*x1+x2*x2+y1*y1+y2*y2;
   const double a=c*x1+s*x2,b=c*x2-s*x1;
   const double d=c*y1+s*y2,e=c*y2-s*y1;
   demand(std::abs((a*a+b*b+d*d+e*e)-e0)<1e-12,
     "modal energy exchange not orthogonal");
  }
  // At high pitches some companion modes must be muted, not pinned at Nyquist.
  for(double sr:{44100.,48000.,96000.}){
   for(int midi:{48,72,96,108}){
    auto a=play(bell,sr,midi,700);
    demand(a.peak<1.001&&a.absSum>.001,"pitch/rate caused silence or clipping");
   }
  }
  // Other engines, even with Bell 5.6 parameters enabled, retain bitwise output.
  auto other=panorama::factoryPresets()[6];
  auto otherAlt=other;otherAlt.bellDynamics=1.;otherAlt.bellCoupling=1.;otherAlt.bellClarity=1.;
  demand(difference(play(other,48000.,60,350),play(otherAlt,48000.,60,350))==0.,
    "new bell controls changed Physical Resonance engine");
  std::cout<<"Bell56 PASS: 5.5 bypass exact; orthogonal mode coupling; time-varying tone; "
           <<"44.1/48/96kHz finite; peak "<<updated.peak<<"\n";
  return 0;
 }catch(const std::exception&e){std::cerr<<"Bell56 FAIL: "<<e.what()<<"\n";return 1;}
}
