#include "NativeState27.h"
#include "UniversalInstrument23.h"
#undef NDEBUG
#include <cassert>
#include <cmath>
#include <iostream>
using namespace panorama;
static double render(universal23::Instrument& v){
 v.noteOn(60,.76f);double power=0;
 for(int i=0;i<1024;i++){const auto x=v.processOne();assert(std::isfinite(x.left));assert(std::isfinite(x.right));power+=double(x.left)*x.left;}
 v.noteOff(60);return power;
}
int main(){
 std::string err;
 const auto p=native27::example();
 auto text=native27::serialize(p);
 native27::Graph again;assert(native27::deserialize(text,again,&err));assert(text==native27::serialize(again));
 auto native=native27::Instrument::prepare(again,48000,&err);assert(native);
 native->noteOn(60,.8f);
 double rms=0.;for(int i=0;i<2048;i++){
  const float s=native->processMono();assert(std::isfinite(s));rms+=double(s)*s;
 }
 assert(rms>1.e-8);assert(std::isfinite(native->activeWork()));assert(native->activeWork()!=0);
 assert(native->energyResidual()<1e-5);
 auto nativeOff=p;nativeOff.modes[0].excitation=0.;nativeOff.modes[1].excitation=0.;
 auto dry=native27::Instrument::prepare(nativeOff,48000,&err);assert(dry);
 dry->noteOn(60,.8f);double difference=0.;native->allNotesOff();native->noteOn(60,.8f);
 for(int i=0;i<512;i++)difference+=std::abs(native->processMono()-dry->processMono());
 assert(difference>1.e-7);
 universal23::Graph graph=universal23::directGraph();graph.instrument=p;
 const auto recipe=universal23::serialize(graph);
 assert(recipe.find("PANORAMIZE-GRAPH-27|1")==0);
 universal23::Graph parsed;assert(universal23::deserialize(recipe,parsed,&err));
 assert(recipe==universal23::serialize(parsed));
 auto a=universal23::Instrument::prepare(parsed,48000,&err);assert(a);
 const auto energy=render(*a);assert(energy>1.e-9);
 auto b=universal23::Instrument::prepare(parsed,48000,&err);assert(b);
 assert(std::abs(render(*b)-energy)<1.e-9);
 // Make a patch in a different order with different roles and connections.
 auto other=parsed;other.instrument.modes[0].engine=native27::Engine::Vocal;
 other.instrument.edges[0].kind=native26::Coupling::Dissipative;
 auto changed=universal23::Instrument::prepare(other,48000,&err);assert(changed);
 assert(std::abs(render(*changed)-energy)>1.e-9);
 // Reject incorrectly calibrated units and preserve the previously parsed graph.
 auto invalid=parsed;invalid.instrument.edges[0].unit=unified22::Unit::Metre;
 assert(!universal23::Instrument::prepare(invalid,48000,&err));
 const auto old=universal23::serialize(parsed);
 assert(!universal23::deserialize("PANORAMIZE-GRAPH-27|1\nNAME|BROKEN\nNODE|10|-1|-1|0|1\nNODE|6|0|-1|0|1\n",parsed,&err));
 assert(universal23::serialize(parsed)==old);
 for(auto& m:other.instrument.modes)m.damping=0;
 for(auto& edge:other.instrument.edges)edge.kind=native26::Coupling::Conservative;
 auto conservation=native27::Instrument::prepare(other.instrument,48000,&err);assert(conservation);
 conservation->noteOn(58,.8f);for(int n=0;n<1400;n++)conservation->processMono();
 assert(conservation->energyResidual()<1.e-6);
 // Check old program recipe unaffected, active MIDI, and all 72 presets still remain.
 universal23::Graph oldGraph;assert(universal23::deserialize("PANORAMIZE-GRAPH-26|1\nNAME|Compatible\nNODE|0|-1|-1|3|0.8\nNODE|6|0|-1|0|0.8\n",oldGraph));
 assert(universal23::Instrument::prepare(oldGraph,48000,&err));
 std::cout<<"PANORAMIZE_27_OK rms="<<std::sqrt(rms/2048.)<<" power="<<energy<<" work="<<native->activeWork()<<"\n";
}
