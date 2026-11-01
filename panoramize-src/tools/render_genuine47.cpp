#include "GenuineNative47.h"
#include "UniversalInstrument23.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
namespace fs=std::filesystem;
using namespace panorama;
static bool writeWav(const fs::path&path,const std::vector<float>&v,int sr){
 std::ofstream f(path,std::ios::binary);if(!f)return false;
 auto u16=[&](std::uint32_t x){for(int i=0;i<2;i++)f.put(char(x>>(8*i)));};
 auto u32=[&](std::uint32_t x){for(int i=0;i<4;i++)f.put(char(x>>(8*i)));};
 f.write("RIFF",4);u32(36+4*std::uint32_t(v.size()));f.write("WAVEfmt ",8);u32(16);u16(3);u16(1);
 u32(std::uint32_t(sr));u32(std::uint32_t(sr*4));u16(4);u16(32);f.write("data",4);u32(4*std::uint32_t(v.size()));
 for(float x:v)f.write(reinterpret_cast<const char*>(&x),4);return bool(f);
}
static int render(genuine47::Recipe r,const fs::path&output,int rate,double length){
 std::string err;auto instrument=genuine47::Instrument::prepare(r,rate,&err);
 if(!instrument){std::cerr<<err<<'\n';return 3;}
 instrument->noteOn(60,.8f);
 std::vector<float> samples(size_t(rate*length));double energy=0;
 for(size_t i=0;i<samples.size();i++){
  if(i==size_t(rate*.35))instrument->modWheel(.85f);
  if(i==size_t(rate*.55)){instrument->channelPressure(.8f);instrument->setMacro(0,.9f);}
  if(i==size_t(rate*.72))instrument->pitchBend(2.f);
  samples[i]=instrument->processOne();energy+=double(samples[i])*samples[i];
 }
 if(!writeWav(output,samples,rate))return 4;
 auto m=instrument->meter();std::cout<<output.filename().string()<<" rms="<<std::sqrt(energy/samples.size())<<" peak="<<m.peak<<" nativeActiveWork="<<m.nativeActiveWork<<" physicalExternalWork="<<m.physicalActiveWork<<" ledgerResidual="<<m.nativeLedgerResidual<<'\n';return 0;
}
int main(int argc,char**argv){
 try{
  if(argc==3&&std::string(argv[1])=="--demo"){
   fs::path out=argv[2];fs::create_directories(out);auto r=genuine47::example();
   std::ofstream(out/"full_genuine.pgraph")<<universal23::serialize(universal23::genuineNativeGraph());
   std::ofstream(out/"01_native_and_tone.genuine47")<<genuine47::serialize(r);
   if(render(r,out/"01_native_and_tone.wav",48000,1.2))return 1;
   r.routes.clear();std::ofstream(out/"02_no_modulation.genuine47")<<genuine47::serialize(r);
   if(render(r,out/"02_no_modulation.wav",48000,1.2))return 2;
   r.tone.wet=0;r.tone.output=1;std::ofstream(out/"03_original_native_bypass.genuine47")<<genuine47::serialize(r);
   return render(r,out/"03_original_native_bypass.wav",48000,1.2);
  }
  if(argc<3||argc>5){std::cerr<<"usage: render_genuine47 --demo DIR | patch.genuine47 output.wav [seconds] [sample_rate]\n";return 1;}
  const double seconds=argc>=4?std::stod(argv[3]):1.2;const int sr=argc==5?std::stoi(argv[4]):48000;
  if(!std::isfinite(seconds)||seconds<.01||seconds>10||sr<22050||sr>192000)return 1;
  std::ifstream f(argv[1]);if(!f)return 2;
  std::string content((std::istreambuf_iterator<char>(f)),{});std::string err;genuine47::Recipe r;
  if(!genuine47::deserialize(content,r,&err)){std::cerr<<err<<'\n';return 2;}
  return render(r,argv[2],sr,seconds);
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
