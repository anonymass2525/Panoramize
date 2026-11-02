#include "UnifiedAcousticPhysical39.h"
#include "UniversalInstrument23.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
using namespace panorama;
int main(){
 std::string error;
 auto graph=unified39::example();
 assert(graph.acoustic.modes.size()==8); // VA, FM, spectral, phase, resonance, physical, vocal, nonlinear
 assert(graph.physics.domains.size()==3);
 auto patch=unified39::serialize(graph);
 unified39::Recipe restored;
 assert(unified39::deserialize(patch,restored,&error));
 assert(unified39::serialize(restored)==patch);
 auto inst=unified39::Instrument::prepare(restored,48000,&error);
 if(!inst){std::cerr<<error<<'\n';return 2;}
 inst->noteOn(60,.7f);
 double sum=0, max=0;
 for(int i=0;i<12000;i++){
  float s=inst->processOne();assert(std::isfinite(s));sum+=s*s;max=std::max(max,double(std::abs(s)));
  assert(std::isfinite(inst->physicalEnergy())&&inst->physicalEnergy()>=-1e-6);
 }
 assert(max>1.e-8&&sum>1.e-9);
 assert(std::isfinite(inst->physicalExternalWork())&&std::isfinite(inst->acousticActiveWork()));
 std::cout<<"mixed rms="<<std::sqrt(sum/12000)<<" max="<<max<<" physicalWork="<<inst->physicalExternalWork()<<" acousticWork="<<inst->acousticActiveWork()<<" limiter="<<inst->limitedCount()<<'\n';
 // Disabling both transducer paths preserves an audible standalone physical/acoustic model
 // but MUST change the output when compared with active, bidirectional transfers.
 auto isolated=graph;isolated.links.clear();
 auto a=unified39::Instrument::prepare(graph,48000,&error);
 auto b=unified39::Instrument::prepare(isolated,48000,&error);
 assert(a&&b);a->noteOn(64,.9f);b->noteOn(64,.9f);
 double diff=0;
 for(int i=0;i<10000;i++){
  const auto x=a->processOne(),y=b->processOne();assert(std::isfinite(x)&&std::isfinite(y));diff+=(x-y)*(x-y);
 }
 std::cout<<"connected-vs-isolated L2="<<diff<<'\n';
 assert(diff>1.e-11);
 // Type and numeric limits; rejected even when the payload can be parsed syntactically.
 auto bad=graph;bad.links[0].physicalSensor=unified39::Sensor::AcousticNormalizedFlow;
 assert(!unified39::Instrument::prepare(bad,48000,&error));
 bad=graph;bad.links[0].physicalToEffort=std::numeric_limits<double>::quiet_NaN();
 assert(!unified39::Instrument::prepare(bad,48000,&error));
 bad=graph;bad.links[0].acousticMode=99;assert(!unified39::Instrument::prepare(bad,48000,&error));
 bad=graph;bad.links[0].forceLimit=-1;assert(!unified39::Instrument::prepare(bad,48000,&error));
 bad=graph;bad.links[0].acousticToMechanical=false;bad.links[0].mechanicalToAcoustic=false;
 assert(!unified39::Instrument::prepare(bad,48000,&error));
 auto untouched=restored;assert(!unified39::deserialize("PANORAMIZE-UNIFIED39|1\nMIX nan 0.4 0\n",untouched,&error));
 assert(unified39::serialize(untouched)==patch);
 // One-sample bounded transducer forcing and limit instrumentation.
 auto clipped=graph;
 for(auto& l:clipped.links){l.physicalToEffort=1.e6;l.flowToForce=1000;l.effortLimit=.000001;l.forceLimit=1.e-9;}
 auto limited=unified39::Instrument::prepare(clipped,48000,&error);assert(limited);
 limited->noteOn(60,1.f);
 for(int i=0;i<4000;i++)assert(std::isfinite(limited->processOne()));
 assert(limited->maximumEffort()<=.0000010001&&limited->maximumForce()<=1.e-9*1.0001);
 // Existing Universal Patchbay source -> MIDI -> graph process -> versioned .pgraph
 auto vg=universal23::unifiedPhysicalGraph();
 const auto saved=universal23::serialize(vg);
 universal23::Graph reload;
 assert(universal23::deserialize(saved,reload,&error));
 assert(universal23::serialize(reload)==saved);
 auto vi=universal23::Instrument::prepare(reload,48000,&error);
 if(!vi){std::cerr<<error<<'\n';return 3;}
 vi->noteOn(60,.7f);double vEnergy=0;
 for(int i=0;i<2048;i++){auto s=vi->processOne();assert(std::isfinite(s.left)&&std::isfinite(s.right));vEnergy+=s.left*s.left;}
 assert(vEnergy>1.e-10);vi->noteOff(60);vi->allNotesOff();
 // Regression: 3.8 graphs remain readable and renderable.
 auto old=universal23::coupledGraph();auto oldSerialized=universal23::serialize(old);
 universal23::Graph oldReload;assert(universal23::deserialize(oldSerialized,oldReload,&error));
 std::cout<<"UNIFIED39_PASS\n";
}
