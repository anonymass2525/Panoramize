#include "NonlinearPassive45.h"
#include "UniversalInstrument23.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
namespace {
void u16(std::ofstream&f,unsigned v){f.put(char(v));f.put(char(v>>8));}
void u32(std::ofstream&f,unsigned v){for(int i=0;i<4;i++)f.put(char(v>>(8*i)));}
}
int main(int argc,char**argv){
 if(argc==3&&std::string(argv[1])=="--example"){
  std::ofstream o(argv[2]);o<<panorama::passive45::serialize(panorama::passive45::example());return o?0:2;
 }
 if(argc<3||argc>4){std::cerr<<"usage: renderer --example FILE | MODEL.passive45 output.wav [seconds]\n";return 1;}
 std::ifstream in(argv[1]);if(!in)return 2;
 std::string raw((std::istreambuf_iterator<char>(in)),{}),why;
 panorama::passive45::Recipe r;
 if(raw.rfind("PANORAMIZE-GRAPH-45|1",0)==0){
  panorama::universal23::Graph g;
  if(!panorama::universal23::deserialize(raw,g,&why)){std::cerr<<why<<'\n';return 3;}
  r=g.passive45;
 }else if(!panorama::passive45::deserialize(raw,r,&why)){std::cerr<<why<<'\n';return 3;}
 double seconds=1.;try{if(argc==4)seconds=std::stod(argv[3]);}catch(...){return 3;}
 if(!std::isfinite(seconds)||seconds<.1||seconds>10)return 3;
 const int rate=48000;auto inst=panorama::passive45::Instrument::prepare(r,rate,&why);
 if(!inst){std::cerr<<why<<'\n';return 4;}
 std::vector<float>audio(size_t(rate*seconds));inst->noteOn(57,.7f);
 for(float&v:audio)v=inst->processOne();
 std::ofstream f(argv[2],std::ios::binary);if(!f)return 5;
 unsigned bytes=unsigned(audio.size()*4);f.write("RIFF",4);u32(f,36+bytes);f.write("WAVEfmt ",8);u32(f,16);u16(f,3);u16(f,1);u32(f,rate);u32(f,rate*4);u16(f,4);u16(f,32);f.write("data",4);u32(f,bytes);
 for(float v:audio)f.write((const char*)&v,4);
 auto l=inst->ledger();auto s=inst->stats();std::cout<<"frames="<<s.frames<<" failures="<<s.failures<<" contacts="<<s.contactsEngaged<<" residual_J="<<l.residual<<" active_J="<<l.active<<" dissipation_J="<<l.dissipation<<'\n';
 return f?0:5;
}
