#include "UniversalInstrument23.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <iostream>
using namespace panorama;
using namespace panorama::universal23;
static double play(Instrument& ins, int note=60){
 ins.noteOn(note,.9f);
 double sum=0.;
 for(int i=0;i<512;i++){
  const auto v=ins.processOne();assert(std::isfinite(v.left)&&std::isfinite(v.right));
  sum+=double(v.left)*v.left+double(v.right)*v.right;
 }
 return sum;
}
int main(){
 std::string err;
 Graph graph;graph.name="Panoramaize state port";
 graph.nodes={
  {Kind::Legacy,-1,-1,0,0.f}, // MUTE PCM, not the internal oscillator
  {Kind::StateDrive,0,2,0,.65f}, // phase -> calibrated active effort
  {Kind::UnifiedState,-1,-1,0,1.f},
  {Kind::Output,2,-1,0,.9f}
 };
 const auto txt=serialize(graph);
 assert(txt.find("PANORAMIZE-GRAPH-25|1")==0);
 Graph round;assert(deserialize(txt,round,&err));
 auto with=Instrument::prepare(round,48000,&err);
 assert(with&&err.empty());
 const double changed=play(*with);
 assert(changed>1.e-12);
 assert(std::isfinite(with->activeStateWork()));
 assert(std::abs(with->activeStateWork())>1.e-12);
 Graph control=graph;control.nodes[1].level=0.f;
 auto without=Instrument::prepare(control,48000,&err);assert(without);
 const double reference=play(*without);
 assert(reference>1.e-12);
 assert(std::abs(reference-changed)>1.e-9); // state modulation truly modifies sound
 assert(std::abs(without->activeStateWork())<1.e-12);
 Graph wrong=graph;wrong.nodes[1].b=0;assert(!Instrument::prepare(wrong,48000,&err));
 wrong=graph;wrong.nodes[1].preset=9;assert(!Instrument::prepare(wrong,48000,&err));
 wrong=graph;wrong.nodes[1].a=2;assert(!Instrument::prepare(wrong,48000,&err));
 const char* legacy="PANORAMA-GRAPH-23|1\nNAME|Legacy\nNODE|0|-1|-1|0|1\nNODE|6|0|-1|0|1\n";
 Graph migrated;assert(deserialize(legacy,migrated,&err));
 assert(serialize(migrated).find("PANORAMIZE-GRAPH-25|1")==0);
 // Internal modular spectrum is readable without routing through PCM:
 Synth synth;synth.prepare(48000);
 synth.setPatch(factoryPresets()[24]);synth.noteOn(60,.8f);
 for(int i=0;i<80;i++)synth.processOne();
 auto snapshot=synth.acousticStateForNote(60);
 assert(snapshot.domains!=0u);
 assert((snapshot.domains&AcousticStateFrame::Spectral)!=0u);
 assert((snapshot.domains&AcousticStateFrame::Phase)!=0u);
 // The same node can drive a typed spectral port, not just phase.
 Graph spectral=graph;spectral.nodes[0].preset=24;spectral.nodes[1].preset=1;
 auto spectralSynth=Instrument::prepare(spectral,48000,&err);assert(spectralSynth);
 assert(play(*spectralSynth)>1.e-9);
 assert(std::isfinite(spectralSynth->activeStateWork()));
 // A physical modular instrument exposes the explicitly tagged mechanics domain.
 Synth mechanical;mechanical.prepare(48000);mechanical.setPatch(factoryPresets()[60]);mechanical.noteOn(60,.8f);
 for(int i=0;i<100;i++)mechanical.processOne();
 const auto m=mechanical.acousticStateForNote(60);
 assert((m.domains&AcousticStateFrame::Mechanical)!=0u);
 assert(std::isfinite(m.displacement)&&std::isfinite(m.velocity));
 std::cout << "Panoramaize state bridge: energy=" << changed
           << " no_bridge=" << reference
           << " active_work=" << with->activeStateWork() << " domains="<<snapshot.domains<<"\n";
}
