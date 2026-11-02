#include "ArticulatedPhysical33.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
namespace panorama::articulated33 {
namespace {constexpr double pi=3.14159265358979323846;
 double clamp(double x,double low,double high){return std::clamp(x,low,high);}
 bool finite(double x){return std::isfinite(x);}
}
// Serialize the existing 3.2 instrument base losslessly, with a separately versioned
// articulation payload; no implicit change to any old .pgraph format.
std::string serialize(const Model&m){
 std::ostringstream s;s<<std::setprecision(17)<<"PANORAMIZE-ARTICULATED33-v1\n";
 s<<"contact "<<m.hammerImpact<<' '<<m.pullImpact<<' '<<m.slideFriction<<' '
  <<m.palmDamping<<' '<<m.palmPosition<<'\n';
 s<<"EXPRESSIVE32_BEGIN\n"<<expressive32::serialize(static_cast<const expressive32::Model&>(m))<<"EXPRESSIVE32_END\n";
 return s.str();
}
bool deserialize(const std::string&source,Model&out){
 if(source.size()>24000)return false;
 std::istringstream f(source);std::string line;Model v;
 if(!std::getline(f,line)||line!="PANORAMIZE-ARTICULATED33-v1")return false;
 if(!std::getline(f,line))return false;
 std::istringstream row(line);std::string key,junk;
 if(!(row>>key>>v.hammerImpact>>v.pullImpact>>v.slideFriction>>v.palmDamping>>v.palmPosition)||key!="contact"||(row>>junk))return false;
 if(!finite(v.hammerImpact)||v.hammerImpact<0||v.hammerImpact>2 ||
    !finite(v.pullImpact)||v.pullImpact<0||v.pullImpact>2 ||
    !finite(v.slideFriction)||v.slideFriction<0||v.slideFriction>2 ||
    !finite(v.palmDamping)||v.palmDamping<0||v.palmDamping>1000 ||
    !finite(v.palmPosition)||v.palmPosition<.7||v.palmPosition>.99)return false;
 if(!std::getline(f,line)||line!="EXPRESSIVE32_BEGIN")return false;
 std::string base;bool closed=false;
 while(std::getline(f,line)){
  if(line=="EXPRESSIVE32_END"){closed=true;break;}
  base+=line+'\n';if(base.size()>20000)return false;
 }
 if(!closed||!expressive32::deserialize(base,static_cast<expressive32::Model&>(v)))return false;
 while(std::getline(f,line))if(!line.empty())return false;
 out=v;return true;
}
std::unique_ptr<Instrument> Instrument::prepare(const Model&m,double sr,std::string*error){
 Model validation;
 if(!finite(sr)||sr<8000||sr>192000||!deserialize(serialize(m),validation)){
  if(error)*error="invalid expressive model or sample rate";return {};
 }
 auto ins=std::unique_ptr<Instrument>(new Instrument());ins->config_=m;ins->sr_=sr;ins->palmX_=m.palmPosition;
 static constexpr double ratios[]={1.,.79,1.33};
 for(int i=0;i<3;i++){
  auto init=[&](Bank& bank,double f,double damping,double mix){
   const double r=std::exp(-damping/sr),w=2*pi*f*ratios[i]/sr;
   auto& p=bank.modes[size_t(i)];p.c=2*r*std::cos(w);p.d=-r*r;p.scale=std::max(.00001,1-r);
   bank.amount=mix;
  };
  init(ins->bridge_,m.physical.bridgeHz,m.physical.bridgeDecay,m.physical.bridgeMix);
  init(ins->body_,m.physical.bodyHz,m.physical.bodyDecay,m.physical.bodyMix);
 }
 if(error)error->clear();return ins;
}
void Instrument::strike(Voice&v,int note,float velocity) noexcept{
 const auto&m=config_.physical.string;
 v=Voice{};v.note=note;v.pitchMidi=note;v.targetMidi=note;v.active=true;v.held=true;
 v.level=std::pow(clamp(velocity,0.,1.),config_.velocityForceCurve);
 v.pick=clamp(m.pickPosition+config_.velocityPickRange*(.5-velocity),.03,.97);
 lastPick_=v.pick;v.count=std::clamp(m.harmonics,1,24);
 for(int n=1;n<=v.count;n++){
  auto& p=v.partial[size_t(n-1)];
  p.baseWeight=std::sin(pi*n*v.pick)*std::sin(pi*n*m.pickupPosition)*std::exp(-m.spectralTilt*(n-1))/n;
 }
 updateRotation(v);attacks_++;
}
void Instrument::updateRotation(Voice&v) noexcept{
 if(!v.active)return;
 const auto&m=config_.physical.string;
 const double f=440.*std::exp2((v.pitchMidi+smoothBend_-69.)/12.);
 for(int j=0;j<v.count;j++){
  const int n=j+1;auto&p=v.partial[size_t(j)];
  double hz=f*n*std::sqrt((1.+m.inharmonicity*n*n)/(1.+m.inharmonicity));
  hz=std::min(hz,sr_*.45);
  const double a=2*pi*hz/sr_;p.rotR=std::cos(a);p.rotI=std::sin(a);
 }
}
void Instrument::setNextTransition(Transition mode,float intensity) noexcept{
 pending_=mode;pendingIntensity_=std::isfinite(intensity)?float(clamp(intensity,0.,1.)):1.f;
}
void Instrument::setTransitionIntensity(float intensity) noexcept{
 if(std::isfinite(intensity))pendingIntensity_=float(clamp(intensity,0.,1.));
}
void Instrument::articulate(Voice&v,int note,float intensity,Transition mode) noexcept{
 // Changing the fretted speaking length changes the natural frequencies; do not
 // zero modal displacement or velocity. Approximate string/finger impulse is
 // projected onto standing-wave mode shapes at the fretting location.
 const int delta=note-v.note;
 const double distance=std::abs(delta);
 if(mode==Transition::Legato){legatos_++;}
 else if(mode==Transition::Slide){
  slides_++;v.slideFriction=clamp(intensity,0.,1.);legatos_++;
 }else{
  const double fret=clamp(1.-std::exp2(-distance/12.),.055,.9);
  const double strength=(mode==Transition::HammerOn?config_.hammerImpact:config_.pullImpact)*clamp(intensity,0.,1.);
  const double direction=(mode==Transition::HammerOn?1.:-1.);
  for(int i=0;i<v.count;i++){
   auto&p=v.partial[size_t(i)];const int harmonic=i+1;
   const double shape=std::sin(pi*harmonic*fret);
   // Finite contact impulse mapped to a mode-velocity increment.
   p.im+=direction*strength*shape/std::sqrt(double(harmonic));
  }
  (mode==Transition::HammerOn?hammers_:pulls_)++;
  legatos_++;
 }
 v.note=note;v.targetMidi=note;v.held=true;
}
void Instrument::hammerOn(int midi,float intensity) noexcept{setNextTransition(Transition::HammerOn,intensity);noteOn(midi,1.f);}
void Instrument::pullOff(int midi,float intensity) noexcept{setNextTransition(Transition::PullOff,intensity);noteOn(midi,1.f);}
void Instrument::slideTo(int midi,float friction) noexcept{setNextTransition(Transition::Slide,friction);noteOn(midi,1.f);}
void Instrument::palmMute(float depth) noexcept{if(std::isfinite(depth))palmDepth_=clamp(depth,0.,1.);}
void Instrument::setPalmPosition(float x) noexcept{if(std::isfinite(x))palmX_=clamp(x,.7,.99);}
void Instrument::noteOn(int note,float velocity) noexcept{
 if(note<0||note>127||!finite(velocity)||velocity<=0)return;
 held_[size_t(note)]=true;order_[size_t(note)]=++eventCounter_;
 if(config_.monoLegato){
  auto& v=voice_[0];
  const auto mode=pending_;const float intensity=pendingIntensity_;
  pending_=Transition::Legato;pendingIntensity_=1.f;
  if(v.active&&v.held){articulate(v,note,intensity,mode);return;}
  strike(v,note,velocity);return;
 }
 Voice*target=nullptr;
 for(auto&v:voice_)if(v.active&&v.note==note){target=&v;break;}
 if(!target)for(auto&v:voice_)if(!v.active){target=&v;break;}
 if(!target)target=&*std::min_element(voice_.begin(),voice_.end(),[](const Voice&a,const Voice&b){return a.level<b.level;});
 strike(*target,note,velocity);
}
void Instrument::noteOff(int note) noexcept{
 if(note<0||note>127)return;
 held_[size_t(note)]=false;
 if(config_.monoLegato){
  auto&v=voice_[0];if(!v.active||v.note!=note)return;
  int next=-1;unsigned long long best=0;
  for(int i=0;i<128;i++)if(held_[size_t(i)]&&order_[size_t(i)]>=best){best=order_[size_t(i)];next=i;}
  if(next>=0){v.note=next;v.targetMidi=next;v.held=true;legatos_++;}
  else if(!sustain_)v.held=false;
 }else for(auto&v:voice_)if(v.active&&v.note==note&&!sustain_)v.held=false;
}
void Instrument::pitchBend(float semitones) noexcept{if(finite(semitones))pitchBend_=clamp(semitones,-24.,24.);}
void Instrument::channelPressure(float pressure) noexcept{if(finite(pressure))globalPressure_=clamp(pressure,0.,1.);}
void Instrument::polyPressure(int midi,float pressure) noexcept{
 if(midi>=0&&midi<128&&finite(pressure))pressure_[size_t(midi)]=clamp(pressure,0.,1.);
}
void Instrument::sustain(bool down) noexcept{
 sustain_=down;
 if(!down){for(auto&v:voice_)if(v.active&&!held_[size_t(std::max(0,v.note))])v.held=false;}
}
void Instrument::allNotesOff() noexcept{
 for(auto&v:voice_)v=Voice{};
 for(auto&v:held_)v=false;
 for(auto&v:pressure_)v=0.;
 bridge_=Bank{};body_=Bank{};
 // Retain prepared filter coefficients when stopping all notes.
 // Reprepare is unnecessary because filters below can zero internal memory.
 static constexpr double ratios[]={1.,.79,1.33};
 for(int i=0;i<3;i++){
  auto reload=[&](Bank& b,double f,double decay,double mix){
   const double r=std::exp(-decay/sr_),w=2*pi*f*ratios[i]/sr_;
   auto&p=b.modes[size_t(i)];p.c=2*r*std::cos(w);p.d=-r*r;p.scale=std::max(.00001,1-r);b.amount=mix;};
  reload(bridge_,config_.physical.bridgeHz,config_.physical.bridgeDecay,config_.physical.bridgeMix);
  reload(body_,config_.physical.bodyHz,config_.physical.bodyDecay,config_.physical.bodyMix);
 }
 globalPressure_=0.;sustain_=false;palmDepth_=palmSmooth_=0.;pending_=Transition::Legato;
}
double Instrument::bankTick(Bank&b,double input) noexcept{
 double response=0.,weight=0.;
 for(int i=0;i<3;i++){
  const double projection=config_.physical.excitation[size_t(i)]*config_.physical.pickup[size_t(i)];
  response+=projection*b.modes[size_t(i)].tick(input*config_.physical.excitation[size_t(i)]);
  weight+=std::abs(projection*config_.physical.excitation[size_t(i)]);
 }
 return (1.-b.amount)*input+b.amount*response/std::max(.02,weight);
}
float Instrument::processOne() noexcept{
 // All dynamic controls are bounded and smoothed; no allocation in audio callback.
 const double bendCoef=1.-std::exp(-1./(sr_*std::max(.0001,config_.bendSmoothingMs*.001)));
 smoothBend_+=bendCoef*(pitchBend_-smoothBend_);
 const double palmCoef=1.-std::exp(-1./(sr_*.0025));
 palmSmooth_+=palmCoef*(palmDepth_-palmSmooth_);
 double sum=0.;
 for(auto&v:voice_){
  if(!v.active)continue;
  const double glideCoef=1.-std::exp(-1./(sr_*std::max(.0001,config_.glideMs*.001)));
  const double previous=v.pitchMidi;
  v.pitchMidi+=glideCoef*(v.targetMidi-v.pitchMidi);
  const double actualSlide=std::abs(v.pitchMidi-previous);
  // Contact friction is driven by moving the fretted boundary, not an
  // unrelated noise oscillator. It vanishes when fret motion stops.
  const double slideActivity=v.slideFriction*std::min(1.,actualSlide*400.);
  if(std::abs(v.targetMidi-v.pitchMidi)<.003)v.slideFriction=0.;
  if((tick_&7u)==0)updateRotation(v);
  double x=0.,power=0.;
  const auto&m=config_.physical.string;
  const double pressure=std::max(globalPressure_,pressure_[size_t(std::max(0,v.note))]);
  for(int i=0;i<v.count;i++){
   auto&p=v.partial[size_t(i)];
   const double contactShape=std::sin(pi*(i+1)*palmX_);
   const double localizedPalmLoss=palmSmooth_*config_.palmDamping*contactShape*contactShape;
   const double damping=m.decayBase + m.decayHigh*i*i + pressure*config_.pressureContactDamping
     + localizedPalmLoss + (v.held?0.:config_.releaseContactDamping);
   const double r=std::exp(-damping/sr_);
   const double re=(p.re*p.rotR-p.im*p.rotI)*r;
   const double im=(p.im*p.rotR+p.re*p.rotI)*r;
   p.re=re;p.im=im;
   if(slideActivity>0.){
    v.rng=1664525u*v.rng+1013904223u;
    const double frictionNoise=(double((v.rng>>8)&0xffffu)/32767.5-1.);
    const double fret=clamp(1.-std::exp2(-std::max(0.,v.pitchMidi-40.)/12.),.06,.91);
    const double shape=std::sin(pi*(i+1)*fret);
    p.im+=slideActivity*config_.slideFriction*.00012*frictionNoise*shape;
   }
   x+=p.baseWeight*p.im;power+=p.baseWeight*p.baseWeight*(im*im+re*re);
  }
  // Exponential decay of vibratory energy; without a hard PCM mute at note off.
  const double onset=std::min(1.,v.age/(.0018*sr_));
  sum+=v.level*m.gain*x*onset;v.age+=1.;
  if(v.age>sr_*20.||power*v.level*v.level<1e-12)v.active=false;
 }
 tick_++;
 if(config_.physical.bridge)sum=bankTick(bridge_,sum);
 if(config_.physical.body)sum=bankTick(body_,sum);
 return float(clamp(sum,-2.,2.));
}
int Instrument::activeVoiceCount() const noexcept{
 int n=0;for(auto&v:voice_)n+=int(v.active);return n;
}
} // namespace panorama::articulated33
