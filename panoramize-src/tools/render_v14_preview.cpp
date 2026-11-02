#include "PanoramaDSP.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
using namespace panorama;
namespace {
void u16(std::ofstream&f,uint16_t x){for(int i=0;i<2;i++)f.put(char((x>>(i*8))&255));}
void u32(std::ofstream&f,uint32_t x){for(int i=0;i<4;i++)f.put(char((x>>(i*8))&255));}
void writeWav(const std::string& path,const std::vector<float>& left,const std::vector<float>& right,int sr){
 std::ofstream out(path,std::ios::binary); const uint32_t frames=(uint32_t)left.size();const uint32_t dataSize=frames*6;
 out.write("RIFF",4);u32(out,36+dataSize);out.write("WAVEfmt ",8);u32(out,16);u16(out,1);u16(out,2);u32(out,sr);u32(out,sr*6);u16(out,6);u16(out,24);out.write("data",4);u32(out,dataSize);
 for(size_t i=0;i<frames;i++)for(float s:{left[i],right[i]}){
  auto v=(int32_t)std::llround(std::clamp(s,-.99999f,.99999f)*8388607.f);
  for(int b=0;b<3;b++)out.put(char((v>>(b*8))&255));
 }
}
std::string fileName(int i,const char*name){
 std::string s=name;
 auto slash=s.find('/');if(slash!=std::string::npos)s.erase(slash,1);
 std::string clean;bool lastUnderscore=false;
 for(char c:s){bool u=c==' '||c=='/'||c=='-';
  if(u){if(!lastUnderscore)clean.push_back('_');lastUnderscore=true;}
  else {clean.push_back(c);lastUnderscore=false;}}
 return clean+".wav";
}
void schedule(std::vector<MidiEvent>&ev,int s,int note,bool on,float val=1.f){ev.push_back({on?MidiEvent::Type::NoteOn:MidiEvent::Type::NoteOff,s,note,1,val});}
void render(int preset,const std::string&folder){
 constexpr int sr=48000;
 const int frames=(int)(2.8*sr);
 Synth synth;synth.prepare(sr); synth.setPatch(factoryPresets()[preset]);
 std::vector<MidiEvent> ev;
 // fixed phrase; interval and velocity varied but identical for all presets.
 int melody[]={48,55,60,67};
 for(int i=0;i<4;i++){const int start=(int)((.10+i*.62)*sr);schedule(ev,start,melody[i],true,.75f+((i%3)*.09f));schedule(ev,start+(int)(sr*.45),melody[i],false);}
 // Short monophonic phrase keeps 1.5 preview renders bounded in time.
 std::stable_sort(ev.begin(),ev.end(),[](auto a,auto b){return a.sample<b.sample;});
 std::vector<float> left(frames),right(frames);
 int loc=0,eidx=0;
 for(int pos=0;pos<frames;){int block=std::min(256,frames-pos);std::vector<MidiEvent> sub;
  while(eidx<(int)ev.size()&&ev[eidx].sample<pos+block){auto e=ev[eidx++];e.sample-=pos;sub.push_back(e);}
  synth.render(left.data()+pos,right.data()+pos,block,sub.data(),sub.size());pos+=block;
 }
 float peak=0;double rms=0;for(int i=0;i<frames;i++){peak=std::max(peak,std::max(std::abs(left[i]),std::abs(right[i])));rms+=(double)left[i]*left[i]+(double)right[i]*right[i];}
 rms=std::sqrt(rms/(2*frames));
 const std::string path=folder+"/"+fileName(preset,factoryPresets()[preset].name);
 writeWav(path,left,right,sr);
 std::cout<<std::setw(2)<<preset+1<<" "<<path<<" peak="<<std::fixed<<std::setprecision(4)<<peak<<" rms="<<rms<<"\n";
}
}
int main(int argc,char**argv){
 if(argc<2){std::cerr<<"usage: panorama_render <output-dir> [preset-number 1..64]\n";return 1;}
 std::filesystem::create_directories(argv[1]);
 try{if(argc>2){int n=std::stoi(argv[2]);if(n<1||n>kPresetCount)return 2;render(n-1,argv[1]);}
 else for(int i=0;i<kPresetCount;i++)render(i,argv[1]);}
 catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 3;}
}
