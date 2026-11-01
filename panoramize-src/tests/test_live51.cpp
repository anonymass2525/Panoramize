#include "UniversalInstrument23.h"
#include "NativeState27.h"
#include "UnifiedControls50.h"
#include "UnifiedAcousticPhysical39.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
using namespace panorama;
int main(){
 const int sr=24000;
 auto a=universal23::Instrument::prepare(universal23::directGraph(),sr);
 auto b=universal23::Instrument::prepare(universal23::directGraph(),sr);
 assert(a&&b);
 a->noteOn(57,.8f);b->noteOn(57,.8f);
 double identical=0,modified=0;
 for(int i=0;i<2048;i++){
  auto x=a->processOne(),y=b->processOne();identical+=std::abs(x.left-y.left);
 }
 assert(identical<1e-9); // unmodified native sound remains byte identical
 assert(a->setHostParameterNormalized(80,0.05)); // real state pickup output, not a proxy
 assert(a->setHostParameterNormalized(88,0.10)); // real VA source effort
 assert(!a->setHostParameterNormalized(90,0.9)); // spectral oscillator has no active effort
 assert(!a->setHostParameterNormalized(80,-1));
 assert(!a->setHostParameterNormalized(96,0.5));
 for(int i=0;i<3000;i++){
  auto x=a->processOne(),y=b->processOne();
  assert(std::isfinite(x.left)&&std::isfinite(y.left));
  modified+=std::abs(x.left-y.left);
 }
 assert(modified>1e-5);
 a->clearNativeModeAutomation();
 // actual algorithm's source settings return to original, no graph rebuild
 for(int i=0;i<4000;i++)assert(std::isfinite(a->processOne().left));
 auto persist=universal23::directGraph();
 std::string error;
 assert(controls50::change(persist,"native51.mode.0.pickupScale",.15,&error));
 assert(controls50::change(persist,"native51.mode.0.excitationScale",.25,&error));
 assert(universal23::serialize(persist).find("PANORAMIZE-GRAPH-51|1")==0);
 universal23::Graph recovered;
 assert(universal23::deserialize(universal23::serialize(persist),recovered,&error));
 assert(recovered.nativePickup51[0]>.14f&&recovered.nativePickup51[0]<.16f);
 auto live=universal23::Instrument::prepare(recovered,sr,&error);
 auto baseline=universal23::Instrument::prepare(universal23::directGraph(),sr,&error);
 assert(live&&baseline);
 live->noteOn(57,.8f);baseline->noteOn(57,.8f);
 double savedDelta=0;
 for(int i=0;i<3000;i++)savedDelta+=std::abs(live->processOne().left-baseline->processOne().left);
 assert(savedDelta>1e-5);
 // Verifies that the new patch is editable, serialized and actually changes DSP sound.
 auto tampered=universal23::serialize(persist);
 tampered+="NATIVE_LIVE|0|0|.3\n";
 assert(!universal23::deserialize(tampered,recovered,&error));
 auto g=universal23::genuineNativeGraph();
 auto mixed=universal23::Instrument::prepare(g,sr);
 assert(mixed);
 assert(mixed->setHostParameterNormalized(80,.2));
 assert(mixed->setHostParameterNormalized(88,.2));
 mixed->noteOn(57,.8f);
 for(int i=0;i<3000;i++)assert(std::isfinite(mixed->processOne().left));
 std::cout<<"Live51 genuine Native27 internal modulation: bypass matched, controls active, no invalid values PASS; diff="<<modified<<"\n";
}
