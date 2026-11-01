#include "RecordedValidation51.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <numeric>
#include <sstream>
namespace panorama::recorded51 {
Audit inspect(const joint50::Take&t,int sr){
 Audit r;if(sr<8000||t.audio.size()<128){r.silent=true;return r;}
 double sq=0,dc=0,beginSq=0;const size_t first=std::max<size_t>(1,t.audio.size()/20);
 for(size_t i=0;i<t.audio.size();i++){
  const double a=t.audio[i];
  if(!std::isfinite(a)){r.silent=true;return r;}
  sq+=a*a;dc+=a;r.peak=std::max(r.peak,std::abs(a));
  if(std::abs(a)>=.999)r.clippedFraction+=1;
  if(i<first)beginSq+=a*a;
 }
 const double n=double(t.audio.size());r.rms=std::sqrt(sq/n);
 r.dc=dc/n;r.initialRms=std::sqrt(beginSq/first);r.clippedFraction/=n;
 r.silent=r.rms<1e-6;r.clipped=r.clippedFraction>1e-5;
 const double hz=440.*std::pow(2.,(t.midi-69)/12.);
 const int nominal=int(std::lround(sr/hz));
 const int minLag=std::max(1,int(.96*nominal)),maxLag=std::max(minLag+1,int(1.04*nominal));
 const size_t start=std::min(t.audio.size()/4,size_t(sr/40));
 const size_t len=std::min(size_t(sr/12),t.audio.size()-start);
 double best=0;
 for(int lag=minLag;lag<=maxLag;lag++){
  if(size_t(lag)>=len)continue;
  double cross=0,a=0,b=0;
  for(size_t i=0;i<len-size_t(lag);i++){
   const double x=t.audio[start+i]-r.dc,y=t.audio[start+i+size_t(lag)]-r.dc;
   cross+=x*y;a+=x*x;b+=y*y;
  }
  if(a>1e-12&&b>1e-12)best=std::max(best,cross/std::sqrt(a*b));
 }
 r.periodicity=std::clamp(best,0.,1.);
 r.weakPeriodicity=r.periodicity<.40;
 return r;
}
std::string auditCSV(const std::vector<joint50::Take>&takes,int sr){
 std::ostringstream f;f<<std::setprecision(8);
 f<<"take,midi,training,samples,seconds,rms,peak,dc,clipped_fraction,initial_rms,periodicity,pitch_is_weak,clipped,silent\n";
 for(size_t i=0;i<takes.size();i++){
  auto a=inspect(takes[i],sr);
  f<<i<<','<<takes[i].midi<<','<<int(takes[i].training)<<','<<takes[i].audio.size()<<','<<double(takes[i].audio.size())/sr<<','<<a.rms<<','<<a.peak<<','<<a.dc<<','<<a.clippedFraction<<','<<a.initialRms<<','<<a.periodicity<<','<<int(a.weakPeriodicity)<<','<<int(a.clipped)<<','<<int(a.silent)<<'\n';
 }
 return f.str();
}
RankReport logSensitivityRank(const std::vector<double>&params,
 const std::function<std::vector<double>(const std::vector<double>&)>&observe,double step,double tolerance){
 RankReport result;result.parameters=int(params.size());
 if(params.empty()||params.size()>32||!(step>0&&step<.1)||!(tolerance>0&&tolerance<.1)){
  result.warning="invalid sensitivity setup";return result;
 }
 for(double x:params)if(!std::isfinite(x)||x<=0){result.warning="parameters must be positive";return result;}
 const auto ref=observe(params);result.observations=int(ref.size());
 if(ref.empty()||ref.size()>256){result.warning="invalid observation dimension";return result;}
 for(double x:ref)if(!std::isfinite(x)){result.warning="nonfinite observation";return result;}
 std::vector<std::vector<double>> cols;
 for(size_t j=0;j<params.size();j++){
  auto hi=params,lo=params;hi[j]*=std::exp(step);lo[j]*=std::exp(-step);
  auto a=observe(hi),b=observe(lo);
  if(a.size()!=ref.size()||b.size()!=ref.size()){
   result.warning="model changed observable dimensions";return result;
  }
  std::vector<double> v(a.size());double norm2=0;
  for(size_t k=0;k<v.size();k++){
   if(!std::isfinite(a[k])||!std::isfinite(b[k])){result.warning="nonfinite difference";return result;}
   const double scale=std::max(1e-9,std::abs(ref[k]));
   v[k]=(a[k]-b[k])/(2.*step*scale);
   norm2+=v[k]*v[k];
  }
  // Normalization exposes *structural* ambiguity, not absolute sensitivity.
  if(norm2>1e-24){const double inv=1./std::sqrt(norm2);for(auto&x:v)x*=inv;}
  cols.push_back(std::move(v));
 }
 // Pivoted modified Gram-Schmidt, all columns scaled to unit length.
 std::vector<std::vector<double>> independent;
 std::vector<bool> used(cols.size(),false);
 double minNorm=1.;
 for(size_t iter=0;iter<cols.size();iter++){
  int best=-1;double bestNorm=0;std::vector<double>bestResidual;
  for(size_t j=0;j<cols.size();j++)if(!used[j]){
   auto v=cols[j];
   // reorthogonalize to reduce numerical cancellation
   for(int pass=0;pass<2;pass++)for(const auto&basis:independent){
    double dot=0;for(size_t i=0;i<v.size();i++)dot+=v[i]*basis[i];
    for(size_t i=0;i<v.size();i++)v[i]-=dot*basis[i];
   }
   double sq=0;for(double x:v)sq+=x*x;
   if(sq>bestNorm){bestNorm=sq;best=int(j);bestResidual=std::move(v);}
  }
  if(best<0||std::sqrt(bestNorm)<tolerance)break;
  used[size_t(best)]=true;minNorm=std::min(minNorm,std::sqrt(bestNorm));
  for(auto&x:bestResidual)x/=std::sqrt(bestNorm);
  independent.push_back(std::move(bestResidual));
 }
 result.rank=int(independent.size());result.minIndependentNorm=result.rank?minNorm:0;
 result.locallyIdentifiable=result.rank==result.parameters;
 result.warning=result.locallyIdentifiable?
  "Local full rank does NOT prove global uniqueness or recovery of real geometry and materials.":
  "Rank deficient: at least one parameter combination cannot be locally distinguished by these observables.";
 return result;
}
}
