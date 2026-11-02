#include "ProductionInverse48.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>
namespace fs=std::filesystem;
using namespace panorama;
namespace {
uint32_t u32(const unsigned char*b){return uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);}
uint16_t u16(const unsigned char*b){return uint16_t(b[0])|(uint16_t(b[1])<<8);}
void put32(std::ofstream&f,uint32_t x){for(int i=0;i<4;i++)f.put(char(x>>(i*8)));}
void put16(std::ofstream&f,uint16_t x){for(int i=0;i<2;i++)f.put(char(x>>(i*8)));}
bool wavOut(const fs::path&p,const std::vector<float>&a,int rate){
 std::ofstream f(p,std::ios::binary);if(!f)return false;
 f.write("RIFF",4);put32(f,uint32_t(36+a.size()*4));f.write("WAVEfmt ",8);put32(f,16);put16(f,3);put16(f,1);put32(f,uint32_t(rate));put32(f,uint32_t(rate*4));put16(f,4);put16(f,32);f.write("data",4);put32(f,uint32_t(a.size()*4));for(float x:a)f.write((char*)&x,4);return bool(f);
}
bool wavIn(const fs::path&p,std::vector<float>&audio,int&rate){
 std::ifstream f(p,std::ios::binary);if(!f)return false;
 char head[12];if(!f.read(head,12)||std::string(head,4)!="RIFF"||std::string(head+8,4)!="WAVE")return false;
 int codec=0,bits=0,ch=0;rate=0;std::vector<unsigned char>bytes;
 while(f){unsigned char hdr[8];if(!f.read((char*)hdr,8))break;
  uint32_t sz=u32(hdr+4);if(sz>192000000u)return false;std::string tag((char*)hdr,4);
  if(tag=="fmt "){if(sz<16||sz>4096)return false;std::vector<unsigned char>fmt(sz);if(!f.read((char*)fmt.data(),sz))return false;codec=u16(fmt.data());ch=u16(fmt.data()+2);rate=int(u32(fmt.data()+4));bits=u16(fmt.data()+14);}
  else if(tag=="data"){bytes.resize(sz);if(!f.read((char*)bytes.data(),sz))return false;}
  else f.seekg(sz,std::ios::cur);
  if(sz%2)f.seekg(1,std::ios::cur);
 }
 if(rate<8000||rate>96000||ch<1||ch>2||!((codec==1&&(bits==16||bits==24))||(codec==3&&bits==32)))return false;
 const size_t b=bits/8,frame=b*size_t(ch);if(bytes.empty()||bytes.size()%frame)return false;
 const size_t len=bytes.size()/frame;if(len<size_t(rate)/3||len>size_t(rate)*600)return false;
 audio.resize(len);
 for(size_t i=0;i<len;i++){
  double val=0;for(int c=0;c<ch;c++){
   const auto*p=bytes.data()+i*frame+b*size_t(c);double one=0;
   if(codec==3){float f32;std::memcpy(&f32,p,4);one=f32;}
   else if(bits==16)one=int16_t(u16(p))/32768.;
   else{int v=int(p[0]|(p[1]<<8)|(p[2]<<16));if(v&0x800000)v|=~0xffffff;one=v/8388608.;}
   val+=one;
  }
  audio[i]=float(val/ch);if(!std::isfinite(audio[i]))return false;
 }return true;
}
void output(const production48::LongAnalysis&a,const fs::path&folder){
 fs::create_directories(folder);std::ofstream q(folder/"recording_quality.csv");
 q<<"duration_s,rms,peak,clipped_fraction,silent_fraction,noise_floor_rms,estimated_dynamic_range_db,questionable\n";
 const auto&v=a.quality;q<<std::setprecision(10)<<v.duration<<','<<v.rms<<','<<v.peak<<','<<v.clippedFraction<<','<<v.silentFraction<<','<<v.noiseFloorRms<<','<<v.estimatedDynamicRangeDb<<','<<int(v.questionable)<<'\n';
 std::ofstream s(folder/"long_sections.csv");s<<"from_sec,to_sec,rms,centroid_hz,brightness\n";
 for(auto&x:a.sections)s<<x.fromSec<<','<<x.toSec<<','<<x.rms<<','<<x.centroidHz<<','<<x.brightness<<'\n';
 std::ofstream g(folder/"gesture_candidates.csv");g<<"kind,time_s,confidence,magnitude,rate_hz\n";
 for(auto&x:a.gestures)g<<deep46::gestureName(x.kind)<<','<<x.timeSec<<','<<x.confidence<<','<<x.magnitude<<','<<x.rateHz<<'\n';
 std::ofstream info(folder/"limitations.txt");info<<"Uncalibrated real-recording descriptors, not proven gesture labels.\n";
 info<<"Unknown microphone/room effects and geometry/material ambiguity remain.\n";
 info<<"Estimated noise floor from quiet recording segments may contain real musical tails.\n";
}
}
int main(int argc,char**argv){
 if(argc==4&&std::string(argv[1])=="--analyze"){
  std::vector<float>audio;int rate=0;
  if(!wavIn(argv[2],audio,rate)){std::cerr<<"Unsupported WAV (PCM16/24 or float32, 1-2 ch, <=600s)\n";return 2;}
  auto result=production48::analyzeLong(audio,rate);
  if(!result.error.empty()){std::cerr<<result.error<<'\n';return 3;}
  output(result,argv[3]);std::cout<<"ANALYSIS48_OK seconds="<<result.quality.duration<<" sections="<<result.sections.size()<<" gestures="<<result.gestures.size()<<" clipped_fraction="<<result.quality.clippedFraction<<'\n';return 0;
 }
 if(argc==3&&std::string(argv[1])=="--fit-demo"){
  const fs::path dir=argv[2];fs::create_directories(dir);
  const int rate=22050;
  production48::Freeform free;free.controls={{.15,.0,.0,.0}};
  fem36::Model truth;
  if(!production48::buildFreeform(free,truth.mesh))return 4;
  truth.meshYoung=6.5e8;truth.meshDensity=1.0e5;
  fem36::Model seed;production48::Freeform original;
  if(!production48::buildFreeform(original,seed.mesh))return 4;
  seed.meshYoung=5e8;seed.meshDensity=1e5;
  std::vector<inverse36::Take>takes;
  for(int j=0;j<3;j++){
   inverse36::Take t;t.training=j!=2;t.midi=40+j*5;t.velocity=.7f;
   auto signal=inverse36::render(truth,rate,t.midi,t.velocity,768);
   if(signal.size()!=768)return 4;
   t.audio.resize(signal.size());
   for(size_t k=0;k<signal.size();k++){
    // Simulated microphone gain mismatch, low noise and 3-sample onset shift.
    const auto sample=k>=3?signal[k-3]:0.f;
    t.audio[k]=float((.7+.08*j)*sample+.000001*std::sin(k*1.3+j));
   }
   wavOut(dir/("recorded_take_"+std::to_string(j)+".wav"),t.audio,rate);
   takes.push_back(std::move(t));
  }
  auto result=production48::fitSurfaceMaterial(takes,rate,seed,24);
  if(!result.error.empty()){std::cerr<<result.error<<'\n';return 5;}
  std::ofstream rank(dir/"shape_material_ranking.csv");
  rank<<"rank,training_error,holdout_error,young_pa,density,warp0,warp1,warp2,warp3\n";
  for(size_t i=0;i<result.ranked.size();i++){
   const auto&x=result.ranked[i];rank<<i+1<<','<<x.trainError<<','<<x.holdoutError<<','<<x.material.meshYoung<<','<<x.material.meshDensity;
   for(double k:x.surface.controls)rank<<','<<k;
   rank<<'\n';
   std::ofstream(dir/("candidate_"+std::to_string(i+1)+".fem36"))<<fem36::serialize(x.material);
  }
  std::ofstream(dir/"synthetic_truth_for_validation.fem36")<<fem36::serialize(truth);
  std::ofstream(dir/"inverse_limitations.txt")<<"Synthetic, aligned within 24 samples; assumes known MIDI and base mesh. Non-unique material/shape.\n";
  std::cout<<"FIT48_OK evaluated="<<result.evaluations<<" best_train="<<result.ranked.front().trainError<<" heldout="<<result.ranked.front().holdoutError<<" unique=false\n";
  return 0;
 }
 if(argc==3&&std::string(argv[1])=="--demo"){
  const fs::path folder=argv[2];fs::create_directories(folder);
  const int sr=8000;constexpr double pi=3.14159265358979323846;
  std::vector<float>a(size_t(sr)*32);double phase=0;
  for(size_t i=0;i<a.size();i++){
   double t=double(i)/sr,hz=190+((t>14&&t<15)?30*(t-14):t>=15?30:0)+2.5*std::sin(2*pi*5.4*t);
   phase+=2*pi*hz/sr;a[i]=float(.17*std::sin(phase)+.17*(t<19?.12:.4)*std::sin(2*phase)+.001*std::sin(i*1.618));
  }
  if(!wavOut(folder/"synthetic_long_performance.wav",a,sr))return 4;
  const auto result=production48::analyzeLong(a,sr);
  if(!result.error.empty())return 5;output(result,folder);
  auto graph=universal23::genuineNativeGraph();auto p=production48::parameterRegistry(graph);
  std::ofstream registry(folder/"parameter_registry.csv");registry<<"stable_id,name,unit,min,max,live,requires_rebuild\n";
  for(auto&v:p)registry<<v.id<<','<<v.name<<','<<v.unit<<','<<v.min<<','<<v.max<<','<<v.live<<','<<v.requiresRebuild<<'\n';
  std::ofstream(folder/"genuine_original.pgraph")<<universal23::serialize(graph);
  production48::Freeform f;f.controls={{.12,-.06,.07,.08}};fem36::Mesh mesh;
  if(!production48::buildFreeform(f,mesh))return 6;
  fem36::Model m;m.mesh=std::move(mesh);
  std::ofstream(folder/"freeform_FEM36.fem36")<<fem36::serialize(m);
  std::cout<<"DEMO48_OK segments="<<result.sections.size()<<" controls="<<p.size()<<"\n";return 0;
 }
 std::cerr<<"usage: panoramize_production48 --analyze input.wav output_dir | --demo output_dir | --fit-demo output_dir\n";return 1;
}
