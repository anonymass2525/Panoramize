#include "EnergyPorts40.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
namespace panorama::energy40 {
namespace {
bool finite(double x,double a,double b){return std::isfinite(x)&&x>=a&&x<=b;}
const double twopi=6.2831853071795864769;
}
Recipe example(){
 Recipe r;Node string;string.frequency=110;string.mass=.025;string.damping=.04;string.drive=1.5;string.output=20.;string.x=.13;string.y=.35;
 Node plate=string;plate.frequency=185;plate.mass=.07;plate.damping=.12;plate.drive=0;plate.output=20.;plate.x=.51;plate.y=.28;
 Node air=string;air.domain=Domain::Acoustic;air.mass=10000.;air.frequency=132.;air.damping=8000.;air.area=.04;air.output=20.;air.drive=0;air.x=.81;air.y=.7;
 r.nodes={string,plate,air};
 Link l;l.from=0;l.to=1;l.kind=LinkKind::Combined;l.stiffness=170.;l.damping=.08;r.links.push_back(l);
 l.from=1;l.to=2;l.kind=LinkKind::Combined;l.stiffness=80.;l.damping=.02;r.links.push_back(l);
 return r;
}
bool valid(const Recipe&r,std::string*err){
 auto fail=[&](const char*s){if(err)*err=s;return false;};
 if(r.nodes.empty()||r.nodes.size()>12||r.links.size()>24||r.exciteNode<0||r.exciteNode>=int(r.nodes.size())||!finite(r.masterGain,0,20)||!finite(r.smoothingMs,1.,1000.))return fail("invalid node count or master/smoothing");
 for(auto&n:r.nodes){
  if((n.domain!=Domain::Mechanical&&n.domain!=Domain::Acoustic)||!finite(n.mass,1e-8,1e10)||!finite(n.frequency,1.,5000.)||!finite(n.damping,0,1e8)||!finite(n.area,1e-4,10.)||!finite(n.output,-1000,1000)||!finite(n.drive,0,1e6)||!finite(n.x,0,1)||!finite(n.y,0,1))return fail("invalid typed SI node");
 }
 for(auto&l:r.links){if(l.from<0||l.to<0||l.from>=int(r.nodes.size())||l.to>=int(r.nodes.size())||l.from==l.to||(l.kind!=LinkKind::Conservative&&l.kind!=LinkKind::Dissipative&&l.kind!=LinkKind::Combined)||!finite(l.stiffness,0,1e8)||!finite(l.damping,0,1e6))return fail("invalid passive link");}
 if(err)err->clear();return true;
}
std::string serialize(const Recipe&r){
 std::ostringstream s;s<<std::setprecision(17)<<"PANORAMIZE-ENERGY40|1\n";
 s<<"SET "<<r.nodes.size()<<' '<<r.links.size()<<' '<<r.exciteNode<<' '<<r.masterGain<<' '<<r.smoothingMs<<'\n';
 for(auto&n:r.nodes)s<<"N "<<int(n.domain)<<' '<<n.mass<<' '<<n.frequency<<' '<<n.damping<<' '<<n.area<<' '<<n.output<<' '<<n.drive<<' '<<n.x<<' '<<n.y<<'\n';
 for(auto&l:r.links)s<<"L "<<l.from<<' '<<l.to<<' '<<int(l.kind)<<' '<<l.stiffness<<' '<<l.damping<<'\n';
 return s.str();
}
bool deserialize(const std::string&text,Recipe&out,std::string*err){
 auto fail=[&](const char*s){if(err)*err=s;return false;};
 if(text.size()>12000)return fail("energy patch too long");
 std::istringstream in(text);std::string line;
 if(!std::getline(in,line)||line!="PANORAMIZE-ENERGY40|1")return fail("wrong energy header");
 if(!std::getline(in,line))return fail("missing setup");
 Recipe r;int nn=-1,nl=-1;std::istringstream set(line);std::string tok;
 if(!(set>>tok>>nn>>nl>>r.exciteNode>>r.masterGain>>r.smoothingMs)||tok!="SET"||nn<1||nn>12||nl<0||nl>24||(set>>tok))return fail("bad setup");
 for(int i=0;i<nn;i++){
  if(!std::getline(in,line))return fail("missing node");std::istringstream row(line);Node n;int domain=-1;
  if(!(row>>tok>>domain>>n.mass>>n.frequency>>n.damping>>n.area>>n.output>>n.drive>>n.x>>n.y)||tok!="N"||(row>>tok))return fail("bad node");
  n.domain=Domain(domain);r.nodes.push_back(n);
 }
 for(int i=0;i<nl;i++){
  if(!std::getline(in,line))return fail("missing link");std::istringstream row(line);Link l;int kind=-1;
  if(!(row>>tok>>l.from>>l.to>>kind>>l.stiffness>>l.damping)||tok!="L"||(row>>tok))return fail("bad link");
  l.kind=LinkKind(kind);r.links.push_back(l);
 }
 if(std::getline(in,line))return fail("trailing data");
 if(!valid(r,err))return false;out=std::move(r);return true;
}
std::unique_ptr<Instrument> Instrument::prepare(const Recipe&r,double sampleRate,std::string*err){
 if(!valid(r,err)||!finite(sampleRate,22050,192000)){if(err&&err->empty())*err="invalid sample rate";return {};}
 auto x=std::unique_ptr<Instrument>(new Instrument());x->recipe_=r;x->n_=int(r.nodes.size());x->dt_=1./sampleRate;
 x->alpha_=1.-std::exp(-1./(sampleRate*r.smoothingMs*.001));
 for(int i=0;i<x->n_;i++){
  const auto&n=r.nodes[size_t(i)];x->diagMass_[size_t(i)]=n.mass;x->nodeK_[size_t(i)]=n.mass*std::pow(twopi*n.frequency,2);
  x->targetK_[size_t(i)].store(x->nodeK_[size_t(i)]);x->baseFrequency_[size_t(i)].store(n.frequency);
  x->nodeD_[size_t(i)]=n.damping;x->targetD_[size_t(i)].store(n.damping);
  x->transform_[size_t(i)]=n.domain==Domain::Acoustic?1./n.area:1.;
 }
 for(size_t j=0;j<r.links.size();j++){const auto&l=r.links[j];
  x->linkK_[j]=l.kind==LinkKind::Dissipative?0:l.stiffness;
  x->targetLK_[j].store(x->linkK_[j]);
  x->linkD_[j]=l.kind==LinkKind::Conservative?0:l.damping;
  x->targetLD_[j].store(x->linkD_[j]);
 }
 x->initialEnergy_=x->energy();if(err)err->clear();return x;
}
void Instrument::assemble(std::array<double,MaxN*MaxN>&K,std::array<double,MaxN*MaxN>&D)const noexcept{
 K.fill(0);D.fill(0);
 for(int i=0;i<n_;i++){K[size_t(i*MaxN+i)]=nodeK_[size_t(i)];D[size_t(i*MaxN+i)]=nodeD_[size_t(i)];}
 for(size_t j=0;j<recipe_.links.size();j++){
  const auto&l=recipe_.links[j];int a=l.from,b=l.to;
  const double c=transform_[size_t(a)],d=transform_[size_t(b)];
  const double k=linkK_[j],z=linkD_[j];
  K[size_t(a*MaxN+a)]+=k*c*c;K[size_t(b*MaxN+b)]+=k*d*d;
  K[size_t(a*MaxN+b)]-=k*c*d;K[size_t(b*MaxN+a)]-=k*c*d;
  D[size_t(a*MaxN+a)]+=z*c*c;D[size_t(b*MaxN+b)]+=z*d*d;
  D[size_t(a*MaxN+b)]-=z*c*d;D[size_t(b*MaxN+a)]-=z*c*d;
 }
}
bool Instrument::solve(const std::array<double,MaxN*MaxN>&a,const std::array<double,MaxN>&b)noexcept{
 chol_.fill(0);
 for(int i=0;i<n_;i++)for(int j=0;j<=i;j++){
  double v=a[size_t(i*MaxN+j)];for(int k=0;k<j;k++)v-=chol_[size_t(i*MaxN+k)]*chol_[size_t(j*MaxN+k)];
  if(i==j){if(!(v>0)||!std::isfinite(v))return false;chol_[size_t(i*MaxN+j)]=std::sqrt(v);}
  else chol_[size_t(i*MaxN+j)]=v/chol_[size_t(j*MaxN+j)];
 }
 for(int i=0;i<n_;i++){
  double v=b[size_t(i)];for(int j=0;j<i;j++)v-=chol_[size_t(i*MaxN+j)]*vmid_[size_t(j)];
  vmid_[size_t(i)]=v/chol_[size_t(i*MaxN+i)];
 }
 for(int i=n_-1;i>=0;i--){
  double v=vmid_[size_t(i)];for(int j=i+1;j<n_;j++)v-=chol_[size_t(j*MaxN+i)]*vmid_[size_t(j)];
  vmid_[size_t(i)]=v/chol_[size_t(i*MaxN+i)];
 }
 return true;
}
void Instrument::noteOn(int midi,float velocity)noexcept{
 if(midi<0||midi>127||!std::isfinite(velocity)||velocity<=0)return;
 midi_.store(midi,std::memory_order_relaxed);setNoteFrequencyTargets();pulse_=std::clamp(double(velocity),0.,1.)*1.5;
}
void Instrument::noteOff(int midi)noexcept{if(midi==midi_.load(std::memory_order_relaxed))midi_.store(-1,std::memory_order_relaxed);}
void Instrument::allNotesOff()noexcept{midi_.store(-1,std::memory_order_relaxed);pulse_=0;}
void Instrument::setNoteFrequencyTargets() noexcept{
 const int midi=midi_.load(std::memory_order_relaxed);
 if(midi<0)return;
 const double ratio=std::pow(2.,(double(midi-57)+bendSemitones_.load(std::memory_order_relaxed))/12.);
 for(int i=0;i<n_;i++){
  double freq=std::clamp(baseFrequency_[size_t(i)].load(std::memory_order_relaxed)*ratio,1.,5000.);
  targetK_[size_t(i)].store(diagMass_[size_t(i)]*std::pow(twopi*freq,2),std::memory_order_relaxed);
 }
}
void Instrument::pitchBend(float semitones)noexcept{
 if(std::isfinite(semitones)){
  bendSemitones_.store(std::clamp(double(semitones),-24.,24.),std::memory_order_relaxed);
  setNoteFrequencyTargets();
 }
}
bool Instrument::setLinkTarget(int j,double k,double d)noexcept{
 if(j<0||j>=int(recipe_.links.size())||!finite(k,0,1e8)||!finite(d,0,1e6))return false;
 const auto kind=recipe_.links[size_t(j)].kind;
 targetLK_[size_t(j)].store(kind==LinkKind::Dissipative?0:k,std::memory_order_relaxed);
 targetLD_[size_t(j)].store(kind==LinkKind::Conservative?0:d,std::memory_order_relaxed);return true;
}
bool Instrument::setNodeTarget(int i,double freq,double damping)noexcept{
 if(i<0||i>=n_||!finite(freq,1,5000)||!finite(damping,0,1e8))return false;
 baseFrequency_[size_t(i)].store(freq,std::memory_order_relaxed);
 const int midi=midi_.load(std::memory_order_relaxed);
 const double ratio=midi>=0?std::pow(2.,(double(midi-57)+bendSemitones_.load(std::memory_order_relaxed))/12.):1.;
 const double hz=std::clamp(freq*ratio,1.,5000.);
 targetK_[size_t(i)].store(diagMass_[size_t(i)]*std::pow(twopi*hz,2),std::memory_order_relaxed);
 targetD_[size_t(i)].store(damping,std::memory_order_relaxed);return true;
}
double Instrument::energy()const noexcept{
 double e=0;
 for(int i=0;i<n_;i++)e+=.5*diagMass_[size_t(i)]*v_[size_t(i)]*v_[size_t(i)]+.5*nodeK_[size_t(i)]*q_[size_t(i)]*q_[size_t(i)];
 for(size_t j=0;j<recipe_.links.size();j++){
  const auto&l=recipe_.links[j];double dis=transform_[size_t(l.from)]*q_[size_t(l.from)]-transform_[size_t(l.to)]*q_[size_t(l.to)];
  e+=.5*linkK_[j]*dis*dis;
 }return e;
}
double Instrument::nodeEnergy(int i)const noexcept{return i>=0&&i<n_?.5*diagMass_[size_t(i)]*v_[size_t(i)]*v_[size_t(i)]+.5*nodeK_[size_t(i)]*q_[size_t(i)]*q_[size_t(i)]:0;}
double Instrument::nodeVelocity(int i)const noexcept{return i>=0&&i<n_?v_[size_t(i)]:0;}
Ledger Instrument::ledger()const noexcept{
 Ledger l;l.initial=initialEnergy_;l.active=activeWork_;l.parameter=parameterWork_;l.dissipated=dissipated_;l.finalEnergy=energy();
 l.residual=l.finalEnergy-l.initial-l.active-l.parameter+l.dissipated;return l;
}
float Instrument::processOne()noexcept{
 // Separate energy injected by *changing* physical parameters (neither passive nor damping).
 const double before=energy();
 for(int i=0;i<n_;i++){
  nodeK_[size_t(i)]+=alpha_*(targetK_[size_t(i)].load(std::memory_order_relaxed)-nodeK_[size_t(i)]);
  nodeD_[size_t(i)]+=alpha_*(targetD_[size_t(i)].load(std::memory_order_relaxed)-nodeD_[size_t(i)]);
 }
 for(size_t j=0;j<recipe_.links.size();j++){
  linkK_[j]+=alpha_*(targetLK_[j].load(std::memory_order_relaxed)-linkK_[j]);linkD_[j]+=alpha_*(targetLD_[j].load(std::memory_order_relaxed)-linkD_[j]);
 }
 parameterWork_+=energy()-before;
 std::array<double,MaxN*MaxN>K{},D{},lhs{};assemble(K,D);
 f_.fill(0);
 if(pulse_>1e-11){f_[size_t(recipe_.exciteNode)]=pulse_*recipe_.nodes[size_t(recipe_.exciteNode)].drive;pulse_*=.8;}
 std::array<double,MaxN>rhs{};
 for(int i=0;i<n_;i++){
  double sum=0;for(int j=0;j<n_;j++)sum+=K[size_t(i*MaxN+j)]*q_[size_t(j)];
  rhs[size_t(i)]=2*diagMass_[size_t(i)]/dt_*v_[size_t(i)]-sum+f_[size_t(i)];
  for(int j=0;j<n_;j++)lhs[size_t(i*MaxN+j)]=D[size_t(i*MaxN+j)]+.5*dt_*K[size_t(i*MaxN+j)];
  lhs[size_t(i*MaxN+i)]+=2*diagMass_[size_t(i)]/dt_;
 }
 if(!solve(lhs,rhs))return 0;
 double aPower=0,dPower=0;
 for(int i=0;i<n_;i++){
  aPower+=f_[size_t(i)]*vmid_[size_t(i)];
  for(int j=0;j<n_;j++)dPower+=vmid_[size_t(i)]*D[size_t(i*MaxN+j)]*vmid_[size_t(j)];
 }
 activeWork_+=dt_*aPower;dissipated_+=std::max(0.,dt_*dPower);
 double output=0;
 for(int i=0;i<n_;i++){
  q_[size_t(i)]+=dt_*vmid_[size_t(i)];
  v_[size_t(i)]=2*vmid_[size_t(i)]-v_[size_t(i)];
  output+=recipe_.nodes[size_t(i)].output*transform_[size_t(i)]*v_[size_t(i)];
 }
 samples_++;
 return std::isfinite(output)?float(std::tanh(output*recipe_.masterGain)):0.f;
}
} // namespace panorama::energy40
