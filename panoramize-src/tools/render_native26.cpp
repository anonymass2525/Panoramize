#include "NativeState26.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
using namespace panorama;
static void u16(std::ofstream& f,uint16_t x){char b[2]={char(x&255),char(x>>8)};f.write(b,2);}
static void u32(std::ofstream& f,uint32_t x){char b[4]={char(x&255),char((x>>8)&255),char((x>>16)&255),char((x>>24)&255)};f.write(b,4);}
int main(int argc,char**argv){
 if(argc<2){std::cerr<<"usage: panoramize_native26_render OUTPUT.wav [PROGRAM 0..5] [PATCH.native26|-] [EXPORT.native26]\n";return 1;}
 const int variant=argc>2?std::clamp(std::atoi(argv[2]),0,5):0;
 auto graph=native26::example(variant);
 std::string why;
 if(argc>3 && std::string(argv[3])!="-"){std::ifstream file(argv[3]);std::string saved((std::istreambuf_iterator<char>(file)),{});
  if(!native26::deserialize(saved,graph,&why)){std::cerr<<why<<"\n";return 2;}}
 if(argc>4){std::ofstream patch(argv[4]);patch<<native26::serialize(graph);}
 auto synth=native26::Instrument::prepare(graph,48000,&why);
 if(!synth){std::cerr<<why<<"\n";return 2;}
 constexpr int sr=48000;constexpr int frames=sr*3;constexpr int nChannels=2;
 std::ofstream f(argv[1],std::ios::binary);if(!f)return 3;
 f.write("RIFF",4);u32(f,36+frames*4);f.write("WAVEfmt ",8);u32(f,16);u16(f,1);u16(f,nChannels);u32(f,sr);u32(f,sr*4);u16(f,4);u16(f,16);
 f.write("data",4);u32(f,frames*4);
 synth->noteOn(48+variant*2,.85f);
 for(int i=0;i<frames;i++){
  if(i==sr*2)synth->noteOff(48+variant*2);
  const double fade=std::min(1.,std::min(i/(sr*.02),(frames-i)/(sr*.09)));
  const double x=std::clamp(double(synth->processMono())*.75*fade,-.99,.99);
  auto pcm=int16_t(std::lrint(x*32767.));u16(f,uint16_t(pcm));u16(f,uint16_t(pcm));
 }
 std::cout<<graph.name<<" => "<<argv[1]<<" energy residual "<<synth->energyResidual()<<"\n";
}
