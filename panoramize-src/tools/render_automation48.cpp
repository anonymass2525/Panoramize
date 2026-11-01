#include "GenuineNative47.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
using namespace panorama;
namespace fs=std::filesystem;
static bool wav(const fs::path&p,const std::vector<float>&samples,int sr){
 std::ofstream f(p,std::ios::binary);if(!f)return false;
 const auto put16=[&](uint32_t v){f.put(char(v));f.put(char(v>>8));};
 const auto put32=[&](uint32_t v){for(int j=0;j<4;j++)f.put(char(v>>(8*j)));};
 f.write("RIFF",4);put32(uint32_t(36+4*samples.size()));f.write("WAVEfmt ",8);
 put32(16);put16(3);put16(1);put32(sr);put32(sr*4);put16(4);put16(32);
 f.write("data",4);put32(uint32_t(4*samples.size()));
 for(float x:samples)f.write(reinterpret_cast<const char*>(&x),4);
 return bool(f);
}
int main(int argc,char**argv){
 if(argc!=2){std::cerr<<"usage: panoramize_automation48_demo output_directory\n";return 1;}
 const fs::path dir=argv[1];fs::create_directories(dir);
 constexpr int sr=48000,frames=sr*2;
 std::vector<std::vector<float>> outputs;
 std::ofstream stats(dir/"audio_quality_metrics.csv");stats<<"variant,rms,peak,mean_dc,seconds,render_ms\n";
 for(int type=0;type<3;type++){
  auto r=genuine47::example();
  if(type==2){r.tone.wet=0;r.tone.output=1;r.routes.clear();}
  auto voice=genuine47::Instrument::prepare(r,sr);if(!voice)return 2;
  voice->noteOn(60,.85f);
  std::vector<float>a(frames);
  auto started=std::chrono::steady_clock::now();
  for(int i=0;i<frames;i++){
   if(type==1 && i%64==0){
    const double t=double(i)/frames;
    voice->setHostParameterNormalized(0,.15+.75*t); // automated cutoff
    voice->setHostParameterNormalized(2,.55+.4*t); // drive
    voice->setHostParameterNormalized(5,.75); // macro1
   }
   a[size_t(i)]=voice->processOne();
   if(!std::isfinite(a[size_t(i)]))return 3;
  }
  const auto elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count();
  double sum=0,dc=0,peak=0;
  for(float v:a){sum+=double(v)*v;dc+=v;peak=std::max(peak,double(std::abs(v)));}
  const char*name=type==0?"native_47_unchanged":type==1?"native_47_host_automation":"native_39_bypass";
  if(!wav(dir/(std::string(name)+".wav"),a,sr))return 4;
  stats<<name<<','<<std::sqrt(sum/frames)<<','<<peak<<','<<dc/frames<<','<<frames/double(sr)<<','<<elapsed<<'\n';
  outputs.push_back(std::move(a));
 }
 double l2=0;for(size_t i=0;i<outputs[0].size();i++){
  const double d=outputs[0][i]-outputs[1][i];l2+=d*d;
 }
 std::cout<<"AUTOMATION48_OK audio_difference_L2="<<l2<<"\n";
 return l2>1e-9?0:5;
}
