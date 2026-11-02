#include "CoupledPhysics38.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
namespace fs=std::filesystem;
using namespace panorama::coupled38;
namespace {
void u16(std::ofstream&f,std::uint32_t n){f.put(char(n));f.put(char(n>>8));}
void u32(std::ofstream&f,std::uint32_t n){for(int i=0;i<4;i++)f.put(char(n>>(i*8)));}
bool wav(const fs::path&p,const std::vector<float>&a,int rate){
 if(a.empty()||a.size()>std::size_t(rate)*10)return false;
 std::ofstream f(p,std::ios::binary);if(!f)return false;
 std::uint32_t bytes=std::uint32_t(a.size()*4);
 f.write("RIFF",4);u32(f,36+bytes);f.write("WAVEfmt ",8);u32(f,16);u16(f,3);u16(f,1);
 u32(f,std::uint32_t(rate));u32(f,std::uint32_t(rate*4));u16(f,4);u16(f,32);
 f.write("data",4);u32(f,bytes);
 for(auto x:a)f.write(reinterpret_cast<const char*>(&x),4);
 return bool(f);
}
int render(const Recipe&r,const fs::path&output,double seconds,int rate){
 std::string err;auto ins=Instrument::prepare(r,rate,&err);
 if(!ins){std::cerr<<"graph rejected: "<<err<<'\n';return 3;}
 size_t n=size_t(rate*seconds);std::vector<float> audio(n);
 double peak=0,rms=0;
 for(size_t i=0;i<n;i++){float v=ins->processOne();audio[i]=v;peak=std::max(peak,double(std::abs(v)));rms+=double(v)*v;}
 if(!wav(output,audio,rate))return 4;
 std::cout<<"render="<<output<<" peak="<<peak<<" rms="<<std::sqrt(rms/n)<<" end_energy="<<ins->totalEnergy()<<" substeps="<<ins->substeps()<<'\n';
 return 0;
}
Recipe make(int type){
 Recipe r;Domain string;string.family=Family::String1D;string.nx=28;string.fundamentalHz=165;string.damping=2.;string.amplitude=.00010;
 Domain membrane=string;membrane.family=Family::Membrane2D;membrane.nx=20;membrane.ny=14;membrane.fundamentalHz=125;membrane.damping=2.;
 Domain plate=membrane;plate.family=Family::Plate2D;plate.fundamentalHz=185;
 Domain tube=string;tube.family=Family::Tube1D;tube.nx=28;tube.fundamentalHz=155;tube.damping=3.;
 Domain body=string;body.family=Family::TetraResonator3D;body.shape.xCells=1;body.shape.yCells=1;body.shape.zCells=1;body.damping=2.5;
 if(type==0)r.domains={string,membrane};
 else if(type==1)r.domains={string,plate,body};
 else if(type==2)r.domains={membrane,body,tube};
 else r.domains={string,membrane,plate,body,tube};
 for(int i=0;i<int(r.domains.size())-1;i++){
  Connection c;c.from=i;c.to=i+1;c.stiffness=r.domains[size_t(i+1)].family==Family::Tube1D?30.:700000.;
  c.damping=r.domains[size_t(i+1)].family==Family::Tube1D?.005:12.;
  c.positionFrom=.5;c.positionTo=.5;
  r.connections.push_back(c);
 }
 // Extra cyclic reciprocal contact; not a PCM feedback loop.
 if(type==3){Connection c;c.from=0;c.to=4;c.stiffness=20;c.damping=.005;r.connections.push_back(c);}
 r.outputGain=5000;
 return r;
}
}
int main(int argc,char**argv){
 if(argc==3&&std::string(argv[1])=="--demo"){
  fs::path dir=argv[2];fs::create_directories(dir);
  const char*names[]={"string_membrane","string_plate_body","membrane_body_tube","all_five_reciprocal"};
  for(int i=0;i<4;i++){
   auto r=make(i);std::ofstream(dir/(std::string(names[i])+".phys38"))<<serialize(r);
   if(render(r,dir/(std::string(names[i])+".wav"),1.5,48000))return 4;
  }return 0;
 }
 if(argc<3||argc>5){std::cerr<<"usage: panoramize_coupled38 --demo DIR | patch.phys38 output.wav [seconds] [rate]\n";return 1;}
 int rate=48000;double secs=2.;
 try{if(argc>=4)secs=std::stod(argv[3]);if(argc==5)rate=std::stoi(argv[4]);}catch(...){return 1;}
 if(!std::isfinite(secs)||secs<.01||secs>10)return 1;
 std::ifstream f(argv[1]);if(!f)return 2;
 std::string content((std::istreambuf_iterator<char>(f)),std::istreambuf_iterator<char>());
 Recipe r;std::string err;
 if(!deserialize(content,r,&err)){std::cerr<<err<<'\n';return 2;}
 return render(r,argv[2],secs,rate);
}
