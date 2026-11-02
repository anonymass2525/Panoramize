#include "UnifiedAcousticPhysical39.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <utility>
namespace panorama::unified39 {
namespace {
bool between(double x,double a,double b){return std::isfinite(x)&&x>=a&&x<=b;}
bool validLink(const Transducer&t,const Recipe&r){
 return t.physicalDomain>=0&&t.physicalDomain<int(r.physics.domains.size())&&
 t.acousticMode>=0&&t.acousticMode<int(r.acoustic.modes.size())&&
 t.physicalSensor==Sensor::PhysicalVelocityMS&&t.acousticSensor==Sensor::AcousticNormalizedFlow&&
 t.acousticActuator==Actuator::NormalizedEffort&&t.physicalActuator==Actuator::MechanicalForceN&&
 between(t.physicalToEffort,0,1.e6)&&between(t.flowToForce,0,1000)&&
 between(t.effortLimit,1.e-6,1.e5)&&between(t.forceLimit,1.e-9,1000)&&
 (t.mechanicalToAcoustic||t.acousticToMechanical);
}
bool valid(const Recipe&r){
 if(!between(r.physicalMix,0,1)||!between(r.acousticMix,0,1)||r.links.size()>24)return false;
 for(auto&t:r.links)if(!validLink(t,r))return false;
 return true;
}
}
Recipe example(){
 Recipe r;
 coupled38::Domain string;string.family=gen37::Family::String1D;string.nx=20;
 string.fundamentalHz=150;string.damping=2.;string.amplitude=.00010;
 coupled38::Domain membrane=string;membrane.family=gen37::Family::Membrane2D;
 membrane.nx=12;membrane.ny=10;membrane.fundamentalHz=120.;membrane.damping=2.;
 coupled38::Domain body=string;body.family=gen37::Family::TetraResonator3D;
 body.shape.xCells=1;body.shape.yCells=1;body.shape.zCells=1;body.damping=3.;
 r.physics.domains={string,membrane,body};
 coupled38::Connection c;c.from=0;c.to=1;c.stiffness=12000;c.damping=1.;r.physics.connections.push_back(c);
 c.from=1;c.to=2;c.stiffness=8000.;c.damping=1.;r.physics.connections.push_back(c);
 r.physics.outputGain=3200.;
 r.acoustic=native27::example();
 // All six requested domains receive their own typed active transducer.
 for(int mode=0;mode<int(r.acoustic.modes.size());mode++){
  Transducer t;t.physicalDomain=mode%3;t.acousticMode=mode;
  t.physicalToEffort=1800.;t.flowToForce=.35;
  t.effortLimit=1800.;t.forceLimit=3.;
  r.links.push_back(t);
 }
 r.physicalMix=.8;r.acousticMix=.18;
 return r;
}
std::string serialize(const Recipe&r){
 std::ostringstream o;o<<std::setprecision(17)<<"PANORAMIZE-UNIFIED39|1\n";
 o<<"MIX "<<r.physicalMix<<' '<<r.acousticMix<<' '<<r.links.size()<<"\n";
 for(auto&t:r.links)o<<"PORT "<<t.physicalDomain<<' '<<t.acousticMode<<' '
  <<int(t.physicalSensor)<<' '<<int(t.acousticActuator)<<' '<<int(t.acousticSensor)<<' '<<int(t.physicalActuator)<<' '
  <<t.physicalToEffort<<' '<<t.flowToForce<<' '<<t.effortLimit<<' '<<t.forceLimit<<' '
  <<int(t.mechanicalToAcoustic)<<' '<<int(t.acousticToMechanical)<<"\n";
 o<<"PHYS38_BEGIN\n"<<coupled38::serialize(r.physics)<<"PHYS38_END\n";
 o<<"STATE27_BEGIN\n"<<native27::serialize(r.acoustic)<<"STATE27_END\n";
 return o.str();
}
bool deserialize(const std::string&src,Recipe&out,std::string*error){
 auto fail=[&](const char*reason){if(error)*error=reason;return false;};
 if(src.size()>60000)return fail("unified patch too large");
 std::istringstream in(src);std::string line,token;
 if(!std::getline(in,line)||line!="PANORAMIZE-UNIFIED39|1")return fail("unsupported unified patch version");
 if(!std::getline(in,line))return fail("missing mixer");
 Recipe tmp;int n=-1;char tail;
 if(std::sscanf(line.c_str(),"MIX %lf %lf %d %c",&tmp.physicalMix,&tmp.acousticMix,&n,&tail)!=3||n<0||n>24)return fail("invalid mixer");
 for(int i=0;i<n;i++){
  if(!std::getline(in,line))return fail("missing port");
  Transducer t;int s1,a1,s2,a2,d1,d2;
  if(std::sscanf(line.c_str(),"PORT %d %d %d %d %d %d %lf %lf %lf %lf %d %d %c",
   &t.physicalDomain,&t.acousticMode,&s1,&a1,&s2,&a2,&t.physicalToEffort,&t.flowToForce,
   &t.effortLimit,&t.forceLimit,&d1,&d2,&tail)!=12||d1<0||d1>1||d2<0||d2>1)return fail("invalid port");
  t.physicalSensor=Sensor(s1);t.acousticActuator=Actuator(a1);t.acousticSensor=Sensor(s2);
  t.physicalActuator=Actuator(a2);t.mechanicalToAcoustic=bool(d1);t.acousticToMechanical=bool(d2);
  tmp.links.push_back(t);
 }
 auto block=[&](const std::string&begin,const std::string&end,std::string&target){
  if(!std::getline(in,line)||line!=begin)return false;
  bool found=false;
  while(std::getline(in,line)){
   if(line==end){found=true;break;}
   target+=line+'\n';if(target.size()>50000)return false;
  }
  return found;
 };
 std::string phys,state;
 if(!block("PHYS38_BEGIN","PHYS38_END",phys)||!block("STATE27_BEGIN","STATE27_END",state))return fail("bad nested block");
 if(std::getline(in,line))return fail("unexpected trailing records");
 if(!coupled38::deserialize(phys,tmp.physics,error))return false;
 if(!native27::deserialize(state,tmp.acoustic,error))return false;
 if(!valid(tmp))return fail("incompatible units, range or topology");
 out=std::move(tmp);if(error)error->clear();return true;
}
std::unique_ptr<Instrument> Instrument::prepare(const Recipe&r,int sr,std::string*error){
 auto fail=[&](const char*m)->std::unique_ptr<Instrument>{if(error)*error=m;return {};};
 if(sr<22050||sr>192000||!valid(r))return fail("invalid typed transducer graph");
 auto p=coupled38::Instrument::prepare(r.physics,sr,error);if(!p)return {};
 auto a=native27::Instrument::prepare(r.acoustic,sr,error);if(!a)return {};
 auto x=std::unique_ptr<Instrument>(new Instrument());
 x->recipe_=r;x->physical_=std::move(p);x->acoustic_=std::move(a);x->physicalMix_=r.physicalMix;x->acousticMix_=r.acousticMix;
 if(error)error->clear();return x;
}
void Instrument::setOutputMix(double physical,double acoustic)noexcept{
 if(std::isfinite(physical)&&std::isfinite(acoustic)&&physical>=0&&physical<=1&&acoustic>=0&&acoustic<=1){physicalMix_=physical;acousticMix_=acoustic;}
}
void Instrument::noteOn(int midi,float velocity) noexcept{
 if(midi<0||midi>127||!std::isfinite(velocity)||velocity<=0)return;
 currentMidi_=midi;acoustic_->noteOn(midi,velocity);
 // The mechanical graph's already-initialized structure is never reset on noteOn.
 // Add a small, *bounded* displacement impulse for new keys.
 const int d=recipe_.physics.exciteDomain;
 physical_->excite(d,std::min(.005,recipe_.physics.domains[size_t(d)].amplitude*double(std::clamp(velocity,0.f,1.f))));
}
void Instrument::noteOff(int midi) noexcept{acoustic_->noteOff(midi);if(midi==currentMidi_)currentMidi_=-1;}
void Instrument::allNotesOff() noexcept{acoustic_->allNotesOff();currentMidi_=-1;}
void Instrument::pitchBend(float value) noexcept{acoustic_->pitchBend(value);}
void Instrument::modWheel(float value) noexcept{acoustic_->modWheel(value);}
bool Instrument::setNativeModeNormalized(int mode,bool excitation,double normalized) noexcept{
 return acoustic_->setLiveModeNormalized(mode,excitation,normalized);
}
void Instrument::clearNativeModeAutomation() noexcept{acoustic_->clearLiveModeControls();}
float Instrument::processOne() noexcept{
 // Both sides use their previous completed sample state as sensor input.
 // Delayed active force feedback avoids unsolved algebraic loops, but it does NOT
 // imply that the inter-domain transducer is passive or physically calibrated.
 for(const auto&t:recipe_.links){
  if(t.mechanicalToAcoustic&&currentMidi_>=0){
   double raw=physical_->domainVelocity(t.physicalDomain)*t.physicalToEffort;
   double effort=std::clamp(raw,-t.effortLimit,t.effortLimit);
   if(raw!=effort)limited_++;
   maxEffort_=std::max(maxEffort_,std::abs(effort));
   acoustic_->addExternalEffort(currentMidi_,t.acousticMode,effort);
  }
  if(t.acousticToMechanical&&currentMidi_>=0){
   double raw=acoustic_->modeFlow(currentMidi_,t.acousticMode)*t.flowToForce;
   double force=std::clamp(raw,-t.forceLimit,t.forceLimit);
   if(raw!=force)limited_++;
   maxForce_=std::max(maxForce_,std::abs(force));
   physical_->addExternalForce(t.physicalDomain,force);
  }
 }
 const double p=physical_->processOne();
 const double a=acoustic_->processMono();
 const double mixed=physicalMix_*p+acousticMix_*a;
 return std::isfinite(mixed)?float(std::tanh(mixed)):0.f;
}
double Instrument::physicalEnergy() const noexcept{return physical_->totalEnergy();}
double Instrument::physicalExternalWork() const noexcept{return physical_->externalWork();}
double Instrument::acousticActiveWork() const noexcept{return acoustic_->activeWork();}
double Instrument::energyResidual() const noexcept{return acoustic_->energyResidual();}
} // namespace panorama::unified39
