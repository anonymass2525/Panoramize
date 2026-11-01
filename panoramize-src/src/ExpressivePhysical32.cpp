#include "ExpressivePhysical32.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
namespace panorama::expressive32 {
namespace {constexpr double pi=3.14159265358979323846;
 double clamp(double x,double low,double high){return std::clamp(x,low,high);}
 bool finite(double x){return std::isfinite(x);}
}
std::string serialize(const Model&m){
 std::ostringstream s;s<<std::setprecision(17)<<"PANORAMIZE-EXPRESSIVE32-v1\n";
 s<<"settings "<<int(m.monoLegato)<<' '<<m.glideMs<<' '<<m.bendSmoothingMs<<' '
  <<m.velocityPickRange<<' '<<m.pressureContactDamping<<' '<<m.releaseContactDamping<<' '<<m.velocityForceCurve<<'\n';
 s<<"PHYSICAL30_BEGIN\n"<<performance30::serialize(m.physical)<<"PHYSICAL30_END\n";
 return s.str();
}
bool deserialize(const std::string&source,Model&out){
 if(source.size()>20000)return false;
 std::istringstream f(source);std::string line;Model v;
 if(!std::getline(f,line)||line!="PANORAMIZE-EXPRESSIVE32-v1")return false;
 if(!std::getline(f,line))return false;
 std::istringstream row(line);std::string tag;int mono=0;std::string junk;
 if(!(row>>tag>>mono>>v.glideMs>>v.bendSmoothingMs>>v.velocityPickRange>>v.pressureContactDamping
   >>v.releaseContactDamping>>v.velocityForceCurve)||tag!="settings"||(mono!=0&&mono!=1)||(row>>junk))return false;
 v.monoLegato=mono!=0;
 if(!finite(v.glideMs)||v.glideMs<0||v.glideMs>3000 ||!finite(v.bendSmoothingMs)||v.bendSmoothingMs<0||v.bendSmoothingMs>1000||
    !finite(v.velocityPickRange)||v.velocityPickRange<0||v.velocityPickRange>.9||
    !finite(v.pressureContactDamping)||v.pressureContactDamping<0||v.pressureContactDamping>1000||
    !finite(v.releaseContactDamping)||v.releaseContactDamping<0||v.releaseContactDamping>1000||
    !finite(v.velocityForceCurve)||v.velocityForceCurve<.2||v.velocityForceCurve>5)return false;
 if(!std::getline(f,line)||line!="PHYSICAL30_BEGIN")return false;
 std::string physical;bool closed=false;
 while(std::getline(f,line)){
  if(line=="PHYSICAL30_END"){closed=true;break;}
  physical+=line+'\n';if(physical.size()>16000)return false;
 }
 if(!closed||!performance30::deserialize(physical,v.physical))return false;
 while(std::getline(f,line))if(!line.empty())return false;
 out=v;return true;
}
std::unique_ptr<Instrument> Instrument::prepare(const Model&m,double sr,std::string*error){
 Model validation;
 if(!finite(sr)||sr<8000||sr>192000||!deserialize(serialize(m),validation)){
  if(error)*error="invalid expressive model or sample rate";return {};
 }
 auto ins=std::unique_ptr<Instrument>(new Instrument());ins->config_=m;ins->sr_=sr;
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
void Instrument::noteOn(int note,float velocity) noexcept{
 if(note<0||note>127||!finite(velocity)||velocity<=0)return;
 held_[size_t(note)]=true;order_[size_t(note)]=++eventCounter_;
 if(config_.monoLegato){
  auto& v=voice_[0];
  if(v.active&&v.held){v.note=note;v.targetMidi=note;v.held=true;legatos_++;return;}
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
 globalPressure_=0.;sustain_=false;
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
 double sum=0.;
 for(auto&v:voice_){
  if(!v.active)continue;
  const double glideCoef=1.-std::exp(-1./(sr_*std::max(.0001,config_.glideMs*.001)));
  v.pitchMidi+=glideCoef*(v.targetMidi-v.pitchMidi);
  if((tick_&7u)==0)updateRotation(v);
  double x=0.,power=0.;
  const auto&m=config_.physical.string;
  const double pressure=std::max(globalPressure_,pressure_[size_t(std::max(0,v.note))]);
  for(int i=0;i<v.count;i++){
   auto&p=v.partial[size_t(i)];
   const double damping=m.decayBase + m.decayHigh*i*i + pressure*config_.pressureContactDamping
     + (v.held?0.:config_.releaseContactDamping);
   const double r=std::exp(-damping/sr_);
   const double re=(p.re*p.rotR-p.im*p.rotI)*r;
   const double im=(p.im*p.rotR+p.re*p.rotI)*r;
   p.re=re;p.im=im;
   x+=p.baseWeight*im;power+=p.baseWeight*p.baseWeight*(im*im+re*re);
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
} // namespace panorama::expressive32
