#include "JointPhysicalInverse31.h"
#include "UniversalInstrument23.h"
#undef NDEBUG
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
#include <limits>
using namespace panorama;
int main(){
 performance30::CoupledModel truth;
 truth.string.frequencyHz=196.;truth.bridge=true;truth.body=true;
 truth.bridgeHz=525.;truth.bodyHz=295.;
 const double picks[]={.16,.26,.37,.22},decays[]={2.1,3.8,7.4,2.6};
 const double gains[]={.43,.59,.35,.52},steps[]={0.,5.,-5.,7.};
 std::vector<std::vector<float>> wav;
 std::vector<joint31::Take> takes;
 for(int i=0;i<4;i++){
  auto m=truth;m.string.pickPosition=picks[i];m.string.decayBase=decays[i];m.string.gain=gains[i];
  const double fac=std::pow(2.,steps[i]/12.);
  wav.push_back(performance30::render(m,12000,1.3,.042,fac));
 }
 for(int i=0;i<4;i++)takes.push_back({wav[i].data(),int(wav[i].size()),12000,
                                      196.*std::pow(2.,steps[i]/12.),i!=3});
 performance30::CoupledModel prior;prior.string.frequencyHz=196.;
 prior.string.decayBase=3.2;prior.string.pickPosition=.27;
 auto r=joint31::infer(takes,prior,1100,4);
 assert(r.success&&r.trainingTakes==3&&r.holdoutTakes==1&&r.evaluatedModels<=1100);
 assert(!r.proposals.empty()&&r.proposals.size()<=4);
 for(const auto&p:r.proposals){
  assert(p.performances.size()==4);
  assert(std::isfinite(p.trainingLoss)&&std::isfinite(p.holdoutLoss));
  auto m=joint31::forTake(p,0);
  assert(m.string.pickPosition>=.08&&m.string.pickPosition<=.48);
 }
 const auto&best=r.proposals[0];
 std::cout<<"JOINT31 best="<<best.topology<<" initial="<<best.initialTrainingLoss
          <<" optimized="<<best.trainingLoss<<" hold="<<best.holdoutLoss<<" evaluations="<<r.evaluatedModels<<'\n';
 assert(best.trainingLoss < best.initialTrainingLoss);
 joint31::Candidate roundtrip;
 const auto disk=joint31::serialize(best);
 assert(joint31::deserialize(disk,roundtrip));
 assert(joint31::serialize(roundtrip)==disk);
 assert(!joint31::deserialize(disk+"CORRUPT",roundtrip));
 assert(!joint31::deserialize("bad data",roundtrip));
 assert(best.topology=="String+Bridge+Body");
 assert(std::abs(best.structure.bridgeHz-truth.bridgeHz)<65.);
 assert(std::abs(best.structure.bodyHz-truth.bodyHz)<65.);
 for(int j=0;j<3;j++)assert(std::abs(best.performances[size_t(j)].pickPosition-picks[j])<.055);
 // changing holdout samples cannot influence training candidates: identities and params constant.
 auto altered=wav[3];for(float&f:altered)f=0;
 takes[3].audio=altered.data();
 // Invalid holdout recordings reject rather than silently leaking into training.
 assert(!joint31::infer(takes,prior,90,4).success);
 assert(!joint31::infer({},prior).success);
 assert(!joint31::infer(takes,prior,0).success);
 panorama::universal23::Graph graph;
 graph.name="Joint31 playable";
 graph.nodes={{panorama::universal23::Kind::PhysicalCoupled30,-1,-1,0,.7f},
              {panorama::universal23::Kind::Output,0,-1,0,.7f}};
 graph.physical30=joint31::forTake(best,0);
 const auto text=panorama::universal23::serialize(graph);
 panorama::universal23::Graph restored;std::string error;
 assert(panorama::universal23::deserialize(text,restored,&error));
 auto synth=panorama::universal23::Instrument::prepare(restored,12000,&error);
 assert(synth);synth->noteOn(55,.8f);
 double total=0.;for(int n=0;n<5000;n++){const auto f=synth->processOne();assert(std::isfinite(f.left));total+=f.left*f.left;}
 assert(total>1.e-7);
 std::cout<<"JOINT31_TEST_PASS\n";
}
