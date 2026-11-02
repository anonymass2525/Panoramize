#include "UniversalInstrument23.h"
#include "NativeState26.h"
#include "GuitarVoice29.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <functional>
#include <sstream>
#include <utility>
namespace panorama::universal23 {
namespace {
constexpr int kMaxNodes=64;
constexpr float kMaxSample=8.f;
std::string cleanName(std::string text){
 for(char&c:text)if(c=='\n'||c=='\r'||c=='|'||c==',')c=' ';
 if(text.size()>128)text.resize(128);
 return text;
}
Sample clipSample(Sample x) noexcept{
 x.left=std::isfinite(x.left)?std::clamp(x.left,-kMaxSample,kMaxSample):0.f;
 x.right=std::isfinite(x.right)?std::clamp(x.right,-kMaxSample,kMaxSample):0.f;
 return x;
}
}
Graph directGraph(){
 Graph g;g.name="Panoramize / New Horizon";
 g.nodes={{Kind::DirectState27,-1,-1,0,.88f},{Kind::Output,0,-1,0,.88f}};
 return g;
}
Graph expressiveGraph(){
 Graph g;g.name="Panoramize / Expressive Guitar 3.2";
 g.expressive32.physical.bridge=true;g.expressive32.physical.body=true;
 g.nodes={{Kind::PhysicalExpressive32,-1,-1,0,.9f},{Kind::Output,0,-1,0,.85f}};
 return g;
}
Graph articulatedGraph(){
 Graph g;g.name="Panoramize / Physical Articulation 3.3";
 g.articulated33.physical.bridge=true;g.articulated33.physical.body=true;
 g.nodes={{Kind::PhysicalArticulated33,-1,-1,0,.9f},{Kind::Output,0,-1,0,.85f}};
 return g;
}
Graph contactGraph(){
 Graph g;g.name="Panoramize / Physical Contact 3.4";
 g.nodes={{Kind::PhysicalContact34,-1,-1,0,.9f},{Kind::Output,0,-1,0,.85f}};return g;
}
Graph coupledGraph(){
 Graph g;g.name="Panoramize / Six String Coupled 3.5";
 g.nodes={{Kind::PhysicalCoupled35,-1,-1,0,.85f},{Kind::Output,0,-1,0,.85f}};return g;
}
Graph unifiedPhysicalGraph(){
 Graph g;g.name="Panoramize / Unified Acoustic-Physical 3.9";
 g.nodes={{Kind::UnifiedPhysical39,-1,-1,0,.9f},{Kind::Output,0,-1,0,.85f}};
 return g;
}
Graph energyGraph(){
 Graph g;g.name="Panoramize / Energy-Based Unified Ports 4.0";
 g.nodes={{Kind::EnergyGraph40,-1,-1,0,.9f},{Kind::Output,0,-1,0,.85f}};return g;
}
Graph boundaryGraph(){
 Graph g;g.name="Panoramize / SI Boundary FEM 4.1";
 g.nodes={{Kind::BoundaryGraph41,-1,-1,0,.85f},{Kind::Output,0,-1,0,.8f}};
 return g;
}
Graph sparseHamiltonianGraph(){
 Graph g;g.name="Panoramize / Sparse Port-Hamiltonian 4.2";
 g.nodes={{Kind::SparseHamiltonian42,-1,-1,0,.85f},{Kind::Output,0,-1,0,.85f}};return g;
}
Graph sparseEnhancedGraph(){Graph g;g.name="Panoramize / Sparse Physical Spectral 4.3";
 g.nodes={{Kind::SparseHamiltonian43,-1,-1,0,.85f},{Kind::Output,0,-1,0,.85f}};return g;}
Graph reducedMultirateGraph(){Graph g;g.name="Panoramize / Boundary-Preserving Multirate 4.4";
 g.nodes={{Kind::ReducedHamiltonian44,-1,-1,0,.85f},{Kind::Output,0,-1,0,.85f}};return g;}
Graph nonlinearPassiveGraph(){
 Graph g;g.name="Panoramize / Nonlinear Energy Ports 4.5";
 g.nodes={{Kind::NonlinearPassive45,-1,-1,0,.85f},{Kind::Output,0,-1,0,.85f}};return g;
}
Graph genuineNativeGraph(){Graph g;g.name="Panoramize / Genuine Native 4.7";
 g.globalRoutes={{genuine47::Source::Macro1,0,.20f},{genuine47::Source::Wheel,0,.15f}};
 g.nodes={{Kind::GenuineNative47,-1,-1,0,.9f},{Kind::Output,0,-1,0,.85f}};return g;}
Graph defaultGraph(){
 Graph g;g.name="Horizon Matter";
 g.nodes={
   {Kind::Legacy,-1,-1,24,0.6f},          // Midnight Geometry Reforged
   {Kind::Legacy,-1,-1,52,0.5f},          // Emergent Choir
   {Kind::UnifiedState,-1,-1,0,0.50f}, // all six acoustic domains
   {Kind::Mix,0,1,0,0.62f},
   {Kind::Mix,3,2,0,0.8f},
   {Kind::Fold,4,-1,0,0.16f},
   {Kind::Output,5,-1,0,0.85f}
 };
 return g;
}
std::string serialize(const Graph& g){
 std::ostringstream s;
 const bool globalMod47=!g.globalRoutes.empty()||g.globalMacros!=std::array<float,4>{{.5f,.5f,.5f,.5f}};
 const bool genuine47=std::any_of(g.nodes.begin(),g.nodes.end(),[](const Node&n){return n.kind==Kind::GenuineNative47;});
 const bool passive45=std::any_of(g.nodes.begin(),g.nodes.end(),[](const Node&n){return n.kind==Kind::NonlinearPassive45;});
 const bool reduced44=std::any_of(g.nodes.begin(),g.nodes.end(),[](const Node&n){return n.kind==Kind::ReducedHamiltonian44;});
 const bool sparse43=std::any_of(g.nodes.begin(),g.nodes.end(),[](const Node&n){return n.kind==Kind::SparseHamiltonian43;});
 const bool sparse42=std::any_of(g.nodes.begin(),g.nodes.end(),[](const Node&n){return n.kind==Kind::SparseHamiltonian42;});
 const bool boundary41=std::any_of(g.nodes.begin(),g.nodes.end(),[](const Node&n){return n.kind==Kind::BoundaryGraph41;});
 const bool energy40=std::any_of(g.nodes.begin(),g.nodes.end(),[](const Node&n){return n.kind==Kind::EnergyGraph40;});
 const bool unified39=std::any_of(g.nodes.begin(),g.nodes.end(),[](const Node& n){return n.kind==Kind::UnifiedPhysical39;});
 const bool coupled35=std::any_of(g.nodes.begin(),g.nodes.end(),[](const Node& n){return n.kind==Kind::PhysicalCoupled35;});
 const bool contact34=std::any_of(g.nodes.begin(),g.nodes.end(),[](const Node& n){return n.kind==Kind::PhysicalContact34;});
 const bool articulated33=std::any_of(g.nodes.begin(),g.nodes.end(),[](const Node& n){return n.kind==Kind::PhysicalArticulated33;});
 const bool expressive32=std::any_of(g.nodes.begin(),g.nodes.end(),[](const Node& n){return n.kind==Kind::PhysicalExpressive32;});
 const bool physical30=std::any_of(g.nodes.begin(),g.nodes.end(),[](const Node& n){return n.kind==Kind::PhysicalCoupled30;});
 const bool guitar29=std::any_of(g.nodes.begin(),g.nodes.end(),[](const Node& n){return n.kind==Kind::GuitarString29;});
 const bool state27=std::any_of(g.nodes.begin(),g.nodes.end(),[](const Node& n){return n.kind==Kind::DirectState27;});
 bool native51=false;
 for(int k=0;k<8;k++)native51|=(g.nativePickup51[size_t(k)]!=.5f||g.nativeExcite51[size_t(k)]!=.5f);
 const bool native=std::any_of(g.nodes.begin(),g.nodes.end(),[](const Node& n){return n.kind==Kind::NativeState;});
 s<<(native51?"PANORAMIZE-GRAPH-51|1\n":(genuine47||globalMod47)?"PANORAMIZE-GRAPH-47|1\n":passive45?"PANORAMIZE-GRAPH-45|1\n":reduced44?"PANORAMIZE-GRAPH-44|1\n":sparse43?"PANORAMIZE-GRAPH-43|1\n":sparse42?"PANORAMIZE-GRAPH-42|1\n":boundary41?"PANORAMIZE-GRAPH-41|1\n":energy40?"PANORAMIZE-GRAPH-40|1\n":unified39?"PANORAMIZE-GRAPH-39|1\n":coupled35?"PANORAMIZE-GRAPH-35|1\n":contact34?"PANORAMIZE-GRAPH-34|1\n":articulated33?"PANORAMIZE-GRAPH-33|1\n":expressive32?"PANORAMIZE-GRAPH-32|1\n":physical30?"PANORAMIZE-GRAPH-30|1\n":guitar29?"PANORAMIZE-GRAPH-29|1\n":state27?"PANORAMIZE-GRAPH-27|1\n":native?"PANORAMIZE-GRAPH-26|1\n":"PANORAMIZE-GRAPH-25|1\n")<<"NAME|"<<cleanName(g.name)<<"\n";
 if(globalMod47){
  s<<"GLOBAL_LFO|"<<g.globalLfoHz<<"\n";
  for(int k=0;k<4;k++)s<<"GLOBAL_MACRO|"<<k<<'|'<<g.globalMacros[size_t(k)]<<"\n";
  for(const auto&route:g.globalRoutes)s<<"GLOBAL_ROUTE|"<<int(route.source)<<'|'<<route.node<<'|'<<route.depth<<"\n";
 }
 for(int k=0;k<8;k++){
  if(g.nativePickup51[size_t(k)]!=.5f)s<<"NATIVE_LIVE|0|"<<k<<'|'<<g.nativePickup51[size_t(k)]<<"\n";
  if(g.nativeExcite51[size_t(k)]!=.5f)s<<"NATIVE_LIVE|1|"<<k<<'|'<<g.nativeExcite51[size_t(k)]<<"\n";
 }
 for(const auto& n:g.nodes)
  s<<"NODE|"<<int(n.kind)<<'|'<<n.a<<'|'<<n.b<<'|'<<n.preset<<'|'<<n.level<<"\n";
 if(genuine47)s<<"GENUINE47_BEGIN\n"<<genuine47::serialize(g.genuine47)<<"GENUINE47_END\n";
 if(passive45)s<<"PASSIVE45_BEGIN\n"<<passive45::serialize(g.passive45)<<"PASSIVE45_END\n";
 if(reduced44)s<<"REDUCED44_BEGIN\n"<<reduced44::serialize(g.reduced44)<<"REDUCED44_END\n";
 if(sparse43)s<<"SPARSE43_BEGIN\n"<<sparse43::serialize(g.sparse43)<<"SPARSE43_END\n";
 if(sparse42)s<<"SPARSE42_BEGIN\n"<<sparse42::serialize(g.sparse42)<<"SPARSE42_END\n";
 if(boundary41)s<<"BOUNDARY41_BEGIN\n"<<boundary41::serialize(g.boundary41)<<"BOUNDARY41_END\n";
 if(energy40)s<<"ENERGY40_BEGIN\n"<<energy40::serialize(g.energy40)<<"ENERGY40_END\n";
 if(unified39)s<<"UNIFIED39_BEGIN\n"<<unified39::serialize(g.unified39)<<"UNIFIED39_END\n";
 if(coupled35)s<<"COUPLED35_BEGIN\n"<<coupled35::serialize(g.coupled35)<<"COUPLED35_END\n";
 if(contact34)s<<"CONTACT34_BEGIN\n"<<contact34::serialize(g.contact34)<<"CONTACT34_END\n";
 if(articulated33)s<<"ARTICULATED33_BEGIN\n"<<articulated33::serialize(g.articulated33)<<"ARTICULATED33_END\n";
 if(expressive32)s<<"EXPRESSIVE32_BEGIN\n"<<expressive32::serialize(g.expressive32)<<"EXPRESSIVE32_END\n";
 if(physical30)s<<"PHYSICAL30_BEGIN\n"<<performance30::serialize(g.physical30)<<"PHYSICAL30_END\n";
 if(guitar29)s<<"GUITAR29_BEGIN\n"<<guitar29::serialize(g.guitar)<<"GUITAR29_END\n";
 if(state27){
  s<<"STATE27_BEGIN\n"<<native27::serialize(g.instrument)<<"STATE27_END\n";
 }
 return s.str();
}
bool deserialize(const std::string& text,Graph& target,std::string* error){
 auto fail=[&](const char* why){if(error)*error=why;return false;};
 if(text.size()>2000000)return fail("graph file too large");
 std::istringstream input(text);std::string line;
 if(!std::getline(input,line)||(line!="PANORAMA-GRAPH-23|1"&&line!="PANORAMAIZE-GRAPH-24|1"&&line!="PANORAMIZE-GRAPH-25|1"&&line!="PANORAMIZE-GRAPH-26|1"&&line!="PANORAMIZE-GRAPH-27|1"&&line!="PANORAMIZE-GRAPH-29|1"&&line!="PANORAMIZE-GRAPH-30|1"&&line!="PANORAMIZE-GRAPH-32|1"&&line!="PANORAMIZE-GRAPH-33|1"&&line!="PANORAMIZE-GRAPH-34|1"&&line!="PANORAMIZE-GRAPH-35|1"&&line!="PANORAMIZE-GRAPH-51|1"&&line!="PANORAMIZE-GRAPH-47|1"&&line!="PANORAMIZE-GRAPH-45|1"&&line!="PANORAMIZE-GRAPH-44|1"&&line!="PANORAMIZE-GRAPH-43|1"&&line!="PANORAMIZE-GRAPH-42|1"&&line!="PANORAMIZE-GRAPH-41|1"&&line!="PANORAMIZE-GRAPH-40|1"&&line!="PANORAMIZE-GRAPH-39|1"))return fail("unsupported graph version");
 Graph parsed; bool stateBlock=false,guitarBlock=false,physicalBlock=false,expressiveBlock=false,articulatedBlock=false,contactBlock=false,coupledBlock=false,unified39Block=false,energy40Block=false,boundary41Block=false,sparse42Block=false,sparse43Block=false,reduced44Block=false,passive45Block=false,genuine47Block=false;
 if(!std::getline(input,line)||line.rfind("NAME|",0)!=0)return fail("missing graph name");
 parsed.name=cleanName(line.substr(5));
 bool nativeSeen[2][8]{};
 while(std::getline(input,line)){
  if(line.empty())continue;
  if(line=="GENUINE47_BEGIN"){
   if(genuine47Block)return fail("duplicate genuine47 payload");genuine47Block=true;
   std::string blob;bool closed=false;
   while(std::getline(input,line)){
    if(line=="GENUINE47_END"){closed=true;break;}
    blob+=line+"\n";if(blob.size()>72000)return fail("oversized genuine47 payload");
   }
   if(!closed||!genuine47::deserialize(blob,parsed.genuine47,error))return fail("invalid genuine47 payload");continue;
  }
  if(line=="PASSIVE45_BEGIN"){
   if(passive45Block)return fail("duplicate passive45 payload");passive45Block=true;std::string blob;bool closed=false;
   while(std::getline(input,line)){if(line=="PASSIVE45_END"){closed=true;break;}blob+=line+"\n";if(blob.size()>2100000)return fail("oversized passive45 payload");}
   if(!closed||!passive45::deserialize(blob,parsed.passive45,error))return fail("invalid passive45 payload");continue;
  }
  if(line=="REDUCED44_BEGIN"){
   if(reduced44Block)return fail("duplicate reduced44 payload");reduced44Block=true;std::string blob;bool closed=false;
   while(std::getline(input,line)){if(line=="REDUCED44_END"){closed=true;break;}blob+=line+"\n";if(blob.size()>1950000)return fail("oversized reduced44 block");}
   if(!closed||!reduced44::deserialize(blob,parsed.reduced44,error))return fail("invalid reduced44 payload");continue;
  }
  if(line=="SPARSE43_BEGIN"){
   if(sparse43Block)return fail("duplicate sparse43 payload");sparse43Block=true;std::string blob;bool closed=false;
   while(std::getline(input,line)){if(line=="SPARSE43_END"){closed=true;break;}blob+=line+"\n";if(blob.size()>1900000)return fail("oversized sparse43 block");}
   if(!closed||!sparse43::deserialize(blob,parsed.sparse43,error))return fail("invalid sparse43 payload");continue;
  }
  if(line=="SPARSE42_BEGIN"){
   if(sparse42Block)return fail("duplicate sparse42");sparse42Block=true;std::string blob;bool closed=false;
   while(std::getline(input,line)){if(line=="SPARSE42_END"){closed=true;break;}blob+=line+"\n";if(blob.size()>1900000)return fail("oversized sparse42 block");}
   if(!closed||!sparse42::deserialize(blob,parsed.sparse42,error))return fail("invalid sparse42 payload");continue;
  }
  if(line=="BOUNDARY41_BEGIN"){
   if(boundary41Block)return fail("duplicate boundary41 payload");boundary41Block=true;
   std::string payload;bool closed=false;
   while(std::getline(input,line)){if(line=="BOUNDARY41_END"){closed=true;break;}payload+=line+"\n";if(payload.size()>40000)return fail("boundary41 too large");}
   if(!closed||!boundary41::deserialize(payload,parsed.boundary41,error))return fail("invalid boundary41 payload");
   continue;
  }
  if(line=="ENERGY40_BEGIN"){
   if(energy40Block)return fail("duplicate energy40 payload");energy40Block=true;
   std::string payload;bool closed=false;
   while(std::getline(input,line)){if(line=="ENERGY40_END"){closed=true;break;}payload+=line+"\n";if(payload.size()>12000)return fail("energy40 too large");}
   if(!closed||!energy40::deserialize(payload,parsed.energy40,error))return fail("invalid energy40 payload");continue;
  }
  if(line=="UNIFIED39_BEGIN"){
   if(unified39Block)return fail("duplicate unified39 payload");unified39Block=true;
   std::string payload;bool closed=false;
   while(std::getline(input,line)){if(line=="UNIFIED39_END"){closed=true;break;}payload+=line+"\n";
    if(payload.size()>60000)return fail("unified39 payload too large");}
   if(!closed||!unified39::deserialize(payload,parsed.unified39,error))return fail("invalid unified39 payload");continue;
  }
  if(line=="COUPLED35_BEGIN"){
   if(coupledBlock)return fail("duplicate coupled35 payload");coupledBlock=true;std::string payload;bool closed=false;
   while(std::getline(input,line)){if(line=="COUPLED35_END"){closed=true;break;}payload+=line+"\n";if(payload.size()>2048)return fail("coupled35 payload too large");}
   if(!closed||!coupled35::deserialize(payload,parsed.coupled35))return fail("invalid coupled35 payload");continue;
  }
  if(line=="CONTACT34_BEGIN"){
   if(contactBlock)return fail("duplicate contact34 payload");contactBlock=true;std::string payload;bool closed=false;
   while(std::getline(input,line)){if(line=="CONTACT34_END"){closed=true;break;}payload+=line+"\n";if(payload.size()>4096)return fail("contact34 payload too large");}
   if(!closed||!contact34::deserialize(payload,parsed.contact34))return fail("invalid contact34 payload");continue;
  }
  if(line=="ARTICULATED33_BEGIN"){
   if(articulatedBlock)return fail("duplicate articulated33 payload");
   articulatedBlock=true;std::string payload;bool closed=false;
   while(std::getline(input,line)){if(line=="ARTICULATED33_END"){closed=true;break;}payload+=line+"\n";
    if(payload.size()>24000)return fail("articulation payload too large");}
   if(!closed||!articulated33::deserialize(payload,parsed.articulated33))return fail("invalid articulation payload");
   continue;
  }
  if(line=="EXPRESSIVE32_BEGIN"){
   if(expressiveBlock)return fail("duplicate expressive32 payload");
   expressiveBlock=true;std::string payload;bool closed=false;
   while(std::getline(input,line)){if(line=="EXPRESSIVE32_END"){closed=true;break;}payload+=line+"\n";
    if(payload.size()>20000)return fail("expressive payload too large");}
   if(!closed||!expressive32::deserialize(payload,parsed.expressive32))return fail("invalid expressive payload");
   continue;
  }
  if(line=="PHYSICAL30_BEGIN"){
   if(physicalBlock)return fail("duplicate physical30 payload");
   physicalBlock=true;
   std::string payload;bool closed=false;
   while(std::getline(input,line)){if(line=="PHYSICAL30_END"){closed=true;break;}payload+=line+"\n";
    if(payload.size()>12000)return fail("physical30 payload too large");}
   if(!closed||!performance30::deserialize(payload,parsed.physical30))return fail("invalid physical30 payload");
   continue;
  }
  if(line=="GUITAR29_BEGIN"){
   if(guitarBlock)return fail("duplicate guitar payload");guitarBlock=true;
   std::string payload;bool closed=false;
   while(std::getline(input,line)){if(line=="GUITAR29_END"){closed=true;break;}payload+=line+"\n";
    if(payload.size()>10000)return fail("guitar payload too large");}
   if(!closed||!guitar29::deserialize(payload,parsed.guitar))return fail("invalid guitar payload");
   continue;
  }
  if(line=="STATE27_BEGIN"){
   if(stateBlock)return fail("duplicate state payload");
   stateBlock=true;
   std::string payload;bool closed=false;
   while(std::getline(input,line)){if(line=="STATE27_END"){closed=true;break;}payload+=line+"\n";
    if(payload.size()>40000)return fail("state payload too large");}
   if(!closed)return fail("unterminated state payload");
   if(!native27::deserialize(payload,parsed.instrument,error))return false;
   continue;
  }
  if(line.rfind("NATIVE_LIVE|",0)==0){
   int kind=0,index=0;float v=0;char extra;
   if(std::sscanf(line.c_str(),"NATIVE_LIVE|%d|%d|%f%c",&kind,&index,&v,&extra)!=3||
      kind<0||kind>1||index<0||index>=8||!std::isfinite(v)||v<0||v>1||nativeSeen[kind][index])
    return fail("invalid or duplicate Native51 automation default");
   nativeSeen[kind][index]=true;
   if(kind==0)parsed.nativePickup51[size_t(index)]=v;
   else parsed.nativeExcite51[size_t(index)]=v;
   continue;
  }
  if(line.rfind("GLOBAL_LFO|",0)==0){float hz;char tail;
   if(std::sscanf(line.c_str(),"GLOBAL_LFO|%f%c",&hz,&tail)!=1||!std::isfinite(hz)||hz<0||hz>30)return fail("invalid global LFO");
   parsed.globalLfoHz=hz;continue;
  }
  if(line.rfind("GLOBAL_MACRO|",0)==0){int index;float v;char tail;
   if(std::sscanf(line.c_str(),"GLOBAL_MACRO|%d|%f%c",&index,&v,&tail)!=2||index<0||index>3||!std::isfinite(v)||v<0||v>1)return fail("invalid global macro");
   parsed.globalMacros[size_t(index)]=v;continue;
  }
  if(line.rfind("GLOBAL_ROUTE|",0)==0){int src,id;float depth;char tail;
   if(std::sscanf(line.c_str(),"GLOBAL_ROUTE|%d|%d|%f%c",&src,&id,&depth,&tail)!=3||src<0||src>8||id<0||id>=kMaxNodes||!std::isfinite(depth)||std::abs(depth)>2||parsed.globalRoutes.size()>=32)return fail("invalid global route");
   parsed.globalRoutes.push_back({genuine47::Source(src),id,depth});continue;
  }
  if(parsed.nodes.size()>=kMaxNodes)return fail("too many nodes (64 maximum)");
  if(line.rfind("NODE|",0)!=0)return fail("unexpected graph record");
  int kind,a,b,preset;float level;char extra;
  if(std::sscanf(line.c_str(),"NODE|%d|%d|%d|%d|%f%c",&kind,&a,&b,&preset,&level,&extra)!=5)
   return fail("invalid node fields");
  if(kind<0||kind>int(Kind::GenuineNative47)||!std::isfinite(level)||level<0.f||level>1.f)
   return fail("invalid kind or level");
  parsed.nodes.push_back({Kind(kind),a,b,preset,level});
 }
 if(parsed.nodes.empty())return fail("empty graph");
 for(const auto&r:parsed.globalRoutes)if(r.node>=int(parsed.nodes.size()))return fail("global route targets non-existent node");
 if(std::any_of(parsed.nodes.begin(),parsed.nodes.end(),[](const Node&n){return n.kind==Kind::GenuineNative47;})&&!genuine47Block)return fail("genuine47 source needs payload");
 if(std::any_of(parsed.nodes.begin(),parsed.nodes.end(),[](const Node&n){return n.kind==Kind::NonlinearPassive45;})&&!passive45Block)return fail("passive45 graph needs payload");
 if(std::any_of(parsed.nodes.begin(),parsed.nodes.end(),[](const Node&n){return n.kind==Kind::ReducedHamiltonian44;})&&!reduced44Block)return fail("reduced44 graph needs payload");
 if(std::any_of(parsed.nodes.begin(),parsed.nodes.end(),[](const Node&n){return n.kind==Kind::SparseHamiltonian43;})&&!sparse43Block)return fail("sparse43 graph needs payload");
 if(std::any_of(parsed.nodes.begin(),parsed.nodes.end(),[](const Node&n){return n.kind==Kind::SparseHamiltonian42;})&&!sparse42Block)return fail("sparse42 graph must include mesh payload");
 if(std::any_of(parsed.nodes.begin(),parsed.nodes.end(),[](const Node&n){return n.kind==Kind::BoundaryGraph41;})&&!boundary41Block)return fail("boundary41 source needs SI spatial graph");
 if(std::any_of(parsed.nodes.begin(),parsed.nodes.end(),[](const Node&n){return n.kind==Kind::EnergyGraph40;})&&!energy40Block)return fail("energy40 source needs power-port graph");
 if(std::any_of(parsed.nodes.begin(),parsed.nodes.end(),[](const Node& n){return n.kind==Kind::UnifiedPhysical39;})&&!unified39Block)return fail("unified39 source needs typed graph");
 if(std::any_of(parsed.nodes.begin(),parsed.nodes.end(),[](const Node& n){return n.kind==Kind::GuitarString29;})&&!guitarBlock)return fail("guitar source needs physical model");
 if(std::any_of(parsed.nodes.begin(),parsed.nodes.end(),[](const Node& n){return n.kind==Kind::PhysicalCoupled35;})&&!coupledBlock)return fail("coupled35 source needs model payload");
 if(std::any_of(parsed.nodes.begin(),parsed.nodes.end(),[](const Node& n){return n.kind==Kind::PhysicalContact34;})&&!contactBlock)return fail("contact34 source needs model payload");
 if(std::any_of(parsed.nodes.begin(),parsed.nodes.end(),[](const Node& n){return n.kind==Kind::PhysicalArticulated33;})&&!articulatedBlock)return fail("articulated source needs model payload");
 if(std::any_of(parsed.nodes.begin(),parsed.nodes.end(),[](const Node& n){return n.kind==Kind::PhysicalExpressive32;})&&!expressiveBlock)return fail("expressive source needs model payload");
 if(std::any_of(parsed.nodes.begin(),parsed.nodes.end(),[](const Node& n){return n.kind==Kind::PhysicalCoupled30;})&&!physicalBlock)return fail("physical30 source needs model payload");
 if(std::any_of(parsed.nodes.begin(),parsed.nodes.end(),[](const Node& n){return n.kind==Kind::DirectState27;})&&!stateBlock)return fail("editable graph needs state payload");
 target=std::move(parsed);
 if(error)error->clear();
 return true;
}
struct Instrument::Runtime {
 std::unique_ptr<Synth> legacy;
 std::unique_ptr<native26::Instrument> native;
 std::unique_ptr<native27::Instrument> direct;
 std::unique_ptr<guitar29::Instrument> guitar;
 std::unique_ptr<performance30::Instrument> physical30;
 std::unique_ptr<expressive32::Instrument> expressive32;
 std::unique_ptr<contact34::Instrument> contact34;
 std::unique_ptr<coupled35::Instrument> coupled35;
 std::unique_ptr<unified39::Instrument> unified39;
 std::unique_ptr<energy40::Instrument> energy40;
 std::unique_ptr<boundary41::Instrument> boundary41;
 std::unique_ptr<sparse42::Instrument> sparse42;
 std::unique_ptr<sparse43::Instrument> sparse43;
 std::unique_ptr<reduced44::Instrument> reduced44;
 std::unique_ptr<passive45::Instrument> passive45;
 std::unique_ptr<genuine47::Instrument> genuine47;
 std::unique_ptr<articulated33::Instrument> articulated33;
 std::vector<std::unique_ptr<unified22::Voice>> stateVoices; // allocated before playback
 std::array<float,128> env{};
 std::array<bool,128> held{};
 Sample output{},delayed{};
};
std::shared_ptr<Instrument> Instrument::prepare(const Graph& g,double sr,std::string* error){
 auto fail=[&](const char* why)->std::shared_ptr<Instrument>{if(error)*error=why;return {};};
 const int n=int(g.nodes.size());
 if(n<2||n>kMaxNodes||!std::isfinite(sr)||sr<8000||sr>192000)return fail("bad graph count or sample rate");
 int outputs=0, out=-1;
 for(int i=0;i<n;i++){
  const auto&node=g.nodes[size_t(i)];
  const auto inRange=[&](int index){return index>=0&&index<n;};
  if(!std::isfinite(node.level)||node.level<0.f||node.level>1.f)return fail("invalid node level");
  if(int(node.kind)<0||int(node.kind)>int(Kind::GenuineNative47))return fail("unknown node type");
  if(node.kind==Kind::Output){outputs++;out=i;}
  if(node.kind==Kind::Legacy){if(node.preset<0||node.preset>=kPresetCount||node.a!=-1||node.b!=-1)return fail("legacy source requires a valid factory preset, no inputs");}
  else if(node.kind==Kind::UnifiedState){if(node.a!=-1||node.b!=-1)return fail("state source does not accept audio inputs");}
  else if(node.kind==Kind::NativeState){if(node.a!=-1||node.b!=-1||node.preset<0||node.preset>5)return fail("native typed state requires program 0..5 and no inputs");}
  else if(node.kind==Kind::GenuineNative47){if(node.a!=-1||node.b!=-1)return fail("genuine47 source takes no PCM input");}
  else if(node.kind==Kind::NonlinearPassive45){if(node.a!=-1||node.b!=-1)return fail("passive45 source takes no PCM input");}
  else if(node.kind==Kind::ReducedHamiltonian44){if(node.a!=-1||node.b!=-1)return fail("reduced44 source takes no PCM input");}
  else if(node.kind==Kind::SparseHamiltonian43){if(node.a!=-1||node.b!=-1)return fail("sparse43 source takes no PCM input");}
  else if(node.kind==Kind::SparseHamiltonian42){if(node.a!=-1||node.b!=-1)return fail("sparse42 source takes no PCM inputs");}
  else if(node.kind==Kind::BoundaryGraph41){if(node.a!=-1||node.b!=-1)return fail("boundary41 source takes no PCM inputs");}
  else if(node.kind==Kind::EnergyGraph40){if(node.a!=-1||node.b!=-1)return fail("energy40 source takes no PCM inputs");}
  else if(node.kind==Kind::UnifiedPhysical39){if(node.a!=-1||node.b!=-1)return fail("unified39 source requires no PCM inputs");}
  else if(node.kind==Kind::PhysicalCoupled35){if(node.a!=-1||node.b!=-1)return fail("coupled35 source must have no PCM inputs");}
  else if(node.kind==Kind::PhysicalContact34){if(node.a!=-1||node.b!=-1)return fail("contact34 source must have no PCM inputs");}
  else if(node.kind==Kind::PhysicalArticulated33){if(node.a!=-1||node.b!=-1)return fail("articulated33 source must have no PCM inputs");}
  else if(node.kind==Kind::PhysicalExpressive32){if(node.a!=-1||node.b!=-1)return fail("expressive source must have no PCM inputs");}
  else if(node.kind==Kind::PhysicalCoupled30){if(node.a!=-1||node.b!=-1)return fail("physical30 source must have no PCM inputs");}
  else if(node.kind==Kind::GuitarString29){if(node.a!=-1||node.b!=-1)return fail("physical guitar source must have no PCM inputs");}
  else if(node.kind==Kind::DirectState27){if(node.a!=-1||node.b!=-1)return fail("editable state engine does not take PCM audio inputs");}
  else if(node.kind==Kind::StateDrive){
   if(!inRange(node.a)||!inRange(node.b)||node.a==node.b||
      g.nodes[size_t(node.a)].kind!=Kind::Legacy||
      g.nodes[size_t(node.b)].kind!=Kind::UnifiedState ||
      node.preset<0||node.preset>3)return fail("StateDrive needs Legacy source A, UnifiedState target B and typed port 0..3");
  }
  else if(node.kind==Kind::StateReturn){
   if(!inRange(node.a)||!inRange(node.b)||node.a==node.b||
      g.nodes[size_t(node.a)].kind!=Kind::UnifiedState||
      g.nodes[size_t(node.b)].kind!=Kind::Legacy||
      node.preset<0||node.preset>3)return fail("StateReturn needs UnifiedState source A, Legacy target B and typed port 0..3");
  }
  else if(node.kind==Kind::Mix){if(!inRange(node.a)||!inRange(node.b))return fail("mix requires A and B");}
  else if(!inRange(node.a))return fail("node requires input A");
  if(node.kind!=Kind::Mix&&node.kind!=Kind::StateDrive&&node.kind!=Kind::StateReturn&&node.b!=-1)return fail("only mix has a B audio input");
 }
 if(outputs!=1)return fail("graph must have exactly one output");
 // A Delay node publishes its previous-sample output; incoming edge is
 // consumed after evaluation, breaking only causal cycles.
 std::vector<int> colors(size_t(n),0),order;
 std::function<bool(int)> visit=[&](int i){
  if(colors[size_t(i)]==2)return true;
  if(colors[size_t(i)]==1)return false;
  colors[size_t(i)]=1;
  const auto& node=g.nodes[size_t(i)];
  if(node.kind==Kind::UnifiedState){
   // Active state force must be delivered before the receiving solver is stepped.
   for(int j=0;j<n;j++)if(g.nodes[size_t(j)].kind==Kind::StateDrive && g.nodes[size_t(j)].b==i)
    if(!visit(j))return false;
  }else if(node.kind!=Kind::Delay){
   // StateReturn's feedback into the Legacy input is explicitly z^-1.
   // It is evaluated after UnifiedState but does not impose an algebraic
   // same-sample dependency on its Legacy destination.

   if(node.a>=0&&!visit(node.a))return false;
   if(node.kind==Kind::Mix && node.b>=0&&!visit(node.b))return false;
  }
  colors[size_t(i)]=2;order.push_back(i);return true;
 };
 for(int i=0;i<n;i++)if(!visit(i))return fail("zero-delay cycle; insert Delay node");
 // Prebuild all sources, Cholesky matrices, voices, and all allocations OFF audio thread.
 auto instrument=std::shared_ptr<Instrument>(new Instrument());
 for(auto&r:g.globalRoutes)if(r.node<0||r.node>=n||int(r.source)<0||int(r.source)>8||!std::isfinite(r.depth)||std::abs(r.depth)>2)return fail("invalid global modulation route");
 for(float m:g.globalMacros)if(!std::isfinite(m)||m<0||m>1)return fail("invalid macro value");
 if(!std::isfinite(g.globalLfoHz)||g.globalLfoHz<0||g.globalLfoHz>30)return fail("invalid global LFO rate");
 instrument->graph_=g;instrument->globalMacros_=g.globalMacros;instrument->sampleRate_=sr;instrument->order_=std::move(order);instrument->output_=out;
 instrument->clearGlobalHostAutomation();
 instrument->runtime_.resize(size_t(n));
 for(int i=0;i<n;i++){
  auto& rt=instrument->runtime_[size_t(i)];rt=std::make_unique<Runtime>();
  const auto& node=g.nodes[size_t(i)];
  if(node.kind==Kind::Legacy){
   rt->legacy=std::make_unique<Synth>();rt->legacy->prepare(sr);
   rt->legacy->setPatch(factoryPresets()[size_t(node.preset)]);
  }
  if(node.kind==Kind::NativeState){
   std::string why;rt->native=native26::Instrument::prepare(native26::example(node.preset),sr,&why);
   if(!rt->native)return fail("native state program preparation failed");
  }
  if(node.kind==Kind::DirectState27){
   std::string why;rt->direct=native27::Instrument::prepare(g.instrument,sr,&why);
   if(!rt->direct)return fail("editable state preparation failed");
  }
  if(node.kind==Kind::GenuineNative47){std::string why;rt->genuine47=genuine47::Instrument::prepare(g.genuine47,int(sr),&why);if(!rt->genuine47)return fail("genuine47 preparation failed");}
  if(node.kind==Kind::NonlinearPassive45){std::string why;rt->passive45=passive45::Instrument::prepare(g.passive45,sr,&why);if(!rt->passive45)return fail("passive45 preparation failed");}
  if(node.kind==Kind::ReducedHamiltonian44){std::string why;rt->reduced44=reduced44::Instrument::prepare(g.reduced44,sr,&why);if(!rt->reduced44)return fail("reduced44 preparation failed");}
  if(node.kind==Kind::SparseHamiltonian43){std::string why;rt->sparse43=sparse43::Instrument::prepare(g.sparse43,sr,&why);if(!rt->sparse43)return fail("sparse43 preparation failed");}
  if(node.kind==Kind::SparseHamiltonian42){
   std::string why;rt->sparse42=sparse42::Instrument::prepare(g.sparse42,sr,&why);
   if(!rt->sparse42)return fail("sparse42 preparation failed");
  }
  if(node.kind==Kind::BoundaryGraph41){std::string why;rt->boundary41=boundary41::Instrument::prepare(g.boundary41,sr,&why);if(!rt->boundary41)return fail("boundary41 preparation failed");}
  if(node.kind==Kind::EnergyGraph40){
   std::string why;rt->energy40=energy40::Instrument::prepare(g.energy40,sr,&why);
   if(!rt->energy40)return fail("energy40 preparation failed");
  }
  if(node.kind==Kind::UnifiedPhysical39){
   std::string why;rt->unified39=unified39::Instrument::prepare(g.unified39,int(sr),&why);
   if(!rt->unified39)return fail("unified39 graph preparation failed");
  }
  if(node.kind==Kind::PhysicalCoupled35){
   std::string why;rt->coupled35=coupled35::Instrument::prepare(g.coupled35,sr,&why);
   if(!rt->coupled35)return fail("coupled35 source invalid");
  }
  if(node.kind==Kind::PhysicalContact34){
   std::string why;rt->contact34=contact34::Instrument::prepare(g.contact34,sr,&why);
   if(!rt->contact34)return fail("contact34 source invalid");
  }
  if(node.kind==Kind::PhysicalArticulated33){
   std::string why;rt->articulated33=articulated33::Instrument::prepare(g.articulated33,sr,&why);
   if(!rt->articulated33)return fail("articulated33 source invalid");
  }
  if(node.kind==Kind::PhysicalExpressive32){
   std::string why;rt->expressive32=expressive32::Instrument::prepare(g.expressive32,sr,&why);
   if(!rt->expressive32)return fail("expressive32 source invalid");
  }
  if(node.kind==Kind::PhysicalCoupled30){
   std::string why;rt->physical30=performance30::Instrument::prepare(g.physical30,sr,&why);
   if(!rt->physical30)return fail("physical30 source invalid");
  }
  if(node.kind==Kind::GuitarString29){
   std::string why;rt->guitar=guitar29::Instrument::prepare(g.guitar,sr,&why);
   if(!rt->guitar)return fail("physical guitar model invalid");
  }
  if(node.kind==Kind::UnifiedState){
   rt->stateVoices.resize(128);
   for(int midi=0;midi<128;midi++){
    // A six-domain state solver at the MIDI frequency, built per note only at setup.
    const double f=440.*std::pow(2.,(midi-69)/12.);
    unified22::Builder b;
    constexpr unified22::Domain domains[]={unified22::Domain::Physical,unified22::Domain::Spectral,
      unified22::Domain::Phase,unified22::Domain::Resonance,unified22::Domain::Nonlinear,unified22::Domain::VocalFormant};
    for(int d=0;d<6;d++){
     unified22::Mode m;m.name="domain_"+std::to_string(d);m.domain=domains[d];
     m.frequencyHz=std::clamp(f*(d==5?1.5:1.+.08*d),.1,18500.);
     m.dampingPerSecond=d==0?3.:double(7+d*2);
     m.quartic=d==4?20000.:0.;b.addMode(m);
    }
    for(int d=0;d<5;d++){
     if(!b.connectTyped({d,unified22::Quantity::NormalizedCoordinate,unified22::Unit::RootJouleSeconds},
        {d+1,unified22::Quantity::NormalizedCoordinate,unified22::Unit::RootJouleSeconds},
        unified22::LinkKind::Conservative,4000.))return fail("invalid cross-domain link");
    }
    std::string why;
    auto prepared=b.compile(sr,&why);
    if(!prepared)return fail("state source failed to compile");
    rt->stateVoices[size_t(midi)]=std::make_unique<unified22::Voice>(std::move(prepared));
   }
  }
 }
 if(error)error->clear();
 instrument->clearNativeModeAutomation();
 return instrument;
}
bool Instrument::tryUpdateEnergyGraph(const Graph& next) noexcept{
 // Called on the message thread while audio may run. Only prepared atomic targets
 // may be changed; no topology/mass/unit/drive/audio gain changes. A later GUI edit
 // requiring a new graph still uses the prepared atomic shared_ptr replacement.
 if(next.nodes.size()!=graph_.nodes.size()||
    next.energy40.nodes.size()!=graph_.energy40.nodes.size()||
    next.energy40.links.size()!=graph_.energy40.links.size())return false;
 if(next.energy40.exciteNode!=graph_.energy40.exciteNode||
    next.energy40.masterGain!=graph_.energy40.masterGain||
    next.energy40.smoothingMs!=graph_.energy40.smoothingMs)return false;
 for(size_t i=0;i<next.nodes.size();i++){
  const auto&a=next.nodes[i],&b=graph_.nodes[i];
  if(a.kind!=b.kind||a.a!=b.a||a.b!=b.b||a.preset!=b.preset||a.level!=b.level)return false;
 }
 for(size_t i=0;i<next.energy40.nodes.size();i++){
  const auto&a=next.energy40.nodes[i],&b=graph_.energy40.nodes[i];
  if(a.domain!=b.domain||a.mass!=b.mass||a.area!=b.area||a.output!=b.output||a.drive!=b.drive)return false;
 }
 for(size_t i=0;i<next.energy40.links.size();i++){
  const auto&a=next.energy40.links[i],&b=graph_.energy40.links[i];
  if(a.from!=b.from||a.to!=b.to||a.kind!=b.kind)return false;
 }
 if(!energy40::valid(next.energy40))return false;
 bool hasEnergy=false;
 for(auto&rt:runtime_)if(rt->energy40){
  hasEnergy=true;
  for(size_t i=0;i<next.energy40.nodes.size();i++)
   if(!rt->energy40->setNodeTarget(int(i),next.energy40.nodes[i].frequency,next.energy40.nodes[i].damping))return false;
  for(size_t i=0;i<next.energy40.links.size();i++)
   if(!rt->energy40->setLinkTarget(int(i),next.energy40.links[i].stiffness,next.energy40.links[i].damping))return false;
 }
 return hasEnergy;
}
void Instrument::noteOn(int note,float velocity,int channel) noexcept{
 if(note<0||note>127||velocity<=0||!std::isfinite(velocity))return;
 globalVelocity_=std::clamp(velocity,0.f,1.f);globalKey_=float(note)/127.f;
 for(auto& rt:runtime_){
  if(rt->legacy)rt->legacy->noteOn(note,velocity,channel);
  if(rt->native)rt->native->noteOn(note,velocity);
  if(rt->direct)rt->direct->noteOn(note,velocity);
  if(rt->guitar)rt->guitar->noteOn(note,velocity);
  if(rt->physical30)rt->physical30->noteOn(note,velocity);
  if(rt->expressive32)rt->expressive32->noteOn(note,velocity);
  if(rt->contact34)rt->contact34->noteOn(note,velocity);
  if(rt->coupled35)rt->coupled35->noteOn(note,velocity);
  if(rt->unified39)rt->unified39->noteOn(note,velocity);
  if(rt->energy40)rt->energy40->noteOn(note,velocity);
  if(rt->boundary41)rt->boundary41->noteOn(note,velocity);
  if(rt->sparse42)rt->sparse42->noteOn(note,velocity);
  if(rt->sparse43)rt->sparse43->noteOn(note,velocity);
  if(rt->reduced44)rt->reduced44->noteOn(note,velocity);
  if(rt->passive45)rt->passive45->noteOn(note,velocity);
  if(rt->genuine47)rt->genuine47->noteOn(note,velocity);
  if(rt->articulated33)rt->articulated33->noteOn(note,velocity);
  if(!rt->stateVoices.empty()){
   auto& v=*rt->stateVoices[size_t(note)];v.reset();v.excite(0,velocity*.7);v.excite(1,velocity*.4);
   rt->env[size_t(note)]=1.f;rt->held[size_t(note)]=true;
  }
 }
}
void Instrument::noteOff(int note,int channel) noexcept{
 if(note<0||note>127)return;
 for(auto& rt:runtime_){if(rt->legacy)rt->legacy->noteOff(note,channel);if(rt->native)rt->native->noteOff(note);if(rt->direct)rt->direct->noteOff(note);if(rt->guitar)rt->guitar->noteOff(note);if(rt->physical30)rt->physical30->noteOff(note);if(rt->expressive32)rt->expressive32->noteOff(note);if(rt->articulated33)rt->articulated33->noteOff(note);if(rt->contact34)rt->contact34->noteOff(note);if(rt->coupled35)rt->coupled35->noteOff(note);if(rt->unified39)rt->unified39->noteOff(note);if(rt->energy40)rt->energy40->noteOff(note);if(rt->boundary41)rt->boundary41->noteOff(note);if(rt->sparse42)rt->sparse42->noteOff(note);if(rt->sparse43)rt->sparse43->noteOff(note);if(rt->reduced44)rt->reduced44->noteOff(note);if(rt->passive45)rt->passive45->noteOff(note);if(rt->genuine47)rt->genuine47->noteOff(note);
  if(!rt->stateVoices.empty()&&!sustainDown_)rt->held[size_t(note)]=false;}
}
void Instrument::allNotesOff() noexcept{
 for(auto& rt:runtime_){if(rt->legacy)rt->legacy->allNotesOff();if(rt->native)rt->native->allNotesOff();if(rt->direct)rt->direct->allNotesOff();if(rt->guitar)rt->guitar->allNotesOff();if(rt->physical30)rt->physical30->allNotesOff();if(rt->expressive32)rt->expressive32->allNotesOff();if(rt->articulated33)rt->articulated33->allNotesOff();if(rt->contact34)rt->contact34->allNotesOff();if(rt->coupled35)rt->coupled35->allNotesOff();if(rt->unified39)rt->unified39->allNotesOff();if(rt->energy40)rt->energy40->allNotesOff();if(rt->boundary41)rt->boundary41->allNotesOff();if(rt->sparse42)rt->sparse42->allNotesOff();if(rt->sparse43)rt->sparse43->allNotesOff();if(rt->reduced44)rt->reduced44->allNotesOff();if(rt->passive45)rt->passive45->allNotesOff();if(rt->genuine47)rt->genuine47->allNotesOff();
  if(!rt->stateVoices.empty())for(int i=0;i<128;i++){rt->env[size_t(i)]=0;rt->held[size_t(i)]=false;rt->stateVoices[size_t(i)]->reset();}}
}
void Instrument::pitchBend(float bend) noexcept{for(auto& rt:runtime_){if(rt->legacy)rt->legacy->pitchBend(bend);if(rt->direct)rt->direct->pitchBend(bend);if(rt->guitar)rt->guitar->pitchBend(bend);if(rt->physical30)rt->physical30->pitchBend(bend);if(rt->expressive32)rt->expressive32->pitchBend(bend);if(rt->articulated33)rt->articulated33->pitchBend(bend);if(rt->contact34)rt->contact34->pitchBend(bend);if(rt->coupled35)rt->coupled35->pitchBend(bend);if(rt->unified39)rt->unified39->pitchBend(bend);if(rt->genuine47)rt->genuine47->pitchBend(bend);if(rt->energy40)rt->energy40->pitchBend(bend);if(rt->boundary41)rt->boundary41->pitchBend(bend);}}
void Instrument::channelPressure(float value) noexcept{if(std::isfinite(value))globalPressure_=std::clamp(value,0.f,1.f);for(auto&rt:runtime_)if(rt->genuine47)rt->genuine47->channelPressure(value);for(auto&rt:runtime_)if(rt->expressive32)rt->expressive32->channelPressure(value);else if(rt->articulated33)rt->articulated33->channelPressure(value);else if(rt->contact34)rt->contact34->channelPressure(value);else if(rt->coupled35)rt->coupled35->channelPressure(value);}
void Instrument::polyPressure(int note,float value) noexcept{for(auto&rt:runtime_)if(rt->expressive32)rt->expressive32->polyPressure(note,value);else if(rt->articulated33)rt->articulated33->polyPressure(note,value);else if(rt->contact34)rt->contact34->polyPressure(note,value);else if(rt->coupled35)rt->coupled35->channelPressure(value);}
bool Instrument::setHostParameterNormalized(int index,double value) noexcept{
 if(!std::isfinite(value)||value<0||value>1)return false;
 if(index>=9&&index<=12){hostMacros_[size_t(index-9)].store(float(value),std::memory_order_relaxed);return true;}
 if(index>=16&&index<80){
  if(index-16>=int(graph_.nodes.size()))return false;
  hostNodeLevels_[size_t(index-16)].store(float(value),std::memory_order_relaxed);
  return true;
 }
 if(index>=80&&index<96){
  const int mode=(index<88)?index-80:index-88;
  const bool excitation=index>=88;
  bool updated=false;
  for(auto&rt:runtime_){
   if(rt->direct)updated=rt->direct->setLiveModeNormalized(mode,excitation,value)||updated;
   if(rt->unified39)updated=rt->unified39->setNativeModeNormalized(mode,excitation,value)||updated;
   if(rt->genuine47)updated=rt->genuine47->setNativeModeNormalized(mode,excitation,value)||updated;
  }
  return updated;
 }
 bool updated=false;
 if(index>=0&&index<9)for(auto&rt:runtime_)if(rt->genuine47)
  updated=rt->genuine47->setHostParameterNormalized(index,value)||updated;
 return updated;
}
void Instrument::clearNativeHostAutomation() noexcept{
 for(auto&rt:runtime_)if(rt->genuine47)rt->genuine47->clearHostAutomation();
 clearNativeModeAutomation();
}
void Instrument::clearNativeModeAutomation() noexcept{
 for(int mode=0;mode<8;mode++){
  const double pickup=graph_.nativePickup51[size_t(mode)];
  const double excite=graph_.nativeExcite51[size_t(mode)];
  for(auto&rt:runtime_){
   if(rt->genuine47){rt->genuine47->setNativeModeNormalized(mode,false,pickup);rt->genuine47->setNativeModeNormalized(mode,true,excite);}
   if(rt->direct){rt->direct->setLiveModeNormalized(mode,false,pickup);rt->direct->setLiveModeNormalized(mode,true,excite);}
   if(rt->unified39){rt->unified39->setNativeModeNormalized(mode,false,pickup);rt->unified39->setNativeModeNormalized(mode,true,excite);}
  }
 }
}
void Instrument::clearGlobalHostAutomation() noexcept{
 for(auto&v:hostMacros_)v.store(-1.f,std::memory_order_relaxed);
 for(auto&v:hostNodeLevels_)v.store(-1.f,std::memory_order_relaxed);
}
void Instrument::clearHostAutomation() noexcept{
 clearNativeHostAutomation();clearGlobalHostAutomation();
}
void Instrument::modWheel(float value) noexcept{if(std::isfinite(value))globalWheel_=std::clamp(value,0.f,1.f);for(auto& rt:runtime_){if(rt->legacy)rt->legacy->modWheel(value);if(rt->direct)rt->direct->modWheel(value);if(rt->unified39)rt->unified39->modWheel(value);if(rt->genuine47)rt->genuine47->modWheel(value);}}
void Instrument::articulationControl(int cc,float value) noexcept{
 if(!std::isfinite(value))return;
 value=std::clamp(value,0.f,1.f);
 if(cc>=24&&cc<=27)globalMacros_[size_t(cc-24)]=value;
 for(auto&rt:runtime_)if(rt->genuine47&&cc>=24&&cc<=27)rt->genuine47->setMacro(cc-24,value);
 for(auto&rt:runtime_)if(rt->coupled35){
  if(cc==20)rt->coupled35->nextGesture(static_cast<coupled35::Instrument::Gesture>(std::clamp(int(value*3.999f),0,3)));
  else if(cc==22)rt->coupled35->palmMute(value);
 }
 for(auto&rt:runtime_)if(rt->contact34){
   if(cc==20)rt->contact34->nextGesture(static_cast<contact34::Instrument::Gesture>(std::clamp(int(value*3.999f),0,3)));
   else if(cc==21)rt->contact34->gestureIntensity(value);
   else if(cc==22)rt->contact34->palmMute(value);
   else if(cc==23)rt->contact34->palmPosition(.75f+.245f*value);
 }
 for(auto&rt:runtime_)if(rt->articulated33){
  if(cc==20){const int kind=std::clamp(int(value*3.999f),0,3);
    rt->articulated33->setNextTransition(static_cast<articulated33::Instrument::Transition>(kind));}
  else if(cc==21){ // intensity of next articulated note-on
    rt->articulated33->setTransitionIntensity(value);
  }else if(cc==22)rt->articulated33->palmMute(value);
  else if(cc==23)rt->articulated33->setPalmPosition(.7f+.29f*value);
 }
}
void Instrument::sustain(bool down) noexcept{
 sustainDown_=down;
 for(auto&rt:runtime_){if(rt->legacy)rt->legacy->sustain(down);if(rt->direct)rt->direct->sustain(down);if(rt->guitar)rt->guitar->sustain(down);if(rt->physical30)rt->physical30->sustain(down);if(rt->expressive32)rt->expressive32->sustain(down);if(rt->articulated33)rt->articulated33->sustain(down);if(rt->contact34)rt->contact34->sustain(down);if(rt->coupled35)rt->coupled35->sustain(down);}
 // For the state core, sustain release is a simplified global pedal off.
 if(!down)for(auto&rt:runtime_)if(!rt->stateVoices.empty())rt->held.fill(false);
}
Sample Instrument::processOne() noexcept{
 // PCM-level global matrix: common to every engine, does NOT pretend to be
 // a passive change of an underlying spatial/material state.
 globalPhase_+=6.283185307179586*graph_.globalLfoHz/sampleRate_;
 if(globalPhase_>=6.283185307179586)globalPhase_-=6.283185307179586;
 const float m0=hostMacros_[0].load(std::memory_order_relaxed),m1=hostMacros_[1].load(std::memory_order_relaxed);
 const float m2=hostMacros_[2].load(std::memory_order_relaxed),m3=hostMacros_[3].load(std::memory_order_relaxed);
 const float sources[]={ (m0<0?globalMacros_[0]:m0)-.5f,(m1<0?globalMacros_[1]:m1)-.5f,
    (m2<0?globalMacros_[2]:m2)-.5f,(m3<0?globalMacros_[3]:m3)-.5f,
    globalWheel_-.5f,globalPressure_-.5f,globalVelocity_-.5f,globalKey_-.5f,float(std::sin(globalPhase_))};
 for(int id:order_){
  const auto& node=graph_.nodes[size_t(id)];
  float factor=1.;
  for(const auto&route:graph_.globalRoutes)if(route.node==id)factor+=route.depth*sources[int(route.source)];
  const float host=hostNodeLevels_[size_t(id)].load(std::memory_order_relaxed);
  const float renderLevel=std::clamp((host<0?node.level:host)*factor,0.f,2.f);
  auto& r=*runtime_[size_t(id)];
  const auto input=[&](int index){return index<0?Sample{}:runtime_[size_t(index)]->output;};
  Sample a=input(node.a),b=input(node.b),y{};
  switch(node.kind){
   case Kind::Legacy:{y=r.legacy->processOne();y.left*=renderLevel;y.right*=renderLevel;break;}
   case Kind::NativeState:{const float v=r.native->processMono()*renderLevel;y={v,v};break;}
   case Kind::GenuineNative47:{const float v=r.genuine47->processOne()*renderLevel;y={v,v};break;}
   case Kind::NonlinearPassive45:{const float v=r.passive45->processOne()*renderLevel;y={v,v};break;}
   case Kind::ReducedHamiltonian44:{const float v=r.reduced44->processOne()*renderLevel;y={v,v};break;}
   case Kind::SparseHamiltonian43:{const float v=r.sparse43->processOne()*renderLevel;y={v,v};break;}
   case Kind::SparseHamiltonian42:{const float v=r.sparse42->processOne()*renderLevel;y={v,v};break;}
   case Kind::BoundaryGraph41:{const float v=r.boundary41->processOne()*renderLevel;y={v,v};break;}
   case Kind::EnergyGraph40:{const float v=r.energy40->processOne()*renderLevel;y={v,v};break;}
   case Kind::UnifiedPhysical39:{const float v=r.unified39->processOne()*renderLevel;y={v,v};break;}
   case Kind::PhysicalCoupled35:{const float v=r.coupled35->processOne()*renderLevel;y={v,v};break;}
   case Kind::PhysicalContact34:{const float v=r.contact34->processOne()*renderLevel;y={v,v};break;}
   case Kind::PhysicalArticulated33:{const float v=r.articulated33->processOne()*renderLevel;y={v,v};break;}
   case Kind::PhysicalExpressive32:{const float v=r.expressive32->processOne()*renderLevel;y={v,v};break;}
   case Kind::PhysicalCoupled30:{const float v=r.physical30->processOne()*renderLevel;y={v,v};break;}
   case Kind::GuitarString29:{const float v=r.guitar->processOne()*renderLevel;y={v,v};break;}
   case Kind::DirectState27:{const float v=r.direct->processMono()*renderLevel;y={v,v};break;}
   case Kind::UnifiedState:{
    double val=0.;
    for(int note=0;note<128;note++){
     float& env=r.env[size_t(note)];
     if(env<1.e-5f)continue;
     auto& voice=*r.stateVoices[size_t(note)];
     if(!voice.processSample()){env=0;continue;}
     val+=(voice.audioSample(0,.25)+voice.audioSample(1,.25)+voice.audioSample(5,.12))*env;
     if(!r.held[size_t(note)])env*=.99945f;
    }
    const float o=static_cast<float>(std::tanh(val*.065)*renderLevel);y={o,o};break;
   }
   case Kind::StateDrive:{
    // Only valid typed state is transported. The Legacy audio level is not a
    // substitute for state: a muted legacy sample can still drive the solver.
    auto& target=*runtime_[size_t(node.b)];
    for(int note=0;note<128;note++){
     if(target.env[size_t(note)]<1.e-5f)continue;
     const auto f=runtime_[size_t(node.a)]->legacy->acousticStateForNote(note);
     double drive=0.;
     switch(node.preset){
      case 0:
       if(f.domains&AcousticStateFrame::Phase)
        drive=std::sin(6.283185307179586*f.phaseCycle[0]);
       break;
      case 1:if(f.domains&AcousticStateFrame::Spectral){
       for(double e:f.spectralEnergy)drive+=std::max(0.,e);
       drive=std::sqrt(std::min(1.,drive/12.));}break;
      case 2:if(f.domains&AcousticStateFrame::Mechanical)drive=f.displacement;break;
      case 3:if(f.domains&AcousticStateFrame::Mechanical)drive=f.velocity;break;
     }
     // Dimensionless source state converted to normalized effort by a
     // defined ACTIVE calibration (NORM-EFFORT / source unit). Work is
     // accounted by UnifiedGraph22::Voice::addActiveForce/processSample.
     const double force=std::clamp(drive,-1.,1.)*(double(renderLevel)*2000.);
     target.stateVoices[size_t(note)]->addActiveForce(0,force);
    }
    y={};break;
   }
   case Kind::StateReturn:{
    // Return a true prepared-solver state, not its PCM sample.
    // Delivery happens after the solver processes this sample and changes
    // oscillator/resonator state used by the legacy source NEXT sample.
    // The readouts are energy-normalized, *not* directly SI units.
    auto& source=*runtime_[size_t(node.a)];
    auto& target=*runtime_[size_t(node.b)];
    for(int note=0;note<128;note++){
     if(source.env[size_t(note)]<1.e-5f)continue;
     auto& voice=*source.stateVoices[size_t(note)];
     const int mode=node.preset==0?2:node.preset==1?1:0;
     const auto quantity=node.preset==2?unified22::Quantity::NormalizedCoordinate:unified22::Quantity::NormalizedFlow;
     const auto unit=node.preset==2?unified22::Unit::RootJouleSeconds:unified22::Unit::RootJoule;
     const double response=std::tanh(voice.read({mode,quantity,unit})*8.0)*renderLevel;
     if(target.legacy->injectStateResponseForNote(note,node.preset,response))
      returnResponseMagnitude_+=std::abs(response);
    }
    y={};break;
   }
   case Kind::Mix: {y.left=(1-renderLevel)*a.left+renderLevel*b.left;y.right=(1-renderLevel)*a.right+renderLevel*b.right;break;}
   case Kind::Gain: {y.left=renderLevel*a.left;y.right=renderLevel*a.right;break;}
   case Kind::Fold:{
    const auto fold=[&](float x){float k=1+renderLevel*6;return std::sin(std::clamp(x*k,-25.f,25.f));};
    y={fold(a.left),fold(a.right)};break;}
   case Kind::Delay:y=r.delayed;break;
   case Kind::Output:y={a.left*renderLevel,a.right*renderLevel};break;
  }
  r.output=clipSample(y);
 }
 // Delayed edges read current inputs AFTER all other nodes have completed.
 for(size_t i=0;i<graph_.nodes.size();i++)
  if(graph_.nodes[i].kind==Kind::Delay)runtime_[i]->delayed=clipSample(runtime_[size_t(graph_.nodes[i].a)]->output);
 return clipSample(runtime_[size_t(output_)]->output);
}
double Instrument::returnResponseMagnitude() const noexcept{return returnResponseMagnitude_;}
double Instrument::activeStateWork() const noexcept{
 double work=0.;
 for(const auto& rt:runtime_)if(!rt->stateVoices.empty())
  for(const auto& v:rt->stateVoices)if(v)work+=v->ledger().activeWork;
 return work;
}
} // namespace panorama::universal23
