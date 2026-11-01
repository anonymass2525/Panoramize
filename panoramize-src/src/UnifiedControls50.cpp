#include "UnifiedControls50.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>
namespace panorama::controls50 {
namespace {
bool has(const Graph&g,universal23::Kind k){return std::any_of(g.nodes.begin(),g.nodes.end(),[k](const auto&n){return n.kind==k;});}
}
std::vector<Parameter> catalog(const Graph&g){
 std::vector<Parameter> out;out.reserve(256);
 auto add=[&](const std::string&id,const std::string&group,const std::string&name,const std::string&unit,
  double lo,double hi,bool live,int host,std::function<double(const Graph&)>get,std::function<void(Graph&,double)>set){
   out.push_back({id,group,name,unit,lo,hi,live,host,std::move(get),std::move(set)});
 };
#define D(ID,GR,EXPR,FIELD,LO,HI,UNIT) add(ID,GR,#FIELD,UNIT,LO,HI,false,-1,[](const Graph&g){return double(g.EXPR.FIELD);},[](Graph&g,double x){g.EXPR.FIELD=x;})
#define I(ID,GR,EXPR,FIELD,LO,HI,UNIT) add(ID,GR,#FIELD,UNIT,LO,HI,false,-1,[](const Graph&g){return double(g.EXPR.FIELD);},[](Graph&g,double x){g.EXPR.FIELD=int(std::lround(x));})
 for(int j=0;j<4;j++){
  add("graph.macro."+std::to_string(j+1),"Global","macro","normalized",0,1,true,9+j,
  [j](const Graph&g){return double(g.globalMacros[size_t(j)]);},
  [j](Graph&g,double x){g.globalMacros[size_t(j)]=float(x);});
 }
 // Stable Native51 host slots correspond to mode 0..7 in genuine/direct/unified Native27.
 const bool native51=has(g,universal23::Kind::DirectState27)||has(g,universal23::Kind::UnifiedPhysical39)||has(g,universal23::Kind::GenuineNative47);
 if(native51)for(int i=0;i<8;i++){
  add("native51.mode."+std::to_string(i)+".pickupScale","Native51 Live","mode pickup multiplier","normalized",0,1,true,80+i,
    [i](const Graph&g){return double(g.nativePickup51[size_t(i)]);},
    [i](Graph&g,double x){g.nativePickup51[size_t(i)]=float(x);});
  add("native51.mode."+std::to_string(i)+".excitationScale","Native51 Live","VA/FM active effort multiplier","normalized",0,1,true,88+i,
    [i](const Graph&g){return double(g.nativeExcite51[size_t(i)]);},
    [i](Graph&g,double x){g.nativeExcite51[size_t(i)]=float(x);});
 }
 for(size_t i=0;i<g.nodes.size();i++){
  add("node."+std::to_string(i)+".level","Patchbay","output level","normalized",0,1,i<64,int(16+i),
  [i](const Graph&g){return double(g.nodes[i].level);},
  [i](Graph&g,double x){g.nodes[i].level=float(x);});
 }
 add("graph.lfo_hz","Global","global LFO","Hz",0.01,50,false,-1,[](const Graph&g){return double(g.globalLfoHz);},[](Graph&g,double x){g.globalLfoHz=float(x);});
 if(has(g,universal23::Kind::DirectState27)){
  for(size_t i=0;i<g.instrument.modes.size();i++){
   add("native27.mode."+std::to_string(i)+".ratio","native27","ratio","ratio",.01,40,false,-1,
    [i](const Graph&g){return g.instrument.modes[i].ratio;},
    [i](Graph&g,double x){g.instrument.modes[i].ratio=x;});
   add("native27.mode."+std::to_string(i)+".damping","native27","damping","1/s",0,10000,false,-1,
    [i](const Graph&g){return g.instrument.modes[i].damping;},
    [i](Graph&g,double x){g.instrument.modes[i].damping=x;});
   add("native27.mode."+std::to_string(i)+".nonlinear","native27","nonlinear","unitless",0,1000,false,-1,
    [i](const Graph&g){return g.instrument.modes[i].nonlinear;},
    [i](Graph&g,double x){g.instrument.modes[i].nonlinear=x;});
   add("native27.mode."+std::to_string(i)+".pickup","native27","pickup","normalized",-10,10,false,-1,
    [i](const Graph&g){return g.instrument.modes[i].pickup;},
    [i](Graph&g,double x){g.instrument.modes[i].pickup=x;});
   add("native27.mode."+std::to_string(i)+".excitation","native27","excitation","active effort",-1000,1000,false,-1,
    [i](const Graph&g){return g.instrument.modes[i].excitation;},
    [i](Graph&g,double x){g.instrument.modes[i].excitation=x;});
  }
 }
 if(has(g,universal23::Kind::GenuineNative47)){
  for(size_t i=0;i<g.genuine47.original.acoustic.modes.size();i++){
   add("genuine47.native.mode."+std::to_string(i)+".ratio","genuine47.native","ratio","ratio",.01,40,false,-1,
    [i](const Graph&g){return g.genuine47.original.acoustic.modes[i].ratio;},
    [i](Graph&g,double x){g.genuine47.original.acoustic.modes[i].ratio=x;});
   add("genuine47.native.mode."+std::to_string(i)+".damping","genuine47.native","damping","1/s",0,10000,false,-1,
    [i](const Graph&g){return g.genuine47.original.acoustic.modes[i].damping;},
    [i](Graph&g,double x){g.genuine47.original.acoustic.modes[i].damping=x;});
   add("genuine47.native.mode."+std::to_string(i)+".nonlinear","genuine47.native","nonlinear","unitless",0,1000,false,-1,
    [i](const Graph&g){return g.genuine47.original.acoustic.modes[i].nonlinear;},
    [i](Graph&g,double x){g.genuine47.original.acoustic.modes[i].nonlinear=x;});
   add("genuine47.native.mode."+std::to_string(i)+".pickup","genuine47.native","pickup","normalized",-10,10,false,-1,
    [i](const Graph&g){return g.genuine47.original.acoustic.modes[i].pickup;},
    [i](Graph&g,double x){g.genuine47.original.acoustic.modes[i].pickup=x;});
   add("genuine47.native.mode."+std::to_string(i)+".excitation","genuine47.native","excitation","active effort",-1000,1000,false,-1,
    [i](const Graph&g){return g.genuine47.original.acoustic.modes[i].excitation;},
    [i](Graph&g,double x){g.genuine47.original.acoustic.modes[i].excitation=x;});
  }
 }
 if(has(g,universal23::Kind::UnifiedPhysical39)){
  for(size_t i=0;i<g.unified39.acoustic.modes.size();i++){
   add("unified39.native.mode."+std::to_string(i)+".ratio","unified39.native","ratio","ratio",.01,40,false,-1,
    [i](const Graph&g){return g.unified39.acoustic.modes[i].ratio;},
    [i](Graph&g,double x){g.unified39.acoustic.modes[i].ratio=x;});
   add("unified39.native.mode."+std::to_string(i)+".damping","unified39.native","damping","1/s",0,10000,false,-1,
    [i](const Graph&g){return g.unified39.acoustic.modes[i].damping;},
    [i](Graph&g,double x){g.unified39.acoustic.modes[i].damping=x;});
   add("unified39.native.mode."+std::to_string(i)+".nonlinear","unified39.native","nonlinear","unitless",0,1000,false,-1,
    [i](const Graph&g){return g.unified39.acoustic.modes[i].nonlinear;},
    [i](Graph&g,double x){g.unified39.acoustic.modes[i].nonlinear=x;});
   add("unified39.native.mode."+std::to_string(i)+".pickup","unified39.native","pickup","normalized",-10,10,false,-1,
    [i](const Graph&g){return g.unified39.acoustic.modes[i].pickup;},
    [i](Graph&g,double x){g.unified39.acoustic.modes[i].pickup=x;});
   add("unified39.native.mode."+std::to_string(i)+".excitation","unified39.native","excitation","active effort",-1000,1000,false,-1,
    [i](const Graph&g){return g.unified39.acoustic.modes[i].excitation;},
    [i](Graph&g,double x){g.unified39.acoustic.modes[i].excitation=x;});
  }
 }
 if(has(g,universal23::Kind::GuitarString29)){
  D("guitar29.frequencyHz","guitar29",guitar,frequencyHz,40,1100,"Hz");
  D("guitar29.pickPosition","guitar29",guitar,pickPosition,0.05,0.48,"ratio");
  D("guitar29.pickupPosition","guitar29",guitar,pickupPosition,0.05,0.95,"ratio");
  D("guitar29.decayBase","guitar29",guitar,decayBase,0.1,15,"1/s");
  D("guitar29.decayHigh","guitar29",guitar,decayHigh,0,2,"1/s");
  D("guitar29.inharmonicity","guitar29",guitar,inharmonicity,0,0.05,"ratio");
  D("guitar29.spectralTilt","guitar29",guitar,spectralTilt,0,1,"ratio");
  D("guitar29.gain","guitar29",guitar,gain,0,4,"gain");
  I("guitar29.harmonics","guitar29",guitar,harmonics,1,32,"count");
 }
 if(has(g,universal23::Kind::GenuineNative47)){
  D("genuine47.tone.cutoff","genuine47.tone",genuine47.tone,cutoff,20,20000,"Hz");
  D("genuine47.tone.resonance","genuine47.tone",genuine47.tone,resonance,0.08,2,"ratio");
  D("genuine47.tone.drive","genuine47.tone",genuine47.tone,drive,0.01,20,"ratio");
  D("genuine47.tone.output","genuine47.tone",genuine47.tone,output,0,4,"gain");
  D("genuine47.tone.wet","genuine47.tone",genuine47.tone,wet,0,1,"ratio");
  D("genuine47.tone.lfoHz","genuine47.tone",genuine47.tone,lfoHz,0.01,50,"Hz");
 }
 if(has(g,universal23::Kind::PhysicalCoupled35)){
  D("coupled35.stringLoss","coupled35",coupled35,stringLoss,0,100,"1/s");
  D("coupled35.meshYoung","coupled35",coupled35,meshYoung,10000.0,10000000000.0,"Pa");
  D("coupled35.meshDensity","coupled35",coupled35,meshDensity,10,1000000.0,"kg/m3");
  D("coupled35.bridgeCoupling","coupled35",coupled35,bridgeCoupling,0,2,"ratio");
  D("coupled35.outputGain","coupled35",coupled35,outputGain,0,20,"gain");
 }
 if(has(g,universal23::Kind::PhysicalContact34)){
  D("contact34.openHz","contact34",contact34,openHz,35,1000,"Hz");
  D("contact34.stringLoss","contact34",contact34,stringLoss,0,100,"1/s");
  D("contact34.fingerStiffness","contact34",contact34,fingerStiffness,0,10,"ratio");
  D("contact34.fretGap","contact34",contact34,fretGap,0,0.01,"m");
  D("contact34.bridgeHz","contact34",contact34,bridgeHz,20,10000,"Hz");
  D("contact34.bodyHzX","contact34",contact34,bodyHzX,20,10000,"Hz");
  D("contact34.bodyHzY","contact34",contact34,bodyHzY,20,10000,"Hz");
  D("contact34.bodyHzZ","contact34",contact34,bodyHzZ,20,10000,"Hz");
  D("contact34.outputGain","contact34",contact34,outputGain,0,20,"gain");
 }
 if(has(g,universal23::Kind::PhysicalExpressive32)){
  D("expressive32.glideMs","expressive32",expressive32,glideMs,0,500,"ms");
  D("expressive32.velocityPickRange","expressive32",expressive32,velocityPickRange,0,1,"ratio");
  D("expressive32.pressureContactDamping","expressive32",expressive32,pressureContactDamping,0,500,"1/s");
  D("expressive32.velocityForceCurve","expressive32",expressive32,velocityForceCurve,0.1,5,"ratio");
 }
 if(has(g,universal23::Kind::PhysicalArticulated33)){
  D("articulated33.glideMs","articulated33",articulated33,glideMs,0,500,"ms");
  D("articulated33.hammerImpact","articulated33",articulated33,hammerImpact,0,3,"ratio");
  D("articulated33.pullImpact","articulated33",articulated33,pullImpact,0,3,"ratio");
  D("articulated33.slideFriction","articulated33",articulated33,slideFriction,0,3,"ratio");
  D("articulated33.palmDamping","articulated33",articulated33,palmDamping,0,1000,"1/s");
 }
 if(has(g,universal23::Kind::EnergyGraph40)){
  D("energy40.masterGain","energy40",energy40,masterGain,0,20,"gain");
  D("energy40.smoothingMs","energy40",energy40,smoothingMs,1,1000,"ms");
 }
 if(has(g,universal23::Kind::ReducedHamiltonian44)){
  I("reduced44.modesPerRegion","reduced44",reduced44,modesPerRegion,0,80,"count");
  I("reduced44.maxIterations","reduced44",reduced44,maxIterations,1,1000,"count");
  D("reduced44.relativeTolerance","reduced44",reduced44,relativeTolerance,1e-14,0.001,"ratio");
 }
 if(has(g,universal23::Kind::NonlinearPassive45)){
  D("passive45.modalTransformer","passive45",passive45,modalTransformer,1e-06,1,"ratio");
  D("passive45.modalCoupling","passive45",passive45,modalCoupling,0,100000.0,"N/m");
  I("passive45.nonlinearIterations","passive45",passive45,nonlinearIterations,1,64,"count");
  D("passive45.nonlinearTolerance","passive45",passive45,nonlinearTolerance,1e-15,0.001,"ratio");
 }
 if(has(g,universal23::Kind::SparseHamiltonian43)){
  D("sparse43.impulseForce","sparse43",sparse43,impulseForce,0,100,"N");
  D("sparse43.outputGain","sparse43",sparse43,outputGain,0,1000000.0,"gain");
  I("sparse43.maxIterations","sparse43",sparse43,maxIterations,1,1000,"count");
  D("sparse43.relativeTolerance","sparse43",sparse43,relativeTolerance,1e-14,0.001,"ratio");
 }
 if(has(g,universal23::Kind::SparseHamiltonian43)){
  for(size_t i=0;i<g.sparse43.regions.size();i++){
   add("sparse43.region."+std::to_string(i)+".density","FEM region","density","kg/m3",1e-6,1e8,false,-1,
    [i](const Graph&g){return g.sparse43.regions[i].density;},
    [i](Graph&g,double x){g.sparse43.regions[i].density=x;});
   add("sparse43.region."+std::to_string(i)+".tension","FEM region","tension","N",1e-8,1e8,false,-1,
    [i](const Graph&g){return g.sparse43.regions[i].tension;},
    [i](Graph&g,double x){g.sparse43.regions[i].tension=x;});
   add("sparse43.region."+std::to_string(i)+".young","FEM region","young","Pa",1e2,1e13,false,-1,
    [i](const Graph&g){return g.sparse43.regions[i].young;},
    [i](Graph&g,double x){g.sparse43.regions[i].young=x;});
   add("sparse43.region."+std::to_string(i)+".poisson","FEM region","poisson","ratio",0,0.49,false,-1,
    [i](const Graph&g){return g.sparse43.regions[i].poisson;},
    [i](Graph&g,double x){g.sparse43.regions[i].poisson=x;});
   add("sparse43.region."+std::to_string(i)+".damping","FEM region","damping","1/s",0,1e6,false,-1,
    [i](const Graph&g){return g.sparse43.regions[i].damping;},
    [i](Graph&g,double x){g.sparse43.regions[i].damping=x;});
   add("sparse43.region."+std::to_string(i)+".soundSpeed","FEM region","soundSpeed","m/s",1,10000,false,-1,
    [i](const Graph&g){return g.sparse43.regions[i].soundSpeed;},
    [i](Graph&g,double x){g.sparse43.regions[i].soundSpeed=x;});
   add("sparse43.region."+std::to_string(i)+".gain","FEM region","gain","gain",-100,100,false,-1,
    [i](const Graph&g){return g.sparse43.regions[i].gain;},
    [i](Graph&g,double x){g.sparse43.regions[i].gain=x;});
  }
 }
 if(has(g,universal23::Kind::EnergyGraph40)){
  for(size_t i=0;i<g.energy40.nodes.size();i++){
   add("energy40.node."+std::to_string(i)+".mass","Energy port","mass","SI inertance",1e-8,1e10,false,-1,
    [i](const Graph&g){return g.energy40.nodes[i].mass;},
    [i](Graph&g,double x){g.energy40.nodes[i].mass=x;});
   add("energy40.node."+std::to_string(i)+".frequency","Energy port","frequency","Hz",1,5000,false,-1,
    [i](const Graph&g){return g.energy40.nodes[i].frequency;},
    [i](Graph&g,double x){g.energy40.nodes[i].frequency=x;});
   add("energy40.node."+std::to_string(i)+".damping","Energy port","damping","damping",0,1e8,false,-1,
    [i](const Graph&g){return g.energy40.nodes[i].damping;},
    [i](Graph&g,double x){g.energy40.nodes[i].damping=x;});
   add("energy40.node."+std::to_string(i)+".area","Energy port","area","m2",1e-4,10,false,-1,
    [i](const Graph&g){return g.energy40.nodes[i].area;},
    [i](Graph&g,double x){g.energy40.nodes[i].area=x;});
   add("energy40.node."+std::to_string(i)+".drive","Energy port","drive","active force",0,1e6,false,-1,
    [i](const Graph&g){return g.energy40.nodes[i].drive;},
    [i](Graph&g,double x){g.energy40.nodes[i].drive=x;});
   add("energy40.node."+std::to_string(i)+".output","Energy port","output","gain",-1000,1000,false,-1,
    [i](const Graph&g){return g.energy40.nodes[i].output;},
    [i](Graph&g,double x){g.energy40.nodes[i].output=x;});
  }
 }

 if(has(g,universal23::Kind::BoundaryGraph41)){
  D("boundary41.bodyYoung","Boundary41",boundary41,bodyYoung,1e3,1e12,"Pa");
  D("boundary41.bodyDensity","Boundary41",boundary41,bodyDensity,1,1e6,"kg/m3");
  D("boundary41.bodyDamping","Boundary41",boundary41,bodyDamping,0,1e5,"1/s");
  D("boundary41.cavityVolume","Boundary41",boundary41,cavityVolume,1e-6,1e3,"m3");
  D("boundary41.pistonArea","Boundary41",boundary41,pistonArea,1e-6,1,"m2");
  D("boundary41.bodyCavityK","Boundary41",boundary41,bodyCavityK,0,1e12,"Pa/m3");
  D("boundary41.soundGain","Boundary41",boundary41,soundGain,0,10000,"gain");
 }
 if(has(g,universal23::Kind::UnifiedPhysical39)){
  D("unified39.physical_mix","Unified39",unified39,physicalMix,0,1,"ratio");
  D("unified39.acoustic_mix","Unified39",unified39,acousticMix,0,1,"ratio");
  for(size_t i=0;i<g.unified39.links.size();i++){
   add("unified39.link."+std::to_string(i)+".physicalToEffort","Native transducer","active transformer","sqrtJ/s per m/s",0,1e6,false,-1,
      [i](const Graph&g){return g.unified39.links[i].physicalToEffort;},
      [i](Graph&g,double x){g.unified39.links[i].physicalToEffort=x;});
   add("unified39.link."+std::to_string(i)+".flowToForce","Native transducer","active transformer","N/normalized_flow",0,1e4,false,-1,
      [i](const Graph&g){return g.unified39.links[i].flowToForce;},
      [i](Graph&g,double x){g.unified39.links[i].flowToForce=x;});
  }
 }
 if(has(g,universal23::Kind::NonlinearPassive45))for(size_t i=0;i<g.passive45.contacts.size();i++){
   add("passive45.contact."+std::to_string(i)+".gap","Contact","gap","m",0,1,false,-1,
    [i](const Graph&g){return g.passive45.contacts[i].gap;},[i](Graph&g,double x){g.passive45.contacts[i].gap=x;});
   add("passive45.contact."+std::to_string(i)+".friction","Contact","friction","ratio",0,10,false,-1,
    [i](const Graph&g){return g.passive45.contacts[i].friction;},[i](Graph&g,double x){g.passive45.contacts[i].friction=x;});
 }
#undef D
#undef I
 return out;
}
bool read(const Graph&g,const std::string&id,double&value){
 const auto list=catalog(g);
 for(const auto&x:list)if(x.id==id){value=x.read(g);return true;}
 return false;
}
bool change(Graph&g,const std::string&id,double value,std::string*error){
 if(!std::isfinite(value)){if(error)*error="non-finite parameter";return false;}
 auto list=catalog(g);
 for(auto&x:list)if(x.id==id){
  if(value<x.lo||value>x.hi){if(error)*error="out-of-range: "+id;return false;}
  x.write(g,value);if(error)error->clear();return true;
 }
 if(error)*error="unknown or unregistered internal parameter: "+id;
 return false;
}
bool transaction(Graph&g,const std::vector<std::pair<std::string,double>>&edits,double rate,std::string*error){
 if(edits.size()>256){if(error)*error="too many edits";return false;}
 Graph clone=g;
 for(const auto&e:edits)if(!change(clone,e.first,e.second,error))return false;
 // Validating by preparing the actual graph ensures domain-specific restrictions
 // and topology; all changes are committed together or not at all.
 std::string why;auto compiled=universal23::Instrument::prepare(clone,rate,&why);
 if(!compiled){if(error)*error="compile rejection: "+why;return false;}
 g=std::move(clone);if(error)error->clear();return true;
}
std::string catalogCSV(const Graph&g){
 std::ostringstream s;
 s<<"id,group,name,unit,min,max,realtime,host_index,current\n";
 s<<std::setprecision(14);
 for(const auto&p:catalog(g))
  s<<p.id<<','<<p.group<<','<<p.name<<','<<p.unit<<','<<p.lo<<','<<p.hi<<','<<int(p.realtime)<<','<<p.hostIndex<<','<<p.read(g)<<'\n';
 return s.str();
}
} // namespace panorama::controls50
