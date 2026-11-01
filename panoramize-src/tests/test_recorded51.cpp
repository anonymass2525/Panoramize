#include "RecordedValidation51.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace panorama::recorded51;
int main(){
 panorama::joint50::Take t;t.audio.resize(24000);t.midi=69;
 for(size_t i=0;i<t.audio.size();i++)t.audio[i]=float(.4*std::sin(2.*3.141592653589793*440.*i/24000.));
 auto a=inspect(t,24000);assert(!a.silent&&!a.clipped&&a.rms>.25&&a.periodicity>.95);
 assert(auditCSV({t},24000).find("periodicity")!=std::string::npos);
 // An exactly known string's f0 gives one combination T/mu, never T and mu separately.
 auto f=[](const std::vector<double>&p){return std::vector<double>{.5*std::sqrt(p[0]/p[1])};};
 auto r=logSensitivityRank({25.,.01},f);
 assert(r.rank==1&&r.parameters==2&&!r.locallyIdentifiable);
 auto g=[](const std::vector<double>&p){return std::vector<double>{p[0]+2*p[1],p[0]-p[1]};};
 auto good=logSensitivityRank({2.,3.},g);
 assert(good.rank==2&&good.locallyIdentifiable);
 auto bad=logSensitivityRank({0.,3.},g);assert(!bad.locallyIdentifiable);
 std::cout<<"Recorded51 audit + non-identifiability detection PASS: T/mu rank "<<r.rank<<"/"<<r.parameters<<"\n";
}
