#include "MeshInverse36.h"
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
using panorama::fem36::Model;
using panorama::inverse36::Take;
using panorama::inverse36::render;
namespace {
unsigned u16(const unsigned char*p){return unsigned(p[0]|(p[1]<<8));}
unsigned u32(const unsigned char*p){return unsigned(p[0])|(unsigned(p[1])<<8)|(unsigned(p[2])<<16)|(unsigned(p[3])<<24);}
void w16(std::ofstream&f,unsigned v){f.put(char(v));f.put(char(v>>8));}
void w32(std::ofstream&f,unsigned v){for(int i=0;i<4;i++)f.put(char(v>>(i*8)));}
bool readWav(const fs::path&path,std::vector<float>&samples,int&rate){
 std::ifstream f(path,std::ios::binary);char header[12];f.read(header,12);
 if(!f||std::string(header,4)!="RIFF"||std::string(header+8,4)!="WAVE")return false;
 unsigned fmt=0,bits=0,ch=0,sr=0;std::vector<unsigned char>data;
 while(f){unsigned char h[8];f.read((char*)h,8);if(!f)break;
  unsigned count=u32(h+4);if(count>25000000)return false;
  std::string tag((char*)h,4);
  if(tag=="fmt "){
   if(count<16||count>1024)return false;
   std::vector<unsigned char>block(count);f.read((char*)block.data(),count);
   fmt=u16(block.data());ch=u16(block.data()+2);sr=u32(block.data()+4);bits=u16(block.data()+14);
  }else if(tag=="data"){
   data.resize(count);f.read((char*)data.data(),count);
  }else f.seekg(count,std::ios::cur);
  if(count&1)f.seekg(1,std::ios::cur);
 }
 if(!f.eof()&&!f.good())return false;
 if(ch<1||ch>2||sr<22050||sr>96000||!((fmt==1&&(bits==16||bits==24))||(fmt==3&&bits==32)))return false;
 unsigned width=bits/8,frame=width*ch;
 if(data.empty()||data.size()%frame)return false;
 rate=int(sr);const size_t n=data.size()/frame;
 if(n<256||n>size_t(sr)*2)return false;
 samples.resize(n);
 for(size_t i=0;i<n;i++){
  double sum=0;
  for(unsigned c=0;c<ch;c++){
   const auto*ptr=data.data()+i*frame+c*width;
   if(fmt==3){unsigned b=u32(ptr);float x;std::memcpy(&x,&b,4);sum+=x;}
   else if(bits==16)sum+=double(std::int16_t(u16(ptr)))/32768.;
   else{int x=int(ptr[0]|(ptr[1]<<8)|(ptr[2]<<16));if(x&0x800000)x|=~0xffffff;sum+=double(x)/8388608.;}
  }
  samples[i]=float(sum/ch);if(!std::isfinite(samples[i]))return false;
 }return true;
}
bool writeWav(const fs::path&p,const std::vector<float>&x,int rate){
 std::ofstream f(p,std::ios::binary);if(!f)return false;unsigned bytes=unsigned(x.size()*4);
 f.write("RIFF",4);w32(f,36+bytes);f.write("WAVEfmt ",8);w32(f,16);w16(f,3);w16(f,1);w32(f,unsigned(rate));w32(f,unsigned(rate*4));w16(f,4);w16(f,32);
 f.write("data",4);w32(f,bytes);for(float v:x)f.write((const char*)&v,4);return bool(f);
}
bool lines(const fs::path&p,std::vector<std::string>&out){
 std::ifstream f(p);if(!f)return false;std::string s;
 while(std::getline(f,s)){if(s.empty()||s[0]=='#')continue;out.push_back(s);}
 return !out.empty();
}
bool safe(const std::string&s){return !s.empty()&&s.find("..") ==std::string::npos&&!fs::path(s).is_absolute();}
int invert(const fs::path&manifest,const fs::path&models,const fs::path&out,int limit){
 std::vector<std::string> tlines,mlines;
 if(!lines(manifest,tlines)||!lines(models,mlines))return 2;
 std::vector<Take> takes;std::vector<Model> seeds;int rate=0;
 for(const auto&line:tlines){
  std::istringstream ss(line);std::string split,file,note,vel;
  if(!std::getline(ss,split,'|')||!std::getline(ss,file,'|')||!std::getline(ss,note,'|')||!std::getline(ss,vel))return 2;
  if((split!="train"&&split!="holdout")||!safe(file))return 2;
  Take t;int r;
  try{t.midi=std::stoi(note);t.velocity=std::stof(vel);}catch(...){return 2;}
  if(!readWav(manifest.parent_path()/file,t.audio,r))return 3;
  if(rate && rate!=r)return 3;rate=r;t.training=split=="train";
  takes.push_back(std::move(t));
 }
 for(const auto&file:mlines){
  if(!safe(file))return 2;
  std::ifstream f(models.parent_path()/file);if(!f)return 2;
  std::string content((std::istreambuf_iterator<char>(f)),{});
  Model m;if(!panorama::fem36::deserialize(content,m))return 3;
  seeds.push_back(std::move(m));
 }
 auto r=panorama::inverse36::infer(takes,seeds,rate,limit,3);
 if(!r.error.empty()){std::cerr<<r.error<<'\n';return 4;}
 fs::create_directories(out);
 std::ofstream rank(out/"ranking.csv");rank<<"rank,mesh_index,training_loss,holdout_loss,vertices,tetrahedra,young_pa,bridge_coupling,pluck_position,string_tension_scale,anisotropy_x,anisotropy_y,anisotropy_z\n";
 for(size_t i=0;i<r.proposals.size();i++){
  const auto&p=r.proposals[i];const auto&m=p.model;
  std::string stem="candidate_"+std::to_string(i+1);
  std::ofstream model(out/(stem+".fem36"));model<<panorama::fem36::serialize(m);
  std::ofstream mesh(out/(stem+".mesh36"));mesh<<panorama::fem36::serializeMesh(m.mesh);
  rank<<i+1<<','<<p.meshCandidate<<','<<std::setprecision(12)<<p.trainingLoss<<','<<p.validationLoss<<','<<m.mesh.vertices.size()<<','<<m.mesh.tetrahedra.size()<<','<<m.meshYoung<<','<<m.bridgeCoupling<<','<<m.pluckPosition<<','<<m.stringTensionScale<<','<<m.anisotropy[0]<<','<<m.anisotropy[1]<<','<<m.anisotropy[2]<<'\n';
  for(size_t j=0;j<takes.size();j++){
   const auto&take=takes[j];
   auto audio=render(m,rate,take.midi,take.velocity,take.audio.size());
   if(!writeWav(out/(stem+"_take_"+std::to_string(j+1)+".wav"),audio,rate))return 5;
  }
 }
 std::ofstream limits(out/"LIMITATIONS.txt");limits<<"Experimental research model: finite tetra mesh library (<=32 vertices, <=96 tetrahedra). No free-form unknown shape discovery.\n"
 <<"Anisotropic SPD elastic tensor = S * D_iso * S, not calibrated wood grain or nonlinear constitutive law.\n"
 <<"Input must be onset aligned, dry mono/stereo PCM16/24 or float32 WAV, midi/velocity supplied. 2sec maximum.\n"
 <<"Optimization only uses training clips; holdout notes used for independent scoring but known MIDI/velocity.\n"
 <<"Ranking depends on candidate mesh library and initialization; same timbre can result from many material/geometry configurations.\n"
 <<"3.6 solver is a standalone C++ core, not yet available as a new VST3 GUI node. Studio One/Windows untested.\n";
 std::cout<<"INVERSE36_OK evals="<<r.evaluations<<" candidates="<<r.proposals.size()<<" best="<<r.proposals.front().trainingLoss<<"\n";
 return 0;
}
int demo(const fs::path&dir){
 fs::create_directories(dir);
 Model base;Model target=base;
 target.mesh.vertices.push_back({.1,.06,.14});
 target.mesh.tetrahedra.push_back({4,5,6,8});
 target.mesh.bridgeVertex[5]=8;
 target.anisotropy={1.3,.65,1.8,.8,1.4,.55};
 target.pluckPosition=.72;target.bridgeCoupling=.095;target.meshYoung=5.4e8;
 std::ofstream a(dir/"input_models.txt");
 for(int i=0;i<2;i++){
  auto m=i==0?base:target;
  m.pluckPosition=.71;m.meshYoung=4.5e8;m.bridgeCoupling=.08;
  m.anisotropy={1,1,1,1,1,1};
  const auto name="mesh_hypothesis_"+std::to_string(i+1)+".fem36";
  std::ofstream file(dir/name);file<<panorama::fem36::serialize(m);
  a<<name<<'\n';
 }
 a.close();std::ofstream manifest(dir/"recordings.txt");
 for(int i=0;i<3;i++){
  int midi=40+5*i;float velocity=.6f+.1f*i;
  const auto audio=render(target,22050,midi,velocity,1152);
  if(audio.empty())return 6;
  const auto name="recording_"+std::to_string(i+1)+".wav";
  if(!writeWav(dir/name,audio,22050))return 6;
  manifest<<(i==2?"holdout":"train")<<'|'<<name<<'|'<<midi<<'|'<<velocity<<'\n';
 }
 manifest.close();
 std::ofstream truth(dir/"GROUND_TRUTH_NOT_USED_BY_OPTIMIZER.fem36");truth<<panorama::fem36::serialize(target);truth.close();
 return invert(dir/"recordings.txt",dir/"input_models.txt",dir/"inferred",42);
}
}
int main(int argc,char**argv){
 if(argc==3&&std::string(argv[1])=="--demo")return demo(argv[2]);
 if(argc!=4&&argc!=5){std::cerr<<"usage: panoramize_inverse36 recordings.txt input_models.txt output_dir [evaluations]\n"
 <<"       panoramize_inverse36 --demo output_dir\n";return 1;}
 int evaluations=80;try{if(argc==5)evaluations=std::stoi(argv[4]);}catch(...){return 1;}
 return invert(argv[1],argv[2],argv[3],evaluations);
}
