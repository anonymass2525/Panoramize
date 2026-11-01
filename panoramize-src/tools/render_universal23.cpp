// Play an editable .pgraph in the same new engine used by the VST3.
#include "UniversalInstrument23.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
namespace {
void word(std::ofstream& out,uint16_t x){for(int i=0;i<2;i++)out.put(char((x>>(i*8))&255));}
void dword(std::ofstream& out,uint32_t x){for(int i=0;i<4;i++)out.put(char((x>>(i*8))&255));}
void wav(const std::string& name,const std::vector<panorama::Sample>& samples,int sr){
 std::ofstream out(name,std::ios::binary);
 if(!out)throw std::runtime_error("output file open failure");
 const uint32_t bytes=uint32_t(samples.size())*2u*3u;
 out.write("RIFF",4);dword(out,36+bytes);out.write("WAVEfmt ",8);dword(out,16);
 word(out,1);word(out,2);dword(out,uint32_t(sr));dword(out,uint32_t(sr*6));word(out,6);word(out,24);
 out.write("data",4);dword(out,bytes);
 for(const auto& sample:samples)for(float x:{sample.left,sample.right}){
  const auto v=int32_t(std::lrintf(std::clamp(x,-1.f,1.f)*8388607.f));
  for(int b=0;b<3;b++)out.put(char((uint32_t(v)>>(8*b))&255));
 }
}
}
int main(int argc,char**argv){
 if(argc<3){std::cerr<<"Usage: panorama_universal23_render patch.pgraph out.wav [seconds] [note]\n";return 2;}
 std::ifstream in(argv[1],std::ios::binary);
 if(!in){std::cerr<<"Unable to open graph\n";return 2;}
 const std::string data((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
 panorama::universal23::Graph graph;std::string error;
 if(!panorama::universal23::deserialize(data,graph,&error)){std::cerr<<error<<"\n";return 3;}
 auto instrument=panorama::universal23::Instrument::prepare(graph,48000,&error);
 if(!instrument){std::cerr<<error<<"\n";return 3;}
 const double seconds=argc>3?std::clamp(std::stod(argv[3]),.1,30.):3.;
 const int note=argc>4?std::clamp(std::stoi(argv[4]),24,108):60;
 const int frames=int(seconds*48000);std::vector<panorama::Sample> output(static_cast<size_t>(frames));
 const int noteOff=int(48000*seconds*.67);
 instrument->noteOn(note,.87f);
 double sq=0.;float peak=0.f;
 for(int i=0;i<frames;i++){
  if(i==noteOff)instrument->noteOff(note);
  output[size_t(i)]=instrument->processOne();
  const auto s=output[size_t(i)];sq+=double(s.left)*s.left+double(s.right)*s.right;
  peak=std::max(peak,std::max(std::abs(s.left),std::abs(s.right)));
 }
 wav(argv[2],output,48000);
 std::cout<<graph.name<<" | "<<graph.nodes.size()<<" nodes | RMS "<<std::sqrt(sq/(2*frames))<<" | peak "<<peak<<"\n";
}
