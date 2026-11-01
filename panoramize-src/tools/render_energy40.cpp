#include "EnergyPorts40.h"
#include "UniversalInstrument23.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
using namespace panorama;
static void wav(const std::string&path,const std::vector<float>&samples,int sr){
 std::ofstream f(path,std::ios::binary);if(!f)throw std::runtime_error("output WAV failed");
 auto u16=[&](unsigned x){for(int i=0;i<2;i++)f.put(char((x>>(8*i))&255));};
 auto u32=[&](unsigned x){for(int i=0;i<4;i++)f.put(char((x>>(8*i))&255));};
 f.write("RIFF",4);u32(36+4*unsigned(samples.size()));f.write("WAVEfmt ",8);u32(16);u16(3);u16(1);u32(unsigned(sr));u32(unsigned(sr)*4);u16(4);u16(32);f.write("data",4);u32(unsigned(samples.size())*4);
 for(float v:samples)f.write(reinterpret_cast<const char*>(&v),4);
}
int main(int argc,char**argv){
 if(argc<3){std::cerr<<"usage: panoramize_energy40_demo input.energy40 output.wav [duration_seconds] [sample_rate]\n";return 2;}
 const int sr=argc>4?std::stoi(argv[4]):48000;const double sec=argc>3?std::stod(argv[3]):1.5;
 if(!(sec>0&&sec<=30&&sr>=22050&&sr<=192000))return 2;
 if(std::string(argv[1])=="--write-example"){
  std::ofstream file(argv[2]);file<<energy40::serialize(energy40::example());return file?0:2;
 }
 std::ifstream file(argv[1]);if(!file){std::cerr<<"input missing\n";return 2;}
 std::string text((std::istreambuf_iterator<char>(file)),{}),err;energy40::Recipe r;
 if(text.rfind("PANORAMIZE-GRAPH-40|1",0)==0){
  universal23::Graph g;
  if(!universal23::deserialize(text,g,&err)){std::cerr<<err<<"\n";return 2;}
  r=g.energy40;
 }else if(!energy40::deserialize(text,r,&err)){std::cerr<<err<<"\n";return 2;}
 auto x=energy40::Instrument::prepare(r,sr,&err);if(!x){std::cerr<<err<<"\n";return 2;}
 std::vector<float> samples(size_t(sr*sec));x->noteOn(57,1.f);
 auto begin=std::chrono::steady_clock::now();for(auto&v:samples)v=x->processOne();
 auto end=std::chrono::steady_clock::now();wav(argv[2],samples,sr);
 auto stats=x->ledger();std::cout<<"samples="<<samples.size()<<" render_ms="<<std::chrono::duration<double,std::milli>(end-begin).count()
 <<" initial_J="<<stats.initial<<" active_J="<<stats.active<<" modulation_J="<<stats.parameter<<" dissipation_J="<<stats.dissipated
 <<" final_J="<<stats.finalEnergy<<" ledger_residual_J="<<stats.residual<<"\n";
}
