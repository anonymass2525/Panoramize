#include "MeshInverse36.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
namespace panorama::inverse36 {
namespace {
constexpr double eps=1e-12;
// Optimize waveform alignment and relative temporal energy. This descriptor is
// deliberately modest; dry onset-aligned short clips only.
double loss(const std::vector<float>& ref,const std::vector<float>& syn){
 if(ref.size()!=syn.size()||ref.empty())return 1e12;
 double rr=0,ss=0,rs=0;
 for(size_t i=0;i<ref.size();i++){
  if(!std::isfinite(ref[i])||!std::isfinite(syn[i]))return 1e12;
  rr+=double(ref[i])*ref[i];ss+=double(syn[i])*syn[i];rs+=double(ref[i])*syn[i];
 }
 if(rr<1e-15||ss<1e-18)return 1e12;
 // Least squares single scalar output gain: signal phase mismatch is penalized.
 const double gain=std::clamp(rs/(ss+eps),-20.,20.);
 double residue=0,envelope=0;
 constexpr size_t window=256;
 for(size_t begin=0;begin<ref.size();begin+=window){
  const size_t end=std::min(begin+window,ref.size());
  double ea=0,eb=0;
  for(size_t i=begin;i<end;i++){
   const double a=ref[i],b=syn[i]*gain,d=a-b;
   residue+=d*d;ea+=a*a;eb+=b*b;
  }
  envelope+=std::pow(std::sqrt(ea)-std::sqrt(eb),2);
 }
 return .7*residue/(rr+eps)+.3*envelope/(rr+eps);
}
double evaluate(const fem36::Model&m,const std::vector<Take>&takes,int rate,bool training){
 double total=0;int n=0;
 for(const auto&t:takes){
  if(t.training!=training)continue;
  auto synth=render(m,rate,t.midi,t.velocity,t.audio.size());
  if(synth.empty())return 1e12;
  total+=loss(t.audio,synth);++n;
 }
 return n?total/n:0;
}
}
std::vector<float> render(const fem36::Model&m,int rate,int midi,float velocity,size_t count){
 if(count>size_t(rate)*10 || rate<22050 || rate>96000 || midi<0 || midi>127)return {};
 auto voice=fem36::Instrument::prepare(m,rate);
 if(!voice)return {};
 voice->noteOn(midi,velocity);
 std::vector<float> out(count);
 for(float&x:out){x=voice->processOne();if(!std::isfinite(x))return {};}
 return out;
}
Result infer(const std::vector<Take>& takes,const std::vector<fem36::Model>&seeds,
             int rate,int maxEval,int maxProps){
 Result r;
 auto fail=[&](const char*m){r.error=m;return r;};
 if(takes.empty()||takes.size()>12||seeds.empty()||seeds.size()>10||rate<22050||rate>96000||
    maxEval<1||maxEval>4096||maxProps<1||maxProps>10)return fail("invalid inference settings");
 int training=0;
 for(const auto&t:takes){
  if(t.audio.size()<256||t.audio.size()>size_t(rate)*2||t.midi<0||t.midi>127||t.velocity<=0||t.velocity>1)return fail("invalid take");
  if(t.training)++training;
  for(float v:t.audio)if(!std::isfinite(v))return fail("nonfinite input");
 }
 if(!training)return fail("no training take");
 // Optimize each geometry hypothesis on training only, using deterministic
 // bounded coordinate search; validation audio is never accessed here.
 for(size_t s=0;s<seeds.size() && r.evaluations<maxEval;s++){
  const int remainingModels=int(seeds.size()-s);
  const int budgetPerShape=std::max(1,(maxEval-r.evaluations)/remainingModels);
  const int stopAt=std::min(maxEval,r.evaluations+budgetPerShape);
  fem36::Model model=seeds[s];
  if(!fem36::Instrument::prepare(model,rate))continue;
  double best=evaluate(model,takes,rate,true);++r.evaluations;
  // Step multipliers; each parameter represents one physical family.
  for(double step:{.28,.13,.06}){
   bool improved=false;
   for(int par=0;par<9 && r.evaluations<stopAt;par++){
    for(int sign:{-1,1}){
     if(r.evaluations>=stopAt)break;
     auto candidate=model;
     const double factor=1+step*sign;
     if(par==0) candidate.pluckPosition=std::clamp(candidate.pluckPosition+step*sign*.45,.05,.95);
     if(par==1) candidate.bridgeCoupling=std::clamp(candidate.bridgeCoupling*factor,0.,.5);
     if(par==2) candidate.meshYoung=std::clamp(candidate.meshYoung*factor,1e6,2e9);
     if(par==3) candidate.stringTensionScale=std::clamp(candidate.stringTensionScale*factor,.25,4.);
     if(par>=4 && par<=6)candidate.anisotropy[size_t(par-4)]=std::clamp(candidate.anisotropy[size_t(par-4)]*factor,.01,100.);
     if(par>=7){
      int axis=par-7; // geometric scale x / y; z is identified via mesh candidates
      for(auto&v:candidate.mesh.vertices)v[size_t(axis)]*=factor;
     }
     const double e=evaluate(candidate,takes,rate,true);++r.evaluations;
     if(e+1e-15<best){best=e;model=std::move(candidate);improved=true;}
    }
   }
   if(!improved && step==.06)break;
  }
  Proposal p;p.model=std::move(model);p.trainingLoss=best;p.meshCandidate=int(s);
  // No holdout data ever participates in search or proposal ordering.
  r.proposals.push_back(std::move(p));
 }
 if(r.proposals.empty())return fail("no mesh candidate is physically valid");
 std::sort(r.proposals.begin(),r.proposals.end(),[](const Proposal&a,const Proposal&b){return a.trainingLoss<b.trainingLoss;});
 if(int(r.proposals.size())>maxProps)r.proposals.resize(size_t(maxProps));
 for(auto&p:r.proposals)p.validationLoss=evaluate(p.model,takes,rate,false);
 return r;
}
}
