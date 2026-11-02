#include "UniversalInstrument23.h"
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <string>
using namespace panorama::universal23;
static void le16(std::ofstream& f,uint16_t x){for(int i=0;i<2;i++)f.put(char((x>>(8*i))&255));}
static void le32(std::ofstream& f,uint32_t x){for(int i=0;i<4;i++)f.put(char((x>>(8*i))&255));}
int main(int argc,char**argv){
 if(argc!=3 && argc!=4){std::cerr<<"Usage: panoramize_render_native27 <output.wav> <patch.pgraph> [input.pgraph]\n";return 2;}
 auto graph=directGraph();
 if(argc==4){
  std::ifstream recipe(argv[3],std::ios::binary);if(!recipe){std::cerr<<"Input .pgraph not found";return 2;}
  const std::string data((std::istreambuf_iterator<char>(recipe)),std::istreambuf_iterator<char>());
  std::string parseError;if(!deserialize(data,graph,&parseError)){std::cerr<<parseError;return 2;}
 }
 std::string err;auto instrument=Instrument::prepare(graph,48000,&err);if(!instrument){std::cerr<<err;return 1;}
 constexpr uint32_t frames=48000*5,rate=48000,ch=2;
 std::ofstream wav(argv[1],std::ios::binary);if(!wav)return 2;
 wav.write("RIFF",4);le32(wav,36+frames*ch*2);wav.write("WAVEfmt ",8);le32(wav,16);le16(wav,1);le16(wav,ch);
 le32(wav,rate);le32(wav,rate*ch*2);le16(wav,4);le16(wav,16);wav.write("data",4);le32(wav,frames*ch*2);
 const int notes[]={48,55,60,67};
 for(uint32_t i=0;i<frames;i++){
  for(int j=0;j<4;j++){
   if(i==uint32_t(j*48000)){instrument->noteOn(notes[j],.72f);}
   if(i==uint32_t(j*48000+36000))instrument->noteOff(notes[j]);
  }
  const auto s=instrument->processOne();const double v=std::tanh(double(s.left)*.85);
  const auto pcm=int16_t(std::lround(std::clamp(v,-1.,1.)*32767.));le16(wav,uint16_t(pcm));le16(wav,uint16_t(pcm));
 }
 std::ofstream patch(argv[2],std::ios::binary);patch<<serialize(graph);
 std::cout<<"Rendered 5s editable direct-state synthesizer\n";
}
