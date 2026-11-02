#include "InverseDecision54.h"
#include "AudioDesign49.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
using namespace panorama;
int main(){
 joint50::Result r;
 r.ranked.push_back({"hybrid",design49::editableGraph(design49::Family::NativePhysicalHybrid,220.),.01,.08,3});
 r.ranked.push_back({"physical_string",design49::editableGraph(design49::Family::PhysicalString,220.),.012,.015,3});
 auto x=decision54::review(r);
 assert(x.bestTrain==0&&x.bestHoldout==1&&x.rankingDisagrees&&x.lowSeparation==false);
 assert(x.candidates.size()==2&&x.candidates[0].overfitRisk);
 const auto probes=decision54::suggestedMeasurements();
 assert(probes.size()>=6);
 int fixedString=0,fixedTube=0;
 for(const auto&p:probes){
  assert(p.parameterCount==2&&p.rankBefore==1);
  if(p.family=="string_T_mu"&&p.independentlyDistinguishes)fixedString++;
  if(p.family=="tube_c_length"&&p.independentlyDistinguishes)fixedTube++;
 }
 assert(fixedString==2&&fixedTube==2);
 assert(decision54::measurementsCSV(probes).find("linear density")!=std::string::npos);
 assert(decision54::candidatesCSV(x).find("physical_string")!=std::string::npos);
 assert(decision54::reportText(x).find("Ranking disagrees=1")!=std::string::npos);
 std::cout<<"Decision54 PASS: holdout disagreement, uncalibrated margins and rank-improving physical measurements\n";
}
