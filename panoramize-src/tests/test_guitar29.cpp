#include "GuitarInverse29.h"
#include "UniversalInstrument23.h"
#undef NDEBUG
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
using namespace panorama::guitar29;
int main(){
 Model original;original.frequencyHz=196.;original.pickPosition=.27;original.pickupPosition=.78;
 original.decayBase=2.3;original.decayHigh=.028;original.inharmonicity=.0001;original.spectralTilt=.035;original.gain=.45;
 auto a=render(original,16000,1.8,.04,1.);
 auto b=render(original,16000,1.8,.05,std::pow(2.,5./12.));
 auto c=render(original,16000,1.8,.07,std::pow(2.,-5./12.));
 assert(!a.empty()&&a.size()==28800);
 std::vector<Take> takes={{a.data(),int(a.size()),16000,196.,true},{b.data(),int(b.size()),16000,196.*std::pow(2.,5./12.),true},
  {c.data(),int(c.size()),16000,196.*std::pow(2.,-5./12.),false}};
 auto output=infer(takes);assert(output.success&&output.takes.size()==3);
 assert(output.model.pickPosition>=.08&&output.model.pickPosition<=.48);
 assert(output.model.decayBase>=.1&&output.model.decayBase<30);
 assert(std::isfinite(output.holdoutLoss)&&output.holdoutLoss>=0.);
 assert(std::isfinite(output.trainingLoss));
 auto single=analyze({a.data(),int(a.size()),16000,0.,true});assert(single.success);
 assert(std::abs(single.frequencyHz-original.frequencyHz)<5.);
 Model restored;const auto text=serialize(output.model);assert(deserialize(text,restored));assert(text==serialize(restored));
 auto modified=output.model;modified.pickPosition=.09;
 assert(serialize(modified)!=text);
 assert(!deserialize(text+"BAD|foo\n",restored));
 assert(!deserialize("garbage",restored));
 std::vector<float> silent(a.size());assert(!analyze({silent.data(),int(silent.size()),16000,196.,true}).success);
 assert(!infer({{a.data(),int(a.size()),16000,196.,false}}).success);
 panorama::universal23::Graph graph;graph.name="Guitar Source";
 graph.nodes={{panorama::universal23::Kind::GuitarString29,-1,-1,0,.9f},{panorama::universal23::Kind::Output,0,-1,0,.85f}};
 graph.guitar=output.model;
 std::string err;
 auto serialized=panorama::universal23::serialize(graph);
 panorama::universal23::Graph parsed;
 assert(panorama::universal23::deserialize(serialized,parsed,&err));
 assert(serialized==panorama::universal23::serialize(parsed));
 auto playable=panorama::universal23::Instrument::prepare(parsed,16000,&err);assert(playable);
 playable->noteOn(60,.85f);double energy=0.;
 for(int i=0;i<10000;i++){auto x=playable->processOne();assert(std::isfinite(x.left));energy+=double(x.left)*x.left;}
 assert(energy>1.e-6);playable->noteOff(60);playable->allNotesOff();
 auto invalid=serialized;auto pos=invalid.find("GUITAR29_BEGIN");assert(pos!=std::string::npos);
 invalid.replace(pos,14,"GUITAR29_BROKEN");
 assert(!panorama::universal23::deserialize(invalid,parsed,&err));
 auto good=render(output.model,16000,1.8);assert(good.size()==a.size());
 double rms=0.;for(float x:good){assert(std::isfinite(x));rms+=x*x;}assert(rms>1.e-8);
 std::cout<<"GUITAR_29_OK f0="<<single.frequencyHz<<" pick="<<output.model.pickPosition
          <<" d0="<<output.model.decayBase<<" d2="<<output.model.decayHigh
          <<" train="<<output.trainingLoss<<" hold="<<output.holdoutLoss<<"\n";
}
