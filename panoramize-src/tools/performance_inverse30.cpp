#include "PerformancePhysical30.h"
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
struct Clip {std::string name;bool train=true;double pitch=0.;int sr=0;std::vector<float> samples;};
int invert(const fs::path&manifest,const fs::path&out){
 std::ifstream in(manifest);if(!in)return 2;
 std::vector<Clip> clips;std::string line;
 while(std::getline(in,line)){
  if(line.empty()||line[0]=='#')continue;
  std::istringstream s(line);std::string kind,name,hz;
  if(!std::getline(s,kind,'|')||!std::getline(s,name,'|')||!std::getline(s,hz))return 2;
  Clip c;c.train=kind=="train";c.name=name;
  if(kind!="train"&&kind!="holdout")return 2;
  if(name.empty()||name.find("..")!=std::string::npos||fs::path(name).is_absolute())return 2;
  try {c.pitch=std::stod(hz);if(!std::isfinite(c.pitch)||c.pitch<0||c.pitch>1000)return 2;}catch(...){return 2;}
  if(!readWav(manifest.parent_path()/name,c.samples,c.sr)){std::cerr<<"Invalid WAV: "<<name<<'\n';return 2;}
  clips.push_back(std::move(c));if(clips.size()>12)return 2;
 }
 if(clips.empty())return 2;
 panorama::guitar29::Model prior;prior.frequencyHz=clips.front().pitch>75?clips.front().pitch:196.;
 // The string parameters below are explicit priors. The 3.0 search only optimizes
 // effective bridge/body topology and their tested modal center frequencies.
 std::vector<Take> takes;for(auto&c:clips)takes.push_back({c.samples.data(),int(c.samples.size()),c.sr,c.pitch,c.train});
 auto discovery=discover(takes,prior,70,4);if(!discovery.success)return 3;
 fs::create_directories(out);
 std::ofstream ranks(out/"candidates.csv");ranks<<"rank,topology,train_feature_loss,holdout_feature_loss,bridge_hz,body_hz,excite_x,excite_y,excite_z,pickup_x,pickup_y,pickup_z\n"<<std::setprecision(10);
 for(size_t i=0;i<discovery.proposals.size();i++){
  const auto&p=discovery.proposals[i];const auto name="candidate_"+std::to_string(i+1)+".physical30";
  std::ofstream patch(out/name);patch<<serialize(p.model);
  panorama::universal23::Graph playable;
  playable.name="Panoramize 3.0 / "+p.topology;
  playable.nodes={{panorama::universal23::Kind::PhysicalCoupled30,-1,-1,0,.85f},
                  {panorama::universal23::Kind::Output,0,-1,0,.85f}};
  playable.physical30=p.model;
  std::ofstream vstPatch(out/("candidate_"+std::to_string(i+1)+".pgraph"));
  vstPatch<<panorama::universal23::serialize(playable);
  ranks<<i+1<<','<<p.topology<<','<<p.trainingLoss<<','<<p.holdoutLoss<<','<<p.model.bridgeHz<<','<<p.model.bodyHz;
  for(double v:p.model.excitation)ranks<<','<<v;
  for(double v:p.model.pickup)ranks<<','<<v;
  ranks<<'\n';
  const auto&reference=clips.front();const double f=reference.pitch>75?reference.pitch:discovery.performances[0].medianPitchHz;
  double onset=.035;
  for(const auto&event:discovery.performances[0].events)if(event.kind==EventKind::Pluck){onset=event.seconds;break;}
  const auto resynth=render(p.model,reference.sr,double(reference.samples.size())/reference.sr,onset,f/prior.frequencyHz);
  if(!saveWav(out/("candidate_"+std::to_string(i+1)+".wav"),resynth,reference.sr))return 4;
 }
 for(size_t i=0;i<clips.size();i++){
  writeFrames(out/("take_"+std::to_string(i+1)+"_frames.csv"),discovery.performances[i]);
  writeEvents(out/("take_"+std::to_string(i+1)+"_events.csv"),discovery.performances[i]);
 }
 std::ofstream limitations(out/"README_LIMITATIONS.txt");limitations<<
  "Panoramize 3.0 RESEARCH PROTOTYPE / OFFLINE ANALYSIS ONLY\n"
  "Event labels are heuristic candidates, NOT verified guitar technique recognition.\n"
  "Known pitch is an optional manifest prior. Multiple simultaneous notes and\n"
  "strong room reverberation, distortion, unknown pickup EQ are unsupported.\n"
  "Topology: string, effective bridge and body modal filters with 3 fixed axes.\n"
  "NOT a reconstructed 3D guitar shape, material density or FEM geometry.\n"
  "String properties and effective 3-axis directions fixed, not searched.\n"
  "The training score ranks candidates; holdout is evaluation ONLY.\n"
  "Topologies can be acoustically nonidentifiable from a short recording.\n"
  "These .physical30 files are OFFLINE research presets, not VST3 .pgraph patches.\n";
 std::cout<<"OK best="<<discovery.proposals.front().topology<<" train="<<discovery.proposals.front().trainingLoss
          <<" holdout="<<discovery.proposals.front().holdoutLoss<<"\n";
 return 0;
}
int play(const fs::path&graphFile,const fs::path&outWav){
 std::ifstream in(graphFile);if(!in)return 2;
 const std::string data((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
 panorama::universal23::Graph graph;std::string error;
 if(!panorama::universal23::deserialize(data,graph,&error)){
  std::cerr<<"Invalid .pgraph: "<<error<<'\n';return 3;
 }
 constexpr int sr=48000;
 auto synth=panorama::universal23::Instrument::prepare(graph,sr,&error);
 if(!synth){std::cerr<<"Cannot prepare: "<<error<<'\n';return 4;}
 std::vector<float> samples(size_t(sr)*4);
 for(size_t i=0;i<samples.size();i++){
  if(i==0)synth->noteOn(52,.75f);
  if(i==sr/2)synth->noteOn(57,.70f);
  if(i==sr)synth->noteOn(60,.78f);
  if(i==sr*3/2)synth->noteOn(64,.70f);
  if(i==sr*2)synth->noteOn(67,.65f);
  if(i==sr*5/2){for(auto n:{52,57,60,64,67})synth->noteOff(n);}
  const auto v=synth->processOne();samples[i]=.5f*(v.left+v.right);
 }
 return saveWav(outWav,samples,sr)?0:5;
}
int single(const fs::path&,const fs::path&);
int generateEventExample(const fs::path&dir){
 constexpr int sr=16000;constexpr double pi=3.141592653589793;
 std::vector<float> phrase(size_t(sr*23/10),0.f);double phase=0.;
 for(size_t i=0;i<phrase.size();i++){
  const double sec=double(i)/sr;if(sec<.10)continue;
  double f=sec<1.15?196.:246.94;
  if(sec>1.6)f*=std::pow(2.,.65*std::sin(2*pi*5*(sec-1.6))/12.);
  phase+=2*pi*f/sr;
  double level=sec<1.8?.35:.035;
  phrase[i]=float(level*std::sin(phase));
 }
 if(!saveWav(dir/"synthetic_performance_events.wav",phrase,sr))return 3;
 return single(dir/"synthetic_performance_events.wav",dir/"performance_analysis");
}
int demo(const fs::path&out){
 fs::create_directories(out);CoupledModel source;source.bridge=true;source.body=true;
 source.bridgeHz=480.;source.bodyHz=270.;source.string.frequencyHz=196.;
 source.string.pickPosition=.24;source.string.decayBase=2.4;source.string.decayHigh=.015;
 std::ofstream m(out/"recordings.txt");m<<"# train|relative wav|known F0 Hz (0 for auto)\n";
 for(int i=0;i<4;i++){
  const double offset=(i==0?0:i==1?5:i==2?-5:7);
  const auto name="recording_"+std::to_string(i+1)+".wav";
  auto played=source;
  static constexpr double pickPositions[4]={.20,.24,.28,.22};
  static constexpr double decay[4]={2.4,2.8,3.4,2.6};
  played.string.pickPosition=pickPositions[i];played.string.decayBase=decay[i];
  const auto samples=render(played,16000,1.7,.035,std::pow(2.,offset/12.));
  std::ofstream each(out/("GROUND_TRUTH_TAKE_"+std::to_string(i+1)+".physical30"));each<<serialize(played);
  if(!saveWav(out/name,samples,16000))return 3;
  m<<(i==3?"holdout":"train")<<'|'<<name<<'|'<<std::setprecision(10)<<196.*std::pow(2.,offset/12.)<<'\n';
 }
 m.close();
 std::ofstream truth(out/"GROUND_TRUTH_NOT_FOR_TRAINING.physical30");truth<<serialize(source);truth.close();
 const int result=invert(out/"recordings.txt",out/"inferred");if(result)return result;
 if(play(out/"inferred"/"candidate_1.pgraph",out/"inferred"/"midi_performance.wav"))return 5;
 return generateEventExample(out);
}
int single(const fs::path&p,const fs::path&out){
 std::vector<float> samples;int sr=0;if(!readWav(p,samples,sr))return 2;
 auto result=analyze(samples.data(),int(samples.size()),sr);if(!result.valid)return 3;
 fs::create_directories(out);writeEvents(out/"events.csv",result);writeFrames(out/"tracking.csv",result);
 std::cout<<"OK estimated_median_pitch="<<result.medianPitchHz<<" events="<<result.events.size()<<'\n';
 return 0;
}
}
int main(int argc,char**argv){
 if(argc!=3){std::cerr<<"Usage: panoramize_performance30 --demo output_dir\n"
   "       panoramize_performance30 recording.wav output_dir (audio events only)\n"
   "       panoramize_performance30 recordings.txt output_dir (shared topology search)\n"
   "       panoramize_performance30 candidate.pgraph output.wav (MIDI preview)\n";return 2;}
 try{const fs::path input(argv[1]),output(argv[2]);
  if(input=="--demo")return demo(output);
  if(input.extension()==".wav")return single(input,output);
  if(input.extension()==".pgraph")return play(input,output);
  return invert(input,output);
 }catch(const std::exception& e){std::cerr<<"Error: "<<e.what()<<'\n';return 4;}
}
