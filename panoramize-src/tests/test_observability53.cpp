#include "ExperimentalObservability53.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
using namespace panorama::observability53;
int main(){
 // A single string frequency measures T/mu only; T and mu remain ambiguous.
 const std::vector<double> theta={100.,.005};
 const Measurement f{"fundamental",{.3},[](const std::vector<double>&x){return std::vector<double>{std::sqrt(x[0]/x[1])/(2*.65)};}};
 const Measurement octave{"another octave",{.3},[](const std::vector<double>&x){return std::vector<double>{std::sqrt(x[0]/x[1])/.65};}};
 const Measurement mass{"measured linear density",{.0001},[](const std::vector<double>&x){return std::vector<double>{x[1]};}};
 const auto initially=evaluate(theta,{f});assert(initially.rank==1&&!initially.locallyIdentifiable);
 const auto twoNotes=evaluate(theta,{f,octave});assert(twoNotes.rank==1);
 const auto planned=selectNext(theta,{f},{octave,mass});
 assert(planned.selected==1&&planned.after.rank==2&&planned.after.locallyIdentifiable);
 assert(std::isfinite(planned.after.condition));
 // A simple tube resonance alone measures c/L, not length and sound speed separately.
 const Measurement resonance{"tract resonance",{1.},[](const std::vector<double>&x){return std::vector<double>{x[0]/(4*x[1])};}};
 const Measurement speed{"measured propagation speed",{1.},[](const std::vector<double>&x){return std::vector<double>{x[0]};}};
 assert(evaluate({340.,.17},{resonance}).rank==1);
 assert(evaluate({340.,.17},{resonance,speed}).rank==2);
 const auto invalid=evaluate({100.,-1.},{f});assert(!invalid.locallyIdentifiable&&!invalid.warning.empty());
 const auto nan=evaluate({100.,std::numeric_limits<double>::quiet_NaN()},{f});assert(!nan.locallyIdentifiable);
 std::cout<<"Observability53 PASS: ambiguous string/tube detected; next measurement closes local rank; condition="<<planned.after.condition<<"\n";
}
