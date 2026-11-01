#include "ShapeInverse37.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
using namespace panorama;
int main(){
 gen37::Shape truth;truth.xCells=2;truth.yCells=1;truth.zCells=1;
 truth.camber=.12;truth.taper=-.1;
 fem36::Model source;assert(gen37::buildTetraMesh(truth,source.mesh));
 source.pluckPosition=.72;source.bridgeCoupling=.085;source.meshYoung=5.0e8;
 std::vector<inverse36::Take> takes;
 for(int i=0;i<3;i++){
  inverse36::Take take;take.midi=40+5*i;take.velocity=.6f+i*.1f;take.training=i<2;
  take.audio=gen37::renderTetra(source,22050,take.midi,take.velocity,384);
  assert(take.audio.size()==384);takes.push_back(std::move(take));
 }
 auto r=gen37::discover(takes,22050,40,3);
 assert(r.error.empty());assert(!r.proposals.empty());assert(r.generatedTopologies>2);assert(r.evaluations<=40);
 for(auto&v:r.proposals){assert(std::isfinite(v.trainingLoss));assert(std::isfinite(v.validationLoss));
  fem36::Model loaded;assert(fem36::deserialize(fem36::serialize(v.model),loaded));}
 auto changed=takes;for(auto&x:changed[2].audio)x*=.25f;
 auto q=gen37::discover(changed,22050,40,3);
 assert(q.error.empty());assert(r.proposals.size()==q.proposals.size());
 for(size_t i=0;i<r.proposals.size();i++){
  assert(r.proposals[i].candidateIndex==q.proposals[i].candidateIndex);
  assert(r.proposals[i].trainingLoss==q.proposals[i].trainingLoss);
  assert(fem36::serialize(r.proposals[i].model)==fem36::serialize(q.proposals[i].model));
 }
 auto bad=takes;bad[0].audio[0]=std::numeric_limits<float>::quiet_NaN();
 assert(!gen37::discover(bad,22050,20,1).error.empty());
 std::cout<<"SHAPE37 PASS gen="<<r.generatedTopologies<<" evals="<<r.evaluations
          <<" best="<<r.proposals[0].trainingLoss<<" holdout="<<r.proposals[0].validationLoss<<"\n";
}
