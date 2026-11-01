#include "NativeState26.h"
#include "UniversalInstrument23.h"
#undef NDEBUG
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
using namespace panorama;
int main(){
 std::string error;
 for(int variant=0;variant<6;variant++){
  const auto g=native26::example(variant);
  const auto saved=native26::serialize(g);
  native26::Graph parsed;
  assert(native26::deserialize(saved,parsed,&error));
  assert(saved==native26::serialize(parsed));
  auto synth=native26::Instrument::prepare(parsed,48000,&error);assert(synth);
  synth->noteOn(60,.8f);
  assert(synth->addActiveEffort(60,2,125.));
  assert(!synth->addActiveEffort(60,2,std::numeric_limits<double>::infinity()));
  double peak=0.,rms=0.;
  for(int i=0;i<2048;i++){
   const double x=synth->processMono();assert(std::isfinite(x));
   peak=std::max(peak,std::abs(x));rms+=x*x;
  }
  assert(peak>1e-8 && peak<1.01);assert(std::isfinite(synth->dissipated()));
  assert(synth->energyResidual()<1.e-5);
  assert(std::abs(synth->activeWork())>1e-12);
  std::cout<<g.name<<" rms="<<std::sqrt(rms/2048.)<<" peak="<<peak<<" residual="<<synth->energyResidual()<<"\n";
 }
 {auto invalid=native26::example();invalid.edges[0].unit=unified22::Unit::Metre;
  assert(!native26::Instrument::prepare(invalid,48000,&error));
  assert(error.find("port")!=std::string::npos);
 }
 {auto invalid=native26::example();invalid.edges[0].strength=-12.;
  assert(!native26::Instrument::prepare(invalid,48000));}
 {auto invalid=native26::example();invalid.edges[0].a=64;
  assert(!native26::Instrument::prepare(invalid,48000));}
 {native26::Graph g;assert(!native26::deserialize("PANORAMIZE-NATIVE-26|1\nMODE|0|1|2|0|0.2\n",g));}
 // Conserved energy + dissipated ledger on a cyclic six-domain configuration:
 {auto g=native26::example();for(auto&m:g.modes)m.damping=0.;
  for(auto& edge:g.edges)edge.kind=native26::Coupling::Conservative;
  auto x=native26::Instrument::prepare(g,48000,&error);assert(x);
  x->noteOn(50,.6f);for(int i=0;i<2400;i++)x->processMono();
  assert(std::abs(x->dissipated())<1e-8);assert(x->energyResidual()<1e-6);
 }
 // New native state is an actual VST3 graph source; legacy programs remain valid.
 {using namespace universal23;
  Graph g;g.name="Native 2.6";
  g.nodes={{Kind::NativeState,-1,-1,2,.8f},{Kind::Output,0,-1,0,.85f}};
  auto inst=Instrument::prepare(g,48000,&error);assert(inst);
  inst->noteOn(60,.7f);
  double rms=0.;for(int i=0;i<1024;i++){
   const auto x=inst->processOne();assert(std::isfinite(x.left)&&std::isfinite(x.right));rms+=x.left*x.left;
  }
  assert(rms>1.e-9);
  const auto saved=serialize(g);Graph reload;
  assert(deserialize(saved,reload,&error));assert(saved==serialize(reload));
  auto again=Instrument::prepare(reload,48000,&error);assert(again);
  Graph old;assert(deserialize("PANORAMIZE-GRAPH-25|1\nNAME|Old\nNODE|0|-1|-1|0|0.5\nNODE|6|0|-1|0|0.8\n",old));
  assert(Instrument::prepare(old,48000,&error));
  auto bad=g;bad.nodes[0].preset=100;assert(!Instrument::prepare(bad,48000));
 }
 std::cout<<"PANORAMIZE_NATIVE26_SUCCESS\n";
}
