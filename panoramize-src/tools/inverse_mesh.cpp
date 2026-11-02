#include "AdvancedAcoustics.h"
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
using namespace panorama;
namespace {
uint16_t u16(const unsigned char *p){return uint16_t(p[0]|(uint16_t(p[1])<<8));}
uint32_t u32(const unsigned char *p){return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;}
}
int main(int argc,char**argv){
 if(argc!=2){std::cerr<<"Usage: panorama_inverse <PCM WAV (16/24-bit, mono/stereo)>\n";return 1;}
 std::ifstream f(argv[1],std::ios::binary);
 if(!f){std::cerr<<"Cannot open WAV\n";return 2;}
 unsigned char header[12]{};f.read((char*)header,12);
 if(!f||std::string((char*)header,4)!="RIFF"||std::string((char*)header+8,4)!="WAVE"){
  std::cerr<<"RIFF/WAVE only\n";return 3;
 }
 uint16_t format=0,channels=0,bits=0;uint32_t sampleRate=0;
 std::vector<unsigned char> raw;
 while(f){
  unsigned char ch[8]{};f.read((char*)ch,8);if(!f)break;
  const uint32_t bytes=u32(ch+4);if(bytes>128*1024*1024){std::cerr<<"Unreasonable chunk size\n";return 4;}
  const std::string id((char*)ch,4);
  if(id=="fmt "){
   if(bytes<16||bytes>256){std::cerr<<"Bad fmt chunk\n";return 4;}
   std::vector<unsigned char>b(bytes);f.read((char*)b.data(),bytes);if(!f)return 4;
   format=u16(b.data());channels=u16(b.data()+2);sampleRate=u32(b.data()+4);bits=u16(b.data()+14);
  }else if(id=="data"){
   raw.resize(bytes);f.read((char*)raw.data(),bytes);if(!f)return 4;
  }else f.seekg(bytes,std::ios::cur);
  if(bytes%2)f.seekg(1,std::ios::cur);
 }
 if(format!=1||(channels!=1&&channels!=2)||(bits!=16&&bits!=24)||sampleRate<8000||sampleRate>192000||raw.empty()){
  std::cerr<<"Only 16/24-bit integer PCM mono/stereo, 8..192 kHz supported\n";return 5;
 }
 const size_t stride=size_t(bits/8)*channels;
 const size_t frames=std::min(raw.size()/stride,size_t(sampleRate));
 std::vector<float> pcm(frames);
 for(size_t i=0;i<frames;i++){
  double x=0;
  for(int ch=0;ch<channels;ch++){
   const auto *q=raw.data()+i*stride+size_t(ch)*(bits/8);
   int val=0;
   if(bits==16)val=int16_t(u16(q));
   else {val=int(q[0])|(int(q[1])<<8)|(int(q[2])<<16);if(val&0x800000)val|=~0xffffff;}
   x+=double(val)/double(bits==16?32768:8388608);
  }
  pcm[i]=float(x/channels);
 }
 const auto res=estimateMeshFromAudio(pcm.data(),int(pcm.size()),sampleRate);
 if(res.evaluated==0){std::cerr<<"No usable audio signal\n";return 6;}
 std::cout<<"PANORAMA restricted inverse / evaluated "<<res.evaluated<<" templates\n";
 const char*names[]={"Transverse string","Closed ring","3x3 membrane"};
 for(size_t i=0;i<res.best.size();i++){
  const auto &c=res.best[i];
  std::cout<<i+1<<": "<<names[c.shape]<<" | baseHz="<<c.fundamental
           <<" | relative tension="<<c.tension<<" | feature error="<<c.error<<"\n";
 }
 std::cout<<"Warning: this is a limited dictionary fit, not unique instrument identification.\n";
}
