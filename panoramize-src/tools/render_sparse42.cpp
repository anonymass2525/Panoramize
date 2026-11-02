#include "SparseHamiltonian42.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
using namespace panorama::sparse42;
void u16(std::ofstream&f,std::uint32_t v){f.put(char(v));f.put(char(v>>8));}
void u32(std::ofstream&f,std::uint32_t v){for(int k=0;k<4;k++)f.put(char(v>>(k*8)));}
bool wav(const std::string&path,const std::vector<float>&a,int sr){
 std::ofstream f(path,std::ios::binary);if(!f)return false;auto n=std::uint32_t(a.size()*4);
 f.write("RIFF",4);u32(f,36+n);f.write("WAVEfmt ",8);u32(f,16);u16(f,3);u16(f,1);u32(f,std::uint32_t(sr));u32(f,std::uint32_t(sr*4));u16(f,4);u16(f,32);
 f.write("data",4);u32(f,n);for(float s:a)f.write(reinterpret_cast<const char*>(&s),4);return bool(f);
}
int main(int argc,char**argv){
 if(argc==3&&std::string(argv[1])=="--example"){
  std::ofstream f(argv[2]);f<<serialize(example());return f?0:2;
 }
 if(argc<3||argc>5){std::cerr<<"usage: panoramize_sparse42_demo --example file.sparse42 | file.sparse42 output.wav [seconds] [rate]\n";return 2;}
 try{
  double dur=argc>3?std::stod(argv[3]):1.5;int sr=argc>4?std::stoi(argv[4]):48000;
  if(!(dur>.01&&dur<=10&&sr>=22050&&sr<=192000))return 2;
  std::ifstream in(argv[1]);if(!in)return 3;std::string content((std::istreambuf_iterator<char>(in)),{}),error;
  Recipe r;if(!deserialize(content,r,&error)){std::cerr<<error<<'\n';return 4;}
  auto i=Instrument::prepare(r,sr,&error);if(!i){std::cerr<<error<<'\n';return 5;}
  std::vector<float>a(size_t(dur*sr));i->noteOn(57,.85f);
  auto start=std::chrono::steady_clock::now();for(auto&v:a)v=i->processOne();
  auto elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
  if(!wav(argv[2],a,sr))return 6;auto s=i->stats();auto l=i->ledger();
  std::cout<<"dof="<<s.dofs<<" nnz="<<s.nnz<<" iterations_last="<<s.iterations<<" failures="<<s.failed
           <<" render_ms="<<elapsed<<" active_J="<<l.active<<" dissipated_J="<<l.dissipated<<" final_J="<<l.finalEnergy<<" residual_J="<<l.residual<<'\n';
  return s.failed?7:0;
 }catch(...){return 8;}
}
