#include "ShapeInverse37.h"
#include <algorithm>
#include <cmath>
#include <array>
#include <utility>
namespace panorama::gen37 {
namespace {
constexpr double eps=1e-12;
double score(const std::vector<float>&r,const std::vector<float>&v){
 if(r.size()!=v.size()||r.empty())return 1e12;
 double rr=0,ss=0,corr=0;
 for(size_t i=0;i<r.size();i++){rr+=double(r[i])*r[i];ss+=double(v[i])*v[i];corr+=double(r[i])*v[i];}
 if(rr<eps||ss<eps)return 1e12;
 const double gain=std::clamp(corr/(ss+eps),-20.,20.);
 double error=0,envelope=0;
 for(size_t i=0;i<r.size();i+=128){
  const size_t stop=std::min(r.size(),i+128);double e1=0,e2=0;
  for(size_t j=i;j<stop;j++){
   double a=r[j],b=v[j]*gain;error+=(a-b)*(a-b);e1+=a*a;e2+=b*b;
  }
  envelope+=std::pow(std::sqrt(e1)-std::sqrt(e2),2);
 }
 return .7*error/(rr+eps)+.3*envelope/(rr+eps);
}
// Geometry generated from parameter values before material optimization.
bool modelFromShape(const Shape&s,const fem36::Model&seed,fem36::Model&out){
 fem36::Mesh mesh; if(!buildTetraMesh(s,mesh))return false;
 out=seed;out.mesh=std::move(mesh);return true;
}
double evaluate(const Candidate&cand,const std::vector<inverse36::Take>&takes,int rate,bool training){
 double sum=0;int num=0;
 for(const auto&t:takes)if(t.training==training){
  auto audio=inverse36::render(cand.model,rate,t.midi,t.velocity,t.audio.size());
  if(audio.empty())return 1e12;
  sum+=score(t.audio,audio);num++;
 }
 return num?sum/num:0;
}
void perturb(Candidate&x,int par,double delta){
 switch(par){
  case 0:x.shape.length=std::clamp(x.shape.length*(1+delta),.05,.7);break;
  case 1:x.shape.width=std::clamp(x.shape.width*(1+delta),.03,.5);break;
  case 2:x.shape.thickness=std::clamp(x.shape.thickness*(1+delta),.012,.2);break;
  case 3:x.shape.camber=std::clamp(x.shape.camber+delta*.30,-.3,.3);break;
  case 4:x.shape.taper=std::clamp(x.shape.taper+delta*.30,-.35,.35);break;
  case 5:x.model.meshYoung=std::clamp(x.model.meshYoung*(1+delta),1e7,2e9);break;
  case 6:x.model.bridgeCoupling=std::clamp(x.model.bridgeCoupling*(1+delta),.005,.5);break;
  case 7:x.model.pluckPosition=std::clamp(x.model.pluckPosition+delta*.12,.05,.95);break;
  case 8:x.model.anisotropy[0]=std::clamp(x.model.anisotropy[0]*(1+delta),.1,10.);break;
  case 9:x.model.anisotropy[1]=std::clamp(x.model.anisotropy[1]*(1+delta),.1,10.);break;
  default:break;
 }
}
}
SearchResult discover(const std::vector<inverse36::Take>&takes,int rate,int maxEval,int maxProps){
 SearchResult result;
 auto fail=[&](const char*reason){result.error=reason;return result;};
 if(takes.empty()||takes.size()>12||rate<22050||rate>96000||maxEval<1||maxEval>1024||maxProps<1||maxProps>8)
  return fail("invalid shape search settings");
 int train=0;
 for(const auto&t:takes){
  if(t.training)train++;
  if(t.midi<0||t.midi>127||t.velocity<=0||t.velocity>1||t.audio.size()<256||t.audio.size()>size_t(rate)*2)
   return fail("invalid take metadata");
  for(float a:t.audio)if(!std::isfinite(a))return fail("nonfinite audio");
 }
 if(!train)return fail("no training recordings");
 // Family set generated algorithmically, not supplied mesh candidates.
 std::vector<Shape> shapes;
 for(int nx=1;nx<=3;nx++)for(int ny=1;ny<=2;ny++)for(int nz=1;nz<=2;nz++){
  Shape s;s.xCells=nx;s.yCells=ny;s.zCells=nz;
  if(!validShape(s))continue;
  // Prioritize smaller/lighter structures but always enumerate topology diversity.
  shapes.push_back(s);
 }
 std::sort(shapes.begin(),shapes.end(),[](const Shape&a,const Shape&b){
  const int av=(a.xCells+1)*(a.yCells+1)*(a.zCells+1),bv=(b.xCells+1)*(b.yCells+1)*(b.zCells+1);
  return av==bv?a.xCells>b.xCells:av<bv;
 });
 result.generatedTopologies=int(shapes.size());
 // Coarse scan: compare all generated topology classes before spending the
 // remaining optimization budget on the most acoustically plausible shapes.
 const int nModels=std::min(maxEval,int(shapes.size()));
 std::vector<Candidate> archive;
 for(int i=0;i<nModels;i++){
  Candidate c;c.shape=shapes[size_t(i)];c.candidateIndex=i;
  if(!modelFromShape(c.shape,c.model,c.model))continue;
  if(!fem36::Instrument::prepare(c.model,rate))continue;
  c.trainingLoss=evaluate(c,takes,rate,true);result.evaluations++;
  archive.push_back(std::move(c));
 }
 if(archive.empty())return fail("no valid physical geometry candidates");
 std::sort(archive.begin(),archive.end(),[](const Candidate&a,const Candidate&b){return a.trainingLoss<b.trainingLoss;});
 // Refine three independent topological hypotheses rather than one lucky fit.
 const int selected=std::min(int(archive.size()),std::min(3,maxEval-result.evaluations));
 for(int i=0;i<selected;i++){
  Candidate best=archive[size_t(i)];
  const int remainingModels=selected-i;
  const int budget=std::max(0,(maxEval-result.evaluations)/remainingModels);
  const int stopAt=std::min(maxEval,result.evaluations+budget);
  // Perturb *both* physical properties and geometry, including camber/taper.
  // Larger variants are attempted first, then smaller ones if budget remains.
  for(double step:{.35,.15,.06}){
   if(result.evaluations>=stopAt)break;
   for(int par=0;par<10 && result.evaluations<stopAt;par++){
    for(int sign:{-1,1}){
     if(result.evaluations>=stopAt)break;
     Candidate trial=best;perturb(trial,par,step*sign);
     if(!modelFromShape(trial.shape,trial.model,trial.model))continue;
     if(!fem36::Instrument::prepare(trial.model,rate))continue;
     trial.trainingLoss=evaluate(trial,takes,rate,true);result.evaluations++;
     if(trial.trainingLoss+1e-14<best.trainingLoss)best=std::move(trial);
    }
   }
  }
  result.proposals.push_back(std::move(best));
 }
 // Keep unrefined topologies as fallbacks for very small evaluation budgets.
 for(const auto&c:archive)if(int(result.proposals.size())<maxProps){
  bool used=false;for(const auto&p:result.proposals)used|=(p.candidateIndex==c.candidateIndex);
  if(!used)result.proposals.push_back(c);
 }
 if(result.proposals.empty())return fail("no valid physical geometry candidates");
 std::sort(result.proposals.begin(),result.proposals.end(),[](const Candidate&a,const Candidate&b){return a.trainingLoss<b.trainingLoss;});
 if(int(result.proposals.size())>maxProps)result.proposals.resize(size_t(maxProps));
 for(auto&candidate:result.proposals)candidate.validationLoss=evaluate(candidate,takes,rate,false);
 return result;
}
}
