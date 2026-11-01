#include "DeepInverse46.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>
namespace fs=std::filesystem;
namespace deep=panorama::deep46;
namespace gen=panorama::gen37;
namespace fem=panorama::fem36;
namespace inv=panorama::inverse36;
namespace {
uint32_t u32(const unsigned char*p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
uint16_t u16(const unsigned char*p){return uint16_t(p[0]|(p[1]<<8));}
void w16(std::ofstream&f,uint32_t v){for(int i=0;i<2;i++)f.put(char(v>>(8*i)));}
void w32(std::ofstream&f,uint32_t v){for(int i=0;i<4;i++)f.put(char(v>>(8*i)));}
bool saveWav(const fs::path&file,const std::vector<float>&a,int sr){
 if(a.empty()||a.size()>size_t(sr)*30||!fs::exists(file.parent_path()))return false;
 std::ofstream f(file,std::ios::binary);if(!f)return false;
 uint32_t bytes=uint32_t(a.size()*4);f.write("RIFF",4);w32(f,36+bytes);f.write("WAVEfmt ",8);
 w32(f,16);w16(f,3);w16(f,1);w32(f,sr);w32(f,sr*4);w16(f,4);w16(f,32);f.write("data",4);w32(f,bytes);
 for(float x:a)f.write(reinterpret_cast<const char*>(&x),4);return bool(f);
}
bool loadWav(const fs::path&file,std::vector<float>&out,int&sr){
 std::ifstream f(file,std::ios::binary);char header[12];f.read(header,12);
 if(!f||std::string(header,4)!="RIFF"||std::string(header+8,4)!="WAVE")return false;
 uint16_t fmt=0,bits=0,ch=0;uint32_t rate=0;std::vector<unsigned char>data;
 while(f){unsigned char h[8];f.read((char*)h,8);if(!f)break;
  const uint32_t length=u32(h+4);if(length>20000000)return false;
  const std::string tag(reinterpret_cast<char*>(h),4);
  if(tag=="fmt "){
   if(length<16||length>1024)return false;std::vector<unsigned char>b(length);f.read((char*)b.data(),length);
   fmt=u16(b.data());ch=u16(b.data()+2);rate=u32(b.data()+4);bits=u16(b.data()+14);
  }else if(tag=="data"){
   data.resize(length);f.read((char*)data.data(),length);
  }else f.seekg(length,std::ios::cur);
  if(length&1)f.seekg(1,std::ios::cur);
 }
 if(ch<1||ch>2||rate<8000||rate>96000||!((fmt==1&&(bits==16||bits==24))||(fmt==3&&bits==32)))return false;
 const int width=bits/8,frame=width*ch;
 if(data.empty()||data.size()%frame)return false;
 const size_t n=data.size()/frame;if(n<256||n>size_t(rate)*30)return false;
 out.resize(n);sr=int(rate);
 for(size_t i=0;i<n;i++){
  double sum=0;for(int c=0;c<ch;c++){
   const unsigned char*p=data.data()+i*frame+c*width;
   if(fmt==3){float val;std::memcpy(&val,p,4);sum+=val;}
   else if(bits==16)sum+=int16_t(u16(p))/32768.;
   else{int v=int(p[0]|(p[1]<<8)|(p[2]<<16));if(v&0x800000)v|=~0xffffff;sum+=v/8388608.;}
  }
  out[i]=float(sum/ch);if(!std::isfinite(out[i]))return false;
 }return true;
}
void exportAnalysis(const deep::Analysis&a,const fs::path&dir){
 std::ofstream f(dir/"long_timbre_frames.csv");f<<"seconds,rms,spectral_centroid_hz,brightness,flatness,pitch_hz,pitch_confidence\n";
 for(auto&x:a.frames)f<<std::setprecision(8)<<x.seconds<<','<<x.rms<<','<<x.centroidHz<<','<<x.brightness<<','<<x.flatness<<','<<x.pitchHz<<','<<x.pitchConfidence<<'\n';
 std::ofstream s(dir/"half_second_sections.csv");s<<"start_sec,end_sec,rms,spectral_centroid_hz,brightness\n";
 for(auto&x:a.sections)s<<x.fromSec<<','<<x.toSec<<','<<x.rms<<','<<x.centroidHz<<','<<x.brightness<<'\n';
 std::ofstream e(dir/"technique_candidates.csv");e<<"type,time_sec,confidence,magnitude,rate_hz\n";
 for(auto&x:a.gestures)e<<deep::gestureName(x.kind)<<','<<x.timeSec<<','<<x.confidence<<','<<x.magnitude<<','<<x.rateHz<<'\n';
 std::ofstream summary(dir/"analysis_summary.txt");summary<<"recording_seconds="<<a.durationSec<<"\ncentroid_change_hz="<<a.centroidChange<<"\nvalid_pitch_fraction="<<a.validPitchFraction<<"\ntechnique_events="<<a.gestures.size()<<"\n";
}
int analysis(const fs::path&input,const fs::path&dir){
 std::vector<float>audio;int rate=0;if(!loadWav(input,audio,rate))return 3;
 auto out=deep::analyze(audio,rate,rate<=22050?256:512);
 if(!out.error.empty()){std::cerr<<out.error<<'\n';return 3;}
 fs::create_directories(dir);exportAnalysis(out,dir);
 std::cout<<"ANALYSIS46_OK sec="<<out.durationSec<<" frames="<<out.frames.size()<<" events="<<out.gestures.size()<<" centroid_change="<<out.centroidChange<<"\n";
 return 0;
}
int wave(const fs::path&input,const fs::path&validation,const fs::path&dir,int family,double pickup,int budget){
 std::vector<float>a,b;int rate=0,r2=0;if(!loadWav(input,a,rate)||!loadWav(validation,b,r2)||rate!=r2)return 3;
 auto r=deep::fitWave(a,b,rate,family==0?gen::Family::String1D:gen::Family::Membrane2D,pickup,budget);
 if(!r.error.empty()){std::cerr<<r.error<<'\n';return 4;}
 fs::create_directories(dir);
 std::ofstream ranking(dir/"wave_ranking.csv");ranking<<"rank,train_loss,holdout_loss,exciter_position,pickup_position,nx,ny,fundamental_hz,damping\n";
 for(size_t i=0;i<r.proposals.size();i++){
  auto&c=r.proposals[i];const auto name="candidate_"+std::to_string(i+1);
  std::ofstream(dir/(name+".wave37"))<<gen::encodeWaveRecipe(c.model);
  auto generated=gen::renderWave(c.model,rate,a.size());saveWav(dir/(name+".wav"),generated,rate);
  ranking<<i+1<<','<<c.trainingLoss<<','<<c.holdoutLoss<<','<<c.model.exciter<<','<<c.model.pickup<<','<<c.model.nx<<','<<c.model.ny<<','<<c.model.fundamentalHz<<','<<c.model.damping<<'\n';
 }
 std::cout<<"WAVE46_OK evaluations="<<r.evaluations<<" best="<<r.proposals[0].trainingLoss<<" inferred_exciter="<<r.proposals[0].model.exciter<<"\n";return 0;
}
int physical(const fs::path&manifest,const fs::path&dir,int budget){
 std::ifstream f(manifest);if(!f)return 2;std::string line;std::vector<inv::Take>takes;int rate=0;
 while(std::getline(f,line)){
  if(line.empty()||line[0]=='#')continue;
  std::istringstream s(line);std::string split,path,note,velocity;
  if(!std::getline(s,split,'|')||!std::getline(s,path,'|')||!std::getline(s,note,'|')||!std::getline(s,velocity))return 2;
  if((split!="train"&&split!="holdout")||path.empty()||fs::path(path).is_absolute()||path.find("..")!=std::string::npos)return 2;
  inv::Take t;t.training=split=="train";
  try{t.midi=std::stoi(note);t.velocity=std::stof(velocity);}catch(...){return 2;}
  int sr=0;if(!loadWav(manifest.parent_path()/path,t.audio,sr))return 3;
  if(rate&&sr!=rate)return 3;rate=sr;takes.push_back(std::move(t));
 }
 auto r=deep::fit3D(takes,rate,budget);if(!r.error.empty()){std::cerr<<r.error<<'\n';return 3;}
 fs::create_directories(dir);std::ofstream rank(dir/"shape_material_ranking.csv");rank<<"rank,train_loss,holdout_loss,mesh_vertices,tetrahedra,young_pa,density,anisotropy_x,pluck_position,length,width,thickness,camber,taper\n";
 for(size_t i=0;i<r.proposals.size();i++){
  auto&c=r.proposals[i];const auto name="candidate_"+std::to_string(i+1);
  std::ofstream(dir/(name+".fem36"))<<fem::serialize(c.model);
  std::ofstream(dir/(name+".shape37"))<<gen::encodeShape(c.shape);
  rank<<i+1<<','<<c.trainingLoss<<','<<c.validationLoss<<','<<c.model.mesh.vertices.size()<<','<<c.model.mesh.tetrahedra.size()<<','<<c.model.meshYoung<<','<<c.model.meshDensity<<','<<c.model.anisotropy[0]<<','<<c.model.pluckPosition<<','<<c.shape.length<<','<<c.shape.width<<','<<c.shape.thickness<<','<<c.shape.camber<<','<<c.shape.taper<<'\n';
 }
 std::cout<<"FEM46_OK evals="<<r.evaluations<<" train="<<r.proposals[0].trainingLoss<<" holdout="<<r.proposals[0].validationLoss<<" vertices="<<r.proposals[0].model.mesh.vertices.size()<<"\n";return 0;
}
int demo(const fs::path&dir){
 fs::create_directories(dir);
 const int rate=16000;constexpr double pi=3.14159265358979323846;
 std::vector<float>longtone(size_t(rate)*8);double phase=0;
 for(size_t i=0;i<longtone.size();i++){
  double t=double(i)/rate,slide=std::clamp((t-3.)/.25,0.,1.);
  double hz=220*std::pow(2.,slide*4./12.)*std::pow(2.,.45*std::sin(2*pi*5.8*t)/12.);
  phase+=2*pi*hz/rate;
  double overtone=t<5.2?.12:.52;
  longtone[i]=float(.18*(std::sin(phase)+overtone*std::sin(2*phase)));
 }
 saveWav(dir/"synthetic_legato_vibrato_timbre.wav",longtone,rate);
 if(analysis(dir/"synthetic_legato_vibrato_timbre.wav",dir))return 2;
 for(int family=0;family<=1;family++){
  gen::WaveRecipe source;source.family=family?gen::Family::Membrane2D:gen::Family::String1D;
  source.nx=24;source.ny=16;source.fundamentalHz=196;source.exciter=.28;source.pickup=.64;
  auto a=gen::renderWave(source,22050,3072);
  saveWav(dir/(family?"membrane_training.wav":"string_training.wav"),a,22050);
  auto holdout=a;
  for(size_t i=0;i<holdout.size();i++)holdout[i]=.96f*holdout[i]+float(.00003*std::sin(2*pi*i/113.));
  saveWav(dir/(family?"membrane_holdout.wav":"string_holdout.wav"),holdout,22050);
  auto target=dir/(family?"membrane_fit":"string_fit");
  if(wave(dir/(family?"membrane_training.wav":"string_training.wav"),dir/(family?"membrane_holdout.wav":"string_holdout.wav"),target,family,.64,48))return 3;
 }
 fem::Model target;gen::Shape shape;shape.xCells=2;shape.yCells=1;shape.zCells=1;shape.camber=.10;shape.taper=-.1;
 if(!gen::buildTetraMesh(shape,target.mesh))return 4;
 target.meshDensity=1.3e5;target.meshYoung=5.2e8;target.pluckPosition=.72;
 std::ofstream(dir/"GROUND_TRUTH_NOT_USED_BY_INFERENCE.fem36")<<fem::serialize(target);
 std::ofstream manifest(dir/"fem_recordings.txt");
 for(int i=0;i<3;i++){
  int midi=40+i*5;float velocity=.6f+.1f*i;
  auto a=inv::render(target,22050,midi,velocity,1152);
  const auto name="fem_take_"+std::to_string(i+1)+".wav";
  if(!saveWav(dir/name,a,22050))return 4;
  manifest<<(i==2?"holdout":"train")<<'|'<<name<<'|'<<midi<<'|'<<velocity<<'\n';
 }
 manifest.close();
 return physical(dir/"fem_recordings.txt",dir/"fem_fit",64);
}
}
int main(int argc,char**argv){
 if(argc==3&&std::string(argv[1])=="--demo")return demo(argv[2]);
 if(argc==4&&std::string(argv[1])=="--analyze")return analysis(argv[2],argv[3]);
 if(argc>=7&&argc<=8&&std::string(argv[1])=="--fit-wave"){
  const std::string f=argv[2];if(f!="string"&&f!="membrane")return 2;
  double pickup=0;int budget=48;
  try{pickup=std::stod(argv[6]);if(argc==8)budget=std::stoi(argv[7]);}catch(...){return 2;}
  return wave(argv[3],argv[4],argv[5],f=="string"?0:1,pickup,budget);
 }
 if((argc==4||argc==5)&&std::string(argv[1])=="--fit-fem"){
  int budget=50;try{if(argc==5)budget=std::stoi(argv[4]);}catch(...){return 2;}
  return physical(argv[2],argv[3],budget);
 }
 std::cerr<<"usage: deep46 --demo folder | --analyze audio.wav out_folder | --fit-wave [string|membrane] train.wav holdout.wav out_folder pickup_fraction [evaluations] | --fit-fem manifest.txt out_folder [evaluations]\n";
 return 1;
}
