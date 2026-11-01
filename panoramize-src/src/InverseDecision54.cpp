#include "InverseDecision54.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
namespace panorama::decision54 {
Summary review(const joint50::Result&r){
 Summary s;
 if(r.ranked.empty()){s.warning="No valid candidate results; inversion not performed";return s;}
 const double tiny=1e-12;
 double bestTr=std::numeric_limits<double>::infinity(),bestHo=bestTr;
 for(size_t i=0;i<r.ranked.size();++i){
  const auto&c=r.ranked[i];
  if(!std::isfinite(c.trainError)||!std::isfinite(c.holdoutError)||c.trainError<0||c.holdoutError<0){s.warning="Invalid or incomplete candidate scores";s.candidates.clear();return s;}
  if(c.trainError<bestTr){bestTr=c.trainError;s.bestTrain=int(i);}
  if(c.holdoutError<bestHo){bestHo=c.holdoutError;s.bestHoldout=int(i);}
 }
 for(const auto&c:r.ranked){
  const double excess=c.holdoutError-c.trainError;
  s.candidates.push_back({c.family,c.trainError,c.holdoutError,c.trainError-bestTr,c.holdoutError-bestHo,excess>std::max(.025,2*c.trainError)});
 }
 double second=std::numeric_limits<double>::infinity();
 for(size_t i=0;i<r.ranked.size();++i)if(int(i)!=s.bestTrain)second=std::min(second,r.ranked[i].trainError);
 s.normalizedTrainingGap=std::isfinite(second)?(second-bestTr)/(std::max(second,tiny)):0;
 s.lowSeparation=s.normalizedTrainingGap<0.1;
 s.rankingDisagrees=s.bestTrain!=s.bestHoldout;
 s.warning="Candidate scores are bounded normalized spectral/envelope descriptors; they are not likelihoods or calibrated probabilities. ";
 if(s.rankingDisagrees)s.warning+="Training and held-out sets prefer DIFFERENT structures. ";
 if(s.lowSeparation)s.warning+="Top training candidates are close. ";
 s.warning+="No claim of unique 3D geometry, material parameters, or vocal-tract structure from audio alone.";
 return s;
}
std::vector<ProbeSuggestion> suggestedMeasurements(){
 using observability53::Measurement;
 using observability53::evaluate;
 using observability53::selectNext;
 std::vector<ProbeSuggestion> out;
 struct Family {
  std::string name,description;
  std::vector<double> theta;
  Measurement observed;
  std::vector<Measurement> choices;
  std::vector<std::string> reasons;
 };
 const std::vector<Family> families={
  {"string_T_mu", "Frequency determines T/mu, not T and mu separately",{100.,.005},
   {"fundamental frequency (Hz)",{.25},[](const auto&x){return std::vector<double>{std::sqrt(x[0]/x[1])/(2*.65)};}},
   {{"second harmonic (Hz)",{.25},[](const auto&x){return std::vector<double>{std::sqrt(x[0]/x[1])/.65};}},
    {"independent linear density (kg/m)",{.0001},[](const auto&x){return std::vector<double>{x[1]};}},
    {"independent tension (N)",{1.},[](const auto&x){return std::vector<double>{x[0]};}}},
   {"Additional harmonic still depends on T/mu; it cannot resolve T vs mu.",
    "Independently weigh a known string length to distinguish T and mu.",
    "A calibrated force gauge can independently measure tension."}},
  {"tube_c_length", "Resonance determines c/L, not sound speed and tube length",{340.,.17},
   {"quarter-wave resonance (Hz)",{1.},[](const auto&x){return std::vector<double>{x[0]/(4*x[1])};}},
   {{"third resonance (Hz)",{1.},[](const auto&x){return std::vector<double>{3*x[0]/(4*x[1])};}},
    {"independent tube length (m)",{.001},[](const auto&x){return std::vector<double>{x[1]};}},
    {"independent sound speed (m/s)",{1.},[](const auto&x){return std::vector<double>{x[0]};}}},
   {"Another harmonic still cannot separate c from L.",
    "Measure effective tube length independently; end correction and radiation remain unknown.",
    "Measure propagation speed or calibrated temperature/composition independently."}}
 };
 for(const auto&f:families){
  const auto initial=evaluate(f.theta,{f.observed});
  for(size_t i=0;i<f.choices.size();++i){
   const auto after=evaluate(f.theta,{f.observed,f.choices[i]});
   out.push_back({f.name,f.choices[i].label,initial.rank,after.rank,initial.parameterCount,after.rank>initial.rank,f.reasons[i]});
  }
  const auto plan=selectNext(f.theta,{f.observed},f.choices);
  if(plan.selected<0||plan.after.rank<initial.rank){out.push_back({f.name,"NO RELIABLE ADDITIONAL MEASUREMENT",initial.rank,initial.rank,initial.parameterCount,false,"Plan selector failed"});}
 }
 return out;
}
std::string candidatesCSV(const Summary&s){
 std::ostringstream os;os<<std::setprecision(12)<<"family,train_descriptor_error,heldout_descriptor_error,train_gap,heldout_gap,overfit_risk,best_train,best_heldout\n";
 for(size_t i=0;i<s.candidates.size();i++){
  const auto&c=s.candidates[i];os<<c.family<<','<<c.trainError<<','<<c.holdoutError<<','<<c.trainGapToBest<<','<<c.holdoutGapToBest<<','<<int(c.overfitRisk)<<','<<int(int(i)==s.bestTrain)<<','<<int(int(i)==s.bestHoldout)<<'\n';
 }return os.str();
}
std::string measurementsCSV(const std::vector<ProbeSuggestion>&x){
 std::ostringstream os;os<<"family,measurement,rank_before,rank_after,parameter_count,reduces_local_ambiguity,reason\n";
 for(const auto&s:x)os<<s.family<<",\""<<s.measurement<<"\","<<s.rankBefore<<','<<s.rankAfter<<','<<s.parameterCount<<','<<int(s.independentlyDistinguishes)<<",\""<<s.reason<<"\"\n";
 return os.str();
}
std::string reportText(const Summary&s){
 std::ostringstream o;o<<std::setprecision(9);
 o<<"PANORAMIZE v5.4 INVERSE DECISION - NOT A UNIQUE RECONSTRUCTION\n";
 o<<"Training-best rank index="<<s.bestTrain<<"; held-out-best index="<<s.bestHoldout<<"\n";
 o<<"Relative gap between best/second training errors="<<s.normalizedTrainingGap<<" (heuristic not calibrated)\n";
 o<<"Ranking disagrees="<<s.rankingDisagrees<<"; low separation="<<s.lowSeparation<<"\n";
 o<<s.warning<<"\n";
 o<<"Suggested physical measurements use analytic TWO-PARAMETER surrogate models, not full FEM or measured native27 state sensitivities.\n";
 o<<"Other optional observations: geometry scan/photogrammetry with scale; density/mass measurement; excitation force/position; multi-mic directivity; controlled articulation/video/MRI with proper consent. These are recommendations, not validated inverse solutions.\n";
 return o.str();
}
}
