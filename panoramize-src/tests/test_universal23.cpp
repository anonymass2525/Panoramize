#include "UniversalInstrument23.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <iostream>
using namespace panorama::universal23;
static double play(Instrument& instrument,int midi=60){
 instrument.noteOn(midi,.8f);
 double energy=0;
 for(int n=0;n<2048;n++){
  const auto sample=instrument.processOne();
  assert(std::isfinite(sample.left)&&std::isfinite(sample.right));
  energy+=double(sample.left)*sample.left+double(sample.right)*sample.right;
 }
 instrument.noteOff(midi);
 return energy;
}
int main(){
 std::string error;
 auto g=defaultGraph();assert(g.nodes.size()==7);
 const auto text=serialize(g);Graph roundtrip;
 assert(deserialize(text,roundtrip,&error));assert(serialize(roundtrip)==text);
 auto all=Instrument::prepare(roundtrip,48000,&error);
 assert(all&&error.empty());assert(play(*all)>1.e-9);
 // Independently test that the new unified-state source produces audible MIDI
 // without any legacy synth node being present.
 Graph state;state.name="State Only";state.nodes={{Kind::UnifiedState,-1,-1,0,1},{Kind::Output,0,-1,0,1}};
 auto v=Instrument::prepare(state,48000,&error);assert(v);assert(play(*v,60)>1.e-9);
 // Every one of the 72 original presets is available as a graph generator.
 for(int preset=0;preset<panorama::kPresetCount;preset++){
  Graph legacy;legacy.nodes={{Kind::Legacy,-1,-1,preset,1},{Kind::Output,0,-1,0,.8f}};
  auto instrument=Instrument::prepare(legacy,44100,&error);
  assert(instrument);assert(play(*instrument)>1.e-9);
 }
 // A normal cycle is invalid, while the same cycle through an explicit
 // single-sample causal delay is legal and bounded.
 Graph invalid;invalid.nodes={{Kind::Gain,1,-1,0,.5f},{Kind::Gain,0,-1,0,.5f},{Kind::Output,1,-1,0,1}};
 assert(!Instrument::prepare(invalid,48000,&error));
 Graph delayed;delayed.nodes={{Kind::Legacy,-1,-1,0,.4f},{Kind::Mix,0,2,0,.2f},
 {Kind::Delay,1,-1,0,1.f},{Kind::Output,1,-1,0,.7f}};
 auto cyclic=Instrument::prepare(delayed,48000,&error);assert(cyclic);assert(play(*cyclic)>1.e-9);
 // Invalid graph must not overwrite the old editor/host patch.
 Graph unchanged=roundtrip;
 assert(!deserialize("PANORAMA-GRAPH-23|1\nNAME|x\nNODE|98|-1|-1|0|1\n",unchanged,&error));
 assert(serialize(unchanged)==text);
 Graph repeated=roundtrip;repeated.nodes[0].preset=200;
 assert(!Instrument::prepare(repeated,48000,&error));
 Graph disconnected;disconnected.nodes={{Kind::Legacy,-1,-1,0,1},{Kind::Gain,0,-1,0,.8f}};
 assert(!Instrument::prepare(disconnected,48000,&error));
 std::cout<<"PANORAMA Universal 2.3: graph editing, 72 legacy engines, state core, MIDI, save/load, causal feedback passed\n";
}
