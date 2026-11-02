#include "CoupledGuitar35.h"
#include "UniversalInstrument23.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
using namespace panorama;
int main(int argc,char**argv){
 if(argc<3){std::cerr<<"usage: panoramize_coupled35_demo output.wav output.pgraph [dry|coupled|palm|slide]\n";return 1;}
 std::string mode=argc>3?argv[3]:"coupled";
 auto g=universal23::coupledGraph();
 if(mode=="dry")g.coupled35.bridgeCoupling=0;
 else if(mode!="coupled"&&mode!="palm"&&mode!="slide")return 2;
 std::ofstream patch(argv[2]);patch<<universal23::serialize(g);patch.close();
 std::string error;auto instrument=universal23::Instrument::prepare(g,48000,&error);
 if(!instrument){std::cerr<<error<<'\n';return 3;}
 std::vector<std::int32_t> pcm;pcm.reserve(48000*2);
 constexpr int rate=48000,seconds=2;
 constexpr std::array<int,6> notes={40,45,50,55,59,64};
 for(int i=0;i<rate*seconds;i++){
  if(i==rate/10){for(int midi:notes)instrument->noteOn(midi,.65f);}
  if(mode=="palm"&&i==rate/2)instrument->articulationControl(22,.92f);
  if(mode=="slide"&&i==rate/2){instrument->articulationControl(20,.99f);instrument->noteOn(66,.8f);}
  if(i==rate*3/2){for(int midi:notes)instrument->noteOff(midi);}
  const auto s=instrument->processOne();
  pcm.push_back(std::int32_t(std::clamp(s.left*8200000.f,-8388607.f,8388607.f)));
 }
 auto out=std::ofstream(argv[1],std::ios::binary);
 const auto u32=[&](std::uint32_t v){for(int i=0;i<4;i++)out.put(char((v>>(i*8))&255));};
 const auto u16=[&](std::uint16_t v){out.put(char(v&255));out.put(char((v>>8)&255));};
 out.write("RIFF",4);u32(36+std::uint32_t(pcm.size()*3));out.write("WAVEfmt ",8);u32(16);u16(1);u16(1);u32(rate);u32(rate*3);u16(3);u16(24);
 out.write("data",4);u32(std::uint32_t(pcm.size()*3));for(auto x:pcm){out.put(char(x&255));out.put(char((x>>8)&255));out.put(char((x>>16)&255));}
 std::cout<<"rendered "<<argv[1]<<" ("<<pcm.size()<<" samples)\n";
}
