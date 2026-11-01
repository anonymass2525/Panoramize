#include "GraphInverse.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
using namespace panorama;
namespace {
uint16_t read16(const unsigned char*p){return uint16_t(p[0]|(uint16_t(p[1])<<8));}
uint32_t read32(const unsigned char*p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
void put16(std::ofstream&f,uint16_t v){f.put(char(v&255));f.put(char((v>>8)&255));}
void put32(std::ofstream&f,uint32_t v){for(int k=0;k<4;k++)f.put(char((v>>(k*8))&255));}
bool loadWav(const std::string&path,std::vector<float>&samples,int&sampleRate){
 std::ifstream f(path,std::ios::binary);unsigned char head[12]{};f.read((char*)head,12);
 if(!f||std::string((char*)head,4)!="RIFF"||std::string((char*)head+8,4)!="WAVE")return false;
 uint16_t format=0,channels=0,bits=0;uint32_t rate=0;
 std::vector<unsigned char> audio;
 while(f){
  unsigned char h[8]{};f.read((char*)h,8);if(!f)break;
  const uint32_t bytes=read32(h+4);
  if(bytes>64*1024*1024)return false;
  const std::string chunk((char*)h,4);
  if(chunk=="fmt "){
   if(bytes<16||bytes>256)return false;
   std::vector<unsigned char>b(bytes);f.read((char*)b.data(),bytes);if(!f)return false;
   format=read16(b.data());channels=read16(b.data()+2);rate=read32(b.data()+4);bits=read16(b.data()+14);
  }else if(chunk=="data"){
   audio.resize(bytes);f.read((char*)audio.data(),bytes);if(!f)return false;
  }else f.seekg(bytes,std::ios::cur);
  if(bytes&1u)f.seekg(1,std::ios::cur);
 }
 if(format!=1||(channels!=1&&channels!=2)||(bits!=16&&bits!=24)||rate<8000||rate>192000)return false;
 const int stride=(bits/8)*channels;
 if(audio.size()<(size_t)stride*4096)return false;
 sampleRate=int(rate);
 const size_t frameCount=std::min(audio.size()/(size_t)stride,size_t(sampleRate));
 samples.assign(frameCount,0.f);
 for(size_t i=0;i<frameCount;i++){
  double sum=0.;
  for(int ch=0;ch<channels;ch++){
   const unsigned char*q=audio.data()+i*stride+ch*(bits/8);
   int n=0;
   if(bits==16)n=int(int16_t(read16(q)));
   else {n=int(q[0])|(int(q[1])<<8)|(int(q[2])<<16);if(n&0x800000)n|=~0xffffff;}
   sum+=double(n)/(bits==16?32768.:8388608.);
  }
  samples[i]=float(sum/channels);
 }
 return true;
}
bool saveDemoWav(const std::string&path,const Patch&p,int midiNote,int rate){
 const int frames=int(rate*2.5);
 Synth s;s.prepare(rate);s.setPatch(p);if(!s.graphValid())return false;
 s.noteOn(midiNote,.8f);
 std::ofstream f(path,std::ios::binary);if(!f)return false;
 const uint32_t bytes=uint32_t(frames*4);
 f.write("RIFF",4);put32(f,36+bytes);f.write("WAVEfmt ",8);put32(f,16);
 put16(f,1);put16(f,2);put32(f,uint32_t(rate));put32(f,uint32_t(rate*4));put16(f,4);put16(f,16);
 f.write("data",4);put32(f,bytes);
 for(int n=0;n<frames;n++){
  if(n==int(.75*rate))s.noteOff(midiNote);
  const auto out=s.processOne();
  for(float x:{out.left,out.right}){
   const int16_t sample=int16_t(std::lrint(std::clamp(x,-.999f,.999f)*32767.f));
   put16(f,uint16_t(sample));
  }
 }
 return bool(f);
}
}
int main(int argc,char**argv){
 if(argc<3||argc>8){
  std::cerr<<"Usage: panorama_inverse_graph isolated-note.wav generated_graph.json [midiNote 60] [preview.wav] [budget 180] [seed 445518388] [proposals 4]\n";
  return 1;
 }
 int midiNote=60;
 if(argc>=4)try{midiNote=std::stoi(argv[3]);}catch(...){return 2;}
 std::vector<float> input;int sampleRate=0;
 if(!loadWav(argv[1],input,sampleRate)){
  std::cerr<<"Unsupported WAV. Use 16/24-bit PCM, mono/stereo, <= 1 second analyzed.\n";return 3;
 }
 GraphInverseSettings settings{};
 if(argc>=6)try{settings.maxEvaluations=std::stoi(argv[5]);}catch(...){return 2;}
 if(argc>=7)try{settings.seed=std::uint32_t(std::stoul(argv[6]));}catch(...){return 2;}
 int desired=1;
 if(argc>=8)try{desired=std::stoi(argv[7]);}catch(...){return 2;}
 const auto portfolio=inferGraphPortfolioFromAudioEx(
     input.data(),int(input.size()),sampleRate,midiNote,settings,desired);
 const auto&result=portfolio.best;
 if(!result.success){std::cerr<<"No usable isolated note for graph inference\n";return 4;}
 std::ofstream file(argv[2]);file<<graphInverseJson(result);if(!file)return 5;
 std::cout<<"Generated 9-slot PANORAMA patch. Evaluated "<<result.evaluated
          <<" candidates across "<<result.novelTopologiesEvaluated<<" distinct topologies ("
          <<result.mutatedEvaluated<<" mutations); feature distance="<<result.distance<<"; MIDI="<<midiNote<<"\n";
 if(argc>=8){
  const std::string base=argv[2];
  const auto dot=base.find_last_of('.');
  const auto slash=base.find_last_of("/\\");
  const bool ext=(dot!=std::string::npos&&(slash==std::string::npos||dot>slash));
  const std::string stem=ext?base.substr(0,dot):base;
  const std::string suffix=ext?base.substr(dot):".json";
  std::ofstream manifest(stem+"_portfolio.csv");
  manifest<<"rank,graph_file,feature_distance,structural_novelty,connected_nodes,topology_signature\n";
  for(size_t i=0;i<portfolio.proposals.size();i++){
   const auto&proposal=portfolio.proposals[i];
   const std::string fileName=i==0?base:stem+"_alt"+std::to_string(i+1)+suffix;
   GraphInverseResult one=result;
   one.patch=proposal.patch;one.distance=proposal.distance;
   std::ofstream out(fileName);out<<graphInverseJson(one);
   if(!out){std::cerr<<"Cannot write "<<fileName<<"\n";return 5;}
   manifest<<i+1<<","<<fileName<<","<<proposal.distance<<","<<proposal.structuralNovelty
           <<","<<proposal.activeNodes<<","<<proposal.topologySignature<<"\n";
   if(argc>=5 && i>0){
    const std::string w=argv[4];const auto wd=w.rfind('.');
    const std::string alt=wd!=std::string::npos?w.substr(0,wd)+"_alt"+std::to_string(i+1)+w.substr(wd):
           w+"_alt"+std::to_string(i+1)+".wav";
    if(!saveDemoWav(alt,proposal.patch,midiNote,sampleRate))return 6;
   }
  }
  if(!manifest)return 5;
  std::cout<<"Portfolio: "<<portfolio.proposals.size()<<" distinct output-connected graphs saved.\n";
 }
 if(argc>=5){
  if(!saveDemoWav(argv[4],result.patch,midiNote,sampleRate))return 6;
  std::cout<<"Rendered a preview WAV of the generated sound: "<<argv[4]<<"\n";
 }
 std::cout<<"NOTE: Heuristic topology and parameter search, not unique instrument recovery.\n";
 return 0;
}
