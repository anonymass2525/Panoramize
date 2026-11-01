#include "GuitarVoice29.h"
#include <algorithm>
#include <cmath>
namespace panorama::guitar29 {
namespace {constexpr double pi=3.14159265358979323846;}
std::unique_ptr<Instrument> Instrument::prepare(Model m,double sr,std::string*error){
 Model validated;
 if(!(sr>=8000&&sr<=192000&&std::isfinite(sr))||!deserialize(serialize(m),validated)){
  if(error)*error="Invalid physical string model or sample rate";return {};
 }
 auto out=std::unique_ptr<Instrument>(new Instrument);out->m_=m;out->sr_=sr;
 if(error)error->clear();return out;
}
void Instrument::setRotation(int midi) noexcept{
 auto& v=voices_[size_t(midi)];const double f=440.*std::pow(2.,(midi-69)/12.)*bend_;
 for(int n=1;n<=v.count;n++){
  auto& o=v.partial[size_t(n-1)];const double B=m_.inharmonicity;
  const double frequency=f*n*std::sqrt((1.+B*n*n)/(1.+B));
  const double t=2*pi*std::min(.47*sr_,frequency)/sr_;
  o.rotR=std::cos(t);o.rotI=std::sin(t);
 }
}
void Instrument::noteOn(int midi,float velocity) noexcept{
 if(midi<0||midi>127||!std::isfinite(velocity)||velocity<=0)return;
 auto& v=voices_[size_t(midi)];v=Voice{};
 v.active=true;v.held=true;v.keyDown=true;v.level=std::clamp(double(velocity),0.,1.);
 v.count=std::clamp(m_.harmonics,1,24);
 const double f=440.*std::pow(2.,(midi-69)/12.);
 for(int n=1;n<=v.count;n++){
  auto& o=v.partial[size_t(n-1)];
  const auto weight=std::sin(pi*n*m_.pickPosition)*std::sin(pi*n*m_.pickupPosition)*std::exp(-m_.spectralTilt*(n-1))/n;
  o.weight=weight;o.decay=std::exp(-(m_.decayBase+m_.decayHigh*(n-1.)*(n-1.))/sr_);
  if(f*n>=.43*sr_)o.weight=0.;
 }
 setRotation(midi);
}
void Instrument::noteOff(int midi) noexcept{
 if(midi<0||midi>127)return;
 auto& v=voices_[size_t(midi)];v.keyDown=false;if(!pedal_)v.held=false;
}
void Instrument::sustain(bool down) noexcept{
 pedal_=down;if(!down)for(auto&v:voices_)v.held=v.keyDown;
}
void Instrument::pitchBend(float b) noexcept{
 if(!std::isfinite(b))return;bend_=std::pow(2.,std::clamp(double(b),-24.,24.)/12.);
 for(int i=0;i<128;i++)if(voices_[size_t(i)].active)setRotation(i);
}
void Instrument::allNotesOff() noexcept{for(auto&v:voices_)v=Voice{};pedal_=false;}
float Instrument::processOne() noexcept{
 double sum=0.;
 for(auto& v:voices_){
  if(!v.active)continue;
  double x=0.,energy=0.;
  for(int j=0;j<v.count;j++){
   auto& o=v.partial[size_t(j)];
   const double re=(o.re*o.rotR-o.im*o.rotI)*o.decay;
   const double im=(o.im*o.rotR+o.re*o.rotI)*o.decay;
   o.re=re;o.im=im;x+=o.weight*im;
   energy+=o.weight*o.weight*(im*im+re*re);
  }
  const double a=std::min(1.,v.duration/(.002*sr_));
  sum+=v.level*m_.gain*x*a;
  v.duration++;
  if(!v.held&&!pedal_)v.level*=.99985;
  if(energy*v.level*v.level<1.e-12||v.duration>sr_*20)v.active=false;
 }
 return float(std::clamp(sum,-2.,2.));
}
}
