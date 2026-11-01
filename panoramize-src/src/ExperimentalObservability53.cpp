#include "ExperimentalObservability53.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
namespace panorama::observability53 {
namespace {
bool finitePositive(double x) {return std::isfinite(x)&&x>0;}
// Jacobi symmetric eigensolver of PSD J^TJ. Entirely offline, not for audio thread.
std::vector<double> eigenvalues(std::vector<double> a,int n){
 if(n<1)return {};
 const int budget=std::max(16,80*n*n);
 for(int iteration=0;iteration<budget;iteration++){
  int p=0,q=0;double maximum=0;
  for(int i=0;i<n;i++)for(int j=i+1;j<n;j++){
   const double v=std::abs(a[size_t(i)*n+j]);if(v>maximum){maximum=v;p=i;q=j;}
  }
  if(maximum<1e-14)break;
  const double ap=a[size_t(p)*n+p],aq=a[size_t(q)*n+q],v=a[size_t(p)*n+q];
  const double tau=(aq-ap)/(2*v);
  const double t=(tau>=0?1.:-1.)/(std::abs(tau)+std::sqrt(1+tau*tau));
  const double c=1/std::sqrt(1+t*t),s=t*c;
  a[size_t(p)*n+p]=ap-t*v;a[size_t(q)*n+q]=aq+t*v;
  a[size_t(p)*n+q]=a[size_t(q)*n+p]=0;
  for(int k=0;k<n;k++)if(k!=p&&k!=q){
   const double kp=a[size_t(k)*n+p],kq=a[size_t(k)*n+q];
   a[size_t(k)*n+p]=a[size_t(p)*n+k]=c*kp-s*kq;
   a[size_t(k)*n+q]=a[size_t(q)*n+k]=s*kp+c*kq;
  }
 }
 std::vector<double> result(n);
 for(int i=0;i<n;i++)result[i]=std::sqrt(std::max(0.,a[size_t(i)*n+i]));
 std::sort(result.begin(),result.end(),std::greater<double>());
 return result;
}
}
Diagnostics evaluate(const std::vector<double>& theta,const std::vector<Measurement>&probes,double step,double tol){
 Diagnostics out;out.parameterCount=int(theta.size());
 if(theta.empty()||theta.size()>32||!finitePositive(step)||step>=.1||!finitePositive(tol)||tol>=.1){out.warning="Invalid parameter dimension or finite-difference tolerance";return out;}
 for(double x:theta)if(!finitePositive(x)){out.warning="All physical parameters must be positive and finite";return out;}
 const int n=int(theta.size());
 std::vector<std::vector<double>> rows;
 for(const auto& probe:probes){
  if(!probe.observe||probe.sigma.empty()){out.warning="Missing measurement model or positive uncertainties";return out;}
  const auto base=probe.observe(theta);
  if(base.size()!=probe.sigma.size()||base.empty()||base.size()>1024){out.warning="Measurement dimension mismatch";return out;}
  for(size_t k=0;k<base.size();k++)if(!std::isfinite(base[k])||!finitePositive(probe.sigma[k])){
   out.warning="Non-finite observation or non-positive measurement uncertainty";return out;
  }
  std::vector<std::vector<double>> jac(base.size(),std::vector<double>(n));
  for(int j=0;j<n;j++){
   auto plus=theta,minus=theta;
   plus[j]*=std::exp(step);minus[j]*=std::exp(-step);
   const auto yp=probe.observe(plus),ym=probe.observe(minus);
   if(yp.size()!=base.size()||ym.size()!=base.size()){out.warning="Nonconstant measurement dimension";return out;}
   for(size_t k=0;k<base.size();k++){
    if(!std::isfinite(yp[k])||!std::isfinite(ym[k])){out.warning="Nonfinite derivative";return out;}
    jac[k][j]=(yp[k]-ym[k])/(2*step*probe.sigma[k]);
   }
  }
  rows.insert(rows.end(),jac.begin(),jac.end());
 }
 out.measurementCount=int(rows.size());
 std::vector<double> gram(size_t(n)*n,0);
 for(const auto&row:rows)for(int i=0;i<n;i++)for(int j=0;j<n;j++)gram[size_t(i)*n+j]+=row[i]*row[j];
 out.singularValues=eigenvalues(std::move(gram),n);
 out.maxSingular=out.singularValues[0];
 for(double s:out.singularValues)if(out.maxSingular>0&&s>out.maxSingular*tol)out.rank++;
 out.minSingular=out.singularValues.back();
 out.locallyIdentifiable=out.rank==n;
 out.condition=out.minSingular>out.maxSingular*tol?out.maxSingular/out.minSingular:std::numeric_limits<double>::infinity();
 if(!out.locallyIdentifiable)out.warning="Rank deficient: the supplied observations cannot locally distinguish all chosen parameters";
 else out.warning="Locally full rank only: global uniqueness and model correctness remain unproven";
 return out;
}
Plan selectNext(const std::vector<double>&theta,const std::vector<Measurement>&existing,
 const std::vector<Measurement>&choices){
 Plan plan;plan.after=evaluate(theta,existing);
 int bestRank=plan.after.rank;double bestMin=-1;
 for(size_t i=0;i<choices.size();i++){
  auto all=existing;all.push_back(choices[i]);auto d=evaluate(theta,all);
  if(!d.warning.empty()&&d.warning.find("Invalid")!=std::string::npos)continue;
  if(d.rank>bestRank||(d.rank==bestRank&&d.minSingular>bestMin)){
   plan.selected=int(i);plan.after=std::move(d);bestRank=plan.after.rank;bestMin=plan.after.minSingular;
  }
 }
 if(plan.selected<0)plan.rationale="No usable candidate measurement";
 else if(plan.after.locallyIdentifiable)plan.rationale="Selected measurement closes the local sensitivity rank (not global identification)";
 else plan.rationale="Selected measurement improves local sensitivity, but ambiguity remains";
 return plan;
}
}
