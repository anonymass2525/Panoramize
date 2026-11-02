#include "ReducedHamiltonian44.h"
#include "reduced44_large.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
using panorama::reduced44::Instrument;
static void w16(std::ofstream&f,unsigned v){f.put(char(v));f.put(char(v>>8));}
static void w32(std::ofstream&f,unsigned v){for(int i=0;i<4;i++)f.put(char(v>>(i*8)));}
static bool save(const char*filename,const std::vector<float>&v,int rate){
 std::ofstream f(filename,std::ios::binary);if(!f)return false;
 unsigned bytes=unsigned(v.size()*4);f.write("RIFF",4);w32(f,36+bytes);f.write("WAVEfmt ",8);w32(f,16);w16(f,3);w16(f,1);w32(f,rate);w32(f,rate*4);w16(f,4);w16(f,32);f.write("data",4);w32(f,bytes);
 for(float x:v)f.write(reinterpret_cast<const char*>(&x),4);return bool(f);
}
int main(int argc,char**argv){
 if(argc==3&&std::string(argv[1])=="--write-large"){
  std::ofstream f(argv[2]);f<<panorama::reduced44::serialize(panorama::reduced44::largeExample());return f?0:3;
 }
 if(argc==3&&std::string(argv[1])=="--write-example"){
  std::ofstream f(argv[2]);f<<panorama::reduced44::serialize(panorama::reduced44::example());return f?0:3;
 }
 if(argc<3||argc>5){std::cerr<<"usage: reduced44 input.reduced44 output.wav [seconds] [rate] | --write-example output.reduced44\n";return 1;}
 try{
  double seconds=argc>=4?std::stod(argv[3]):1.5;int rate=argc==5?std::stoi(argv[4]):48000;
  if(!std::isfinite(seconds)||seconds<=0||seconds>10||rate<22050||rate>96000)return 2;
  std::ifstream file(argv[1]);if(!file)return 2;
  std::string text((std::istreambuf_iterator<char>(file)),{}),why;
  panorama::reduced44::Recipe r;
  if(!panorama::reduced44::deserialize(text,r,&why)){std::cerr<<why;return 3;}
  auto inst=Instrument::prepare(r,rate,&why);
  if(!inst){std::cerr<<why;return 4;}
  std::vector<float>out(size_t(rate*seconds));inst->noteOn(57,.85f);
  for(float&sample:out)sample=inst->processOne();
  if(!save(argv[2],out,rate))return 5;
  auto stat=inst->stats();auto L=inst->ledger();
  std::cout<<"full_dof="<<stat.fullDofs<<" reduced_dof="<<stat.reducedDofs<<" boundary="<<stat.boundaryDofs
   <<" nnz="<<stat.nnz<<" frames="<<stat.frames<<" failure="<<stat.fails<<" total_energy="<<L.finalEnergy
   <<" split_residual="<<L.splitError<<"\n";
  return stat.fails==0?0:6;
 }catch(...){return 2;}
}
