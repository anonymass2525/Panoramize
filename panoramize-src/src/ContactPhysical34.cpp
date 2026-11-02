#include "ContactPhysical34.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
namespace panorama::contact34 {
namespace {
constexpr double pi=3.14159265358979323846;
inline bool ok(double v,double lo,double hi){return std::isfinite(v)&&v>=lo&&v<=hi;}
inline double clip(double x,double lo,double hi){return std::clamp(x,lo,hi);}
constexpr double kRmsProtect=1.e-18;
}
std::string serialize(const Model&m){
 std::ostringstream ss;ss<<std::setprecision(17)<<"PANORAMIZE-CONTACT34-v1\n";
 ss<<"string "<<m.openHz<<' '<<m.openMidi<<' '<<m.stringLoss<<' '<<m.fingerStiffness<<' '<<m.fingerDamping<<' '\
 <<m.fretGap<<' '<<m.fretStiffness<<' '<<m.fretDamping<<'\n';
 ss<<"body "<<m.bridgeHz<<' '<<m.bridgeMass<<' '<<m.bridgeLoss<<' '<<m.bodyHzX<<' '<<m.bodyHzY<<' '\
 <<m.bodyHzZ<<' '<<m.bodyMass<<' '<<m.bodyLoss<<' '<<m.bodyCoupling<<'\n';
 ss<<"play "<<m.pickPosition<<' '<<m.palmPosition<<' '<<m.palmLoss<<' '<<m.slideFriction<<' '\
 <<m.outputGain<<' '<<m.glideMs<<'\n';return ss.str();
}
bool deserialize(const std::string&src,Model&dst){
 if(src.size()>4096)return false;
 std::istringstream input(src);std::string hdr,line,extra;Model m;
 if(!std::getline(input,hdr)||hdr!="PANORAMIZE-CONTACT34-v1")return false;
 if(!std::getline(input,line))return false;
 {std::istringstream s(line);std::string name;
  if(!(s>>name>>m.openHz>>m.openMidi>>m.stringLoss>>m.fingerStiffness>>m.fingerDamping>>m.fretGap>>m.fretStiffness>>m.fretDamping)||name!="string"||(s>>extra))return false;}
 if(!std::getline(input,line))return false;
 {std::istringstream s(line);std::string name;
  if(!(s>>name>>m.bridgeHz>>m.bridgeMass>>m.bridgeLoss>>m.bodyHzX>>m.bodyHzY>>m.bodyHzZ>>m.bodyMass>>m.bodyLoss>>m.bodyCoupling)||name!="body"||(s>>extra))return false;}
 if(!std::getline(input,line))return false;
 {std::istringstream s(line);std::string name;
  if(!(s>>name>>m.pickPosition>>m.palmPosition>>m.palmLoss>>m.slideFriction>>m.outputGain>>m.glideMs)||name!="play"||(s>>extra))return false;}
 while(std::getline(input,line))if(!line.empty())return false;
 if(!ok(m.openHz,35,400)||m.openMidi<20||m.openMidi>76||!ok(m.stringLoss,0,200)||!ok(m.fingerStiffness,.1,12)||
  !ok(m.fingerDamping,0,1200)||!ok(m.fretGap,.000001,.01)||!ok(m.fretStiffness,.01,2)||!ok(m.fretDamping,0,500)||
  !ok(m.bridgeHz,30,1100)||!ok(m.bridgeMass,1,20)||!ok(m.bridgeLoss,0,300)||
  !ok(m.bodyHzX,20,800)||!ok(m.bodyHzY,20,800)||!ok(m.bodyHzZ,20,800)||
  !ok(m.bodyMass,1,30)||!ok(m.bodyLoss,0,300)||!ok(m.bodyCoupling,0,1)||
  !ok(m.pickPosition,.05,.95)||!ok(m.palmPosition,.75,.995)||!ok(m.palmLoss,0,1000)||
  !ok(m.slideFriction,0,1)||!ok(m.outputGain,.01,20)||!ok(m.glideMs,2,1000))return false;
 dst=m;return true;
}
std::unique_ptr<Instrument> Instrument::prepare(const Model&m,double sr,std::string*error){
 Model check;
 if(!std::isfinite(sr)||sr<22050||sr>192000||!deserialize(serialize(m),check)){
  if(error)*error="invalid spatial string configuration (requires 22.05kHz..192kHz)";return {};}
 auto ins=std::unique_ptr<Instrument>(new Instrument);
 ins->m_=m;ins->sr_=sr;ins->stiffness_=std::pow(2*m.openHz*(kNodes+1),2);
 ins->fretK_=m.fretStiffness*ins->stiffness_;
 ins->fingerK_=m.fingerStiffness*ins->stiffness_;
 ins->palmX_=m.palmPosition;
 // Stable for the strongest local string contact springs (velocity-Verlet).
 const double omega=std::sqrt(4*ins->stiffness_+ins->fingerK_+ins->fretK_);
 ins->steps_=std::clamp(int(std::ceil(omega/sr*1.7)),3,32);
 if(omega/(sr*ins->steps_)>1.0){if(error)*error="unstable contact stiffness at this sample rate";return {};}
 ins->bridgeK_=m.bridgeMass*std::pow(2*pi*m.bridgeHz,2);
 const double hz[]={m.bodyHzX,m.bodyHzY,m.bodyHzZ};const double axis[]={.82,.43,.27};
 for(int j=0;j<3;j++){
  ins->bodyK_[size_t(j)]=m.bodyMass*std::pow(2*pi*hz[j],2);
  ins->couplingK_[size_t(j)]=m.bodyCoupling*ins->bridgeK_*axis[j];
 }
 ins->halfStep_=1./(sr*ins->steps_)*.5;
 ins->dampBase_=std::exp(-m.stringLoss*ins->halfStep_);
 ins->dampRelease_=std::exp(-(m.stringLoss+18.)*ins->halfStep_);
 ins->dampFret_=std::exp(-m.fretDamping*ins->halfStep_);
 ins->dampBridge_=std::exp(-m.bridgeLoss*ins->halfStep_);
 ins->dampBody_=std::exp(-m.bodyLoss*ins->halfStep_);
 ins->glideCoef_=1.-std::exp(-1./(sr*m.glideMs*.001));
 if(error)error->clear();return ins;
}
double Instrument::location(int midi,int openMidi,double bend) noexcept{
 const double semitone=clip(double(midi-openMidi)+bend,0.,33.);
 // Open string: contact point at nut (zero); octave: midpoint; moving clamp = changing speaking length.
 return clip(1.-std::exp2(-semitone/12.),0.,.855);
}
void Instrument::nextGesture(Gesture g,float strength) noexcept{
 gesture_=g;if(std::isfinite(strength))gestureForce_=float(clip(strength,0.,1.));
}
void Instrument::gestureIntensity(float value) noexcept{if(std::isfinite(value))gestureForce_=float(clip(value,0.,1.));}
void Instrument::strike(float velocity) noexcept{
 const double force=std::pow(clip(velocity,0.,1.),1.25)*.00028;
 lastPickX_=clip(m_.pickPosition+(.5-velocity)*.09,.05,.95);
 for(int i=0;i<kNodes;i++){
  const double t=double(i+1)/(kNodes+1),p=lastPickX_;
  x_[size_t(i)]+=force*(t<=p?t/p:(1.-t)/(1.-p));
 }
 attacks_++;
}
void Instrument::noteOn(int midi,float velocity) noexcept{
 if(midi<0||midi>127||!std::isfinite(velocity)||velocity<=0)return;
 const double to=location(midi,m_.openMidi,bend_);
 if(!active_){active_=true;held_=true;note_=midi;fingerX_=to;targetX_=to;strike(velocity);}
 else {
  const int previous=note_;
  // Existing string state is preserved. Hammer/pull introduces local transverse impulse.
  if(gesture_==Gesture::Pluck)strike(velocity);
  else if(gesture_==Gesture::Hammer||gesture_==Gesture::Pull){
   const double sign=gesture_==Gesture::Hammer?1.:-1.;
   const double p=clip(to,.02,.85);
   for(int i=0;i<kNodes;i++){
    const double x=double(i+1)/(kNodes+1),w=std::exp(-std::pow((x-p)*double(kNodes+1)*.5,2));
    v_[size_t(i)]+=sign*w*gestureForce_*velocity*.095;
   }
  }
  else if(gesture_==Gesture::Slide) {slideIntensity_=gestureForce_; /* moving contact friction applies in each substep */ }
  (void)previous;
  held_=true;note_=midi;targetX_=to;
 }
 gesture_=Gesture::Pluck;gestureForce_=1.f;
}
void Instrument::noteOff(int note) noexcept{if(note==note_)held_=false;}
void Instrument::pitchBend(float semitones) noexcept{
 if(!std::isfinite(semitones))return;
 bend_=clip(semitones,-2.,2.);if(note_>=0)targetX_=location(note_,m_.openMidi,bend_);
}
void Instrument::channelPressure(float amount) noexcept{if(std::isfinite(amount))pressure_=clip(amount,0.,1.);}
void Instrument::polyPressure(int midi,float amount) noexcept{if(midi==note_)channelPressure(amount);}
void Instrument::palmMute(float amount) noexcept{if(std::isfinite(amount))palm_=clip(amount,0.,1.);}
void Instrument::palmPosition(float locationValue) noexcept{if(std::isfinite(locationValue))palmX_=clip(locationValue,.75,.995);}
void Instrument::allNotesOff() noexcept{
 active_=false;held_=false;note_=-1;fingerX_=targetX_=bridge_=bv_=0;slideIntensity_=0;
 x_.fill(0);v_.fill(0);bodyX_.fill(0);bodyV_.fill(0);
}
double Instrument::signedNoise() noexcept{
 seed_^=seed_<<13;seed_^=seed_>>17;seed_^=seed_<<5;
 return (double(seed_&65535u)/32767.5)-1.;
}
void Instrument::forces(std::array<double,kNodes>& f,double&fb,std::array<double,3>&body) const noexcept{
 f.fill(0);body.fill(0);fb=0;
 const double k=stiffness_;
 // Fixed nut, moving bridge boundary: this is reciprocal string/bridge force coupling.
 for(int i=0;i<=kNodes;i++){
  const double left=i==0?0:x_[size_t(i-1)];
  const double right=i==kNodes?bridge_:x_[size_t(i)];
  const double tension=k*(right-left);
  if(i>0)f[size_t(i-1)]+=tension;
  if(i<kNodes)f[size_t(i)]-=tension;
  else fb-=tension;
 }
 // Moving bilateral finger clamp, spatial kernel to neighbouring grid cells.
 const double fp=held_?fingerX_:0.;
 if(fp>.01){
  for(int i=0;i<kNodes;i++){
   const double dx=std::abs(double(i+1)/(kNodes+1)-fp)*(kNodes+1);
   const double w=std::max(0.,1.-dx);
   if(w>0)f[size_t(i)]-=fingerK_*(1.+.7*pressure_)*w*x_[size_t(i)];
  }
 }
 // Unilateral fretboard obstacle, only when string moves below fret height.
 for(int i=0;i<kNodes;i++){
  if(x_[size_t(i)]< -m_.fretGap)f[size_t(i)]-=fretK_*(x_[size_t(i)]+m_.fretGap);
 }
 // 3-axis bridge/body, with equal and opposite forces and a genuine elastic potential.
 fb-=bridgeK_*bridge_;
 for(int j=0;j<3;j++){
  const double ks=bodyK_[size_t(j)];
  const double kc=couplingK_[size_t(j)];
  const double d=bridge_-bodyX_[size_t(j)];
  fb-=kc*d;body[size_t(j)]+=kc*d-ks*bodyX_[size_t(j)];
 }
}
void Instrument::integrate(double dt) noexcept{
 // Damping split around velocity-Verlet conservative step. No allocations or locks.
 const auto damp=[&](double half){
  const double baseline=held_?dampBase_:dampRelease_;
  const double fretFactor=dampFret_;
  const double bridgeFactor=dampBridge_;
  const double bodyFactor=dampBody_;
  const double fingerStrength=held_?m_.fingerDamping*(.45+pressure_):0.;
  const double palmStrength=m_.palmLoss*palm_;
  const double sliding=held_&&std::abs(targetX_-fingerX_)>.0001?m_.slideFriction*slideIntensity_*20:0.;
  const double fp=held_?fingerX_:0.;
  for(int i=0;i<kNodes;i++){
   const double t=double(i+1)/(kNodes+1);
   const double weight=fp>.01?std::max(0.,1.-std::abs(t-fp)*(kNodes+1)):0.;
   const double pweight=std::max(0.,1.-std::abs(t-palmX_)*(kNodes+1)*.5);
   double factor=baseline;
   if(weight>0 && (fingerStrength>0||sliding>0))factor*=std::exp(-(fingerStrength+sliding)*weight*half);
   if(pweight>0 && palmStrength>0)factor*=std::exp(-palmStrength*pweight*half);
   if(x_[size_t(i)] < -m_.fretGap && v_[size_t(i)]<0)factor*=fretFactor;
   const double before=v_[size_t(i)];v_[size_t(i)]*=factor;
   lossEnergy_+=.5*(before*before-v_[size_t(i)]*v_[size_t(i)]);
  }
  const double old=bv_;bv_*=bridgeFactor;lossEnergy_+=.5*m_.bridgeMass*(old*old-bv_*bv_);
  for(int j=0;j<3;j++){
   const double v=bodyV_[size_t(j)];bodyV_[size_t(j)]*=bodyFactor;
   lossEnergy_+=.5*m_.bodyMass*(v*v-bodyV_[size_t(j)]*bodyV_[size_t(j)]);
  }
 };
 damp(dt*.5);
 std::array<double,kNodes> f;std::array<double,3> b;double fb=0;
 forces(f,fb,b);
 for(int i=0;i<kNodes;i++){v_[size_t(i)]+=f[size_t(i)]*dt*.5;x_[size_t(i)]+=v_[size_t(i)]*dt;}
 bv_+=fb*dt*.5/m_.bridgeMass;bridge_+=bv_*dt;
 for(int j=0;j<3;j++){bodyV_[size_t(j)]+=b[size_t(j)]*dt*.5/m_.bodyMass;bodyX_[size_t(j)]+=bodyV_[size_t(j)]*dt;}
 forces(f,fb,b);
 for(int i=0;i<kNodes;i++)v_[size_t(i)]+=f[size_t(i)]*dt*.5;
 bv_+=fb*dt*.5/m_.bridgeMass;
 for(int j=0;j<3;j++)bodyV_[size_t(j)]+=b[size_t(j)]*dt*.5/m_.bodyMass;
 damp(dt*.5);
 // Finger slide friction generates energy from the performing hand: active work, not conservative transfer.
 if(held_&&std::abs(targetX_-fingerX_)>.00012){
  const int ix=std::clamp(int(fingerX_*(kNodes+1))-1,0,kNodes-1);
  v_[size_t(ix)]+=signedNoise()*m_.slideFriction*slideIntensity_*std::abs(targetX_-fingerX_)*dt*80.;
 }
 if(active_){
  for(int i=0;i<kNodes;i++)if(x_[size_t(i)]< -m_.fretGap)fretContacts_++;
 }
}
float Instrument::processOne() noexcept{
 if(!active_)return 0.f;
 fingerX_+=(targetX_-fingerX_)*glideCoef_;
 if(std::abs(targetX_-fingerX_)<.0001)slideIntensity_*=.995;
 const double dt=1./(sr_*steps_);
 for(int s=0;s<steps_;s++)integrate(dt);
 const double pickup=v_[size_t(kNodes*3/4)];
 const double value=(pickup*.52+bv_*.20+bodyV_[0]*.36+bodyV_[1]*.18+bodyV_[2]*.10)*m_.outputGain;
 const double safe=std::isfinite(value)?std::tanh(value):0.;
 return float(safe);
}
double Instrument::energy() const noexcept{
 double E=.5*m_.bridgeMass*bv_*bv_;
 for(int i=0;i<kNodes;i++)E+=.5*v_[size_t(i)]*v_[size_t(i)];
 for(int j=0;j<3;j++)E+=.5*m_.bodyMass*bodyV_[size_t(j)]*bodyV_[size_t(j)];
 double prev=0;
 for(int i=0;i<kNodes;i++){
  const double d=x_[size_t(i)]-prev;E+=.5*stiffness_*d*d;prev=x_[size_t(i)];
 }
 E+=.5*stiffness_*std::pow(bridge_-prev,2);
 E+=.5*bridgeK_*bridge_*bridge_;
 for(int j=0;j<3;j++){
  E+=.5*bodyK_[size_t(j)]*bodyX_[size_t(j)]*bodyX_[size_t(j)];
  E+=.5*couplingK_[size_t(j)]*std::pow(bridge_-bodyX_[size_t(j)],2);
 }
 if(held_&&fingerX_>.01)for(int i=0;i<kNodes;i++){
  const double w=std::max(0.,1.-std::abs(double(i+1)/(kNodes+1)-fingerX_)*(kNodes+1));
  E+=.5*fingerK_*(1.+.7*pressure_)*w*x_[size_t(i)]*x_[size_t(i)];
 }
 for(int i=0;i<kNodes;i++)if(x_[size_t(i)]< -m_.fretGap)
  E+=.5*fretK_*std::pow(x_[size_t(i)]+m_.fretGap,2);
 return E;
}
} // namespace panorama::contact34
