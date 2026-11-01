#include "UniversalInstrument23.h"
#include "UnifiedControls50.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
using namespace panorama;
static void u16(std::ofstream& f,uint16_t a){for(int i=0;i<2;i++)f.put(char(a>>(8*i)));}
static void u32(std::ofstream& f,uint32_t a){for(int i=0;i<4;i++)f.put(char(a>>(8*i)));}
static bool wav(const std::string&path,const std::vector<float>&a,int sr){
 std::ofstream f(path,std::ios::binary);if(!f)return false;
 f.write("RIFF",4);u32(f,36+uint32_t(a.size()*4));f.write("WAVEfmt ",8);
 u32(f,16);u16(f,3);u16(f,1);u32(f,uint32_t(sr));u32(f,uint32_t(sr*4));u16(f,4);u16(f,32);
 f.write("data",4);u32(f,uint32_t(a.size()*4));for(float s:a)f.write(reinterpret_cast<const char*>(&s),4);
 return bool(f);
}
int main(int argc,char**argv){
 if(argc!=2)return 2;const std::string base=argv[1];const int sr=24000;
 auto make=[&](int variant){
  auto patch=universal23::directGraph();
  if(variant==2){std::string why;controls50::change(patch,"native51.mode.0.pickupScale",.12,&why);
   controls50::change(patch,"native51.mode.0.excitationScale",.10,&why);}
  auto x=universal23::Instrument::prepare(patch,sr);if(!x)return std::vector<float>{};
  x->noteOn(57,.85f);std::vector<float>out;
  for(int i=0;i<sr;i++){
   if(variant==1&&i==sr/3){x->setHostParameterNormalized(80,.12);x->setHostParameterNormalized(88,.10);}
   if(variant==1&&i==sr*2/3)x->clearNativeModeAutomation();
   if(i==sr*3/4)x->noteOff(57);
   auto y=x->processOne();out.push_back(y.left);
  }
  return out;
 };
 std::vector<float>a=make(0),b=make(1),c=make(2);
 if(a.empty()||b.empty()||c.empty())return 3;
 if(!wav(base+"_original.wav",a,sr)||!wav(base+"_live.wav",b,sr)||!wav(base+"_saved.wav",c,sr))return 4;
 double dif=0;for(size_t i=0;i<a.size();i++)dif+=std::abs(double(a[i]-b[i]));
 std::cout<<"original_live_absdiff="<<dif<<" sampleRate="<<sr<<"\n";
 return dif>1.e-4?0:5;
}
