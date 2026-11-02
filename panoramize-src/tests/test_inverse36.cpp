#include "MeshInverse36.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace panorama::inverse36;
int main(){
 panorama::fem36::Model truth;
 truth.pluckPosition=.72; truth.bridgeCoupling=.1;
 truth.meshYoung=5e8; truth.anisotropy[0]=1.2;
 std::vector<Take> takes;
 for(int i=0;i<3;i++){
  Take t;t.midi=40+i*5;t.velocity=.7f+.05f*i;t.training=i<2;
  t.audio=render(truth,22050,t.midi,t.velocity,640);
  assert(t.audio.size()==640);takes.push_back(std::move(t));
 }
 panorama::fem36::Model seed=truth;seed.pluckPosition=.58;
 seed.bridgeCoupling=.075;seed.meshYoung=4e8;
 auto another=seed;another.mesh.vertices[1][0]*=.85;
 auto result=infer(takes,{seed,another},22050,38,2);
 assert(result.error.empty());assert(!result.proposals.empty());
 for(auto&p:result.proposals){
  assert(std::isfinite(p.trainingLoss));assert(std::isfinite(p.validationLoss));
  assert(p.trainingLoss>=0);assert(p.validationLoss>=0);
 }
 assert(result.evaluations<=38);
 auto bad=takes;bad[0].audio[0]=std::nanf("");
 assert(!infer(bad,{seed},22050,6,1).error.empty());
 // Validation holdout must not change fitted parameters or order.
 auto changed=takes;for(auto&x:changed.back().audio)x*=.2f;
 auto b=infer(changed,{seed,another},22050,38,2);
 assert(b.error.empty());
 assert(result.proposals.size()==b.proposals.size());
 for(size_t j=0;j<result.proposals.size();j++){
  assert(result.proposals[j].meshCandidate==b.proposals[j].meshCandidate);
  assert(result.proposals[j].trainingLoss==b.proposals[j].trainingLoss);
  assert(panorama::fem36::serialize(result.proposals[j].model)==panorama::fem36::serialize(b.proposals[j].model));
 }
 std::cout<<"Inverse36 PASS top training="<<result.proposals[0].trainingLoss<<" validation="<<result.proposals[0].validationLoss<<" evals="<<result.evaluations<<"\n";
}
