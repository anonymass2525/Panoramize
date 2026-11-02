#include "SmoothGraphSwap52.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
using namespace panorama;
namespace {
void le(std::ofstream&f,uint32_t v,int n){for(int i=0;i<n;++i)f.put(char((v>>(8*i))&255));}
void wav(const std::string&name,const std::vector<float>&a,int sr){
 std::ofstream f(name,std::ios::binary);f.write("RIFF",4);le(f,uint32_t(36+a.size()*4),4);f.write("WAVEfmt ",8);
 le(f,16,4);le(f,3,2);le(f,1,2);le(f,sr,4);le(f,sr*4,4);le(f,4,2);le(f,32,2);
 f.write("data",4);le(f,uint32_t(a.size()*4),4);f.write((const char*)a.data(),std::streamsize(a.size()*4));
}
}
int main(int argc,char**argv){
 if(argc!=2){std::cerr<<"Usage: render_safety52 OUTPUT_PREFIX\n";return 2;}
 constexpr int sr=24000,duration=sr*2,swapAt=sr/2;
 auto a=universal23::Instrument::prepare(universal23::directGraph(),sr);
 auto b=universal23::Instrument::prepare(universal23::genuineNativeGraph(),sr);
 auto fresh=universal23::Instrument::prepare(universal23::genuineNativeGraph(),sr);
 if(!a||!b||!fresh)return 3;
 swap52::SmoothGraphSwap fade;fade.setSampleRate(sr);fade.beginStopped(a);fade.noteOn(57,.7f);
 auto prev=universal23::Instrument::prepare(universal23::directGraph(),sr);
 prev->noteOn(57,.7f);
 std::vector<float>joined(duration),hard(duration);
 for(int i=0;i<duration;i++){
  if(i==swapAt){fade.publish(b);fresh->noteOn(57,.7f);}
  if(i==duration-3500){fade.noteOff(57);prev->noteOff(57);fresh->noteOff(57);}
  joined[i]=fade.processOne().left;
  hard[i]=(i<swapAt?prev:fresh)->processOne().left;
  if(!std::isfinite(joined[i])||!std::isfinite(hard[i]))return 4;
 }
 const auto jumpFade=std::abs(joined[swapAt]-joined[swapAt-1]);
 const auto jumpHard=std::abs(hard[swapAt]-hard[swapAt-1]);
 wav(std::string(argv[1])+"_crossfade.wav",joined,sr);
 wav(std::string(argv[1])+"_hard_switch.wav",hard,sr);
 std::cout<<"switch_sample="<<swapAt<<" immediate_delta_crossfade="<<jumpFade
          <<" immediate_delta_hard="<<jumpHard<<" (these are not total audibility metrics)\n";
}
