#include "ExpressivePhysical32.h"
#include "UniversalInstrument23.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
using namespace panorama;
static void emitWav(const std::string& path,const std::vector<float>& samples,int rate){
 std::ofstream f(path,std::ios::binary);const uint32_t sz=uint32_t(samples.size()*2),riff=sz+36;
 auto u16=[&](uint16_t x){f.put(char(x&255));f.put(char(x>>8));};
 auto u32=[&](uint32_t x){for(int i=0;i<4;i++)f.put(char((x>>(8*i))&255));};
 f.write("RIFF",4);u32(riff);f.write("WAVEfmt ",8);u32(16);u16(1);u16(1);u32(rate);u32(rate*2);u16(2);u16(16);
 f.write("data",4);u32(sz);
 for(const auto x:samples){auto y=int(std::lrint(std::clamp(double(x),-1.,1.)*32767.));u16(uint16_t(int16_t(y)));}
}
int main(int argc,char**argv){
 if(argc!=2){std::cerr<<"Usage: panoramize_expressive32_demo output_directory\n";return 2;}
 const std::string base=argv[1];constexpr int sr=48000;
 expressive32::Model m;m.physical.string.gain=.68;m.physical.string.decayBase=.85;
 m.physical.bridge=true;m.physical.body=true;
 auto render=[&](int scenario){
  auto inst=expressive32::Instrument::prepare(m,sr);if(!inst)return std::vector<float>{};
  std::vector<float> wave(size_t(sr)*3);
  for(size_t i=0;i<wave.size();i++){
   const double t=double(i)/sr;
   if(i==sr/20)inst->noteOn(55,scenario==1?.27f:.81f);
   if(scenario==1 && i==sr*6/5)inst->noteOn(59,.95f); // stronger pluck
   if(scenario==2 && i==sr*2/3)inst->pitchBend(2.f);
   if(scenario==2 && i==sr*4/3)inst->pitchBend(-1.f);
   if(scenario==2 && i==sr*2)inst->pitchBend(0.f);
   if(scenario==3 && i==sr/2)inst->channelPressure(.92f);
   if(scenario==3 && i==sr*7/5)inst->channelPressure(0.f);
   if(scenario==4 && i==sr*3/4)inst->noteOn(59,.82f);
   if(scenario==4 && i==sr*3/2)inst->noteOn(62,.70f);
   if(scenario==4 && i==sr*7/4)inst->noteOff(62);
   if(scenario==4 && i==sr*2)inst->noteOff(59);
   if(i==sr*11/5)inst->noteOff(55);
   wave[i]=inst->processOne()*.84f;
  }
  return wave;
 };
 const std::vector<std::string> names={"00_Baseline","01_Velocity","02_PitchBend","03_Aftertouch","04_Legato"};
 for(int i=0;i<int(names.size());i++){
  auto wave=render(i);emitWav(base+"/"+names[size_t(i)]+".wav",wave,sr);
  double peak=0,e=0;for(const auto x:wave){peak=std::max(peak,std::abs(double(x)));e+=x*x;}
  std::cout<<names[size_t(i)]<<" peak="<<peak<<" rms="<<std::sqrt(e/wave.size())<<'\n';
 }
 universal23::Graph patch;patch.name="Expressive Resonant Guitar 3.2";
 patch.nodes={{universal23::Kind::PhysicalExpressive32,-1,-1,0,.85f},
              {universal23::Kind::Output,0,-1,0,.85f}};
 patch.expressive32=m;
 std::ofstream(base+"/Expressive_Resonant_Guitar.pgraph")<<universal23::serialize(patch);
 std::ofstream(base+"/Expressive_Model.expr32")<<expressive32::serialize(m);
 return 0;
}
