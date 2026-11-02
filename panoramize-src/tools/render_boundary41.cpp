#include "BoundaryPorts41.h"
#include "UniversalInstrument23.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
using namespace panorama;
static void saveWav(const std::string&path,const std::vector<float>&samples){
 std::ofstream o(path,std::ios::binary);const uint32_t count=uint32_t(samples.size()*2);
 auto u16=[&](uint16_t n){o.put(char(n&255));o.put(char((n>>8)&255));};
 auto u32=[&](uint32_t n){u16(uint16_t(n));u16(uint16_t(n>>16));};
 o.write("RIFF",4);u32(36+count);o.write("WAVEfmt ",8);u32(16);u16(1);u16(1);u32(48000);u32(96000);u16(2);u16(16);
 o.write("data",4);u32(count);
 for(float x:samples){const int16_t v=int16_t(std::lround(std::clamp(x,-1.f,1.f)*32767));u16(uint16_t(v));}
}
int main(int argc,char**argv){
 if(argc<2){std::cerr<<"Usage: panoramize_boundary41_demo <output-prefix>\n";return 2;}
 auto recipe=boundary41::example();
 for(int j=0;j<3;j++){
  auto r=recipe;
  if(j==1){r.stringMembraneK=0;r.stringMembraneD=0;r.membraneBodyK=0;r.membraneBodyD=0;r.bodyCavityK=0;r.bodyCavityD=0;}
  if(j==2){r.nativeEnabled=true;r.nativeMode=0;r.velocityToEffort=5;r.flowToForce=.01;}
  std::string err;auto p=boundary41::Instrument::prepare(r,48000,&err);
  if(!p){std::cerr<<"prepare: "<<err<<"\n";return 1;}
  p->noteOn(57,.85f);std::vector<float>a;a.reserve(120000);
  for(int i=0;i<120000;i++){if(i==24000)p->noteOn(64,.75f);if(i==60000)p->noteOn(52,.9f);if(i==96000)p->noteOn(69,.65f);a.push_back(p->processOne());}
  const std::string name=std::string(argv[1])+(j==0?"_coupled.wav":j==1?"_disconnected.wav":"_active_native.wav");
  saveWav(name,a);
  const auto l=p->ledger();
  std::cout<<name<<" dofs="<<p->dofCount()<<" E="<<l.finalEnergy<<" active="<<l.active<<" loss="<<l.dissipated<<" residual="<<l.residual<<" nativeWork="<<l.nativeActiveWork<<"\n";
 }
 std::ofstream f(std::string(argv[1])+".boundary41");f<<boundary41::serialize(recipe);
 std::ofstream g(std::string(argv[1])+".pgraph");g<<universal23::serialize(universal23::boundaryGraph());
}
