#include "ExpressivePhysical32.h"
#include "UniversalInstrument23.h"
#undef NDEBUG
#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
using namespace panorama;
using namespace panorama::expressive32;
static double power(Instrument& ins,int n){double e=0.;for(int i=0;i<n;i++){const auto x=ins.processOne();assert(std::isfinite(x));e+=x*x;}return e/n;}
int main(){
 constexpr int sr=48000;
 Model model;
 model.physical.bridge=true;model.physical.body=true;
 model.physical.string.pickPosition=.23;
 model.physical.string.decayBase=.8;
 auto encoded=serialize(model);Model recovered{};
 assert(deserialize(encoded,recovered)&&serialize(recovered)==encoded);
 assert(!deserialize(encoded+"CORRUPT",recovered));
 assert(!deserialize("NO",recovered));
 auto broken=model;broken.pressureContactDamping=-2;
 assert(!deserialize(serialize(broken),recovered));
 std::string error;
 auto low=Instrument::prepare(model,sr,&error),high=Instrument::prepare(model,sr,&error);
 assert(low&&high);
 low->noteOn(60,.22f);high->noteOn(60,.95f);
 assert(low->lastPickPosition()>high->lastPickPosition());
 assert(power(*low,12000)>1e-12 && power(*high,12000)>1e-12);
 low->allNotesOff();low->noteOn(55,.8f);
 assert(low->attackCount()==2);
 power(*low,2400);const double before=low->currentNotePitch();
 low->noteOn(59,.7f);
 assert(low->legatoCount()==1&&low->attackCount()==2);
 assert(std::abs(low->currentNotePitch()-before)<.01); // no reset/retrigger
 power(*low,4800);
 assert(low->currentNotePitch()>58);
 low->noteOff(59);
 power(*low,4800);
 assert(low->currentNotePitch()<56); // return to previous held note
 low->noteOff(55);
 // Pressure changes damping without silencing the waveform or resetting phases.
 auto dry=Instrument::prepare(model,sr),pressed=Instrument::prepare(model,sr);
 dry->noteOn(57,.75f);pressed->noteOn(57,.75f);
 power(*dry,6000);power(*pressed,6000);
 pressed->channelPressure(1.f);pressed->polyPressure(57,1.f);
 const double eDry=power(*dry,12000),ePress=power(*pressed,12000);
 assert(eDry>ePress*1.4 && ePress>=0.);
 // Pitch wheel must alter ongoing phase rotation WITHOUT new attacks.
 auto flat=Instrument::prepare(model,sr),bent=Instrument::prepare(model,sr);
 flat->noteOn(60,.8f);bent->noteOn(60,.8f);
 power(*flat,2000);power(*bent,2000);bent->pitchBend(2.f);
 double difference=0;
 for(int i=0;i<6000;i++)difference+=std::abs(flat->processOne()-bent->processOne());
 assert(difference>1e-4 && bent->attackCount()==1);
 // Polyphony and sustain pedal in separately configured instrument.
 auto polyModel=model;polyModel.monoLegato=false;
 auto poly=Instrument::prepare(polyModel,sr);for(int n=48;n<64;n++)poly->noteOn(n,.67f);
 assert(poly->activeVoiceCount()==16);
 poly->sustain(true);for(int n=48;n<64;n++)poly->noteOff(n);
 assert(power(*poly,2200)>0.);
 poly->sustain(false);assert(std::isfinite(power(*poly,10000)));
 // The 3.2 source is playable in a serialized VST3 Universal Patchbay.
 universal23::Graph g;g.name="Expressive physical test";
 g.nodes={{universal23::Kind::PhysicalExpressive32,-1,-1,0,.85f},
          {universal23::Kind::Output,0,-1,0,.8f}};
 g.expressive32=model;
 const auto data=universal23::serialize(g);assert(data.rfind("PANORAMIZE-GRAPH-32|1",0)==0);
 universal23::Graph parsed;
 assert(universal23::deserialize(data,parsed,&error));
 assert(universal23::serialize(parsed)==data);
 auto host=universal23::Instrument::prepare(parsed,sr,&error);assert(host);
 host->noteOn(60,.83f);host->pitchBend(1.f);host->channelPressure(.2f);host->polyPressure(60,.8f);
 double energy=0;for(int i=0;i<6000;i++){const auto y=host->processOne();assert(std::isfinite(y.left));energy+=y.left*y.left;}
 assert(energy>1e-10);
 const auto original=data;auto damaged=data;
 const auto pos=damaged.find("EXPRESSIVE32_END");assert(pos!=std::string::npos);
 damaged.replace(pos,16,"EXPRESSIVE32_BAD");
 assert(!universal23::deserialize(damaged,parsed,&error));
 // Previous 3.0 and 3.1 project .pgraph files remain readable.
 universal23::Graph old;old.nodes={{universal23::Kind::PhysicalCoupled30,-1,-1,0,.7f},
                                        {universal23::Kind::Output,0,-1,0,.8f}};
 const auto oldData=universal23::serialize(old);
 assert(universal23::deserialize(oldData,parsed,&error));
 assert(universal23::serialize(parsed)==oldData);
 std::cout<<"EXPRESSIVE32_OK pressure energy "<<ePress<<" / "<<eDry<<" legatos="<<low->legatoCount()<<"\n";
}
