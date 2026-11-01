#include "ContactPhysical34.h"
#include "UniversalInstrument23.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>
using namespace panorama;
using namespace panorama::contact34;
static double render(Instrument& x,int samples=24000){double s=0.;for(int i=0;i<samples;i++){const float v=x.processOne();assert(std::isfinite(v));s+=double(v)*v;}return s/samples;}
static bool same(const Model&a,const Model&b){return serialize(a)==serialize(b);}
int main(){
 Model m,round;assert(deserialize(serialize(m),round)&&same(m,round));
 assert(!deserialize("PANORAMIZE-CONTACT34-v1\nstring 1e300\n",round));
 std::string why;auto invalid=m;invalid.bridgeMass=-1;
 assert(!Instrument::prepare(invalid,48000,&why));
 auto make=[&](Model model){auto p=Instrument::prepare(model,48000,&why);assert(p);return p;};
 auto p=make(m);assert(p->stepsPerSample()>=3);assert(p->noteCount()==0);
 p->noteOn(m.openMidi,.8f);const double openEnergy=p->energy();assert(openEnergy>0);
 const double quiet=render(*p);assert(quiet>1.e-10);
 assert(std::isfinite(p->energy())&&p->energy()>=0);
 assert(p->accumulatedDissipation()>0);
 p->nextGesture(Instrument::Gesture::Hammer,.9f);p->noteOn(m.openMidi+7,.9f);
 const int count=p->noteCount();render(*p,12000);assert(p->noteCount()==count);
 assert(p->fingerPosition()>.01);
 p->nextGesture(Instrument::Gesture::Slide,.8f);p->noteOn(m.openMidi+12,.8f);
 const double before=p->fingerPosition();render(*p,9600);assert(p->fingerPosition()>before);
 auto open=make(m);open->noteOn(m.openMidi,1.f);const double e1=render(*open,8000);
 auto fretted=make(m);fretted->noteOn(m.openMidi+12,1.f);const double e2=render(*fretted,8000);
 assert(std::abs(e1-e2)>1.e-7);
 auto dry=make(m);dry->noteOn(m.openMidi,.75f);render(*dry,10000);const double dryRms=render(*dry,12000);
 auto muted=make(m);muted->noteOn(m.openMidi,.75f);render(*muted,10000);muted->palmMute(1.f);const double muteRms=render(*muted,12000);
 assert(muteRms < dryRms);
 auto bp=m;bp.bodyCoupling=0.;auto noBody=make(bp);auto yesBody=make(m);
 noBody->noteOn(m.openMidi,.8f);yesBody->noteOn(m.openMidi,.8f);
 double difference=0.;for(int i=0;i<10000;i++)difference+=std::abs(noBody->processOne()-yesBody->processOne());
 assert(difference>.0001);
 // Total energy of the lossless, unfretted, freely evolving coupled system must remain bounded.
 auto conservative=m;conservative.stringLoss=0.;conservative.bridgeLoss=0.;conservative.bodyLoss=0.;
 conservative.fingerDamping=0.;conservative.fretDamping=0.;conservative.palmLoss=0.;conservative.fretGap=.01;
 auto lossless=make(conservative);lossless->noteOn(m.openMidi,.7f);const double ea=lossless->energy();
 render(*lossless,80000);const double eb=lossless->energy();
 assert(ea>0 && eb>0 && std::abs(eb/ea-1.)<.02);
 // A Universal Patchbay file must survive serialization/restore, MIDI, and contain only new physical source.
 auto g=universal23::contactGraph();assert(g.nodes.size()==2);
 auto text=universal23::serialize(g);assert(text.find("PANORAMIZE-GRAPH-34")!=std::string::npos);
 universal23::Graph recovered;assert(universal23::deserialize(text,recovered,&why));
 assert(universal23::serialize(recovered)==text);
 auto univ=universal23::Instrument::prepare(recovered,48000,&why);assert(univ);
 univ->noteOn(m.openMidi,.9f);double rms=0;for(int i=0;i<24000;i++){auto v=univ->processOne();assert(std::isfinite(v.left)&&std::isfinite(v.right));rms+=v.left*v.left;}
 assert(rms>1.e-9);
 // Regression: prior v3.3 format is still importable; new format does not require old payload.
 auto old=universal23::serialize(universal23::articulatedGraph());assert(universal23::deserialize(old,recovered,&why));
 assert(old!=text);
 // invalid graph payload and unknown node type are rejected
 auto tampered=text;const auto k=tampered.find("string ");assert(k!=std::string::npos);tampered.replace(k,6,"invalid");
 assert(!universal23::deserialize(tampered,recovered,&why));
 std::cout<<"contact34 pass: open="<<e1<<" fretted="<<e2<<" dry="<<dryRms<<" palm="<<muteRms
          <<" bridge diff="<<difference<<" conservation="<<eb/ea<<" output="<<rms/24000<<" steps="<<p->stepsPerSample()<<"\n";
}
