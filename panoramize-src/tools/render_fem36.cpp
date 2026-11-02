#include "MeshInverse36.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
namespace {
void u16(std::ofstream&f,unsigned x){f.put(char(x));f.put(char(x>>8));}
void u32(std::ofstream&f,unsigned x){for(int i=0;i<4;i++)f.put(char(x>>(8*i)));}
}
int main(int argc,char**argv){
 if(argc!=3){std::cerr<<"usage: panoramize_render_fem36 model.fem36 output.wav\n";return 1;}
 std::ifstream in(argv[1]);if(!in)return 2;
 const std::string text((std::istreambuf_iterator<char>(in)),{});
 panorama::fem36::Model m;
 if(!panorama::fem36::deserialize(text,m))return 3;
 std::string err;auto instrument=panorama::fem36::Instrument::prepare(m,48000,&err);
 if(!instrument){std::cerr<<err<<'\n';return 4;}
 std::vector<float> samples(48000*3/2);
 for(size_t i=0;i<samples.size();i++){
  if(i==4800)instrument->noteOn(40,.7f);
  if(i==19200)instrument->noteOn(45,.7f);
  if(i==28800)instrument->noteOn(50,.7f);
  if(i==48000){instrument->noteOff(40);instrument->noteOff(45);instrument->noteOff(50);}
  samples[i]=instrument->processOne();if(!std::isfinite(samples[i]))return 5;
 }
 std::ofstream out(argv[2],std::ios::binary);if(!out)return 6;
 const unsigned bytes=unsigned(samples.size()*4);
 out.write("RIFF",4);u32(out,36+bytes);out.write("WAVEfmt ",8);u32(out,16);u16(out,3);u16(out,1);u32(out,48000);u32(out,48000*4);u16(out,4);u16(out,32);
 out.write("data",4);u32(out,bytes);
 for(float x:samples)out.write((const char*)&x,4);
 return out?0:7;
}
