#include "ArticulatedPhysical33.h"
#include "UniversalInstrument23.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
using namespace panorama;
static void wav(const std::string&path,const std::vector<float>& samples){
 std::ofstream f(path,std::ios::binary);const uint32_t bytes=uint32_t(samples.size()*2),riff=bytes+36;
 auto u16=[&](uint16_t v){f.put(char(v&255));f.put(char(v>>8));};
 auto u32=[&](uint32_t v){for(int i=0;i<4;i++)f.put(char((v>>(i*8))&255));};
 f.write("RIFF",4);u32(riff);f.write("WAVEfmt ",8);u32(16);u16(1);u16(1);u32(48000);u32(96000);u16(2);u16(16);
 f.write("data",4);u32(bytes);
 for(auto x:samples){u16(uint16_t(int16_t(std::lrint(std::clamp(double(x),-1.,1.)*32767.))));}
}
int main(int argc,char**argv){
 if(argc!=2){std::cerr<<"Usage: panoramize_articulated33_demo output_directory\n";return 2;}
 constexpr int sr=48000;const std::string base=argv[1];
 articulated33::Model m;m.physical.bridge=true;m.physical.body=true;
 m.physical.string.gain=.6;m.physical.string.decayBase=.9;
 m.glideMs=85.;
 const std::vector<std::string> names={"00_Baseline", "01_HammerOn", "02_PullOff", "03_Slide", "04_PalmMute", "05_CombinedPhrase"};
 for(size_t scene=0;scene<names.size();scene++){
  auto inst=articulated33::Instrument::prepare(m,sr);if(!inst)return 3;
  std::vector<float> buffer(size_t(sr)*3);
  for(size_t i=0;i<buffer.size();i++){
   if(i==sr/10)inst->noteOn(scene==2?64:55,.86f);
   if(i==sr*7/10){
    if(scene==1||scene==5)inst->hammerOn(60,.88f);
    else if(scene==2)inst->pullOff(59,.88f);
    else if(scene==3)inst->slideTo(60,.88f);
    else if(scene==4)inst->palmMute(.95f);
    else inst->noteOn(60,.88f);
   }
   if(i==sr*14/10){
    if(scene==4)inst->palmMute(.15f);
    if(scene==5){inst->slideTo(64,.9f);inst->palmMute(.4f);}
   }
   if(i==sr*18/10&&scene==5){inst->pullOff(60,.8f);inst->palmMute(.85f);}
   if(i==sr*22/10)inst->noteOff(60);
   buffer[i]=inst->processOne()*.9f;
  }
  wav(base+"/"+names[scene]+".wav",buffer);
  double sum=0.,maxv=0.;for(float v:buffer){sum+=v*v;maxv=std::max(maxv,std::abs(double(v)));}
  std::cout<<names[scene]<<" rms="<<std::sqrt(sum/buffer.size())<<" peak="<<maxv<<"\n";
 }
 auto g=universal23::articulatedGraph();g.articulated33=m;
 std::ofstream(base+"/Panoramize_Physical_Articulation_33.pgraph")<<universal23::serialize(g);
 std::ofstream(base+"/Physical_Articulation_33.art33")<<articulated33::serialize(m);
 return 0;
}
