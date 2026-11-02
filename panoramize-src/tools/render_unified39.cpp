#include "UnifiedAcousticPhysical39.h"
#include "UniversalInstrument23.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
namespace fs=std::filesystem;
using namespace panorama::unified39;
namespace {
void u16(std::ofstream&f,std::uint32_t n){f.put(char(n));f.put(char(n>>8));}
void u32(std::ofstream&f,std::uint32_t n){for(int i=0;i<4;i++)f.put(char(n>>(i*8)));}
bool wav(const fs::path&path,const std::vector<float>&v,int sr){
 std::ofstream f(path,std::ios::binary);if(!f)return false;
 std::uint32_t bytes=std::uint32_t(v.size()*4);
 f.write("RIFF",4);u32(f,36+bytes);f.write("WAVEfmt ",8);u32(f,16);u16(f,3);u16(f,1);
 u32(f,std::uint32_t(sr));u32(f,std::uint32_t(sr*4));u16(f,4);u16(f,32);
 f.write("data",4);u32(f,bytes);
 for(float x:v)f.write(reinterpret_cast<const char*>(&x),4);
 return bool(f);
}
int render(Recipe r,const fs::path&path,double sec,int rate){
 std::string err;auto i=Instrument::prepare(r,rate,&err);if(!i){std::cerr<<err<<'\n';return 2;}
 i->noteOn(60,.75f);
 std::vector<float> v(size_t(rate*sec));double peak=0,rms=0;
 for(size_t n=0;n<v.size();n++){
  if(n==size_t(rate*.55))i->pitchBend(2.f);
  if(n==size_t(rate*.75))i->modWheel(.6f);
  float x=i->processOne();v[n]=x;peak=std::max(peak,double(std::abs(x)));rms+=double(x)*x;
  if(!std::isfinite(x)){std::cerr<<"nonfinite output\n";return 3;}
 }
 if(!wav(path,v,rate))return 4;
 std::cout<<path.filename().string()<<" peak="<<peak<<" rms="<<std::sqrt(rms/v.size())
          <<" physicalEnergy="<<i->physicalEnergy()<<" physActiveWork="<<i->physicalExternalWork()
          <<" stateActiveWork="<<i->acousticActiveWork()<<" maxEffort="<<i->maximumEffort()
          <<" maxForce="<<i->maximumForce()<<" clipped="<<i->limitedCount()<<'\n';
 return 0;
}
}
int main(int argc,char**argv){
 if(argc==2&&std::string(argv[1])=="--self-test"){
  auto r=example();auto str=serialize(r);Recipe out;std::string e;
  if(!deserialize(str,out,&e)||serialize(out)!=str)return 2;
  auto i=Instrument::prepare(out,48000,&e);if(!i){std::cerr<<e;return 3;}
  i->noteOn(60,.8f);for(int n=0;n<512;n++)if(!std::isfinite(i->processOne()))return 4;
  std::cout<<"unified39_cli_self_test_pass\n";return 0;
 }
 if(argc==3&&std::string(argv[1])=="--demo"){
  fs::path d=argv[2];fs::create_directories(d);auto r=example();
  std::ofstream(d/"all_bidirectional.unified39")<<serialize(r);
  auto g=panorama::universal23::unifiedPhysicalGraph();
  std::ofstream(d/"playable_vst3_source.pgraph")<<panorama::universal23::serialize(g);
  if(render(r,d/"all_bidirectional.wav",1.35,48000))return 5;
  for(auto&l:r.links){l.mechanicalToAcoustic=false;l.acousticToMechanical=true;}
  std::ofstream(d/"state_to_physical.unified39")<<serialize(r);
  if(render(r,d/"state_to_physical.wav",1.35,48000))return 6;
  for(auto&l:r.links){l.mechanicalToAcoustic=true;l.acousticToMechanical=false;}
  std::ofstream(d/"physical_to_state.unified39")<<serialize(r);
  if(render(r,d/"physical_to_state.wav",1.35,48000))return 7;
  r.links.clear();std::ofstream(d/"unlinked.unified39")<<serialize(r);
  return render(r,d/"unlinked.wav",1.35,48000);
 }
 if(argc<3||argc>5){std::cerr<<"usage: panoramize_unified39 --demo DIR | input.unified39 output.wav [seconds] [rate]\n";return 1;}
 try{
  double seconds=argc>=4?std::stod(argv[3]):1.5;int sr=argc==5?std::stoi(argv[4]):48000;
  if(!std::isfinite(seconds)||seconds<.01||seconds>8||sr<22050||sr>192000)return 1;
  std::ifstream input(argv[1]);if(!input)return 1;
  std::string data((std::istreambuf_iterator<char>(input)),std::istreambuf_iterator<char>());
  Recipe r;std::string err;if(!deserialize(data,r,&err)){std::cerr<<err<<'\n';return 2;}
  return render(r,argv[2],seconds,sr);
 }catch(...){return 1;}
}
