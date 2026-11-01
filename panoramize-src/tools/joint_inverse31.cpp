#include "PerformancePhysical30.h"
#include "JointPhysicalInverse31.h"
#include "UniversalInstrument23.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
namespace fs=std::filesystem;
using namespace panorama::performance30;
namespace {
unsigned short u16(const unsigned char*p){return static_cast<unsigned short>(p[0]|(p[1]<<8));}
unsigned u32(const unsigned char*p){return unsigned(p[0])|(unsigned(p[1])<<8)|(unsigned(p[2])<<16)|(unsigned(p[3])<<24);}
void put16(std::ofstream&f,unsigned x){f.put(char(x));f.put(char(x>>8));}
void put32(std::ofstream&f,unsigned x){for(int i=0;i<4;i++)f.put(char(x>>(i*8)));}
bool readWav(const fs::path&p,std::vector<float>&x,int&sr){
 std::ifstream f(p,std::ios::binary);char header[12];f.read(header,12);
 if(!f||std::string(header,4)!="RIFF"||std::string(header+8,4)!="WAVE")return false;
 unsigned fmt=0,bits=0,ch=0,rate=0;std::vector<unsigned char>data;
 while(f){unsigned char h[8];f.read(reinterpret_cast<char*>(h),8);if(!f)break;
  const auto sz=u32(h+4);if(sz>100000000)return false;
  const std::string tag(reinterpret_cast<char*>(h),4);
  if(tag=="fmt "){
   if(sz<16||sz>1024)return false;std::vector<unsigned char>b(sz);f.read(reinterpret_cast<char*>(b.data()),sz);
   fmt=u16(b.data());ch=u16(b.data()+2);rate=u32(b.data()+4);bits=u16(b.data()+14);
  }else if(tag=="data"){data.resize(sz);f.read(reinterpret_cast<char*>(data.data()),sz);}
   else f.seekg(sz,std::ios::cur);
  if(sz&1)f.seekg(1,std::ios::cur);
 }
 if(ch<1||ch>2||rate<8000||rate>96000||!((fmt==1&&(bits==16||bits==24))||(fmt==3&&bits==32)))return false;
 const auto step=ch*(bits/8);if(!step||data.empty()||data.size()%step)return false;
 const size_t n=data.size()/step;
 if(n<rate/3||n>size_t(rate)*30)return false;
 sr=int(rate);x.resize(n);
 for(size_t i=0;i<n;i++){
  double v=0.;for(unsigned c=0;c<ch;c++){
   const unsigned char*b=data.data()+i*step+c*bits/8;
   if(fmt==3){const auto raw=u32(b);float z;std::memcpy(&z,&raw,4);v+=z;}
   else if(bits==16)v+=double(std::int16_t(u16(b)))/32768.;
   else {int k=int(b[0]|(b[1]<<8)|(b[2]<<16));if(k&0x800000)k|=~0xffffff;v+=double(k)/8388608.;}
  }x[i]=float(v/ch);if(!std::isfinite(x[i]))return false;
 }return true;
}
bool saveWav(const fs::path&p,const std::vector<float>&x,int sr){
 std::ofstream f(p,std::ios::binary);if(!f||x.empty()||x.size()>size_t(sr)*30)return false;
 const auto bytes=unsigned(x.size()*2);f.write("RIFF",4);put32(f,bytes+36);f.write("WAVEfmt ",8);
 put32(f,16);put16(f,1);put16(f,1);put32(f,sr);put32(f,sr*2);put16(f,2);put16(f,16);
 f.write("data",4);put32(f,bytes);for(float v:x)put16(f,unsigned(std::uint16_t(std::int16_t(std::lrint(std::clamp(v,-1.f,1.f)*32767.f)))));
 return bool(f);
}
void writeEvents(const fs::path&p,const Performance&performance){
 std::ofstream out(p);out<<"event,seconds,pitch_hz,confidence\n"<<std::setprecision(9);
 for(auto&e:performance.events)out<<eventName(e.kind)<<','<<e.seconds<<','<<e.pitchHz<<','<<e.confidence<<'\n';
}
void writeFrames(const fs::path&p,const Performance&performance){
 std::ofstream out(p);out<<"seconds,pitch_hz,pitch_confidence,rms,onset_flux\n"<<std::setprecision(9);
 for(auto&e:performance.frames)out<<e.seconds<<','<<e.pitchHz<<','<<e.pitchConfidence<<','<<e.rms<<','<<e.flux<<'\n';
}
struct Clip {std::string name;bool training=true;double pitch=0.;int sr=0;std::vector<float> samples;};
int invert(const fs::path& manifest,const fs::path& outDir){
 std::ifstream in(manifest);if(!in)return 2;
 std::vector<Clip> clips;
 std::string line;
 while(std::getline(in,line)){
  if(line.empty()||line[0]=='#')continue;
  std::istringstream ss(line);std::string kind,name,hz;
  if(!std::getline(ss,kind,'|')||!std::getline(ss,name,'|')||!std::getline(ss,hz))return 2;
  if(kind!="train"&&kind!="holdout")return 2;
  if(name.empty()||name.find("..")!=std::string::npos||fs::path(name).is_absolute())return 2;
  Clip c;c.name=name;c.training=kind=="train";
  try{c.pitch=std::stod(hz);if(!std::isfinite(c.pitch)||c.pitch<75||c.pitch>700)return 2;}catch(...){return 2;}
  if(!readWav(manifest.parent_path()/name,c.samples,c.sr))return 2;
  clips.push_back(std::move(c));if(clips.size()>12)return 2;
 }
 if(clips.size()<3)return 2;
 std::vector<panorama::joint31::Take> takes;
 for(auto&c:clips)takes.push_back({c.samples.data(),int(c.samples.size()),c.sr,c.pitch,c.training});
 panorama::performance30::CoupledModel prior;
 prior.string.frequencyHz=clips.front().pitch;
 prior.string.pickPosition=.27;prior.string.decayBase=3.2;prior.string.gain=.45;
 auto result=panorama::joint31::infer(takes,prior,1100,4);
 if(!result.success){std::cerr<<"Joint inverse failed\n";return 4;}
 fs::create_directories(outDir);
 std::ofstream ranking(outDir/"ranking.csv");
 ranking<<"rank,topology,initial_train,optimized_train,holdout,training_takes,holdout_takes\n";
 for(size_t i=0;i<result.proposals.size();i++){
  const auto&candidate=result.proposals[i];
  ranking<<i+1<<','<<candidate.topology<<','<<candidate.initialTrainingLoss<<','<<candidate.trainingLoss<<','
         <<candidate.holdoutLoss<<','<<result.trainingTakes<<','<<result.holdoutTakes<<'\n';
  const auto stem="candidate_"+std::to_string(i+1);
  std::ofstream joint(outDir/(stem+".joint31"));joint<<panorama::joint31::serialize(candidate);
  // Representative timbre for MIDI, not a trained articulation map.
  panorama::universal23::Graph patch;
  patch.name="Joint31 " + candidate.topology;
  patch.nodes={{panorama::universal23::Kind::PhysicalCoupled30,-1,-1,0,.9f},
               {panorama::universal23::Kind::Output,0,-1,0,.7f}};
  patch.physical30=panorama::joint31::forTake(candidate,0);
  std::ofstream midi(outDir/(stem+".pgraph"));midi<<panorama::universal23::serialize(patch);
  for(size_t j=0;j<clips.size();j++){
   const auto&pr=candidate.performances[j];
   const auto m=panorama::joint31::forTake(candidate,j);
   const auto wav=panorama::performance30::render(m,clips[j].sr,
     double(clips[j].samples.size())/clips[j].sr,pr.onsetSeconds,
     pr.pitchHz/candidate.structure.string.frequencyHz);
   if(!saveWav(outDir/(stem+"_take_"+std::to_string(j+1)+".wav"),wav,clips[j].sr))return 4;
  }
 }
 const auto&top=result.proposals.front();
 std::ofstream articulation(outDir/"per_take_parameters.csv");
 articulation<<"take,split,pitch_hz,onset_sec,pick_position,decay_base,gain\n";
 for(size_t j=0;j<top.performances.size();j++){
  const auto&p=top.performances[j];
  articulation<<j+1<<','<<(clips[j].training?"training":"holdout")<<','<<p.pitchHz<<','<<p.onsetSeconds<<','
     <<p.pickPosition<<','<<p.decayBase<<','<<p.gain<<'\n';
 }
 std::ofstream note(outDir/"LIMITATIONS.txt");
 note<<"Panoramize 3.1: shared effective string/bridge/body model plus take-specific pluck, damping and gain.\n"
   <<"Research model, synthetic validation. No recovered true guitar body geometry, material, bridge mechanics.\n"
   <<"Holdout recordings never tune shared topology/parameters, but their own pitch/onset/initial pluck/decrease are estimated independently.\n"
   <<".pgraph stores a representative playable model, not per-take articulation mapping or automatic MIDI expression.\n"
   <<"A lower training loss is not proof of physically correct recovered structure.\n";
 std::cout<<"JOINT31_OK candidates="<<result.evaluatedModels<<" best="<<top.topology
          <<" baseline="<<top.initialTrainingLoss<<" optimized="<<top.trainingLoss
          <<" holdout="<<top.holdoutLoss<<"\n";
 return 0;
}
int demo(const fs::path& dir){
 fs::create_directories(dir);
 panorama::performance30::CoupledModel real;
 real.bridge=true;real.body=true;
 real.bridgeHz=525.;real.bodyHz=295.;real.bridgeMix=.38;real.bodyMix=.33;
 real.string.frequencyHz=196.;real.string.inharmonicity=.00012;
 const double picks[]={.16,.26,.37,.22},decays[]={2.1,3.8,7.4,2.6},
 gains[]={.43,.59,.35,.52},steps[]={0.,5.,-5.,7.};
 std::ofstream manifest(dir/"recordings.txt");
 manifest<<"# train|relative wav|known pitch Hz\n";
 std::ofstream truth(dir/"GROUND_TRUTH_PER_TAKE_NOT_FOR_TRAINING.csv");
 truth<<"take,pick,decay,gain,bridge_hz,body_hz\n";
 for(int i=0;i<4;i++){
  auto state=real;state.string.pickPosition=picks[i];state.string.decayBase=decays[i];state.string.gain=gains[i];
  const double ratio=std::pow(2.,steps[i]/12.),pitch=real.string.frequencyHz*ratio;
  const auto name="recording_"+std::to_string(i+1)+".wav";
  const auto audio=panorama::performance30::render(state,16000,1.55,.042,ratio);
  if(!saveWav(dir/name,audio,16000))return 2;
  manifest<<(i==3?"holdout":"train")<<'|'<<name<<'|'<<std::setprecision(12)<<pitch<<'\n';
  truth<<i+1<<','<<picks[i]<<','<<decays[i]<<','<<gains[i]<<","<<real.bridgeHz<<","<<real.bodyHz<<"\n";
 }
 manifest.close();truth.close();
 std::ofstream realOut(dir/"GROUND_TRUTH_STRUCTURE_NOT_FOR_TRAINING.physical30");realOut<<panorama::performance30::serialize(real);realOut.close();
 return invert(dir/"recordings.txt",dir/"inferred");
}
}
int main(int argc,char**argv){
 if(argc!=3){std::cerr<<"Usage: panoramize_joint31 --demo output_dir OR recordings.txt output_dir\n";return 2;}
 try{
  const fs::path src(argv[1]),out(argv[2]);
  if(src=="--validate"){
   std::ifstream in(out);if(!in)return 2;
   const std::string content((std::istreambuf_iterator<char>(in)),{});
   panorama::joint31::Candidate c;
   if(!panorama::joint31::deserialize(content,c))return 3;
   std::cout<<"VALID_JOINT31 "<<c.topology<<" performances="<<c.performances.size()<<'\n';return 0;
  }
  return src=="--demo"?demo(out):invert(src,out);
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 4;}
}
