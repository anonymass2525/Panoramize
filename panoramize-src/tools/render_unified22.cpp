#include "UnifiedGraph22.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
using namespace panorama::unified22;
namespace {
void put16(std::ostream& o,uint16_t x){const char b[2]={char(x),char(x>>8)};o.write(b,2);}
void put32(std::ostream& o,uint32_t x){const char b[4]={char(x),char(x>>8),char(x>>16),char(x>>24)};o.write(b,4);}
Mode m(const char* name,Domain domain,double freq,double damping,double quartic=0.){Mode a;a.name=name;a.domain=domain;a.frequencyHz=freq;a.dampingPerSecond=damping;a.quartic=quartic;if(domain==Domain::Physical)a.displacementMetresPerQ=1.e-6;if(domain==Domain::Spectral)a.spectralAmplitudePerQ=1.;if(domain==Domain::Phase)a.phaseRadiansPerQ=1.;if(domain==Domain::VocalFormant)a.pressurePascalsPerQ=1.;return a;}
std::shared_ptr<const PreparedGraph> build(int variant){
 Builder b;
 const double base=variant==0?98.:variant==1?130.8:variant==2?164.8:196.;
 const int mech=b.addMode(m("body",Domain::Physical,base,1.2));
 const int spec=b.addMode(m("spectral",Domain::Spectral,base*2.0,1.6));
 const int phase=b.addMode(m("phase",Domain::Phase,base*3.01,1.8));
 const int reson=b.addMode(m("resonance",Domain::Resonance,base*4.12,2.4));
 const int nonlin=b.addMode(m("nonlinear",Domain::Nonlinear,base*2.51,2.2,variant==0?0.:variant==1?4.e11:variant==2?8.e11:1.2e12));
 const int voice=b.addMode(m("vocal_formant",Domain::VocalFormant,variant==0?600.:variant==1?810.:variant==2?540.:720.,3.4));
 const int vowel2=b.addMode(m("vocal_formant_2",Domain::VocalFormant,variant==0?1250.:variant==1?1500.:variant==2?1130.:1020.,5.));
 b.connect({mech,spec,LinkKind::Conservative,1.e5,.85});
 b.connect({spec,phase,LinkKind::Conservative,1.6e5,.4});
 b.connect({phase,reson,LinkKind::Conservative,1.1e5,.7});
 b.connect({reson,nonlin,LinkKind::Conservative,1.9e5,.9});
 b.connect({nonlin,voice,LinkKind::Conservative,2.1e5,.7});
 b.connect({voice,vowel2,LinkKind::Conservative,2.e5,.65});
 b.connect({vowel2,mech,LinkKind::Conservative,8.e4,.3});
 b.connect({mech,phase,LinkKind::Dissipative,.7,.35});
 b.connect({spec,voice,LinkKind::Dissipative,1.2,.7});
 std::string error;auto graph=b.compile(48000.,&error);if(!graph)std::cerr<<error<<"\n";return graph;
}
bool render(const std::string& filename,int variant){
 auto graph=build(variant);if(!graph)return false;
 constexpr uint32_t sr=48000, frames=sr*3;auto voice=graph->makeVoice();
 voice.excite(0,.4);voice.excite(1,.18);voice.excite(2,.1);
 std::vector<double> samples; samples.reserve(size_t(frames)*2);
 double peak=0.;
 for(uint32_t i=0;i<frames;i++){
  if(i==48000 || i==96000){voice.reset();voice.excite(0,.34);voice.excite(1,.2);voice.excite(5,.18);}
  if(i%1600==0 && i<120000)voice.addActiveForce(0,30.+15.*std::sin(double(i)/2300.));
  if(!voice.processSample()){std::cerr<<"unstable at sample "<<i<<"\n";return false;}
  double l=.36*voice.audioSample(0)+.2*voice.audioSample(3)+.6*voice.audioSample(5)+.28*voice.audioSample(4);
  double r=.32*voice.audioSample(1)+.25*voice.audioSample(2)+.45*voice.audioSample(6)+.22*voice.audioSample(4);
  // No unaccounted signal-domain nonlinearity: nonlinear oscillator lives INSIDE energy ledger.
  samples.push_back(l);samples.push_back(r);peak=std::max({peak,std::abs(l),std::abs(r)});
 }
 if(!(peak>1.e-6))return false;
 const double gain=.68/peak;
 std::ofstream out(filename,std::ios::binary);if(!out)return false;
 out.write("RIFF",4);put32(out,36+frames*4);out.write("WAVEfmt ",8);put32(out,16);put16(out,1);put16(out,2);put32(out,sr);put32(out,sr*4);put16(out,4);put16(out,16);out.write("data",4);put32(out,frames*4);
 for(double x:samples)put16(out,uint16_t(int16_t(std::lround(std::clamp(x*gain,-1.,1.)*32767.))));
 const auto e=voice.ledger();std::cout<<filename<<" peak_raw="<<peak<<" final_energy_residual="<<e.residual<<"\n";
 return bool(out);
}
}
int main(int argc,char** argv){if(argc!=5){std::cerr<<"usage: panorama_unified22_render OUT1.wav OUT2.wav OUT3.wav OUT4.wav\n";return 2;}
 for(int i=0;i<4;i++)if(!render(argv[i+1],i))return 1;return 0;}
