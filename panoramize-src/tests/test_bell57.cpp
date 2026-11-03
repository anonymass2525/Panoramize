// Panoramize Bell 5.7: original attack / coupled body bloom regression.
#include "PanoramaDSP.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace {
struct Audio {std::vector<float> left,right; double peak=0.,sum=0.;};
void ensure(bool b,const char*msg){if(!b)throw std::runtime_error(msg);}
Audio render(panorama::Patch patch,double sr,int pitch=72,int lengthMs=1700) {
 panorama::Synth synth;synth.prepare(sr);synth.setPatch(patch);synth.noteOn(pitch,.82f);
 Audio a;const int n=int(sr*lengthMs/1000);
 a.left.reserve(size_t(n));a.right.reserve(size_t(n));
 for(int i=0;i<n;i++){
  if(i==int(sr*.45))synth.noteOff(pitch);
  const auto s=synth.processOne();
  ensure(std::isfinite(s.left)&&std::isfinite(s.right),"non-finite Bell57 output");
  a.peak=std::max(a.peak,std::max(std::fabs(double(s.left)),std::fabs(double(s.right))));
  a.sum+=std::fabs(double(s.left))+std::fabs(double(s.right));
  a.left.push_back(s.left);a.right.push_back(s.right);
 }
 return a;
}
double diff(const Audio&a,const Audio&b,size_t start=0,size_t stop=0) {
 ensure(a.left.size()==b.left.size(),"render length mismatch");
 const size_t end=stop?std::min(stop,a.left.size()):a.left.size();
 double total=0.;
 for(size_t i=start;i<end;i++)
  total+=std::fabs(a.left[i]-b.left[i])+std::fabs(a.right[i]-b.right[i]);
 return total;
}
}
int main(){
 try {
  auto morph=panorama::factoryPresets()[4];
  ensure(morph.engine==panorama::Engine::FractalDigital,"Bell57 engine is not Fractal Digital");
  ensure(morph.bell57Morph>.5f&&morph.bellDynamics>.5f,"Bell57 preset not enabled");
  auto baseline=morph;baseline.bell57Morph=0.f;
  auto baselineBloom=baseline;baselineBloom.bell57Bloom=0.f;
  ensure(diff(render(baseline,48000.),render(baselineBloom,48000.))==0.0,
    "Morph disabled must reproduce 5.6 bit-exactly");
  auto noCoupled=baseline;noCoupled.bellDynamics=0.f;
  auto noCoupledAlt=noCoupled;noCoupledAlt.bell57Morph=1.f;
  ensure(diff(render(noCoupled,48000.),render(noCoupledAlt,48000.))==0.0,
    "Bell57 must not change pure 5.5 sound when 5.6 companion is disabled");
  auto original=noCoupled;original.metalMix=0.f;
  auto originalAlt=original;originalAlt.bell57Morph=1.f;originalAlt.bell57Bloom=1.f;
  ensure(diff(render(original,48000.),render(originalAlt,48000.))==0.0,
    "Original dry sound must be unchanged, even if Bell57 controls are on");
  const auto m=render(morph,48000.);
  const auto b=render(baseline,48000.);
  const auto o=render(original,48000.);
  ensure(m.sum>1.&&m.peak<1.001,"Bell57 is silent or clipped");
  ensure(diff(m,b)>1.,"Bell57 morph has no effect");
  ensure(diff(m,o,48000/2)>1.,"Bell57 post-strike body does not develop");
  // At note onset Morph57 must lean closer to Original's sharp transient
  // than the same patch without morph, while retaining the coupled bloom.
  ensure(diff(m,o,0,480)<diff(b,o,0,480),
    "Morph57 attack is not closer to Original than the 5.6 dry core");
  auto quick=morph;quick.bell57Bloom=0.f;
  auto slow=morph;slow.bell57Bloom=1.f;
  ensure(diff(render(quick,48000.),render(slow,48000.))>.02,
    "Resonance bloom parameter has no audible effect");
  ensure(!panorama::samePatchConfiguration(baseline,morph),
    "new parameters missing from patch comparison cache");
  for(double sr:{44100.,48000.,96000.})
   for(int midi:{48,72,96,108}){
    auto a=render(morph,sr,midi,700);
    ensure(a.peak<1.001&&a.sum>.0001,"Bell57 bad pitch/rate output");
   }
  // Bell57 options must not affect other factory synthesis engines.
  auto physical=panorama::factoryPresets()[6];
  auto altered=physical;altered.bell57Morph=1.f;altered.bell57Bloom=1.f;
  ensure(diff(render(physical,48000.,60,300),render(altered,48000.,60,300))==0.,
    "Morph57 altered Physical Resonance engine");
  std::cout<<"Bell57 PASS: original attack, coupled bloom, exact 5.6 bypass, "
           <<"44.1/48/96kHz and note range stable, peak "<<m.peak<<"\n";
  return 0;
 }catch(const std::exception&e){
  std::cerr<<"Bell57 FAIL: "<<e.what()<<"\n";return 1;
 }
}
