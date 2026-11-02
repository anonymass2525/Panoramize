#include "UnifiedGraph22.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>
using namespace panorama::unified22;
namespace {
std::vector<std::string> split(const std::string& s){
 std::vector<std::string> r;std::istringstream f(s);std::string x;
 while(std::getline(f,x,',')){
  const auto begin=x.find_first_not_of(" \t\r");const auto end=x.find_last_not_of(" \t\r");
  r.push_back(begin==std::string::npos?"":x.substr(begin,end-begin+1));
 }return r;
}
void w16(std::ostream& o,uint16_t v){const char b[2]={char(v),char(v>>8)};o.write(b,2);}
void w32(std::ostream& o,uint32_t v){const char b[4]={char(v),char(v>>8),char(v>>16),char(v>>24)};o.write(b,4);}
Domain domain(const std::string& d){
 if(d=="physical")return Domain::Physical;
 if(d=="spectral")return Domain::Spectral;
 if(d=="phase")return Domain::Phase;
 if(d=="resonance")return Domain::Resonance;
 if(d=="nonlinear")return Domain::Nonlinear;
 if(d=="vocal")return Domain::VocalFormant;
 throw std::runtime_error("unknown mode domain: "+d);
}
double num(const std::string& s){size_t pos=0;double x=std::stod(s,&pos);if(pos!=s.size()||!std::isfinite(x))throw std::runtime_error("bad numeric value: "+s);return x;}
struct Pickup{int mode;double left,right;};
struct Kick{int mode;double velocity;};
struct RepeatDrive{int mode;double force;int every;};
}
int main(int argc,char** argv){
 if(argc!=3){std::cerr<<"usage: panorama_unified22_csv graph.csv output.wav\n";return 2;}
 try{
  std::ifstream file(argv[1]);if(!file)throw std::runtime_error("could not read graph.csv");
  Builder b;std::map<std::string,int> ids;
  std::vector<Pickup> pickups;std::vector<Kick> kicks;std::vector<RepeatDrive> drives;
  double length=2.,sr=48000.;int lineNo=0;
  for(std::string line;std::getline(file,line);){
   lineNo++;if(line.empty()||line[0]=='#')continue;
   auto c=split(line);if(c.empty())continue;
   auto idx=[&](const std::string& s){auto i=ids.find(s);if(i==ids.end())throw std::runtime_error("unknown mode: "+s);return i->second;};
   auto need=[&](size_t n){if(c.size()!=n)throw std::runtime_error("expected "+std::to_string(n)+" fields");};
   try{
    if(c[0]=="mode"){
     need(7);Mode m;m.name=c[1];m.domain=domain(c[2]);m.frequencyHz=num(c[3]);m.dampingPerSecond=num(c[4]);m.quartic=num(c[5]);
     double cal=num(c[6]);
     if(m.domain==Domain::Physical)m.displacementMetresPerQ=cal;
     else if(m.domain==Domain::Spectral)m.spectralAmplitudePerQ=cal;
     else if(m.domain==Domain::Phase)m.phaseRadiansPerQ=cal;
     else if(m.domain==Domain::VocalFormant)m.pressurePascalsPerQ=cal;
     if(ids.count(m.name))throw std::runtime_error("duplicate mode name");
     int id=b.addMode(m);if(id<0)throw std::runtime_error("invalid mode parameters");ids[m.name]=id;
    } else if(c[0]=="link"){
     need(6);const auto kind=c[3]=="conservative"?LinkKind::Conservative:c[3]=="dissipative"?LinkKind::Dissipative:throw std::runtime_error("invalid link kind");
     if(!b.connectTyped({idx(c[1]),Quantity::NormalizedCoordinate,Unit::RootJouleSeconds},
           {idx(c[2]),Quantity::NormalizedCoordinate,Unit::RootJouleSeconds},kind,num(c[4]),num(c[5])))throw std::runtime_error("invalid energy link");
    } else if(c[0]=="kick"){
     need(3);kicks.push_back({idx(c[1]),num(c[2])});
    } else if(c[0]=="drive"){
     need(4);int every=int(num(c[3]));if(every<1)throw std::runtime_error("drive interval must be positive");drives.push_back({idx(c[1]),num(c[2]),every});
    } else if(c[0]=="pickup"){
     need(4);pickups.push_back({idx(c[1]),num(c[2]),num(c[3])});
    } else if(c[0]=="seconds"){
     need(2);length=num(c[1]);
    } else throw std::runtime_error("unknown directive: "+c[0]);
   }catch(const std::exception& e){throw std::runtime_error("line "+std::to_string(lineNo)+": "+e.what());}
  }
  if(length<=0.||length>30.||pickups.empty()||kicks.empty())throw std::runtime_error("invalid duration / missing pickup or kick");
  std::string err;auto compiled=b.compile(sr,&err);if(!compiled)throw std::runtime_error(err);
  auto voice=compiled->makeVoice();for(const auto& k:kicks)voice.excite(k.mode,k.velocity);
  const size_t frames=size_t(sr*length);std::vector<double> pcm;pcm.reserve(frames*2);
  double peak=0.;
  for(size_t i=0;i<frames;i++){
   for(const auto& d:drives)if(i%size_t(d.every)==0 && i<size_t(sr*length*.8))voice.addActiveForce(d.mode,d.force);
   if(!voice.processSample())throw std::runtime_error("state solver did not converge");
   double l=0.,r=0.;for(const auto& p:pickups){l+=voice.audioSample(p.mode,p.left);r+=voice.audioSample(p.mode,p.right);}
   pcm.push_back(l);pcm.push_back(r);peak=std::max({peak,std::abs(l),std::abs(r)});
  }
  if(peak<1.e-9)throw std::runtime_error("silent graph");
  const double gain=.7/peak;std::ofstream out(argv[2],std::ios::binary);if(!out)throw std::runtime_error("cannot write WAV");
  const auto len=uint32_t(frames*4);out.write("RIFF",4);w32(out,len+36);out.write("WAVEfmt ",8);w32(out,16);w16(out,1);w16(out,2);w32(out,uint32_t(sr));w32(out,uint32_t(sr*4));w16(out,4);w16(out,16);out.write("data",4);w32(out,len);
  for(double s:pcm)w16(out,uint16_t(int16_t(std::lround(std::clamp(s*gain,-1.,1.)*32767.))));
  auto e=voice.ledger();std::cout<<"Created "<<argv[2]<<" modes="<<b.modeCount()<<" energy_error="<<e.residual<<"\n";
  return out?0:1;
 }catch(const std::exception& e){std::cerr<<"ERROR: "<<e.what()<<"\n";return 1;}
}
