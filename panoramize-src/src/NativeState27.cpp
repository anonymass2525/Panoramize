#include "NativeState27.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <utility>
namespace panorama::native27 {
namespace {
constexpr double tau=6.283185307179586476925286766559;
bool finite(double x){return std::isfinite(x);}
native26::Domain domain(Engine e){
 switch(e){case Engine::Physical:return native26::Domain::Physical;
 case Engine::VA:case Engine::FM:case Engine::Phase:return native26::Domain::Phase;
 case Engine::Spectral:return native26::Domain::Spectral;
 case Engine::Resonance:return native26::Domain::Resonance;
 case Engine::Nonlinear:return native26::Domain::Nonlinear;
 case Engine::Vocal:return native26::Domain::VocalFormant;}
 return native26::Domain::Resonance;
}
// Bandlimited discontinuity correction (first-order polyBLEP).
double blep(double t,double dt) noexcept{
 if(t<dt){const double x=t/dt;return x+x-x*x-1.;}
 if(t>1.-dt){const double x=(t-1.)/dt;return x*x+x+x+1.;}
 return 0.;
}
}
Graph example(){
 Graph g;g.name="Horizon State / Editable";
 g.modes={{Engine::VA,1.,9.,0.,.12,230.},
          {Engine::FM,2.,10.,0.,.12,165.},
          {Engine::Spectral,1.5,7.,0.,.25,0.},
          {Engine::Phase,1.02,8.,0.,.12,0.},
          {Engine::Resonance,.5,3.,0.,.35,0.},
          {Engine::Physical,1.,4.,0.,.32,0.},
          {Engine::Vocal,3.,12.,0.,.28,0.},
          {Engine::Nonlinear,2.4,7.,4.e10,.12,0.}};
 for(int i=0;i<8;i++)g.edges.push_back({i,(i+1)%8,native26::Coupling::Conservative,5.e4,.87});
 g.edges.push_back({0,5,native26::Coupling::Dissipative,1.4,.9});
 g.strikeMode=4;
 return g;
}
std::string serialize(const Graph& g){
 std::ostringstream out;out.precision(17);
 out<<"PANORAMIZE-STATE-27|1\n";
 std::string name=g.name;
 for(auto& c:name)if(c=='|'||c=='\n'||c=='\r')c=' ';
 out<<"NAME|"<<name.substr(0,120)<<"\nSTRIKE|"<<g.strikeMode<<"\n";
 for(const auto& m:g.modes)
  out<<"MODE|"<<int(m.engine)<<'|'<<m.ratio<<'|'<<m.damping<<'|'<<m.nonlinear<<'|'<<m.pickup<<'|'<<m.excitation<<'\n';
 for(const auto& e:g.edges)
  out<<"EDGE|"<<e.a<<'|'<<e.b<<'|'<<int(e.kind)<<'|'<<e.strength<<'|'<<e.ratio<<'|'<<int(e.quantity)<<'|'<<int(e.unit)<<'\n';
 return out.str();
}
bool deserialize(const std::string& s,Graph& out,std::string* err){
 auto fail=[&](const char* reason){if(err)*err=reason;return false;};
 if(s.size()>40000)return fail("state patch too large");
 std::istringstream input(s);std::string line;
 if(!std::getline(input,line)||line!="PANORAMIZE-STATE-27|1")return fail("wrong state version");
 Graph g;g.modes.clear();g.edges.clear();bool name=false,strike=false;
 while(std::getline(input,line)){
  if(line.empty())continue;
  char tail;int type,a,b,kind,q,u,st;double x,y,z,v,w;
  if(line.rfind("NAME|",0)==0){if(name)return fail("duplicate name");g.name=line.substr(5);name=true;}
  else if(line.rfind("STRIKE|",0)==0){if(strike||std::sscanf(line.c_str(),"STRIKE|%d%c",&st,&tail)!=1)return fail("bad strike");g.strikeMode=st;strike=true;}
  else if(line.rfind("MODE|",0)==0){
   if(g.modes.size()>=64||std::sscanf(line.c_str(),"MODE|%d|%lf|%lf|%lf|%lf|%lf%c",&type,&x,&y,&z,&v,&w,&tail)!=6||type<0||type>7)return fail("bad state mode");
   g.modes.push_back({Engine(type),x,y,z,v,w});
  }else if(line.rfind("EDGE|",0)==0){
   if(g.edges.size()>=256||std::sscanf(line.c_str(),"EDGE|%d|%d|%d|%lf|%lf|%d|%d%c",&a,&b,&kind,&x,&y,&q,&u,&tail)!=7||kind<0||kind>1)return fail("bad state edge");
   g.edges.push_back({a,b,native26::Coupling(kind),x,y,unified22::Quantity(q),unified22::Unit(u)});
  }else return fail("unrecognized state record");
 }
 if(!name||!strike||g.modes.empty()||g.strikeMode<0||size_t(g.strikeMode)>=g.modes.size())return fail("incomplete state patch");
 if(err)err->clear();
 out=std::move(g);return true;
}
std::unique_ptr<Instrument> Instrument::prepare(const Graph& g,double sr,std::string* error){
 auto fail=[&](const char* reason)->std::unique_ptr<Instrument>{if(error)*error=reason;return {};};
 if(!finite(sr)||sr<8000||sr>192000||g.modes.empty()||g.modes.size()>64||g.edges.size()>256||g.strikeMode<0||size_t(g.strikeMode)>=g.modes.size())return fail("invalid state graph");
 native26::Graph physical;physical.name=g.name;physical.strikeMode=g.strikeMode;
 for(const auto& m:g.modes){
  if(int(m.engine)<0||int(m.engine)>7||!finite(m.ratio)||m.ratio<.05||m.ratio>16||!finite(m.damping)||m.damping<0||m.damping>2000||!finite(m.nonlinear)||m.nonlinear<0||m.nonlinear>1.e16||!finite(m.pickup)||std::abs(m.pickup)>10||!finite(m.excitation)||m.excitation<0||m.excitation>1.e5)return fail("invalid mode parameter");
  physical.modes.push_back({domain(m.engine),m.ratio,m.damping,m.nonlinear,m.pickup});
 }
 physical.edges=g.edges;
 auto inst=std::unique_ptr<Instrument>(new Instrument());inst->modal_=native26::Instrument::prepare(physical,sr,error);
 if(!inst->modal_)return {};
 inst->graph_=g;inst->sampleRate_=sr;
 for(auto&v:inst->excitationTargets_)v.store(1.,std::memory_order_relaxed);
 inst->excitationSmooth_.fill(1.);
 for(int i=0;i<128;i++)inst->noteHz_[size_t(i)]=440.*std::pow(2.,(i-69.)/12.);
 if(error)error->clear();
 return inst;
}
void Instrument::noteOn(int midi,float vel) noexcept{
 if(midi<0||midi>127||!finite(vel)||vel<=0)return;
 modal_->noteOn(midi,vel);active_[size_t(midi)]=true;keyHeld_[size_t(midi)]=true;
 for(size_t i=0;i<graph_.modes.size();i++){
  auto& v=notes_[size_t(midi)][i];v.held=true;v.level=std::clamp(vel,0.f,1.f);v.phase=0.;v.subphase=0.;
 }
}
void Instrument::noteOff(int midi) noexcept{
 if(midi<0||midi>127)return;
 keyHeld_[size_t(midi)]=false;
 for(auto& v:notes_[size_t(midi)])v.held=false;
 if(!pedalDown_)modal_->noteOff(midi);
}
bool Instrument::addExternalEffort(int midi,int mode,double effort) noexcept{
 return modal_->addActiveEffort(midi,mode,effort);
}
double Instrument::modeFlow(int midi,int mode) const noexcept{
 return modal_->normalizedFlow(midi,mode);
}
void Instrument::allNotesOff() noexcept{
 modal_->allNotesOff();active_.fill(false);keyHeld_.fill(false);pedalDown_=false;
 for(auto& row:notes_)for(auto& v:row)v={};
}
void Instrument::pitchBend(float b) noexcept{if(finite(b))bendRatio_=std::pow(2.,double(std::clamp(b,-24.f,24.f))/12.);}
void Instrument::modWheel(float v) noexcept{if(finite(v))mod_=std::clamp(v,0.f,1.f);}
void Instrument::sustain(bool down) noexcept{
 pedalDown_=down;
 if(!down)for(int note=0;note<128;note++)
  if(!keyHeld_[size_t(note)])modal_->noteOff(note);
}
bool Instrument::setLiveModeNormalized(int mode,bool excitation,double normalized) noexcept{
 if(mode<0||size_t(mode)>=graph_.modes.size()||!finite(normalized)||normalized<0||normalized>1)return false;
 const double scale=2.*normalized;
 if(excitation){
  auto e=graph_.modes[size_t(mode)].engine;
  if(e!=Engine::VA&&e!=Engine::FM)return false;
  excitationTargets_[size_t(mode)].store(scale,std::memory_order_relaxed);
 }else if(!modal_->setLivePickupScale(mode,scale))return false;
 return true;
}
void Instrument::clearLiveModeControls() noexcept{
 for(auto&v:excitationTargets_)v.store(1.,std::memory_order_relaxed);
 for(size_t i=0;i<graph_.modes.size();i++)modal_->setLivePickupScale(int(i),1.);
}
float Instrument::processMono() noexcept{
 const double alpha=1.-std::exp(-1./(sampleRate_*.005));
 for(size_t i=0;i<graph_.modes.size();i++)
  excitationSmooth_[i]+=alpha*(excitationTargets_[i].load(std::memory_order_relaxed)-excitationSmooth_[i]);
 for(int note=0;note<128;note++){
  if(!active_[size_t(note)])continue;
  const double fundamental=noteHz_[size_t(note)]*bendRatio_;
  bool any=false;
  for(size_t i=0;i<graph_.modes.size();i++){
   const auto& m=graph_.modes[i];if(m.excitation<=0.)continue;
   auto& n=notes_[size_t(note)][i];if(n.level<1.e-4f)continue;
   any=true;
   const double step=std::clamp(fundamental*m.ratio/sampleRate_,0.0000001,0.49);
   double excitation=0.;
   if(m.engine==Engine::VA){excitation=2.*n.phase-1.-blep(n.phase,step);}
   else if(m.engine==Engine::FM){excitation=std::sin(tau*n.phase+(1.+3.*mod_)*std::sin(tau*n.subphase));}
   else continue;
   // Couplings inside the shared solver are conservative/dissipative. This
   // oscillator is a POWERED transducer; its work is explicitly metered.
   modal_->addActiveEffort(note,int(i),std::clamp(excitation*m.excitation*excitationSmooth_[i]*double(n.level),-1.e6,1.e6));
   n.phase+=step;if(n.phase>=1.)n.phase-=1.;
   n.subphase+=std::min(.49,step*1.61);if(n.subphase>=1.)n.subphase-=1.;
   if(!n.held&&!pedalDown_)n.level*=.9995f;
  }
  if(!any){active_[size_t(note)]=false;}
 }
 return modal_->processMono();
}
double Instrument::energyResidual() const noexcept{return modal_->energyResidual();}
double Instrument::activeWork() const noexcept{return modal_->activeWork();}
} // namespace panorama::native27
