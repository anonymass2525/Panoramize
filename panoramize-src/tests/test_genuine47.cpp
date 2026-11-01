#include "GenuineNative47.h"
#include "UniversalInstrument23.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
using namespace panorama;
int main(){
 auto r=genuine47::example();std::string why;
 assert(genuine47::valid(r,&why));
 // All original eight engine types stay in the original Native27 graph.
 assert(r.original.acoustic.modes.size()==8);
 for(size_t j=0;j<8;j++)assert(int(r.original.acoustic.modes[j].engine)==int(j));
 const auto serialized=genuine47::serialize(r);
 genuine47::Recipe decoded;
 assert(genuine47::deserialize(serialized,decoded,&why));
 assert(genuine47::serialize(decoded)==serialized);
 auto inst=genuine47::Instrument::prepare(decoded,48000,&why);
 if(!inst){std::cerr<<why<<'\n';return 2;}
 inst->noteOn(60,.8f);inst->modWheel(.7f);inst->channelPressure(.4f);inst->setMacro(0,.9f);
 double energy=0;for(int i=0;i<12000;i++){
  float value=inst->processOne();assert(std::isfinite(value));energy+=double(value)*value;
 }
 assert(energy>1e-10);
 auto meter=inst->meter();assert(meter.samples==12000&&meter.peak<=1&&meter.nativeLedgerResidual>=0);
 assert(std::isfinite(meter.nativeActiveWork)&&std::isfinite(meter.physicalActiveWork));
 std::cout<<"genuine47 rms="<<std::sqrt(energy/12000)<<" nativeWork="<<meter.nativeActiveWork<<" physicalWork="<<meter.physicalActiveWork<<"\n";
 // Original-algorithm fidelity: PCM bypass matches unmodified 3.9 native+physical engine sample-for-sample.
 auto bypass=r;bypass.tone.wet=0;bypass.tone.output=1;bypass.routes.clear();
 auto a=genuine47::Instrument::prepare(bypass,48000,&why);
 auto b=unified39::Instrument::prepare(bypass.original,48000,&why);assert(a&&b);
 a->noteOn(57,1.f);b->noteOn(57,1.f);
 for(int i=0;i<4096;i++){
  float x=a->processOne(),y=b->processOne();
  assert(x==y);
 }
 // Macro & modulation actually alter the rendered waveform (not UI-only knobs).
 auto modded=genuine47::Instrument::prepare(r,48000);
 auto neutral=r;neutral.routes.clear();auto plain=genuine47::Instrument::prepare(neutral,48000);
 assert(modded&&plain);modded->noteOn(64,.85f);plain->noteOn(64,.85f);
 double difference=0;
 for(int i=0;i<8000;i++){double x=modded->processOne(),y=plain->processOne();difference+=(x-y)*(x-y);}
 assert(difference>1e-12);std::cout<<"modulation_difference="<<difference<<"\n";
 // Active excitation cannot be silently re-labeled as passive on a mode which
 // Native27 does not accept powered oscillator excitation for.
 auto bad=r;bad.original.acoustic.modes[2].excitation=40.;
 assert(!genuine47::valid(bad,&why));
 bad=r;bad.routes[0].depth=std::numeric_limits<double>::quiet_NaN();assert(!genuine47::valid(bad,&why));
 bad=r;bad.routes[0].source=static_cast<genuine47::Source>(333);assert(!genuine47::valid(bad,&why));
 auto keep=r;assert(!genuine47::deserialize(serialized+"CORRUPT",keep,&why));
 assert(genuine47::serialize(keep)==serialized);
 // End-to-end Universal Patchbay, full graph-state save/load and MIDI automation
 auto full=universal23::genuineNativeGraph();auto patch=universal23::serialize(full);
 universal23::Graph decodedGraph;
 assert(universal23::deserialize(patch,decodedGraph,&why));assert(universal23::serialize(decodedGraph)==patch);
 auto synth=universal23::Instrument::prepare(decodedGraph,48000,&why);
 if(!synth){std::cerr<<why<<'\n';return 3;}
 synth->noteOn(60,.8f);synth->modWheel(.9f);synth->channelPressure(.5f);synth->articulationControl(24,.8f);
 double output=0;for(int i=0;i<2048;i++){const auto v=synth->processOne();assert(std::isfinite(v.left)&&std::isfinite(v.right));output+=v.left*v.left;}
 assert(output>1e-12);synth->allNotesOff();
 // Global node-level routing also works on older legacy/physical engines.
 universal23::Graph factory;factory.name="Global Modulation / Factory";
 factory.nodes={{universal23::Kind::Legacy,-1,-1,24,.8f},{universal23::Kind::Output,0,-1,0,.85f}};
 factory.globalRoutes={{genuine47::Source::Wheel,0,1.f},{genuine47::Source::Macro1,1,.2f}};
 factory.globalMacros[0]=.7f;
 const auto routePatch=universal23::serialize(factory);
 universal23::Graph factoryCopy;
 assert(universal23::deserialize(routePatch,factoryCopy,&why));
 assert(universal23::serialize(factoryCopy)==routePatch);
 auto factoryA=universal23::Instrument::prepare(factoryCopy,48000,&why);
 auto noGlobal=factoryCopy;noGlobal.globalRoutes.clear();
 auto factoryB=universal23::Instrument::prepare(noGlobal,48000,&why);
 assert(factoryA&&factoryB);
 factoryA->noteOn(60,.7f);factoryB->noteOn(60,.7f);
 factoryA->modWheel(.9f);factoryB->modWheel(.9f);
 double gainDifference=0.;
 for(int i=0;i<2048;i++){
  const auto x=factoryA->processOne(),y=factoryB->processOne();
  assert(std::isfinite(x.left)&&std::isfinite(y.left));
  gainDifference+=std::abs(double(x.left-y.left));
 }
 assert(gainDifference>1.e-5);
 auto corrupt=factoryCopy;corrupt.globalRoutes[0].node=900;
 assert(!universal23::Instrument::prepare(corrupt,48000,&why));
 std::cout<<"global modulation works on factory audio engine, delta="<<gainDifference<<"\n";
 // Older explicit 4.5 graphs retain their saved-data schema.
 auto legacy=universal23::nonlinearPassiveGraph();auto olderText=universal23::serialize(legacy);
 universal23::Graph old;assert(universal23::deserialize(olderText,old,&why));
 std::cout<<"UNIFIED GENUINE47 PASS\n";
}
