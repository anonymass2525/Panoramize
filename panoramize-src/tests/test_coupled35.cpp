#include "CoupledGuitar35.h"
#include "UniversalInstrument23.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
using panorama::coupled35::Model;
using panorama::coupled35::Instrument;
int main(){
 Model m;std::string error;Model copy;
 assert(panorama::coupled35::deserialize(panorama::coupled35::serialize(m),copy));
 assert(panorama::coupled35::serialize(copy)==panorama::coupled35::serialize(m));
 Model bad=m;bad.meshPoisson=.6;assert(!panorama::coupled35::deserialize(panorama::coupled35::serialize(bad),copy));
 auto a=Instrument::prepare(m,48000,&error);
 if(!a){std::cerr<<error<<'\n';return 1;}
 assert(a->stepsPerSample()>=3);
 a->noteOn(40,.85f);assert(a->activeStrings()==1);
 double sq=0,peak=0;
 for(int i=0;i<9600;i++){
  const double v=a->processOne();assert(std::isfinite(v));sq+=v*v;peak=std::max(peak,std::abs(v));
  assert(std::isfinite(a->mechanicalEnergy()));
 }
 assert(sq>1.e-10);assert(peak<1.01);
 std::cout<<"fret contact calls="<<a->contactCount()<<" axial kinetic="<<a->axialKineticEnergy()<<" torsion kinetic="<<a->torsionalKineticEnergy()<<"\n";
 const double bodyE=a->meshEnergy();
 std::cout<<"single string squared "<<sq<<" mesh energy "<<bodyE<<" steps "<<a->stepsPerSample()<<'\n';
 assert(a->contactCount()>0);
 assert(a->axialKineticEnergy()>1.e-20);
 assert(a->torsionalKineticEnergy()>1.e-20);
 assert(bodyE>1.e-15);  // reciprocal transfer into tetrahedral mesh
 double other=0;
 for(int i=1;i<6;i++)other+=a->lastStringEnergy(i);
 assert(other>1.e-15); // shared body transfers vibration to unplucked strings
 auto separate=m;separate.bridgeCoupling=0;
 auto b=Instrument::prepare(separate,48000);assert(b);b->noteOn(40,.85f);
 for(int i=0;i<9600;i++)b->processOne();
 assert(b->meshEnergy()<bodyE*1.e-8);
 // Six independent notes share one soundboard and 3 axis FEM state.
 a->allNotesOff();
 for(int note: {40,45,50,55,59,64})a->noteOn(note,.9f);
 assert(a->activeStrings()==6);
 for(int i=0;i<4800;i++){const double v=a->processOne();assert(std::isfinite(v));}
 assert(a->meshEnergy()>1.e-15);
 a->channelPressure(.7f);a->palmMute(.8f);a->pitchBend(1.);
 a->nextGesture(Instrument::Gesture::Slide,.7f);a->noteOn(47,.7f);
 for(int i=0;i<4800;i++)assert(std::isfinite(a->processOne()));
 for(int note: {40,45,50,55,59,64,47})a->noteOff(note);
 assert(a->activeStrings()<6);
 a->allNotesOff();assert(a->activeStrings()==0);assert(a->mechanicalEnergy()<1.e-10);
 // Conservative six-string plus tetra FEM integration must preserve energy
 // with ALL dissipation disabled, all open-string residual motion unmuted.
 auto conservative=m;
 conservative.stringLoss=0;conservative.meshLoss=0;conservative.fingerDamping=0;
 conservative.palmDamping=0;conservative.fretFriction=0;conservative.fretGap=.01;
 auto c=Instrument::prepare(conservative,48000);assert(c);
 c->sustain(true);c->noteOn(40,.75f);
 const double initial=c->mechanicalEnergy();assert(initial>1.e-10);
 for(int i=0;i<48000;i++)c->processOne();
 const double energyRatio=c->mechanicalEnergy()/initial;
 assert(energyRatio>.995&&energyRatio<1.005);
 std::cout<<"conservative coupled FEM energy ratio after 1s="<<energyRatio<<'\n';
 // The new source must pass through patch serialization and graph real-time renderer.
 auto g=panorama::universal23::coupledGraph();
 std::string encoded=panorama::universal23::serialize(g);panorama::universal23::Graph parsed;
 assert(panorama::universal23::deserialize(encoded,parsed,&error));
 assert(panorama::universal23::serialize(parsed)==encoded);
 auto graph=panorama::universal23::Instrument::prepare(parsed,48000,&error);
 if(!graph){std::cerr<<error<<'\n';return 2;}
 graph->noteOn(40,.8f);graph->noteOn(47,.7f);double total=0;
 for(int i=0;i<2400;i++){auto v=graph->processOne();assert(std::isfinite(v.left)&&std::isfinite(v.right));total+=std::abs(v.left);}
 assert(total>1.e-9);
 std::cout<<"coupled35 tests PASSED, graph sum="<<total<<'\n';
}
