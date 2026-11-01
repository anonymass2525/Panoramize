#include "AudioDesign49.h"
#include "ProductionInverse48.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
namespace fs=std::filesystem;
using namespace panorama;
namespace {
uint32_t u32(const unsigned char*b){return uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);}
uint16_t u16(const unsigned char*b){return uint16_t(b[0])|(uint16_t(b[1])<<8);}
void put32(std::ofstream&f,uint32_t x){for(int i=0;i<4;i++)f.put(char(x>>(i*8)));}
void put16(std::ofstream&f,uint16_t x){for(int i=0;i<2;i++)f.put(char(x>>(i*8)));}
bool writeWav(const fs::path&p,const std::vector<float>&a,int rate){
 std::ofstream f(p,std::ios::binary);if(!f||a.size()>100000000)return false;
 f.write("RIFF",4);put32(f,uint32_t(36+a.size()*4));f.write("WAVEfmt ",8);
 put32(f,16);put16(f,3);put16(f,1);put32(f,uint32_t(rate));put32(f,uint32_t(rate*4));put16(f,4);put16(f,32);
 f.write("data",4);put32(f,uint32_t(a.size()*4));for(float x:a)f.write((const char*)&x,4);return bool(f);
}
bool readWav(const fs::path&p,std::vector<float>&audio,int&rate){
 std::ifstream f(p,std::ios::binary);if(!f)return false;char h[12];
 if(!f.read(h,12)||std::string(h,4)!="RIFF"||std::string(h+8,4)!="WAVE")return false;
 int codec=0,ch=0,bits=0;std::vector<unsigned char>samples;rate=0;
 while(f){unsigned char hdr[8];if(!f.read((char*)hdr,8))break;
  const auto size=u32(hdr+4);if(size>192000000u)return false;
  const std::string tag((char*)hdr,4);
  if(tag=="fmt "){if(size<16||size>4096)return false;std::vector<unsigned char>v(size);
   if(!f.read((char*)v.data(),size))return false;
   codec=u16(v.data());ch=u16(v.data()+2);rate=int(u32(v.data()+4));bits=u16(v.data()+14);
  }else if(tag=="data"){samples.resize(size);if(!f.read((char*)samples.data(),size))return false;}
  else f.seekg(size,std::ios::cur);
  if(size%2)f.seekg(1,std::ios::cur);
 }
 if(rate<22050||rate>96000||ch<1||ch>2||!((codec==1&&(bits==16||bits==24))||(codec==3&&bits==32)))return false;
 const size_t b=bits/8,frame=b*size_t(ch);if(samples.size()<frame*size_t(rate)/4||samples.size()%frame)return false;
 audio.resize(samples.size()/frame);if(audio.size()>size_t(rate)*600)return false;
 for(size_t i=0;i<audio.size();i++){
  double sum=0;
  for(int c=0;c<ch;c++){const auto*p=samples.data()+i*frame+b*size_t(c);double x=0;
   if(codec==3){float v;std::memcpy(&v,p,4);x=v;}
   else if(bits==16)x=int16_t(u16(p))/32768.;
   else{int v=int(p[0]|(p[1]<<8)|(p[2]<<16));if(v&0x800000)v|=~0xffffff;x=v/8388608.;}
   sum+=x;
  }
  audio[i]=float(sum/ch);if(!std::isfinite(audio[i]))return false;
 }return true;
}
int inferAndWrite(const std::vector<float>&samples,int sr,const fs::path&dir){
 fs::create_directories(dir);auto result=design49::infer(samples,sr,5);
 const auto qc=production48::assess(samples,sr);
 std::ofstream(dir/"source_recording_quality.csv")<<"duration_s,rms,peak,clipped_fraction,noise_floor_rms,questionable\n"
  <<qc.duration<<','<<qc.rms<<','<<qc.peak<<','<<qc.clippedFraction<<','<<qc.noiseFloorRms<<','<<int(qc.questionable)<<'\n';
 if(!result.error.empty()){std::cerr<<result.error<<"\n";return 3;}
 const auto&d=result.observation;
 std::ofstream quality(dir/"observed_features.csv");quality<<"pitch_hz,pitch_confidence,centroid_hz,high_harmonic_ratio,rms,attack_s,duration_s,ambiguous\n"
 <<std::setprecision(9)<<d.pitchHz<<','<<d.pitchConfidence<<','<<d.centroidHz<<','<<d.highHarmonicRatio<<','<<d.rms<<','<<d.attackSeconds<<','<<d.durationSeconds<<','<<int(result.ambiguous)<<'\n';
 std::ofstream scores(dir/"ranked_candidates.csv");scores<<"rank,family,descriptor_distance,spectral_distance,temporal_distance,example_midi,editable_pgraph\n";
 for(size_t i=0;i<result.ranked.size();i++){
  const auto&c=result.ranked[i];const std::string stem="candidate_"+std::to_string(i+1);
  std::ofstream(dir/(stem+".pgraph"))<<universal23::serialize(c.graph);
  std::ofstream param(dir/(stem+"_parameters.csv"));
  param<<"id,name,unit,min,max,live,requires_rebuild\n";
  for(const auto&v:design49::unifiedParameterRegistry(c.graph))
   param<<v.id<<",\""<<v.name<<"\","<<v.unit<<','<<v.min<<','<<v.max<<','<<int(v.live)<<','<<int(v.requiresRebuild)<<'\n';
  scores<<i+1<<",\""<<design49::name(c.family)<<"\","<<c.distance<<','<<c.spectralDistance<<','<<c.temporalDistance<<','<<c.midi<<','<<stem<<".pgraph\n";
  std::string err;auto wav=design49::renderGraph(c.graph,c.midi,sr,.8,&err);
  if(wav.empty())std::cerr<<"candidate render warning: "<<err<<"\n";
  else writeWav(dir/(stem+"_preview.wav"),wav,sr);
 }
 std::ofstream(dir/"IMPORT_INSTRUCTIONS.txt")<<
 "Open a candidate_N.pgraph using Panoramize Universal Patchbay > LOAD GRAPH.\n"
 "Edit source nodes, routes, native tone and physical string parameters. All original engines are retained.\n"
 "Scores are limited 0.2s normalized-harmonic forward comparisons; NOT calibrated probabilities.\n"
 "The inferred geometry, material, microphones, polyphony or technique are NOT uniquely identified.\n"
 "The 4.9 GUI includes a candidate-one WAV import (if compiled with JUCE).\n"<<result.warning<<"\n";
 std::cout<<"PANORAMIZE49_INFER_OK pitch="<<d.pitchHz<<" candidates="<<result.ranked.size()<<" ambiguous="<<result.ambiguous;
 if(!result.ranked.empty())std::cout<<" best="<<design49::name(result.ranked[0].family)<<" distance="<<result.ranked[0].distance;
 std::cout<<"\n";return 0;
}
}
int main(int argc,char**argv){
 if(argc==4&&std::string(argv[1])=="--infer"){
  std::vector<float>a;int sr=0;if(!readWav(argv[2],a,sr)){std::cerr<<"WAV must be 22050..96000Hz PCM16/24 or float32 mono/stereo 0.25..600s\n";return 2;}
  return inferAndWrite(a,sr,argv[3]);
 }
 if(argc==3&&std::string(argv[1])=="--demo"){
  fs::path dir=argv[2];fs::create_directories(dir);const int sr=48000;
  auto g=design49::editableGraph(design49::Family::PhysicalString,220.,.28);
  auto a=design49::renderGraph(g,57,sr,.8);
  if(a.empty())return 3;
  std::vector<float> target(a.size()+3,0);
  for(size_t i=0;i<a.size();i++)target[i+3]=float(.72*a[i]+.000008*std::sin(double(i)*.177));
  if(!writeWav(dir/"synthetic_recording_with_gain_noise_shift.wav",target,sr))return 4;
  const int aStatus=inferAndWrite(target,sr,dir);if(aStatus)return aStatus;
  auto native=design49::editableGraph(design49::Family::NativeVA_FM,220.);
  auto nativeAudio=design49::renderGraph(native,57,sr,.8);
  if(nativeAudio.empty())return 6;
  for(size_t i=0;i<nativeAudio.size();i++)nativeAudio[i]=float(.79*nativeAudio[i]+.000005*std::sin(i*.22));
  auto child=dir/"native_case";fs::create_directories(child);
  writeWav(child/"native_reference.wav",nativeAudio,sr);
  const int bStatus=inferAndWrite(nativeAudio,sr,child);
  if(bStatus)std::cerr<<"Native demonstration: "+std::to_string(bStatus)+" (not always inferable)\n";
  return 0;
 }
 std::cerr<<"usage: panorama_audio_design49 --infer input.wav out_directory | --demo out_directory\n";return 1;
}
