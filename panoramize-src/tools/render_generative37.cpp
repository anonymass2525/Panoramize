#include "ShapeInverse37.h"
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
#include <vector>
namespace fs=std::filesystem;
using panorama::inverse36::Take;
namespace {
unsigned rd16(const unsigned char*p){return unsigned(p[0])|(unsigned(p[1])<<8);}
unsigned rd32(const unsigned char*p){return rd16(p)|(rd16(p+2)<<16);}
void w16(std::ofstream&f,unsigned v){f.put(char(v));f.put(char(v>>8));}
void w32(std::ofstream&f,unsigned v){for(int i=0;i<4;i++)f.put(char(v>>(8*i)));}
bool wav(const fs::path&p,std::vector<float>&out,int&rate){
 std::ifstream in(p,std::ios::binary);char hdr[12];in.read(hdr,12);
 if(!in||std::string(hdr,4)!="RIFF"||std::string(hdr+8,4)!="WAVE")return false;
 unsigned fmt=0,channels=0,sr=0,bits=0;std::vector<unsigned char>data;
 while(in){unsigned char h[8];in.read((char*)h,8);if(!in)break;
  unsigned bytes=rd32(h+4);if(bytes>20000000)return false;
  auto tag=std::string((char*)h,4);
  if(tag=="fmt "){
   if(bytes<16||bytes>1024)return false;std::vector<unsigned char>v(bytes);in.read((char*)v.data(),bytes);
   fmt=rd16(v.data());channels=rd16(v.data()+2);sr=rd32(v.data()+4);bits=rd16(v.data()+14);
  }else if(tag=="data"){
   data.resize(bytes);in.read((char*)data.data(),bytes);
  }else in.seekg(bytes,std::ios::cur);
  if(bytes&1)in.seekg(1,std::ios::cur);
 }
 if(channels<1||channels>2||sr<22050||sr>96000||!((fmt==1&&(bits==16||bits==24))||(fmt==3&&bits==32)))return false;
 const unsigned width=bits/8,frame=width*channels;
 if(data.empty()||data.size()%frame)return false;
 size_t count=data.size()/frame;
 if(count<256||count>size_t(sr)*2)return false;
 out.resize(count);rate=int(sr);
 for(size_t i=0;i<count;i++){
  double value=0;
  for(unsigned c=0;c<channels;c++){
   const unsigned char*v=data.data()+i*frame+c*width;
   if(fmt==3){unsigned raw=rd32(v);float x;std::memcpy(&x,&raw,4);value+=x;}
   else if(bits==16)value+=double(std::int16_t(rd16(v)))/32768.;
   else{int n=int(v[0]|(v[1]<<8)|(v[2]<<16));if(n&0x800000)n|=~0xffffff;value+=double(n)/8388608.;}
  }
  out[i]=float(value/channels);if(!std::isfinite(out[i]))return false;
 }return true;
}
bool save(const fs::path&p,const std::vector<float>&a,int rate){
 if(a.empty())return false;
 std::ofstream f(p,std::ios::binary);if(!f)return false;
 unsigned bytes=unsigned(a.size()*4);
 f.write("RIFF",4);w32(f,36+bytes);f.write("WAVEfmt ",8);w32(f,16);w16(f,3);w16(f,1);w32(f,unsigned(rate));w32(f,unsigned(rate*4));w16(f,4);w16(f,32);f.write("data",4);w32(f,bytes);
 for(auto x:a)f.write((const char*)&x,4);return bool(f);
}
int infer(std::vector<Take>&takes,int rate,const fs::path&out,int eval){
 auto result=panorama::gen37::discover(takes,rate,eval,4);
 if(!result.error.empty()){std::cerr<<result.error<<'\n';return 5;}
 fs::create_directories(out);
 std::ofstream rankings(out/"ranking.csv");
 rankings<<"rank,topology_index,training_loss,holdout_loss,vertices,tetrahedra,length,width,thickness,camber,taper,mesh_young,bridge_coupling\n";
 for(size_t i=0;i<result.proposals.size();i++){
  const auto&c=result.proposals[i];auto stem="proposal_"+std::to_string(i+1);
  std::ofstream(out/(stem+".shape37"))<<panorama::gen37::encodeShape(c.shape);
  std::ofstream(out/(stem+".mesh36"))<<panorama::fem36::serializeMesh(c.model.mesh);
  std::ofstream(out/(stem+".fem36"))<<panorama::fem36::serialize(c.model);
  rankings<<i+1<<','<<c.candidateIndex<<','<<std::setprecision(12)<<c.trainingLoss<<','<<c.validationLoss<<','
    <<c.model.mesh.vertices.size()<<','<<c.model.mesh.tetrahedra.size()<<','<<c.shape.length<<','<<c.shape.width<<','
    <<c.shape.thickness<<','<<c.shape.camber<<','<<c.shape.taper<<','<<c.model.meshYoung<<','<<c.model.bridgeCoupling<<'\n';
  for(size_t j=0;j<takes.size();j++){
   auto b=panorama::gen37::renderTetra(c.model,rate,takes[j].midi,takes[j].velocity,takes[j].audio.size());
   if(!save(out/(stem+"_take_"+std::to_string(j+1)+".wav"),b,rate))return 6;
  }
 }
 std::cout<<"GENERATIVE37_OK generated="<<result.generatedTopologies<<" eval="<<result.evaluations
          <<" best_training="<<result.proposals[0].trainingLoss<<" best_holdout="<<result.proposals[0].validationLoss<<'\n';
 return 0;
}
int manifest(const fs::path&p,const fs::path&out,int eval){
 std::ifstream in(p);if(!in)return 2;
 std::vector<Take>takes;std::string line;int rate=0;
 while(std::getline(in,line)){
  if(line.empty()||line[0]=='#')continue;
  std::istringstream ss(line);std::string kind,file,midi,vel;
  if(!std::getline(ss,kind,'|')||!std::getline(ss,file,'|')||!std::getline(ss,midi,'|')||!std::getline(ss,vel))return 2;
  if((kind!="train"&&kind!="holdout")||file.empty()||fs::path(file).is_absolute()||file.find("..")!=std::string::npos)return 2;
  Take t;int sr=0;t.training=kind=="train";
  try{t.midi=std::stoi(midi);t.velocity=std::stof(vel);}catch(...){return 2;}
  if(!wav(p.parent_path()/file,t.audio,sr))return 3;
  if(rate&&rate!=sr)return 3;rate=sr;takes.push_back(std::move(t));
 }
 if(takes.empty())return 2;
 return infer(takes,rate,out,eval);
}
int demo(const fs::path&out){
 fs::create_directories(out);
 std::ofstream m(out/"recordings.txt");
 panorama::gen37::Shape shape;shape.xCells=2;shape.yCells=1;shape.zCells=1;shape.camber=.14;shape.taper=-.09;
 panorama::fem36::Model truth;truth.pluckPosition=.68;truth.meshYoung=5.4e8;truth.bridgeCoupling=.09;
 if(!panorama::gen37::buildTetraMesh(shape,truth.mesh))return 7;
 std::ofstream(out/"GROUND_TRUTH_NOT_USED.fem36")<<panorama::fem36::serialize(truth);
 for(int i=0;i<3;i++){
  int note=40+i*5;float v=.6f+.1f*i;
  auto a=panorama::gen37::renderTetra(truth,22050,note,v,768);
  auto name="input_"+std::to_string(i+1)+".wav";
  if(!save(out/name,a,22050))return 7;
  m<<(i==2?"holdout":"train")<<'|'<<name<<'|'<<note<<'|'<<v<<'\n';
 }m.close();
 const char*names[]={"string","membrane","plate","tube"};
 for(int k=0;k<4;k++){
  panorama::gen37::WaveRecipe r;r.family=static_cast<panorama::gen37::Family>(k);r.nx=28;r.ny=20;r.fundamentalHz=145;
  std::ofstream(out/(std::string("wave_")+names[k]+".wave37"))<<panorama::gen37::encodeWaveRecipe(r);
  auto audio=panorama::gen37::renderWave(r,48000,48000);
  if(!save(out/(std::string("wave_")+names[k]+".wav"),audio,48000))return 8;
 }
 return manifest(out/"recordings.txt",out/"inferred",70);
}
}
int main(int argc,char**argv){
 if(argc==4&&std::string(argv[1])=="--wave"){
  std::ifstream f(argv[2]);if(!f)return 2;
  std::string contents((std::istreambuf_iterator<char>(f)),{});
  panorama::gen37::WaveRecipe r;
  if(!panorama::gen37::decodeWaveRecipe(contents,r))return 3;
  return save(argv[3],panorama::gen37::renderWave(r,48000,48000),48000)?0:4;
 }
 if(argc==3&&std::string(argv[1])=="--demo")return demo(argv[2]);
 if(argc<3||argc>4){std::cerr<<"usage: panoramize_render37 --demo dir\n       panoramize_render37 --wave instrument.wave37 output.wav\n       panoramize_render37 takes.txt output_dir [evaluations]\n";return 1;}
 int evaluations=70;try{if(argc==4)evaluations=std::stoi(argv[3]);}catch(...){return 1;}
 return manifest(argv[1],argv[2],evaluations);
}
