#include "UnifiedGraph22.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
using namespace panorama::unified22;
static void check(bool c,const char* why){if(!c){std::cerr<<"FAIL: "<<why<<"\n";std::exit(2);}}
static Mode mode(const char* n,Domain d,double hz,double damp=0.,double quartic=0.){
 Mode m;m.name=n;m.domain=d;m.frequencyHz=hz;m.dampingPerSecond=damp;m.quartic=quartic;
 if(d==Domain::Physical)m.displacementMetresPerQ=1.e-6;
 if(d==Domain::Spectral)m.spectralAmplitudePerQ=1.;
 if(d==Domain::Phase)m.phaseRadiansPerQ=1.;
 if(d==Domain::VocalFormant)m.pressurePascalsPerQ=2.;return m;
}
static std::shared_ptr<const PreparedGraph> base(bool damping,bool nonlinear){
 Builder b;
 check(b.addMode(mode("string",Domain::Physical,110.,damping?12.:0.))==0,"physical mode");
 check(b.addMode(mode("spectrum",Domain::Spectral,221.,damping?4.:0.))==1,"spectral mode");
 check(b.addMode(mode("phase",Domain::Phase,330.,damping?6.:0.))==2,"phase mode");
 check(b.addMode(mode("resonator",Domain::Resonance,441.,damping?8.:0.))==3,"resonant mode");
 check(b.addMode(mode("nonlinear",Domain::Nonlinear,550.,damping?10.:0.,nonlinear?1.e11:0.))==4,"nonlinear mode");
 check(b.addMode(mode("vocal",Domain::VocalFormant,660.,damping?9.:0.))==5,"vocal mode");
 for(int i=0;i<6;i++)check(b.connect({i,(i+1)%6,LinkKind::Conservative,1.e5,.6}),"cyclic energy link");
 for(int i=0;i<5;i++)check(b.connect({i,i+1,LinkKind::Dissipative,damping?2.:0.,.7}),"damping link");
 std::string error;auto g=b.compile(48000.,&error);check(bool(g),error.c_str());return g;
}
int main(){
 check(nativeUnit(Quantity::Pressure)==Unit::Pascal,"unit mapping");
 check(nativeUnit(Quantity::NormalizedCoordinate)==Unit::RootJouleSeconds,"power coordinate unit");
 {Builder b;check(b.addMode(mode("p",Domain::Physical,110.))==0,"valid mode");
  check(!b.connect({0,0,LinkKind::Conservative,2.,1.}),"self link rejected");
  check(!b.connect({0,1,LinkKind::Conservative,2.,1.}),"missing node rejected");
  check(b.addMode(mode("s",Domain::Spectral,220.))==1,"second mode");
  check(!b.connect({0,1,LinkKind::Conservative,-3.,1.}),"active negative spring rejected");
  check(!b.connect({0,1,LinkKind::Dissipative,-3.,1.}),"negative damper rejected");
  check(!b.connectTyped({0,Quantity::Displacement,Unit::Metre},
    {1,Quantity::NormalizedCoordinate,Unit::RootJouleSeconds},LinkKind::Conservative,100.),"raw SI port rejected");
  check(!b.connectTyped({0,Quantity::NormalizedCoordinate,Unit::RootJouleSeconds},
    {1,Quantity::NormalizedCoordinate,Unit::Metre},LinkKind::Conservative,100.),"wrong declared unit rejected");
  check(b.connectTyped({0,Quantity::NormalizedCoordinate,Unit::RootJouleSeconds},
    {1,Quantity::NormalizedCoordinate,Unit::RootJouleSeconds},LinkKind::Conservative,1.e5,.8),"typed cross-domain coupling");
  check(!b.compile(0.),"bad sample rate rejected");
 }
 for(bool nonlinear:{false,true}){
  for(bool damp:{false,true}){
   auto g=base(damp,nonlinear);auto v=g->makeVoice();
   check(v.excite(0,.7),"kick");check(v.excite(5,.31),"vocal kick");
   check(v.read({0,Quantity::Displacement,Unit::Metre})==0.,"position starts at zero");
   check(v.read({0,Quantity::Velocity,Unit::Pascal})==0.,"unit mismatch blocked");
   check(v.read({1,Quantity::Pressure,Unit::Pascal})==0.,"domain mismatch blocked");
   check(v.read({1,Quantity::SpectralAmplitude,Unit::RelativeAmplitude})==0.,"initial spectral zero");
   double start=v.ledger().stored,maxResidual=0.,maxVocal=0.;
   for(int i=0;i<12000;i++){
    if(damp && i%1000==0)check(v.addActiveForce(0,100.+i*.001),"active force");
    check(v.processSample(),"process stable");auto e=v.ledger();maxResidual=std::max(maxResidual,std::abs(e.residual));
    maxVocal=std::max(maxVocal,std::abs(v.audioSample(5)));
    check(e.stored>=-1.e-12 && e.dissipated>=-1.e-12,"energy and dissipation nonnegative");
   }
   check(maxVocal>1.e-6,"cross-domain energy transferred to formant");
   auto e=v.ledger();
   std::cout<<"nonlinear="<<nonlinear<<" damp="<<damp<<" start="<<start<<" stored="<<e.stored<<" work="<<e.activeWork<<" loss="<<e.dissipated<<" residual="<<maxResidual<<"\n";
   check(maxResidual<5.e-7,"discrete balance residual");
   if(!damp)check(std::abs(e.stored-start)<5.e-7,"conservative network energy");
   else check(e.dissipated>.01,"dissipative network loses energy");
  }
 }
 // Genuine state nonlinearity changes motion, not just an output waveshaper.
 {Builder linear,nonlinear;
  auto plain=mode("m",Domain::Nonlinear,120.,.01,0.);
  auto quartic=mode("m",Domain::Nonlinear,120.,.01,2.e12);
  linear.addMode(plain);nonlinear.addMode(quartic);
  auto a=linear.compile(48000.),b=nonlinear.compile(48000.);
  check(a&&b,"oscillator construction");auto va=a->makeVoice(),vb=b->makeVoice();
  va.excite(0,.7);vb.excite(0,.7);double sumdiff=0.;
  for(int i=0;i<4096;i++){check(va.processSample()&&vb.processSample(),"nonlinear integration");sumdiff+=std::abs(va.audioSample(0)-vb.audioSample(0));}
  check(sumdiff>100.,"quartic oscillator actually changes the waveform");
  check(std::abs(vb.ledger().residual)<1.e-8,"nonlinear energy conservation");
 }
 // Multiple independent voices share the immutable compiled graph.
 {auto g=base(true,true);std::vector<Voice> voices;for(int i=0;i<16;i++){voices.push_back(g->makeVoice());check(voices.back().excite(i%6,.3),"poly excite");}
  for(int j=0;j<512;j++)for(auto& v:voices)check(v.processSample(),"16 voice poly");
 }
 std::cout<<"UNIFIED22_TEST_SUCCESS\n";
}
