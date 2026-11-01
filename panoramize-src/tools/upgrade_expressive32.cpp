// Convert a Panoramize 3.0/3.1 physically inferred preset into a 3.2
// MIDI expressive preset WITHOUT replacing or overwriting its original.
#include "UniversalInstrument23.h"
#include "JointPhysicalInverse31.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
int main(int argc,char**argv){
 if(argc!=3){std::cerr<<"Usage: panoramize_upgrade32 input.pgraph|input.joint31 output.pgraph\n";return 2;}
 const std::string input=argv[1],output=argv[2];
 if(input==output){std::cerr<<"Output must differ from original\n";return 2;}
 std::ifstream f(input,std::ios::binary);if(!f){std::cerr<<"Cannot read input\n";return 2;}
 std::ostringstream buffer;buffer<<f.rdbuf();auto s=buffer.str();
 panorama::universal23::Graph g;std::string error;
 if(s.rfind("PANORAMIZE-JOINT31",0)==0){
  panorama::joint31::Candidate candidate;
  if(!panorama::joint31::deserialize(s,candidate)){std::cerr<<"Invalid joint model\n";return 3;}
  g=panorama::universal23::expressiveGraph();
  g.expressive32.physical=panorama::joint31::forTake(candidate,0);
  g.name="Converted Joint31 / Expressive32";
 }else if(s.rfind("PANORAMIZE-PHYSICAL30",0)==0){
  panorama::performance30::CoupledModel m;
  if(!panorama::performance30::deserialize(s,m)){std::cerr<<"Invalid physical30 model\n";return 3;}
  g=panorama::universal23::expressiveGraph();g.expressive32.physical=m;
  g.name="Converted Physical30 / Expressive32";
 }else{
  if(!panorama::universal23::deserialize(s,g,&error)){std::cerr<<error<<'\n';return 3;}
  bool converted=false;
  for(auto&node:g.nodes)if(node.kind==panorama::universal23::Kind::PhysicalCoupled30){
   node.kind=panorama::universal23::Kind::PhysicalExpressive32;converted=true;
  }
  if(!converted){std::cerr<<"Input graph needs PHYSICAL COUPLED 3.0 node\n";return 3;}
  g.expressive32.physical=g.physical30;
  g.name+=" / Expressive32";
 }
 const auto result=panorama::universal23::serialize(g);
 panorama::universal23::Graph verified;
 if(!panorama::universal23::deserialize(result,verified,&error)||
    !panorama::universal23::Instrument::prepare(verified,48000,&error)){
  std::cerr<<"Invalid converted graph: "<<error<<'\n';return 4;
 }
 std::ofstream out(output,std::ios::binary);
 if(!out){std::cerr<<"Cannot write output\n";return 4;}
 out<<result;
 if(!out){std::cerr<<"Incomplete output\n";return 4;}
 std::cout<<"Saved "<<output<<" with expressive MIDI mapping\n";
}
