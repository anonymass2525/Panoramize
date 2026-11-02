#include "GraphInverse.h"
#include <algorithm>
#include <array>
#include <cmath>
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
namespace {
uint16_t r16(const unsigned char*p){return uint16_t(p[0]|(uint16_t(p[1])<<8));}
uint32_t r32(const unsigned char*p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
void w16(std::ofstream& f,uint16_t x){f.put(char(x));f.put(char(x>>8));}
void w32(std::ofstream& f,uint32_t x){for(int k=0;k<4;k++)f.put(char(x>>(8*k)));}
bool wavRead(const fs::path& path,std::vector<float>&out,int& rate){
 std::ifstream f(path,std::ios::binary);char header[12]{};f.read(header,12);
 if(!f||std::string(header,4)!="RIFF"||std::string(header+8,4)!="WAVE")return false;
 uint16_t fmt=0,ch=0,bits=0;uint32_t sr=0;
 std::vector<unsigned char> data;
 while(f){unsigned char h[8]{};f.read(reinterpret_cast<char*>(h),8);if(!f)break;
  const uint32_t len=r32(h+4);if(len>64000000)return false;
  const std::string tag(reinterpret_cast<char*>(h),4);
  if(tag=="fmt "){
   if(len<16||len>256)return false;
   std::vector<unsigned char> b(len);f.read(reinterpret_cast<char*>(b.data()),len);
   if(!f)return false;fmt=r16(b.data());ch=r16(b.data()+2);sr=r32(b.data()+4);bits=r16(b.data()+14);
  }else if(tag=="data"){
   data.resize(len);f.read(reinterpret_cast<char*>(data.data()),len);if(!f)return false;
  }else f.seekg(len,std::ios::cur);
  if(len&1)f.seekg(1,std::ios::cur);
 }
 if(fmt!=1||(ch!=1&&ch!=2)||(bits!=16&&bits!=24)||sr<8000||sr>192000)return false;
 const int stride=(bits/8)*ch;
 if(data.size()<size_t(stride)*4096)return false;
 rate=int(sr);out.resize(4096);
 for(int i=0;i<4096;i++){
  float x=0.f;for(int j=0;j<ch;j++){
   const unsigned char*p=data.data()+size_t(i*stride+j*(bits/8));
   int32_t v=0;
   if(bits==16)v=int16_t(r16(p));
   else{v=int32_t(p[0])|(int32_t(p[1])<<8)|(int32_t(p[2])<<16);
        if(v&0x800000)v|=~0xffffff;}
   x+=float(v/double(bits==16?32768:8388608));
  }out[size_t(i)]=x/ch;
 }return true;
}
bool wavWrite(const fs::path&path,const std::vector<float>&samples,int sampleRate){
 std::ofstream f(path,std::ios::binary);if(!f)return false;
 const uint32_t bytes=uint32_t(samples.size()*2);
 f.write("RIFF",4);w32(f,36+bytes);f.write("WAVEfmt ",8);w32(f,16);
 w16(f,1);w16(f,1);w32(f,uint32_t(sampleRate));w32(f,uint32_t(sampleRate*2));w16(f,2);w16(f,16);
 f.write("data",4);w32(f,bytes);
 for(float x:samples){const int16_t a=int16_t(std::lrint(std::clamp(x,-.999f,.999f)*32767.f));w16(f,uint16_t(a));}
 return bool(f);
}
struct Recording {std::vector<float> audio;InstrumentExample gesture;bool holdout=false;std::string label;};
bool parseRecordings(const fs::path& manifest,std::vector<Recording>& records){
 std::ifstream f(manifest);if(!f)return false;std::string line;
 while(std::getline(f,line)){
  if(line.empty()||line[0]=='#'||line.rfind("file,",0)==0)continue;
  std::array<std::string,5> col{};std::istringstream ss(line);
  for(auto&field:col)if(!std::getline(ss,field,','))return false;
  Recording r;
  try{
   r.gesture.midiNote=std::stoi(col[1]);r.gesture.velocity=std::stof(col[2]);
   r.gesture.noteOffFrame=std::stoi(col[3]);
  }catch(...){return false;}
  r.holdout=(col[4]=="holdout");r.label=col[0];
  int rate=0;if(!wavRead(manifest.parent_path()/col[0],r.audio,rate))return false;
  r.gesture.sampleRate=rate;r.gesture.count=4096;
  records.push_back(std::move(r));
 }
 for(auto& r:records)r.gesture.samples=r.audio.data();
 return records.size()>=3;
}
std::vector<float> soundDemo(const Patch&p,int midi,float vel,int gate,int rate=48000){
 const int frames=int(1.10*rate);
 std::vector<float> pcm(size_t(frames),0.f);
 Synth s;s.prepare(rate);s.setPatch(p);s.noteOn(midi,vel);
 for(int i=0;i<frames;i++){
  if(i==gate)s.noteOff(midi);
  const auto frame=s.processOne();pcm[size_t(i)]=.5f*(frame.left+frame.right);
 }
 return pcm;
}
int run(const fs::path& manifest,const fs::path& dest,int budget){
 std::vector<Recording> records;if(!parseRecordings(manifest,records)){
  std::cerr<<"Invalid CSV / WAV input. See README.\n";return 2;
 }
 std::vector<InstrumentExample> train,hold;
 for(const auto& r:records)(r.holdout?hold:train).push_back(r.gesture);
 fs::create_directories(dest);
 InstrumentInverseSettings settings;settings.maxEvaluations=budget;
 const auto result=inferPlayableInstruments(train.data(),int(train.size()),
     hold.data(),int(hold.size()),settings);
 if(!result.success){std::cerr<<"No playable candidates for these recordings.\n";return 3;}
 std::ofstream report(dest/"portfolio.csv");
 report<<"rank,training_error,unseen_holdout_error,structural_novelty,connected_nodes,topology_hash\n";
 std::vector<float> comparison;
 for(size_t i=0;i<result.proposals.size();i++){
  const auto& p=result.proposals[i];
  GraphInverseResult recipe{};recipe.patch=p.patch;recipe.distance=p.trainingError;
  recipe.success=true;recipe.evaluated=result.evaluated;recipe.midiNote=train.front().midiNote;recipe.seed=result.seed;
  std::ofstream json(dest/("instrument_"+std::to_string(i+1)+".json"));
  json<<graphInverseJson(recipe);
  report<<i+1<<","<<p.trainingError<<","<<p.holdoutError<<","<<p.structuralNovelty
       <<","<<p.activeNodes<<","<<p.topologySignature<<"\n";
  if(!json||!report)return 4;
  const auto audio=soundDemo(p.patch,hold.empty()?train[0].midiNote:hold[0].midiNote,
        hold.empty()?train[0].velocity:hold[0].velocity,int(.25*48000));
  if(!wavWrite(dest/("instrument_"+std::to_string(i+1)+".wav"),audio,48000))return 4;
 }
 std::cout<<"Playable instrument discovery: "<<result.evaluated<<" graphs, "
          <<result.distinctTopologies<<" topologies, "<<result.proposals.size()
          <<" instruments. Train="<<train.size()<<", unseen="<<hold.size()<<"\n";
 for(size_t i=0;i<result.proposals.size();i++)
  std::cout<<"proposal "<<i+1<<": train "<<result.proposals[i].trainingError
      <<" heldout "<<result.proposals[i].holdoutError<<"\n";
 return 0;
}
int demo(const fs::path&dir,int budget,bool complex){
 fs::create_directories(dir);
 Patch reference=factoryPresets()[0];reference.engine=Engine::Modular;
 reference.attack=.01f;reference.decay=.35f;reference.sustain=.73f;reference.release=.5f;
 reference.gain=.62f;reference.brightness=.7f;reference.cutoff=.69f;reference.motion=.35f;
 reference.graph={};reference.modRoutes={};
 reference.graph.nodes[0]={NodeType::Harmonic,-1,-1,.82f,.5f};
 reference.graph.nodes[8]={NodeType::Output,0,-1,.88f,0.f};
 if(complex){
  // Not contained in the 54 fixed warm starts: a compound instrument.
  reference.graph.nodes[0]={NodeType::Harmonic,-1,-1,.82f,.47f};
  reference.graph.nodes[1]={NodeType::Fold,0,-1,.83f,.68f};
  reference.graph.nodes[8]={NodeType::Output,1,-1,.88f,0.f};
  reference.fold=.4f;
 }
 const int notes[]={48,60,72,67,55};const float velocities[]={.4f,.8f,.67f,.57f,.92f};
 const int gates[]={1300,4096,2400,1600,900};
 std::ofstream manifest(dir/"performances.csv");
 manifest<<"file,midi_note,velocity,note_off_frame,split\n";
 for(int i=0;i<5;i++){
  std::vector<float> pcm(4096);
  InstrumentExample gesture{pcm.data(),4096,48000.,notes[i],velocities[i],gates[i]};
  if(!renderInstrumentExample(reference,gesture,pcm.data()))return 3;
  const std::string name="reference_"+std::to_string(i+1)+".wav";
  if(!wavWrite(dir/name,pcm,48000))return 4;
  manifest<<name<<","<<notes[i]<<","<<velocities[i]<<","<<gates[i]<<","<<(i<3?"train":"holdout")<<"\n";
 }
 manifest.close();
 if(run(dir/"performances.csv",dir/"result",budget)!=0)return 5;
 // A/B listening clips: longer than the 85ms fitting window, across four
 // different notes and playing gestures including unobserved test notes.
 std::vector<float> original,generated;
 std::vector<Recording> dummy; // generated JSON is also retained for JUCE import
 // Reference melody and fitted melody. Load the result in the main routine via
 // inference again only if needed; here derive it directly from the CSV input.
 std::vector<Recording> recs;if(!parseRecordings(dir/"performances.csv",recs))return 5;
 std::vector<InstrumentExample> tr,ho;for(const auto&r:recs)(r.holdout?ho:tr).push_back(r.gesture);
 InstrumentInverseSettings settings;settings.maxEvaluations=budget;
 auto found=inferPlayableInstruments(tr.data(),int(tr.size()),ho.data(),int(ho.size()),settings);
 if(!found.success)return 5;
 const auto& p=found.proposals.front().patch;
 for(int idx:{0,2,3,4}){
  auto a=soundDemo(reference,notes[idx],velocities[idx],int(.22*48000));
  auto b=soundDemo(p,notes[idx],velocities[idx],int(.22*48000));
  original.insert(original.end(),a.begin(),a.end());
  generated.insert(generated.end(),b.begin(),b.end());
 }
 wavWrite(dir/"reference_performance.wav",original,48000);
 wavWrite(dir/"inferred_performance.wav",generated,48000);
 std::cout<<"Generated matched/unseen performance A/B WAVs.\n";
 return 0;
}
}
int main(int argc,char**argv){
 try{
  if(argc>=3&&std::string(argv[1])=="--demo")
   return demo(argv[2],argc>=4?std::stoi(argv[3]):72,false);
  if(argc>=3&&std::string(argv[1])=="--demo-structural")
   return demo(argv[2],argc>=4?std::stoi(argv[3]):160,true);
  if(argc>=3&&argc<=4)return run(argv[1],argv[2],argc>=4?std::stoi(argv[3]):96);
  std::cerr<<"Usage: panorama_inverse_instrument performances.csv output_dir [budget]\n"
              "       panorama_inverse_instrument --demo output_dir [budget]\n"
              "       panorama_inverse_instrument --demo-structural output_dir [budget]\n";
  return 1;
 }catch(const std::exception&ex){std::cerr<<ex.what()<<"\n";return 9;}
}
