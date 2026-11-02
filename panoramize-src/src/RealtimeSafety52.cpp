#include "RealtimeSafety52.h"
#include <algorithm>
#include <stdexcept>
namespace panorama::safe52 {
CoefficientBank::CoefficientBank(const std::vector<Range>&r,double sr,double ms):ranges_(r),targets_(new std::atomic<double>[r.size()]),values_(r.size()){
 if(!(std::isfinite(sr)&&sr>=8000.&&sr<=384000.&&std::isfinite(ms)&&ms>=0&&ms<=10000.))throw std::invalid_argument("invalid smoothing or sample rate");
 for(std::size_t i=0;i<r.size();++i){
  if(!(std::isfinite(r[i].min)&&std::isfinite(r[i].max)&&std::isfinite(r[i].initial)&&r[i].min<=r[i].initial&&r[i].initial<=r[i].max))throw std::invalid_argument("invalid parameter range");
  targets_[i].store(r[i].initial);values_[i]=r[i].initial;
 }
 decay_=ms>0?std::exp(-1000./(sr*ms)):0.;
}
bool CoefficientBank::publish(std::size_t i,double v)noexcept{
 if(i>=ranges_.size()||!std::isfinite(v)||v<ranges_[i].min||v>ranges_[i].max)return false;
 targets_[i].store(v,std::memory_order_release);return true;
}
bool CoefficientBank::publishBatch(const std::vector<double>&v)noexcept{
 if(v.size()!=ranges_.size())return false;
 for(std::size_t i=0;i<v.size();++i)if(!std::isfinite(v[i])||v[i]<ranges_[i].min||v[i]>ranges_[i].max)return false;
 // Values are validated as one batch, but distinct atomics are not a synchronous
 // multi-field transaction: listeners may observe intermediate fields.
 for(std::size_t i=0;i<v.size();++i)targets_[i].store(v[i],std::memory_order_release);
 return true;
}
double CoefficientBank::tick(std::size_t i)noexcept{
 if(i>=ranges_.size())return 0.;
 const double t=targets_[i].load(std::memory_order_acquire);
 const double next=t+decay_*(values_[i]-t);
 values_[i]=std::clamp(next,ranges_[i].min,ranges_[i].max);
 return values_[i];
}
}
