#include "NativeState26.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <utility>
namespace panorama::native26 {
namespace {
constexpr double pi=3.1415926535897932384626433832795;
const char* domainName(Domain d){
 switch(d){case Domain::Physical:return "Physical";case Domain::Spectral:return "Spectral";
  case Domain::Phase:return "Phase";case Domain::Resonance:return "Resonance";
  case Domain::Nonlinear:return "Nonlinear";case Domain::VocalFormant:return "Vocal";}
 return "Unknown";
}
bool valid(double a){return std::isfinite(a);}
}
Graph example(int variant){
 Graph g;
 static const char* names[]={"Fundamental Matter","Spectral Ecology","Resonant Body","Vocal Alloy","Nonlinear Organism","Kinetic Horizon"};
 variant=std::clamp(variant,0,5);g.name=names[variant];
 // All six acoustic state families participate in one jointly-solved system.
 for(int i=0;i<6;i++){
  Mode m;m.domain=static_cast<Domain>(i);
  m.harmonicRatio=(i==5?2.1:1.+.19*i)+.07*variant*(i%3);
  m.damping=1.8+i*1.3+(variant%2)*1.1;
  m.nonlinear=i==4?(variant+1)*1.5e11:0.;
  m.pickupGain=(i==0?.29:i==3?.28:i==5?.25:.13);
  g.modes.push_back(m);
 }
 // Cycle is a simultaneous (zero additional sample delay) conservative system.
 for(int i=0;i<6;i++)g.edges.push_back({i,(i+1)%6,Coupling::Conservative,8.0e4+variant*2.3e4,.78});
 g.edges.push_back({0,3,Coupling::Dissipative,.7+.3*variant,.8});
 g.edges.push_back({1,5,Coupling::Dissipative,1.2,.75});
 g.strikeMode=variant%6;
 return g;
}
std::string serialize(const Graph& g){
 std::ostringstream o;o.precision(17);
 o<<"PANORAMIZE-NATIVE-26|1\n";
 std::string name=g.name;for(char& c:name)if(c=='|'||c=='\n'||c=='\r')c=' ';
 o<<"NAME|"<<name.substr(0,120)<<"\n";
 o<<"STRIKE|"<<g.strikeMode<<"\n";
 for(const auto& m:g.modes)o<<"MODE|"<<int(m.domain)<<'|'<<m.harmonicRatio<<'|'<<m.damping<<'|'<<m.nonlinear<<'|'<<m.pickupGain<<"\n";
 for(const auto& e:g.edges)o<<"EDGE|"<<e.a<<'|'<<e.b<<'|'<<int(e.kind)<<'|'<<e.strength<<'|'<<e.ratio<<'|'<<int(e.quantity)<<'|'<<int(e.unit)<<"\n";
 return o.str();
}
bool deserialize(const std::string& data,Graph& out,std::string* error){
 auto fail=[&](const char* why){if(error)*error=why;return false;};
 if(data.size()>60000)return fail("native patch too large");
 std::istringstream s(data);std::string line;
 if(!std::getline(s,line)||line!="PANORAMIZE-NATIVE-26|1")return fail("wrong version");
 Graph g;g.modes.clear();g.edges.clear();bool gotName=false,gotStrike=false;
 while(std::getline(s,line)){
  if(line.empty())continue;
  if(line.rfind("NAME|",0)==0){if(gotName)return fail("duplicate name");g.name=line.substr(5);gotName=true;continue;}
  if(line.rfind("STRIKE|",0)==0){int m;char c; if(gotStrike||std::sscanf(line.c_str(),"STRIKE|%d%c",&m,&c)!=1)return fail("bad strike");g.strikeMode=m;gotStrike=true;continue;}
  int d,a,b,k,q,u;double f,x,y,z,r;char c;
  if(line.rfind("MODE|",0)==0){
   if(std::sscanf(line.c_str(),"MODE|%d|%lf|%lf|%lf|%lf%c",&d,&f,&x,&y,&z,&c)!=5||d<0||d>5||g.modes.size()>=64)return fail("bad mode");
   g.modes.push_back({Domain(d),f,x,y,z});continue;
  }
  if(line.rfind("EDGE|",0)==0){
   if(std::sscanf(line.c_str(),"EDGE|%d|%d|%d|%lf|%lf|%d|%d%c",&a,&b,&k,&f,&r,&q,&u,&c)!=7||k<0||k>1||g.edges.size()>=256)return fail("bad edge");
   g.edges.push_back({a,b,Coupling(k),f,r,unified22::Quantity(q),unified22::Unit(u)});continue;
  }
  return fail("unrecognized line");
 }
 if(!gotName||!gotStrike||g.modes.empty()||g.strikeMode<0||size_t(g.strikeMode)>=g.modes.size())return fail("incomplete patch");
 if(error)error->clear();
 out=std::move(g);return true;
}
std::unique_ptr<Instrument> Instrument::prepare(const Graph& g,double sampleRate,std::string* error){
 auto fail=[&](const char* why)->std::unique_ptr<Instrument>{if(error)*error=why;return {};};
 if(!valid(sampleRate)||sampleRate<8000||sampleRate>192000||g.modes.empty()||g.modes.size()>64||g.strikeMode<0||size_t(g.strikeMode)>=g.modes.size())return fail("invalid graph configuration");
 for(const auto& m:g.modes){
  if(int(m.domain)<0||int(m.domain)>5||!valid(m.harmonicRatio)||m.harmonicRatio<.05||m.harmonicRatio>16.||!valid(m.damping)||m.damping<0||m.damping>2000||!valid(m.nonlinear)||m.nonlinear<0||m.nonlinear>1.e16||!valid(m.pickupGain)||std::abs(m.pickupGain)>10)return fail("bad physical mode");
 }
 for(const auto& e:g.edges){
  if(e.a<0||e.b<0||e.a==e.b||size_t(e.a)>=g.modes.size()||size_t(e.b)>=g.modes.size()||int(e.kind)<0||int(e.kind)>1||!valid(e.strength)||e.strength<0||e.strength>1.e12||!valid(e.ratio)||std::abs(e.ratio)>64)return fail("invalid power link");
  if(e.quantity!=unified22::Quantity::NormalizedCoordinate||e.unit!=unified22::Unit::RootJouleSeconds)return fail("incompatible port quantity or unit; calibrated transducer required");
 }
 auto instance=std::unique_ptr<Instrument>(new Instrument());instance->graph_=g;instance->sampleRate_=sampleRate;
 // Immutable graph factors and dynamic voice states prepared off the audio thread.
 for(int midi=0;midi<128;midi++){
  const double pitch=440.*std::pow(2.,(midi-69.)/12.);
  unified22::Builder builder;
  for(size_t i=0;i<g.modes.size();i++){
   const auto& m=g.modes[i];unified22::Mode mod;
   mod.name=domainName(m.domain)+std::to_string(i);
   mod.domain=m.domain;mod.frequencyHz=std::clamp(pitch*m.harmonicRatio,.01,20000.);
   mod.dampingPerSecond=m.damping;mod.quartic=m.nonlinear;
   switch(m.domain){case Domain::Physical:mod.displacementMetresPerQ=1.e-6;break;
    case Domain::Spectral:mod.spectralAmplitudePerQ=1.;break;
    case Domain::Phase:mod.phaseRadiansPerQ=1.;break;
    case Domain::VocalFormant:mod.pressurePascalsPerQ=2.;break;
    default:break;}
   if(builder.addMode(std::move(mod))<0)return fail("cannot add native mode");
  }
  for(const auto& e:g.edges){
   const unified22::Port a{e.a,e.quantity,e.unit},b{e.b,e.quantity,e.unit};
   const auto kind=e.kind==Coupling::Conservative?unified22::LinkKind::Conservative:unified22::LinkKind::Dissipative;
   if(!builder.connectTyped(a,b,kind,e.strength,e.ratio))return fail("incompatible native connection");
  }
  std::string why;auto compiled=builder.compile(sampleRate,&why);
  if(!compiled)return fail("unable to compile native state matrix");
  instance->voices_[size_t(midi)].voice=std::make_unique<unified22::Voice>(std::move(compiled));
 }
 for(size_t i=0;i<64;i++)instance->pickupTarget_[i].store(1.,std::memory_order_relaxed);
 instance->pickupSmooth_.fill(1.);
 if(error)error->clear();
 return instance;
}
void Instrument::noteOn(int midi,float velocity) noexcept{
 if(midi<0||midi>=128||!std::isfinite(velocity)||velocity<=0)return;
 auto& v=voices_[size_t(midi)];v.voice->reset();v.voice->excite(graph_.strikeMode,std::clamp(double(velocity),0.,1.));
 v.envelope=1.f;v.held=true;
}
bool Instrument::addActiveEffort(int midi,int mode,double effort) noexcept{
 if(midi<0||midi>=128||mode<0||size_t(mode)>=graph_.modes.size()||!std::isfinite(effort)||std::abs(effort)>1.e6)return false;
 return voices_[size_t(midi)].voice->addActiveForce(mode,effort);
}
double Instrument::normalizedFlow(int midi,int mode) const noexcept{
 if(midi<0||midi>=128||mode<0||size_t(mode)>=graph_.modes.size()||voices_[size_t(midi)].envelope<1.e-5f)return 0.;
 return voices_[size_t(midi)].voice->read({mode,unified22::Quantity::NormalizedFlow,unified22::Unit::RootJoule});
}
void Instrument::noteOff(int midi) noexcept{if(midi>=0&&midi<128)voices_[size_t(midi)].held=false;}
void Instrument::allNotesOff() noexcept{for(auto& v:voices_){v.voice->reset();v.envelope=0;v.held=false;}}
bool Instrument::setLivePickupScale(int mode,double scale) noexcept{
 if(mode<0||size_t(mode)>=graph_.modes.size()||!std::isfinite(scale)||scale<0.||scale>2.)return false;
 pickupTarget_[size_t(mode)].store(scale,std::memory_order_relaxed);return true;
}
float Instrument::processMono() noexcept{
 // One-pole per-mode smoothing, no allocation, before any voice renders.
 const double a=1.-std::exp(-1./(sampleRate_*.005));
 for(size_t i=0;i<graph_.modes.size();i++)
  pickupSmooth_[i]+=a*(pickupTarget_[i].load(std::memory_order_relaxed)-pickupSmooth_[i]);
 double sum=0.;
 for(auto& v:voices_){
  if(v.envelope<1.e-5f)continue;
  if(!v.voice->processSample()){v.envelope=0;continue;}
  double s=0.;for(size_t i=0;i<graph_.modes.size();i++)s+=v.voice->audioSample(int(i),graph_.modes[i].pickupGain*pickupSmooth_[i]);
  sum+=s*v.envelope;
  if(!v.held)v.envelope*=.9994f;
 }
 return std::isfinite(sum)?float(std::tanh(sum*.16)):0.f;
}
double Instrument::energyResidual() const noexcept{double value=0.;for(const auto& v:voices_)if(v.envelope>1.e-5f)value=std::max(value,std::abs(v.voice->ledger().residual));return value;}
double Instrument::dissipated() const noexcept{double value=0.;for(const auto& v:voices_)value+=v.voice->ledger().dissipated;return value;}
double Instrument::activeWork() const noexcept{double value=0.;for(const auto& v:voices_)value+=v.voice->ledger().activeWork;return value;}
} // namespace panorama::native26
