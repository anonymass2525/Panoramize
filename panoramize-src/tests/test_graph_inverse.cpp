#include "GraphInverse.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
using namespace panorama;
int main(){
 // A known, actual DSP graph is inferred into a real valid patch, not an ID.
 auto reference=factoryPresets()[0];
 reference.name="reference";reference.engine=Engine::Modular;
 reference.graph={};reference.modRoutes={};
 reference.attack=.01f;reference.decay=.35f;reference.sustain=.73f;
 reference.release=.5f;reference.gain=.62f;reference.brightness=.7f;
 reference.cutoff=.69f;reference.motion=.35f;
 reference.graph.nodes[0]={NodeType::Harmonic,-1,-1,.82f,.5f};
 reference.graph.nodes[8]={NodeType::Output,0,-1,.88f,0.f};
 std::array<int,kGraphNodes> topo{};
 assert(compileGraph(reference.graph,topo));
 Synth synth;synth.prepare(48000);synth.setPatch(reference);
 std::vector<float> a(4096),b(4096);
 synth.noteOn(60,.8f);synth.render(a.data(),b.data(),(int)a.size());
 const auto result=inferGraphFromAudio(a.data(),int(a.size()),48000.,60);
 std::cout<<"candidates="<<result.evaluated<<" distance="<<result.distance
          <<" inferred="<<int(result.patch.graph.nodes[0].type)<<"\n";
 assert(result.success&&result.evaluated==180);
 assert(result.mutatedEvaluated>=50);
 assert(result.novelTopologiesEvaluated>9);
 assert(compileGraph(result.patch.graph,topo));
 assert(result.distance<.002);
 assert(result.patch.graph.nodes[0].type==NodeType::Harmonic); // spectral feature match, phase-insensitive
 assert(result.patch.engine==Engine::Modular);
 // Same seed and budget must give repeatable recipes and distances.
 const auto repeated=inferGraphFromAudio(a.data(),int(a.size()),48000.,60);
 assert(repeated.success&&std::abs(repeated.distance-result.distance)<1e-12);
 assert(repeated.evaluated==result.evaluated);
 // A 54-evaluation warm-start-only budget cannot beat the open search.
 GraphInverseSettings baseline{};baseline.maxEvaluations=39;
 const auto oldFamily=inferGraphFromAudioEx(a.data(),int(a.size()),48000.,60,baseline);
 assert(oldFamily.success&&result.distance<=oldFamily.distance+1e-12);
 const std::string json=graphInverseJson(result);
 assert(json.find("\"nodes\"")!=std::string::npos);
 assert(json.find("\"engine\": 4")!=std::string::npos);
 assert(json.find("\"midiNote\": 60")!=std::string::npos);
 assert(json.find("\"distinctTopologiesEvaluated\"")!=std::string::npos);
 assert(json.find("\"topologyMutationsEvaluated\"")!=std::string::npos);
 // An actual multi-instrument portfolio. The first result is exactly the
 // closest single-graph answer, others have different audible graph structure.
 GraphInverseSettings portfolioSettings{};
 portfolioSettings.maxEvaluations=180;
 const auto portfolio=inferGraphPortfolioFromAudioEx(
     a.data(),int(a.size()),48000.,60,portfolioSettings,4,5.0);
 assert(portfolio.best.success);
 assert(portfolio.proposals.size()>=2);
 assert(portfolio.proposals.size()<=4);
 assert(std::abs(portfolio.proposals[0].distance-portfolio.best.distance)<1.e-12);
 for(size_t i=0;i<portfolio.proposals.size();i++){
  const auto&proposal=portfolio.proposals[i];
  assert(compileGraph(proposal.patch.graph,topo));
  assert(proposal.activeNodes>=1);
  for(size_t j=0;j<i;j++)assert(proposal.topologySignature!=portfolio.proposals[j].topologySignature);
  if(i>0)assert(proposal.structuralNovelty>0.);
 }
 const auto portfolioAgain=inferGraphPortfolioFromAudioEx(
     a.data(),int(a.size()),48000.,60,portfolioSettings,4,5.0);
 assert(portfolio.proposals.size()==portfolioAgain.proposals.size());
 for(size_t i=0;i<portfolio.proposals.size();i++){
  assert(portfolio.proposals[i].topologySignature==portfolioAgain.proposals[i].topologySignature);
  assert(std::abs(portfolio.proposals[i].distance-portfolioAgain.proposals[i].distance)<1e-12);
 }
 std::vector<float> silence(4096,0.f);
 assert(!inferGraphFromAudio(silence.data(),4096,48000.,60).success);
 // Invalid limits fail safely; also test a deliberately different, valid
 // topology outside the old fixed list (VA -> Fold, unsupported in v1.7).
 reference.graph={};reference.graph.nodes[0]={NodeType::VA,-1,-1,.82f,.5f};
 reference.graph.nodes[1]={NodeType::Fold,0,-1,.83f,.6f};
 reference.graph.nodes[8]={NodeType::Output,1,-1,.88f,0.f};
 assert(compileGraph(reference.graph,topo));
 Synth novel;novel.prepare(48000);novel.setPatch(reference);novel.noteOn(60,.8f);
 novel.render(a.data(),b.data(),4096);
 const auto broader=inferGraphFromAudio(a.data(),4096,48000.,60);
 assert(broader.success&&broader.evaluated==180&&broader.mutatedEvaluated>=50);
 assert(compileGraph(broader.patch.graph,topo));
 GraphInverseSettings invalid{};invalid.maxEvaluations=100000;
 assert(!inferGraphFromAudioEx(a.data(),4096,48000.,60,invalid).success);
 assert(!inferGraphPortfolioFromAudioEx(a.data(),4096,48000.,60,
              GraphInverseSettings{},0).best.success);
 std::cout<<"PANORAMA_GraphInverse PASSED: "<<result.evaluated
          <<" candidates, "<<result.novelTopologiesEvaluated<<" topologies; non-template input "
          <<broader.distance<<"\n";
}
