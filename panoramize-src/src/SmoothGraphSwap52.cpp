#include "SmoothGraphSwap52.h"
#include <algorithm>
#include <cmath>
namespace panorama::swap52 {
void SmoothGraphSwap::setSampleRate(double rate)noexcept{
 if(std::isfinite(rate)&&rate>=8000.&&rate<=384000.)total_=std::clamp(int(rate*.012),64,8192);
}
void SmoothGraphSwap::beginStopped(std::shared_ptr<Instrument> g)noexcept{
 // Only while the audio callback is stopped; no concurrent processOne().
 current_=std::move(g);fading_.reset();std::atomic_store(&retired_,std::shared_ptr<Instrument>{});std::atomic_store(&pending_,std::shared_ptr<Instrument>{});
 remaining_=0;keys_={};bend_=wheel_=pressure_=0; sustain_=false;
}
void SmoothGraphSwap::collectRetired()noexcept{
 auto old=std::atomic_exchange_explicit(&retired_,std::shared_ptr<Instrument>{},std::memory_order_acq_rel);
 // Last reference is released on this non-audio thread.
}
void SmoothGraphSwap::publish(std::shared_ptr<Instrument> g)noexcept{
 collectRetired();
 // A single producer must publish at a time. Consumers observe a coherent graph.
 std::atomic_store_explicit(&pending_,std::move(g),std::memory_order_release);
}
void SmoothGraphSwap::poll()noexcept{
 if(fading_)return; // do not interrupt an active fade or lose its old engine
 auto incoming=std::atomic_exchange_explicit(&pending_,std::shared_ptr<Instrument>{},std::memory_order_acq_rel);
 if(!incoming||incoming==current_)return;
 incoming->pitchBend(bend_);incoming->modWheel(wheel_);incoming->channelPressure(pressure_);incoming->sustain(sustain_);
 for(int c=1;c<=16;c++)for(int n=0;n<128;n++){
  const auto&h=keys_[key(n,c)];if(h.on)incoming->noteOn(n,h.velocity,c);
 }
 fading_=std::move(current_);current_=std::move(incoming);
 remaining_=fading_?total_:0;
}
panorama::Sample SmoothGraphSwap::processOne()noexcept{
 poll();
 panorama::Sample newest{};if(current_)newest=current_->processOne();
 if(!fading_||remaining_<=0)return newest;
 const auto older=fading_->processOne();
 const double alpha=1.-double(remaining_)/double(total_);
 const auto mixed=panorama::Sample{float(older.left*(1.-alpha)+newest.left*alpha),float(older.right*(1.-alpha)+newest.right*alpha)};
 // Never drop the last old-engine reference on the callback. One retirement
 // slot is reclaimed by the next UI publish(); a subsequent fade stalls its
 // final sample if that previous retirement has not yet been collected.
 if(remaining_==1){
  if(!std::atomic_load_explicit(&retired_,std::memory_order_acquire)){
   std::atomic_store_explicit(&retired_,std::move(fading_),std::memory_order_release);
   remaining_=0;
  }
 }else --remaining_;
 return mixed;
}
void SmoothGraphSwap::noteOn(int n,float v,int c)noexcept{
 if(n<0||n>127||c<1||c>16||!std::isfinite(v))return;
 keys_[key(n,c)]={std::clamp(v,0.f,1.f),true};if(current_)current_->noteOn(n,v,c);if(fading_)fading_->noteOn(n,v,c);
}
void SmoothGraphSwap::noteOff(int n,int c)noexcept{
 if(n<0||n>127||c<1||c>16)return;keys_[key(n,c)].on=false;
 if(current_)current_->noteOff(n,c);if(fading_)fading_->noteOff(n,c);
}
void SmoothGraphSwap::pitchBend(float v)noexcept{bend_=v;if(current_)current_->pitchBend(v);if(fading_)fading_->pitchBend(v);}
void SmoothGraphSwap::modWheel(float v)noexcept{wheel_=v;if(current_)current_->modWheel(v);if(fading_)fading_->modWheel(v);}
void SmoothGraphSwap::sustain(bool v)noexcept{sustain_=v;if(current_)current_->sustain(v);if(fading_)fading_->sustain(v);}
void SmoothGraphSwap::channelPressure(float v)noexcept{pressure_=v;if(current_)current_->channelPressure(v);if(fading_)fading_->channelPressure(v);}
void SmoothGraphSwap::polyPressure(int n,float v)noexcept{if(current_)current_->polyPressure(n,v);if(fading_)fading_->polyPressure(n,v);}
void SmoothGraphSwap::articulationControl(int c,float v)noexcept{if(current_)current_->articulationControl(c,v);if(fading_)fading_->articulationControl(c,v);}
void SmoothGraphSwap::allNotesOff()noexcept{keys_={};if(current_)current_->allNotesOff();if(fading_)fading_->allNotesOff();}
}
