#include "ArticulatedPhysical33.h"
#include "UniversalInstrument23.h"
#undef NDEBUG
#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
using namespace panorama;
using namespace panorama::articulated33;
static double rms(Instrument& s,int count){
 double energy=0.;for(int i=0;i<count;i++){auto x=s.processOne();assert(std::isfinite(x));energy+=x*x;}
 return std::sqrt(energy/count);
}
static double compare(Instrument&a,Instrument&b,int count){
 double diff=0.;for(int i=0;i<count;i++){const auto x=a.processOne(),y=b.processOne();assert(std::isfinite(x)&&std::isfinite(y));diff+=std::abs(x-y);}
 return diff/count;
}
int main(){
 constexpr int sr=48000;
 Model m;m.monoLegato=true;m.physical.bridge=true;m.physical.body=true;
 m.physical.string.decayBase=1.;m.physical.string.gain=.6;
 auto txt=serialize(m);Model read;
 assert(deserialize(txt,read)&&serialize(read)==txt);
 auto bad=m;bad.palmPosition=1.2;assert(!deserialize(serialize(bad),read));
 bad=m;bad.hammerImpact=-1;assert(!deserialize(serialize(bad),read));
 assert(!deserialize(txt+"BAD",read));
 auto hammer=Instrument::prepare(m,sr),legato=Instrument::prepare(m,sr);
 assert(hammer&&legato);
 hammer->noteOn(55,.8f);legato->noteOn(55,.8f);
 rms(*hammer,2500);rms(*legato,2500);
 hammer->hammerOn(60,.85f);legato->noteOn(60,.85f);
 assert(hammer->hammerCount()==1&&hammer->attackCount()==1&&hammer->legatoCount()==1);
 assert(legato->attackCount()==1);
 assert(compare(*hammer,*legato,5000)>1e-5);
 // Releasing at the fretted boundary produces an impulse of opposite polarity,
 // without retriggering the envelope or phase history.
 auto pull=Instrument::prepare(m,sr),plain=Instrument::prepare(m,sr);
 pull->noteOn(64,.75f);plain->noteOn(64,.75f);
 rms(*pull,2400);rms(*plain,2400);
 pull->pullOff(59,.9f);plain->noteOn(59,.75f);
 assert(pull->pullCount()==1&&pull->attackCount()==1);
 assert(compare(*pull,*plain,4500)>1e-5);
 // Slide produces contact friction that follows fret displacement. The pitch
 // transition must not produce an extra strike or reset resonating modes.
 auto slide=Instrument::prepare(m,sr),smooth=Instrument::prepare(m,sr);
 slide->noteOn(52,.75f);smooth->noteOn(52,.75f);
 rms(*slide,1400);rms(*smooth,1400);
 slide->slideTo(59,1.f);smooth->noteOn(59,.8f);
 assert(slide->slideCount()==1&&slide->attackCount()==1);
 const double slideDelta=compare(*slide,*smooth,11000);
 assert(slideDelta>1e-8);
 assert(slide->currentNotePitch()>58.);
 // Contact near the bridge introduces harmonic-dependent loss rather than
 // multiplying output amplitude; release restores natural decay afterwards.
 auto mute=Instrument::prepare(m,sr),free=Instrument::prepare(m,sr);
 mute->noteOn(57,.8f);free->noteOn(57,.8f);
 rms(*mute,2500);rms(*free,2500);
 mute->setPalmPosition(.90f);mute->palmMute(1.f);
 auto emute=rms(*mute,10000),efree=rms(*free,10000);
 assert(emute<efree*.80);
 mute->palmMute(0.f);
 assert(std::isfinite(rms(*mute,3000)));
 // v3.3 graph/preset survives exact roundtrip while v3.2 remains byte stable.
 universal23::Graph g=universal23::articulatedGraph();g.articulated33=m;
 auto graph=universal23::serialize(g);
 assert(graph.rfind("PANORAMIZE-GRAPH-33|1",0)==0);
 universal23::Graph parsed;std::string why;
 assert(universal23::deserialize(graph,parsed,&why));
 assert(universal23::serialize(parsed)==graph);
 auto host=universal23::Instrument::prepare(parsed,sr,&why);assert(host);
 host->noteOn(55,.9f);host->articulationControl(22,1.f);host->articulationControl(20,.3f);
 host->articulationControl(21,.75f);host->noteOn(60,.8f);
 double e=0.;for(int i=0;i<6000;i++){auto y=host->processOne();assert(std::isfinite(y.left));e+=y.left*y.left;}
 assert(e>1e-10);
 auto corrupted=graph;auto at=corrupted.find("ARTICULATED33_END");assert(at!=std::string::npos);
 corrupted.replace(at,17,"BROKEN33_END");assert(!universal23::deserialize(corrupted,parsed,&why));
 universal23::Graph previous=universal23::expressiveGraph();
 auto prior=universal23::serialize(previous);
 assert(prior.rfind("PANORAMIZE-GRAPH-32|1",0)==0);
 assert(universal23::deserialize(prior,parsed,&why)&&universal23::serialize(parsed)==prior);
 auto polyM=m;polyM.monoLegato=false;
 auto poly=Instrument::prepare(polyM,sr);for(int n=48;n<64;n++)poly->noteOn(n,.75f);
 assert(poly->activeVoiceCount()==16);
 poly->palmMute(.7f);assert(rms(*poly,4000)>0.);
 std::cout<<"ARTICULATED33_OK hammer/pull/slide delta="<<slideDelta<<" palm rms="<<emute<<" vs "<<efree<<"\n";
}
