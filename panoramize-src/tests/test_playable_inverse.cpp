#include "GraphInverse.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
using namespace panorama;
int main(){
 auto reference=factoryPresets()[0];
 reference.name="multi-pitch instrument";reference.engine=Engine::Modular;
 reference.attack=.01f;reference.decay=.35f;reference.sustain=.73f;reference.release=.5f;
 reference.gain=.62f;reference.brightness=.7f;reference.cutoff=.69f;reference.motion=.35f;
 reference.modRoutes={};reference.graph={};
 reference.graph.nodes[0]={NodeType::Harmonic,-1,-1,.82f,.5f};
 reference.graph.nodes[8]={NodeType::Output,0,-1,.88f,0.f};
 constexpr int N=4096;constexpr double sr=48000.;
 std::array<std::array<float,N>,5> clips{};
 const int notes[]={48,60,72,67,55};
 const float velocities[]={.4f,.8f,.67f,.57f,.92f};
 const int gates[]={1300,4096,2400,1600,900};
 std::array<InstrumentExample,5> e{};
 for(int i=0;i<5;i++){
  e[size_t(i)]={clips[size_t(i)].data(),N,sr,notes[i],velocities[i],gates[i]};
  assert(renderInstrumentExample(reference,e[size_t(i)],clips[size_t(i)].data()));
 }
 InstrumentInverseSettings settings{};settings.maxEvaluations=64;settings.maxProposals=3;
 const auto discovered=inferPlayableInstruments(e.data(),3,e.data()+3,2,settings);
 std::cout << "Playable inverse: evaluated=" << discovered.evaluated
   << " topologies=" << discovered.distinctTopologies << " proposals=" << discovered.proposals.size() << "\n";
 assert(discovered.success);
 assert(discovered.evaluated==64);
 assert(discovered.trainingExamples==3&&discovered.holdoutExamples==2);
 assert(discovered.proposals.size()>=1);
 const auto& winner=discovered.proposals.front();
 std::array<int,kGraphNodes> order{};
 assert(compileGraph(winner.patch.graph,order));
 assert(std::isfinite(winner.trainingError)&&std::isfinite(winner.holdoutError));
 assert(winner.trainingError<1.e-9); // known synth is present in warm starts
 assert(winner.holdoutError<1.e-9); // MIDI and articulation not seen in training
 assert(winner.patch.graph.nodes[0].type==NodeType::Harmonic);
 // Held-out references cannot influence the search: replace one with synthetic
 // noise while keeping the three TRAINING clips bitwise identical.
 auto corrupted=clips[3];
 for(int i=0;i<N;i++)corrupted[size_t(i)]=0.08f*std::sin(.5f*i);
 auto eAltered=e;
 eAltered[3].samples=corrupted.data();
 const auto alternate=inferPlayableInstruments(eAltered.data(),3,eAltered.data()+3,2,settings);
 assert(alternate.success&&alternate.proposals.size()==discovered.proposals.size());
 assert(std::abs(alternate.proposals[0].trainingError-winner.trainingError)<1.e-12);
 assert(alternate.proposals[0].topologySignature==winner.topologySignature);
 assert(alternate.proposals[0].holdoutError>winner.holdoutError+1.e-5);
 // Explicit invalid or insufficient input fails, rather than silently
 // treating one single note as evidence of a playable instrument.
 assert(!inferPlayableInstruments(e.data(),1,nullptr,0,settings).success);
 auto bad=e;bad[1].midiNote=140;
 assert(!inferPlayableInstruments(bad.data(),3,nullptr,0,settings).success);
 auto noVar=e;for(int i=1;i<3;i++){
  noVar[size_t(i)].midiNote=noVar[0].midiNote;
  noVar[size_t(i)].velocity=noVar[0].velocity;
  noVar[size_t(i)].noteOffFrame=noVar[0].noteOffFrame;
 }
 assert(!inferPlayableInstruments(noVar.data(),3,nullptr,0,settings).success);
 assert(!renderInstrumentExample(reference,e[0],clips[0].data(),5000));
 std::cout << "PANORAMA_PlayableInstrument PASSED: training=" << winner.trainingError
           << " heldout=" << winner.holdoutError << "\n";
}
