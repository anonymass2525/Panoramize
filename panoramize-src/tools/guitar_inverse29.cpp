#include "GuitarInverse29.h"
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
#include <vector>
namespace fs=std::filesystem;
using namespace panorama::guitar29;
namespace {
std::uint16_t r16(const unsigned char*p){return std::uint16_t(p[0]|(p[1]<<8));}
std::uint32_t r32(const unsigned char*p){return std::uint32_t(p[0])|(std::uint32_t(p[1])<<8)|(std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24);}
void w16(std::ofstream&f,std::uint16_t x){f.put(char(x));f.put(char(x>>8));}
void w32(std::ofstream&f,std::uint32_t x){for(int i=0;i<4;i++)f.put(char(x>>(i*8)));}
bool wav(const fs::path&path,std::vector<float>&out,int&sr){
 std::ifstream f(path,std::ios::binary);char head[12];f.read(head,12);
 if(!f||std::string(head,4)!="RIFF"||std::string(head+8,4)!="WAVE")return false;
 unsigned short fmt=0,ch=0,bits=0;std::vector<unsigned char>data;unsigned rate=0;
 while(f){unsigned char b[8];f.read(reinterpret_cast<char*>(b),8);if(!f)break;auto size=r32(b+4);
  if(size>100000000)return false;
  const std::string tag(reinterpret_cast<char*>(b),4);
  if(tag=="fmt "){if(size<16||size>1024)return false;std::vector<unsigned char>raw(size);f.read(reinterpret_cast<char*>(raw.data()),size);
   fmt=r16(raw.data());ch=r16(raw.data()+2);rate=r32(raw.data()+4);bits=r16(raw.data()+14);}
  else if(tag=="data"){data.resize(size);f.read(reinterpret_cast<char*>(data.data()),size);}
  else f.seekg(size,std::ios::cur);
  if(size%2)f.seekg(1,std::ios::cur);
 }
 if(!f.eof()&&!f)return false;
 if(!((fmt==1&&(bits==16||bits==24))||(fmt==3&&bits==32))||!(ch==1||ch==2)||rate<8000||rate>48000||data.empty())return false;
 const size_t step=size_t(ch)*(bits/8);if(data.size()%step)return false;
 const size_t frames=data.size()/step;if(frames<rate*.65||frames>rate*12)return false;
 out.resize(frames);sr=int(rate);
 for(size_t i=0;i<frames;i++){
  double val=0;for(int c=0;c<ch;c++){
   const auto*p=data.data()+i*step+size_t(c)*(bits/8);
   if(fmt==3){float x;auto raw=r32(p);std::memcpy(&x,&raw,4);val+=x;}
   else if(bits==16)val+=double(std::int16_t(r16(p)))/32768.;
   else{int32_t x=p[0]|(p[1]<<8)|(p[2]<<16);if(x&0x800000)x|=~0xffffff;val+=double(x)/8388608.;}
  }out[i]=float(val/ch);
 }
 return true;
}
bool saveWav(const fs::path&p,const std::vector<float>&x,int sr){
 std::ofstream f(p,std::ios::binary);if(!f||x.empty()||x.size()>10000000)return false;
 auto bytes=std::uint32_t(x.size()*2);
 f.write("RIFF",4);w32(f,bytes+36);f.write("WAVEfmt ",8);w32(f,16);w16(f,1);w16(f,1);w32(f,sr);w32(f,sr*2);w16(f,2);w16(f,16);
 f.write("data",4);w32(f,bytes);
 for(float v:x){const int16_t s=int16_t(std::lrint(std::clamp(v,-1.f,1.f)*32767.f));w16(f,std::uint16_t(s));}
 return bool(f);
}
struct Clip {std::string path;bool training=true;double knownHz=0.;std::vector<float> audio;int sr=16000;};
int run(const fs::path& manifest,const fs::path& output){
 std::ifstream file(manifest);if(!file){std::cerr<<"Manifest missing\n";return 2;}
 std::string line;std::vector<Clip> clips;
 while(std::getline(file,line)){
  if(line.empty()||line[0]=='#')continue;
  std::istringstream stream(line);std::string kind,name,pitch;
  if(!std::getline(stream,kind,'|')||!std::getline(stream,name,'|')||!std::getline(stream,pitch,'|'))return 2;
  if(kind!="train"&&kind!="holdout")return 2;
  if(name.empty()||name.find("..")!=std::string::npos||fs::path(name).is_absolute())return 2;
  Clip clip;clip.training=kind=="train";clip.path=name;
  try{size_t n=0;clip.knownHz=std::stod(pitch,&n);if(n!=pitch.size()||clip.knownHz<0||clip.knownHz>1100)return 2;}catch(...){return 2;}
  if(!wav(manifest.parent_path()/name,clip.audio,clip.sr)){std::cerr<<"Invalid WAV "<<name<<"\n";return 2;}
  clips.push_back(std::move(clip));if(clips.size()>16)return 2;
 }
 std::vector<Take> takes;for(const auto&c:clips)takes.push_back({c.audio.data(),int(c.audio.size()),c.sr,c.knownHz,c.training});
 const auto result=infer(takes);if(!result.success){std::cerr<<"No model inferred: use isolated dry plucks (at least .65 sec each)\n";return 3;}
 fs::create_directories(output);
 std::ofstream patch(output/"inferred_string.guitar29");patch<<serialize(result.model);patch.close();
 panorama::universal23::Graph playable;playable.name="Panoramize 2.9 / Inferred String";
 playable.nodes={{panorama::universal23::Kind::GuitarString29,-1,-1,0,.95f},
                 {panorama::universal23::Kind::Output,0,-1,0,.9f}};
 playable.guitar=result.model;
 std::string why;auto playableInstance=panorama::universal23::Instrument::prepare(playable,clips.front().sr,&why);
 if(!playableInstance){std::cerr<<"Playable graph failure: "<<why<<"\n";return 4;}
 std::ofstream statePatch(output/"inferred_string.pgraph");statePatch<<panorama::universal23::serialize(playable);statePatch.close();
 std::ofstream report(output/"analysis.csv");report<<"take,type,estimated_f0_hz,onset_s,attack_s,rms,pick_position,base_decay_s_inv,high_decay_s_inv,inharmonicity,spectral_tilt,shared_model_loss\n";
 for(size_t i=0;i<clips.size();i++){
  const auto&a=result.takes[i];const auto&c=clips[i];
  report<<c.path<<','<<(c.training?"train":"holdout")<<','<<std::setprecision(10)<<a.frequencyHz<<','<<a.onsetSeconds<<','<<a.attackSeconds<<','<<a.rms<<','<<a.pickPosition<<','<<a.estimatedDecayBase<<','<<a.estimatedDecayHigh<<','<<a.inharmonicity<<','<<a.spectralTilt<<','<<a.loss<<'\n';
  const double ratio=a.frequencyHz/result.model.frequencyHz;
  const auto reconstructed=render(result.model,c.sr,double(c.audio.size())/c.sr,a.onsetSeconds,ratio);
  if(!saveWav(output/("recreated_shared_take"+std::to_string(i+1)+".wav"),reconstructed,c.sr))return 4;
  // A separate performance-conditioned resynthesis uses evidence FROM this
  // recording. It must NOT be mislabeled as a blind holdout prediction.
  auto adapted=result.model;adapted.pickPosition=a.pickPosition;
  adapted.decayBase=a.estimatedDecayBase;adapted.decayHigh=a.estimatedDecayHigh;
  adapted.spectralTilt=a.spectralTilt;
  const auto conditioned=render(adapted,c.sr,double(c.audio.size())/c.sr,a.onsetSeconds,ratio);
  if(!saveWav(output/("recreated_conditioned_take"+std::to_string(i+1)+".wav"),conditioned,c.sr))return 4;
 }
 std::ofstream summary(output/"summary.txt");summary<<std::setprecision(9)
  <<"Panoramize 2.9 physical STRING model / isolated monophonic pluck only\n"
  <<"shared estimated pick position fraction: "<<result.model.pickPosition<<" (sensor prior "<<result.model.pickupPosition<<")\n"
  <<"shared damping: base="<<result.model.decayBase<<"  high="<<result.model.decayHigh<<"\n"
  <<"shared effective inharmonicity B="<<result.model.inharmonicity<<"\n"
  <<"train feature loss="<<result.trainingLoss<<"  heldout loss="<<result.holdoutLoss<<"\n"
  <<"LIMITS: multiple distinct string/material/shape solutions may fit the same recording.\n"
  <<"Not a calibrated guitar construction material or 3D geometry estimate.\n";
 summary.close();
 std::cout<<"OK pick="<<result.model.pickPosition<<" damping="<<result.model.decayBase
          <<" train="<<result.trainingLoss<<" holdout="<<result.holdoutLoss<<" takes="<<clips.size()<<"\n";
 return 0;
}
int play(const fs::path& graphFile,const fs::path& outputWav){
 std::ifstream in(graphFile);if(!in)return 2;
 const std::string content((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
 panorama::universal23::Graph graph;std::string why;
 if(!panorama::universal23::deserialize(content,graph,&why)){
  std::cerr<<"Cannot read playable patch: "<<why<<"\n";return 2;
 }
 constexpr int sr=48000;auto synth=panorama::universal23::Instrument::prepare(graph,sr,&why);
 if(!synth){std::cerr<<"Cannot prepare playable patch: "<<why<<"\n";return 3;}
 const int frames=sr*5;std::vector<float> audio(size_t(frames),0.f);
 const int notes[]={48,55,60,64,67};
 for(int frame=0;frame<frames;frame++){
  if(frame==0)synth->noteOn(notes[0],.75f);
  if(frame==sr/2)synth->noteOn(notes[1],.70f);
  if(frame==sr)synth->noteOn(notes[2],.65f);
  if(frame==sr*2)synth->noteOn(notes[3],.65f);
  if(frame==sr*5/2)synth->noteOn(notes[4],.60f);
  if(frame==sr*3)for(int note:notes)synth->noteOff(note);
  const auto sample=synth->processOne();audio[size_t(frame)]=.5f*(sample.left+sample.right);
 }
 return saveWav(outputWav,audio,sr)?0:4;
}
int demo(const fs::path&out,bool varied=false){
 fs::create_directories(out);Model ref;ref.frequencyHz=196.;ref.pickPosition=.26;ref.decayBase=2.6;ref.decayHigh=.026;
 ref.inharmonicity=.00015;ref.spectralTilt=.04;ref.gain=.6;
 std::ofstream manifest(out/"guitar_takes.txt");manifest<<"# type|relative_wav|known_pitch_hz (0 enables auto F0)\n";
 const double factors[]={1.,std::pow(2.,5./12.),std::pow(2.,-5./12.),std::pow(2.,7./12.)};
 for(int i=0;i<4;i++){
  const auto name="guitar_reference_"+std::to_string(i+1)+".wav";
  auto performance=ref;
  if(varied){
   const double positions[]={.16,.26,.38,.21};
   const double damp[]={2.6,2.6,8.4,4.1};
   const double brightness[]={.005,.04,.085,.02};
   performance.pickPosition=positions[i];performance.decayBase=damp[i];performance.spectralTilt=brightness[i];
  }
  auto sound=render(performance,16000,2.25,.035+i*.005,factors[i]);
  if(!saveWav(out/name,sound,16000))return 2;
  manifest<<(i==3?"holdout":"train")<<'|'<<name<<'|'<<std::setprecision(12)<<ref.frequencyHz*factors[i]<<'\n';
  std::ofstream truth_i(out/("GROUND_TRUTH_TECHNIQUE_"+std::to_string(i+1)+"_NOT_FOR_TRAINING.guitar29"));
  truth_i<<serialize(performance);
 }
 manifest.close();std::ofstream truth(out/"GROUND_TRUTH_NOT_FOR_TRAINING.guitar29");truth<<serialize(ref);truth.close();
 const int result=run(out/"guitar_takes.txt",out/"result");
 if(result)return result;
 return play(out/"result"/"inferred_string.pgraph",out/"result"/"midi_performance_48k.wav");
}
}
int main(int argc,char**argv){
 if(argc!=3){std::cerr<<"Usage: panoramize_guitar29 guitar_takes.txt output_folder\n"
   "       panoramize_guitar29 --demo output_folder\n";return 2;}
 if(std::string(argv[1])=="--demo")return demo(argv[2]);
 if(std::string(argv[1])=="--demo-techniques")return demo(argv[2],true);
 if(std::string(argv[1])=="--play")return play(argv[2],"inferred_performance_48k.wav");
 return run(argv[1],argv[2]);
}
