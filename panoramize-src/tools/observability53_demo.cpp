#include "ExperimentalObservability53.h"
#include <cmath>
#include <fstream>
#include <iostream>
using namespace panorama::observability53;
int main(int argc,char**argv){
 if(argc!=2){std::cerr<<"Usage: panoramize_observability53_demo OUTPUT_CSV\n";return 2;}
 const std::vector<double> theta={90.,.007};
 const Measurement frequency{"recorded_note_frequency",{.5},[](const auto&x){return std::vector<double>{std::sqrt(x[0]/x[1])/(2*.65)};}};
 const Measurement secondPitch{"another_note_frequency",{.5},[](const auto&x){return std::vector<double>{std::sqrt(x[0]/x[1])/.65};}};
 const Measurement density{"measured_linear_density",{.0002},[](const auto&x){return std::vector<double>{x[1]};}};
 const Measurement tension{"measured_string_tension",{2.},[](const auto&x){return std::vector<double>{x[0]};}};
 auto baseline=evaluate(theta,{frequency});auto next=selectNext(theta,{frequency},{secondPitch,density,tension});
 std::ofstream o(argv[1]);if(!o)return 3;
 o<<"condition,rank,parameters,locally_identifiable,warning\n";
 o<<"one_recorded_pitch,"<<baseline.rank<<','<<baseline.parameterCount<<','<<baseline.locallyIdentifiable<<','<<'"'<<baseline.warning<<"\"\n";
 o<<"with_selected_measurement,"<<next.after.rank<<','<<next.after.parameterCount<<','<<next.after.locallyIdentifiable<<','<<'"'<<next.rationale<<"\"\n";
 std::cout<<"Initial rank "<<baseline.rank<<"/"<<baseline.parameterCount<<"; selected probe "<<next.selected<<"; after rank "<<next.after.rank<<"/"<<next.after.parameterCount<<"\n";
 return next.after.locallyIdentifiable?0:4;
}
