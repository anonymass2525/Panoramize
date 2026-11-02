#include "TemporalInverse28.h"
#include "UniversalInstrument23.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
namespace fs=std::filesystem;
using namespace panorama;
using namespace panorama::temporal28;
namespace {
struct Take {std::string filename;bool holdout=false;std::vector<float> audio;std::vector<Event> events;int sampleRate=0;};
std::uint16_t r16(const unsigned char*p){return std::uint16_t(p[0]|(unsigned(p[1])<<8));}
std::uint32_t r32(const unsigned char*p){return std::uint32_t(p[0])|(std::uint32_t(p[1])<<8)|(std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24);}
void w16(std::ofstream&f,std::uint16_t x){f.put(char(x));f.put(char(x>>8));}
void w32(std::ofstream&f,std::uint32_t x){for(int i=0;i<4;i++)f.put(char(x>>(8*i)));}
bool writeWav(const fs::path& p,const std::vector<float>& a,int rate){
 std::ofstream f(p,std::ios::binary);if(!f||a.empty()||a.size()>10000000)return false;
 const auto bytes=std::uint32_t(a.size()*2);f.write("RIFF",4);w32(f,36+bytes);f.write("WAVEfmt ",8);w32(f,16);
 w16(f,1);w16(f,1);w32(f,std::uint32_t(rate));w32(f,std::uint32_t(rate*2));w16(f,2);w16(f,16);
 f.write("data",4);w32(f,bytes);
 for(float x:a){const auto pcm=int16_t(std::lrint(std::clamp(x,-1.f,1.f)*32767.f));w16(f,std::uint16_t(pcm));}
 return bool(f);
}
bool readWav(const fs::path& path,std::vector<float>&out,int&rate){
 std::ifstream f(path,std::ios::binary);char riff[12];f.read(riff,12);
 if(!f||std::string(riff,4)!="RIFF"||std::string(riff+8,4)!="WAVE")return false;
 std::uint16_t fmt=0,ch=0,bits=0;std::uint32_t sr=0;std::vector<unsigned char> data;
 while(f){unsigned char h[8];f.read(reinterpret_cast<char*>(h),8);if(!f)break;
  const std::uint32_t len=r32(h+4);if(len>50000000)return false;
  std::string tag(reinterpret_cast<char*>(h),4);
  if(tag=="fmt "){
   if(len<16||len>512)return false;std::vector<unsigned char>b(len);f.read(reinterpret_cast<char*>(b.data()),len);
   if(!f)return false;fmt=r16(b.data());ch=r16(b.data()+2);sr=r32(b.data()+4);bits=r16(b.data()+14);
  }else if(tag=="data"){
   data.resize(len);f.read(reinterpret_cast<char*>(data.data()),len);if(!f)return false;
  }else f.seekg(len,std::ios::cur);
  if(len%2)f.seekg(1,std::ios::cur);
 }
 if(!(fmt==1&&(bits==16||bits==24))&&!(fmt==3&&bits==32))return false;
 if(ch<1||ch>2||sr<8000||sr>48000)return false;
 const size_t stride=size_t(bits/8)*ch;if(data.size()%stride)return false;
 const size_t frames=data.size()/stride;if(frames<1024||frames>sr*12)return false;
 out.resize(frames);rate=int(sr);
 for(size_t n=0;n<frames;n++){
  double sum=0.;for(int channel=0;channel<ch;channel++){
   const unsigned char*p=data.data()+n*stride+size_t(channel*bits/8);
   if(fmt==3){std::uint32_t raw=r32(p);float val;std::memcpy(&val,&raw,4);sum+=val;}
   else if(bits==16)sum+=double(std::int16_t(r16(p)))/32768.;
   else{int32_t v=p[0]|(int32_t(p[1])<<8)|(int32_t(p[2])<<16);if(v&0x800000)v|=~0xffffff;sum+=double(v)/8388608.;}
  }out[n]=float(sum/ch);
 }
 return true;
}
std::string strip(const std::string& s){auto a=s.find_first_not_of(" \t\r\n");if(a==std::string::npos)return {};auto b=s.find_last_not_of(" \t\r\n");return s.substr(a,b-a+1);}
std::vector<std::string> split(const std::string&s){std::vector<std::string>p;std::istringstream stream(s);std::string a;while(std::getline(stream,a,'|'))p.push_back(strip(a));return p;}
const char* typeName(EventType t){switch(t){case EventType::NoteOn:return "ON";case EventType::NoteOff:return "OFF";case EventType::PitchBend:return "BEND";case EventType::ModWheel:return "WHEEL";}return "?";}
bool parse(const fs::path& manifest,std::vector<Take>&takes){
 std::ifstream f(manifest);if(!f)return false;
 std::string line;Take* current=nullptr;bool terminated=true;
 while(std::getline(f,line)){
  line=strip(line);if(line.empty()||line[0]=='#')continue;
  auto parts=split(line);
  if(parts.size()==3&&parts[0]=="TAKE"&&terminated){
   if(parts[1]!="train"&&parts[1]!="holdout")return false;
   if(parts[2].empty()||parts[2].find("..")!=std::string::npos||fs::path(parts[2]).is_absolute())return false;
   takes.emplace_back();current=&takes.back();current->holdout=parts[1]=="holdout";current->filename=parts[2];terminated=false;
   if(takes.size()>16||!readWav(manifest.parent_path()/parts[2],current->audio,current->sampleRate))return false;
  }else if(parts.size()==5&&parts[0]=="EVENT"&&current&&!terminated){
   Event e;try{e.frame=std::stoi(parts[1]);e.note=std::stoi(parts[3]);e.value=std::stof(parts[4]);}catch(...){return false;}
   if(parts[2]=="ON")e.type=EventType::NoteOn;else if(parts[2]=="OFF")e.type=EventType::NoteOff;
   else if(parts[2]=="BEND")e.type=EventType::PitchBend;else if(parts[2]=="WHEEL")e.type=EventType::ModWheel;else return false;
   current->events.push_back(e);
  }else if(parts.size()==1&&parts[0]=="END"&&current&&!terminated){terminated=true;current=nullptr;}
  else return false;
 }
 if(!terminated)return false;
 int count=0,hold=0;
 for(auto& t:takes){
  Example ex{t.audio.data(),int(t.audio.size()),t.sampleRate,t.events.data(),int(t.events.size())};
  if(!validate(ex))return false;
  (t.holdout?hold:count)++;
 }
 return count>=1&&hold>=1;
}
Example example(const Take&t){return {t.audio.data(),int(t.audio.size()),t.sampleRate,t.events.data(),int(t.events.size())};}
int inference(const fs::path&manifest,const fs::path&out,int budget){
 std::vector<Take> takes;if(!parse(manifest,takes)){std::cerr<<"Invalid manifest / waveform / timeline\n";return 2;}
 std::vector<Example> train,hold;for(const auto& t:takes)(t.holdout?hold:train).push_back(example(t));
 Settings opts;opts.evaluations=budget;
 const auto res=infer(train.data(),int(train.size()),hold.data(),int(hold.size()),opts);
 if(!res.success){std::cerr<<"No valid graph candidates\n";return 3;}
 fs::create_directories(out);
 std::ofstream report(out/"temporal_portfolio.csv");report<<"rank,training_loss,unseen_holdout_loss,distinct_topology_hash,native_modes,edges\n";
 for(size_t i=0;i<res.proposals.size();i++){
  const auto& p=res.proposals[i];auto g=universal23::directGraph();g.instrument=p.graph;
  const auto filename="instrument_"+std::to_string(i+1);
  std::ofstream patch(out/(filename+".pgraph"));patch<<universal23::serialize(g);
  report<<i+1<<","<<std::setprecision(12)<<p.trainingLoss<<","<<p.unseenLoss<<","<<p.topology<<","<<p.graph.modes.size()<<","<<p.graph.edges.size()<<"\n";
  for(size_t t=0;t<takes.size();t++){
   std::vector<float> audio;std::string why;
   if(!render(p.graph,example(takes[t]),audio,&why)||!writeWav(out/(filename+"_take"+std::to_string(t+1)+".wav"),audio,takes[t].sampleRate))return 4;
  }
 }
 std::cout<<"Temporal inverse results: "<<res.evaluated<<" evaluated graphs, "<<res.distinctTopologies
          <<" retained structures, "<<res.proposals.size()<<" editable instruments. Train="<<train.size()<<" holdout="<<hold.size()<<"\n";
 for(size_t i=0;i<res.proposals.size();i++)std::cout<<" #"<<i+1<<" train="<<res.proposals[i].trainingLoss<<" unseen="<<res.proposals[i].unseenLoss<<"\n";
 return 0;
}
int demo(const fs::path&out,int budget){
 fs::create_directories(out);
 native27::Graph target;target.name="Reference: FM Vocal Resonator";target.strikeMode=0;
 target.modes={{native27::Engine::FM,1.03,5.8,0.,.32,145.},{native27::Engine::Vocal,2.42,6.1,0.,.28,0.}};
 target.edges={{0,1,native26::Coupling::Conservative,4.7e4,.83}};
 const int sr=12000,frames=26400;std::array<std::vector<Event>,4> gesture;
 gesture[0]={{0,EventType::NoteOn,60,.8f},{7800,EventType::NoteOff,60,0.f}}; // short strike + long decay
 gesture[1]={{0,EventType::NoteOn,55,.65f}};
 for(int frame=2400;frame<20000;frame+=250)gesture[1].push_back({frame,EventType::PitchBend,0,float(1.1*std::sin(2*3.14159265358979323846*5.*frame/sr))});
 gesture[1].push_back({21000,EventType::NoteOff,55,0.f});
 gesture[2]={{0,EventType::NoteOn,48,.55f},{10000,EventType::NoteOn,60,.8f},{12500,EventType::NoteOff,48,0.f},{21500,EventType::NoteOff,60,0.f}};
 gesture[3]={{0,EventType::NoteOn,67,.9f},{10500,EventType::NoteOff,67,0.f}};
 std::ofstream manifest(out/"performances.txt");
 manifest<<"# Panoramize 2.8 — dry mono performance recordings, event frames at WAV sample rate\n";
 for(int t=0;t<4;t++){
  Example ex{nullptr,frames,sr,gesture[size_t(t)].data(),int(gesture[size_t(t)].size())};
  std::vector<float> pcm;std::string why;if(!render(target,ex,pcm,&why))return 3;
  const std::string name="reference_take"+std::to_string(t+1)+".wav";
  if(!writeWav(out/name,pcm,sr))return 3;
  manifest<<"TAKE|"<<(t==3?"holdout":"train")<<"|"<<name<<"\n";
  for(const auto&e:gesture[size_t(t)])manifest<<"EVENT|"<<e.frame<<"|"<<typeName(e.type)<<"|"<<e.note<<"|"<<std::setprecision(8)<<e.value<<"\n";
  manifest<<"END\n";
 }
 manifest.close();
 // Reference graph supplied ONLY for post-hoc ground truth comparison, not read by inference.
 auto ref=universal23::directGraph();ref.instrument=target;
 std::ofstream patch(out/"GROUND_TRUTH_NOT_FOR_TRAINING.pgraph");patch<<universal23::serialize(ref);patch.close();
 const int outcome=inference(out/"performances.txt",out/"result",budget);
 if(outcome)return outcome;
 // Independent 48kHz listening A/B: the inference itself only uses the 12kHz
 // analysis takes. These full-rate clips are not used in optimization.
 auto bestFile=std::ifstream(out/"result"/"instrument_1.pgraph");
 if(!bestFile)return 5;
 const std::string text((std::istreambuf_iterator<char>(bestFile)),std::istreambuf_iterator<char>());
 universal23::Graph best;std::string why;
 if(!universal23::deserialize(text,best,&why))return 5;
 for(int t=0;t<4;t++){
  auto events=gesture[size_t(t)];for(auto& e:events)e.frame*=4;
  const Example listen{nullptr,frames*4,48000,events.data(),int(events.size())};
  std::vector<float> original,recreated;
  if(!render(target,listen,original,&why)||!render(best.instrument,listen,recreated,&why))return 6;
  if(!writeWav(out/("listen_"+std::to_string(t+1)+"_reference_48k.wav"),original,48000))return 6;
  if(!writeWav(out/("listen_"+std::to_string(t+1)+"_inferred_48k.wav"),recreated,48000))return 6;
 }
 return 0;
}
}
int main(int argc,char**argv){
 if(argc<3||argc>4){std::cerr<<"Usage: panoramize_temporal28 <performances.txt> <output-folder> [evaluations 1..128]\n"
    "       panoramize_temporal28 --demo <output-folder> [evaluations]\n";return 2;}
 int budget=20;try{if(argc==4)budget=std::stoi(argv[3]);}catch(...){return 2;}
 if(budget<1||budget>128)return 2;
 return std::string(argv[1])=="--demo"?demo(argv[2],budget):inference(argv[1],argv[2],budget);
}
