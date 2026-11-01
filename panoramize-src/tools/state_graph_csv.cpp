// PANORAMA 2.1 arbitrary offline linear-acoustics instrument definition.
// CSV authoring is deliberately NOT parsed in the audio callback.
#include "AcousticGraph21.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
using namespace panorama::acoustic21;
static void u16(std::ostream& f,unsigned a){f.put(char(a));f.put(char(a>>8));}
static void u32(std::ostream& f,unsigned a){for(int k=0;k<4;k++)f.put(char(a>>(8*k)));}
static std::vector<std::string> fields(const std::string& line){
 std::stringstream ss(line);std::vector<std::string> out;std::string t;
 while(std::getline(ss,t,','))out.push_back(t);
 return out;
}
static bool run(const std::string& input,const std::string& output,double duration){
 std::ifstream in(input);if(!in){std::cerr<<"cannot open input "<<input<<"\n";return false;}
 GraphBuilder g;
 struct Impulse{int body;Vec3 velocity;};std::vector<Impulse> impacts;
 struct Pickup{int body;Vec3 axis;};std::vector<Pickup> pickups;
 unsigned lineNumber=0;std::string line;
 try{
 while(std::getline(in,line)){
  ++lineNumber;if(line.empty()||line[0]=='#')continue;
  const auto f=fields(line);if(f.empty())continue;
  const auto num=[&](size_t i){if(i>=f.size())throw std::runtime_error("missing field");return std::stod(f[i]);};
  const auto index=[&](size_t i){if(i>=f.size())throw std::runtime_error("missing index");return std::stoi(f[i]);};
  if(f[0]=="body"){
   if(f.size()!=8)throw std::runtime_error("body expects 7 fields");
   Body b;b.name=f[1];b.rest={num(2),num(3),num(4)};b.mass=num(5);
   b.anchor.stiffness=Tensor3::isotropic(num(6));
   b.anchor.damping=Tensor3::isotropic(num(7));
   if(g.addBody(b)<0)throw std::runtime_error("invalid body properties");
  }else if(f[0]=="spring"){
   if(f.size()!=5)throw std::runtime_error("spring expects 4 fields");
   Link l;l.a=index(1);l.b=index(2);
   l.coupling.stiffness=Tensor3::isotropic(num(3));
   l.coupling.damping=Tensor3::isotropic(num(4));
   if(!g.addLink(l))throw std::runtime_error("invalid spring coupling");
  }else if(f[0]=="tetra"){
   if(f.size()!=9)throw std::runtime_error("tetra expects 8 fields");
   Tetrahedron t;t.vertices={index(1),index(2),index(3),index(4)};
   t.youngPa=num(5);t.poisson=num(6);t.densityKgM3=num(7);t.viscousSeconds=num(8);
   if(!g.addTetrahedron(t))throw std::runtime_error("invalid tetra material/indices");
  }else if(f[0]=="excite"){
   if(f.size()!=5)throw std::runtime_error("excite expects 4 fields");
   impacts.push_back({index(1),{num(2),num(3),num(4)}});
  }else if(f[0]=="pickup"){
   if(f.size()!=5)throw std::runtime_error("pickup expects 4 fields");
   pickups.push_back({index(1),{num(2),num(3),num(4)}});
  }else throw std::runtime_error("unknown record "+f[0]);
 }
 }catch(const std::exception& ex){std::cerr<<"line "<<lineNumber<<": "<<ex.what()<<"\n";return false;}
 if(pickups.empty()||impacts.empty()){std::cerr<<"at least one excite and pickup required\n";return false;}
 std::string why;auto model=g.compile(48000.,&why);
 if(!model){std::cerr<<"invalid graph: "<<why<<"\n";return false;}
 auto voice=model->makeVoice();
 for(const auto& h:impacts)if(!voice.excite(h.body,h.velocity))return false;
 const unsigned frames=unsigned(std::lround(duration*48000.));
 std::vector<double> wave;wave.reserve(size_t(frames)*2);
 double peak=0;
 for(unsigned i=0;i<frames;i++){
  if(!voice.processSample())return false;
  const auto& a=pickups[0];const auto& b=pickups[pickups.size()>1?1:0];
  const double l=voice.audioVelocity(a.body,a.axis);
  const double r=voice.audioVelocity(b.body,b.axis);
  peak=std::max({peak,std::abs(l),std::abs(r)});
  wave.push_back(l);wave.push_back(r);
 }
 if(!(peak>1.e-12)){std::cerr<<"silent output: check pickup direction\n";return false;}
 std::ofstream out(output,std::ios::binary);if(!out)return false;
 out.write("RIFF",4);u32(out,36+unsigned(wave.size()*2));out.write("WAVEfmt ",8);
 u32(out,16);u16(out,1);u16(out,2);u32(out,48000);u32(out,48000*4);u16(out,4);u16(out,16);
 out.write("data",4);u32(out,unsigned(wave.size()*2));
 const double gain=.65/peak;
 for(double v:wave)u16(out,uint16_t(int16_t(std::lround(std::clamp(v*gain,-1.,1.)*32767.))));
 const auto ledger=voice.ledger();
 std::cout<<"bodies="<<model->bodyCount()<<" dof="<<model->degreesOfFreedom()
          <<" peak="<<peak<<" residual="<<ledger.residual<<"\n";
 return bool(out);
}
int main(int argc,char** argv){
 if(argc<3||argc>4){std::cerr<<"usage: panorama_state_graph_csv instrument.csv output.wav [duration_seconds]\n";return 2;}
 try{
  const double duration=argc==4?std::stod(argv[3]):3.;
  if(!std::isfinite(duration)||duration<.1||duration>30.)return 2;
  return run(argv[1],argv[2],duration)?0:1;
 }catch(const std::exception& ex){std::cerr<<ex.what()<<"\n";return 2;}
}
