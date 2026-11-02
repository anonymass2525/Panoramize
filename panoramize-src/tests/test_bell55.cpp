#include "PanoramaDSP.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
struct Render {
 std::vector<float> left,right;
 double sumAbs=0,peak=0;
};
Render render(const panorama::Patch& patch,double sr,int midi,int count){
 panorama::Synth synth;
 synth.prepare(sr);
 synth.setPatch(patch);
 synth.noteOn(midi,.82f);
 Render result;
 result.left.reserve(size_t(count));
 result.right.reserve(size_t(count));
 for(int i=0;i<count;i++){
  if(i==count/3)synth.noteOff(midi);
  const auto s=synth.processOne();
  if(!std::isfinite(s.left)||!std::isfinite(s.right))
   throw std::runtime_error("Bell55 produced NaN or infinity");
  const double peak=std::max(std::abs(double(s.left)),std::abs(double(s.right)));
  if(peak>1.000001)throw std::runtime_error("Bell55 exceeded normalized output range");
  result.peak=std::max(result.peak,peak);
  result.sumAbs+=std::abs(double(s.left))+std::abs(double(s.right));
  result.left.push_back(s.left);
  result.right.push_back(s.right);
 }
 return result;
}
void require(bool value,const char*message){
 if(!value)throw std::runtime_error(message);
}
double diff(const Render&a,const Render&b){
 require(a.left.size()==b.left.size(),"render size mismatch");
 double out=0.;
 for(size_t i=0;i<a.left.size();i++)
  out+=std::fabs(a.left[i]-b.left[i])+std::fabs(a.right[i]-b.right[i]);
 return out;
}
} // namespace
int main(){
 try{
  const auto bell=panorama::factoryPresets()[4];
  require(bell.engine==panorama::Engine::FractalDigital,"factory Bell engine changed");
  require(bell.metalMix>0.4f,"Bell55 metal body is not enabled");
  auto classic=bell;
  classic.metalMix=0.f;
  const auto a=render(classic,48000.,72,48000);
  const auto b=render(bell,48000.,72,48000);
  require(a.sumAbs>1., "classic bell is silent");
  require(b.sumAbs>1., "enhanced bell is silent");
  require(diff(a,b)>1., "new resonator has no audible numerical influence");
  double spread=0.;
  for(size_t i=0;i<b.left.size();i++)spread+=std::fabs(b.left[i]-b.right[i]);
  require(spread>1.e-4,"new bell stereo modal width has no influence");
  // Metal-off exactly preserves the original Fractal Digital signal regardless
  // of metal-specific settings. This is not an approximate audio comparison.
  auto disabled=classic;disabled.metalDecay=1.f;disabled.metalColor=0.f;
  require(diff(a,render(disabled,48000.,72,48000))==0.0,
    "metal-off must preserve original dry sound bit for bit");
  // Metal parameters must have no effect on the unrelated physical instrument.
  auto physical=panorama::factoryPresets()[6];
  physical.metalMix=0.f;
  const auto originalPhysical=render(physical,48000.,64,14000);
  physical.metalMix=1.f;physical.metalColor=0.f;physical.metalDecay=1.f;
  require(diff(originalPhysical,render(physical,48000.,64,14000))==0.0,
    "bell controls unexpectedly changed physical resonance engine");
  for(double sr:{44100.,48000.,96000.}){
   for(int pitch:{48,72,84}){
    const auto c=render(bell,sr,pitch,int(sr*.35));
    require(c.sumAbs>.01 && c.peak<1.001,"bell is silent or unstable at another sample rate/pitch");
   }
  }
  // Test the body and tone controls independently and ensure no hidden no-ops.
  auto darker=bell;darker.metalColor=0.f;
  auto shorter=bell;shorter.metalDecay=0.f;
  require(diff(b,render(darker,48000.,72,48000))>0.01,"metalColor is ineffective");
  require(diff(b,render(shorter,48000.,72,48000))>0.01,"metalDecay is ineffective");
  std::cout<<"Bell55 PASS. Original mono dry sound preserved with metalMix=0, "
           <<"inharmonic modes affect color/tail, 44.1/48/96kHz and C3-C6 finite. "
           <<"classic peak="<<a.peak<<" enhanced peak="<<b.peak<<"\n";
  return 0;
 }catch(const std::exception&e){
  std::cerr<<"Bell55 FAIL: "<<e.what()<<"\n";
  return 1;
 }
}
