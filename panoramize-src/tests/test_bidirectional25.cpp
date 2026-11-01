#include "UniversalInstrument23.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <iostream>
#include <utility>
using namespace panorama;
using namespace panorama::universal23;
static double energy(Instrument& i){
 i.noteOn(60,.8f);double e=0;
 for(int n=0;n<1024;n++){const Sample s=i.processOne();
 assert(std::isfinite(s.left)&&std::isfinite(s.right));e+=double(s.left)*s.left+double(s.right)*s.right;}
 return e;
}
int main(){
 std::string err;
 Graph g;g.name="Reciprocal Horizon";
 g.nodes={
  {Kind::Legacy,-1,-1,0,1.f},
  {Kind::StateDrive,0,2,0,.8f},
  {Kind::UnifiedState,-1,-1,0,.8f},
  {Kind::StateReturn,2,0,0,.8f},
  {Kind::Mix,0,2,0,.5f},
  {Kind::Output,4,-1,0,.85f}
 };
 auto a=Instrument::prepare(g,48000,&err);assert(a&&err.empty());
 const double both=energy(*a);
 assert(both>1e-8&&std::isfinite(a->activeStateWork()));
 assert(std::abs(a->activeStateWork())>1e-12);
 assert(a->returnResponseMagnitude()>1e-12);
 Graph noReturn=g;noReturn.nodes[3].level=0;
 auto b=Instrument::prepare(noReturn,48000,&err);assert(b);
 const double forward=energy(*b);
 assert(std::abs(forward-both)>1e-9);
 Graph noDrive=g;noDrive.nodes[1].level=0;
 auto c=Instrument::prepare(noDrive,48000,&err);assert(c);
 const double reverse=energy(*c);
 assert(std::abs(reverse-both)>1e-9);
 // The same patch is persistent, and older 2.3/2.4 recipes load.
 const auto text=serialize(g);assert(text.find("PANORAMIZE-GRAPH-25|1")==0);
 Graph restored;assert(deserialize(text,restored,&err));
 auto d=Instrument::prepare(restored,48000,&err);assert(d);
 assert(std::abs(energy(*d)-both)<1e-8);
 for(const char* head:{"PANORAMA-GRAPH-23|1","PANORAMAIZE-GRAPH-24|1"}){
  Graph old;std::string source=std::string(head)+"\nNAME|Old\nNODE|0|-1|-1|0|1\nNODE|6|0|-1|0|1\n";
  assert(deserialize(source,old,&err));assert(Instrument::prepare(old,48000,&err));
 }
 // Invalid directions/ports are rejected.
 for(int port: {-1,4}){Graph bad=g;bad.nodes[3].preset=port;assert(!Instrument::prepare(bad,48000,&err));}
 Graph bad=g;bad.nodes[3].b=2;assert(!Instrument::prepare(bad,48000,&err));
 // Polyphony stress: note streams remain finite under causal bidirectional links.
 auto poly=Instrument::prepare(g,48000,&err);assert(poly);
 for(int note=48;note<64;note++)poly->noteOn(note,.75f);
 for(int n=0;n<256;n++){auto v=poly->processOne();assert(std::isfinite(v.left)&&std::isfinite(v.right));}
 assert(std::isfinite(poly->activeStateWork()));
 poly->allNotesOff();
 // Injected state must alter audible samples, not merely return "applied".
 for(const auto pair:{std::pair{3,0},std::pair{24,1},std::pair{60,2},std::pair{60,3}}){
  const int preset=pair.first,port=pair.second;
  Synth test,reference;test.prepare(48000);reference.prepare(48000);
  test.setPatch(factoryPresets()[size_t(preset)]);reference.setPatch(factoryPresets()[size_t(preset)]);
  test.noteOn(60,.8f);reference.noteOn(60,.8f);
  for(int n=0;n<100;n++){test.processOne();reference.processOne();}
  double deviation=0.;
  for(int n=0;n<350;n++){
   assert(test.injectStateResponseForNote(60,port,.8));
   const auto x=test.processOne(),y=reference.processOne();
   deviation+=std::abs(x.left-y.left)+std::abs(x.right-y.right);
  }
  assert(deviation>1e-8);
 }
 // Actual modular spectral/physical state injection can also change sound.
 for(int preset:{24,60}){
  Synth synth;synth.prepare(48000);synth.setPatch(factoryPresets()[size_t(preset)]);synth.noteOn(60,.8f);
  bool has=false;for(int n=0;n<64;n++)synth.processOne();
  for(int mode=0;mode<4;mode++)has|=synth.injectStateResponseForNote(60,mode,.8);
  assert(has);const auto o=synth.processOne();assert(std::isfinite(o.left));
 }
 std::cout<<"both="<<both<<" forward="<<forward<<" reverse="<<reverse
          <<" return_response="<<a->returnResponseMagnitude()<<"\n";
}
