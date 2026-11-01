#include "AcousticGraph21.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>
using namespace panorama::acoustic21;
static Material3 material(double k,double c=0.){
 Material3 m;m.stiffness=Tensor3::isotropic(k);m.damping=Tensor3::isotropic(c);return m;
}
static std::shared_ptr<const PreparedGraph> model(int bodies,double damping=0.){
 GraphBuilder b;
 for(int i=0;i<bodies;i++){
  Body body;body.name="body"+std::to_string(i);body.rest={double(i),double(i%3),double(i%4)};
  body.mass=0.75+.17*i;body.anchor=material(1000.+i*170.,damping);
  assert(b.addBody(body)==i);
 }
 for(int i=0;i<bodies;i++){
  Link l;l.a=i;l.b=(i+1)%bodies;l.coupling=material(600.+i*21.,damping*.5);
  if(bodies>1)assert(b.addLink(l));
 }
 std::string error;auto g=b.compile(48000.,&error);if(!g)std::cerr<<error<<"\n";
 assert(g&&g->degreesOfFreedom()==bodies*3);return g;
}
int main(){
 // Reject invalid units, nonphysical conversions, masses and non-passive matrices.
 assert(nativeUnit(Quantity::Force)==Unit::Newtons);
 assert(!validPort({0,0,Quantity::Force,Unit::Metres},1));
 assert(validPort({0,0,Quantity::Force,Unit::Newtons},1));
 assert(!validateConversion({Quantity::SpectralEnergy,Quantity::Force,Unit::Joules,Unit::Newtons,1.}));
 assert(validateConversion({Quantity::Velocity,Quantity::Velocity,Unit::MetresPerSecond,Unit::MetresPerSecond,1.}));
 GraphBuilder invalid;Body negative;negative.mass=-1.;assert(invalid.addBody(negative)==-1);
 Body a;a.name="A";a.anchor=material(1500);assert(invalid.addBody(a)==0);
 Body bad;bad.anchor=material(-1);assert(invalid.addBody(bad)==-1);
 assert(!invalid.addLink({0,0,material(12)}));
 // Conservative closed 3-D loop. Every link is solved at the same time.
 auto g=model(8);auto v=g->makeVoice();assert(v.excite(0,{.1,-.2,.15}));
 const double initial=v.ledger().initial;assert(initial>0);
 double maxError=0, maxZ=0;
 for(int t=0;t<12000;t++){
  assert(v.processSample());const auto e=v.ledger();
  maxError=std::max(maxError,std::abs(e.residual)/(1.+initial));
  maxZ=std::max(maxZ,std::abs(v.displacement(5).z));
  assert(std::isfinite(e.mechanical)&&e.mechanical>=0);
 }
 assert(maxError<1.e-8);assert(maxZ>1.e-7);
 assert(v.readPort({5,2,Quantity::Displacement,Unit::Metres})==v.displacement(5).z);
 // Damping moves mechanical energy into the dissipation ledger, never creating it.
 auto damped=model(8,17.);auto d=damped->makeVoice();assert(d.excite(0,{.1,-.2,.15}));
 const double initialD=d.ledger().mechanical;
 for(int t=0;t<12000;t++)assert(d.processSample());
 const auto e=d.ledger();
 assert(e.mechanical<initialD && e.dissipated>0 && std::abs(e.residual)<1.e-7);
 // External energy injection is measured as force*velocity work (passivity balance).
 auto ex=model(3,2.);auto w=ex->makeVoice();
 for(int t=0;t<3000;t++){
  double force=30.*std::sin(2.*3.141592653589793*211.*t/48000.);
  assert(w.addForceAtPort({0,1,Quantity::Force,Unit::Newtons},force));
  assert(!w.addForceAtPort({0,1,Quantity::Velocity,Unit::MetresPerSecond},force));
  assert(w.processSample());
 }
 const auto f=w.ledger();assert(f.work>0&&f.dissipated>0&&std::abs(f.residual)<1.e-7);
 // A complex PSD off-diagonal anisotropic material is permitted and coupled.
 GraphBuilder aniso;Body anis;a.name="anis";anis.mass=2;
 anis.anchor.stiffness.a={1000,200,0, 200,900,0, 0,0,600};
 anis.anchor.damping.a={5,0,0, 0,5,0, 0,0,5};
 assert(aniso.addBody(anis)==0);
 auto ag=aniso.compile(96000.);assert(ag);
 auto av=ag->makeVoice();assert(av.excite(0,{.5,0.,0.}));
 for(int i=0;i<1000;i++)assert(av.processSample());
 assert(std::abs(av.displacement(0).y)>1.e-10);
 // A genuine 3D linear-elastic tetrahedral finite element (not a 1-axis spring).
 GraphBuilder fem;
 const Vec3 pts[]={{0,0,0},{.1,0,0},{0,.1,0},{0,0,.1},{.1,.1,.1}};
 for(const auto& point:pts){
  Body b;b.mass=.02;b.rest=point;b.anchor=material(20.,.1);
  assert(fem.addBody(b)>=0);
 }
 Tetrahedron t;t.vertices={0,1,2,3};t.youngPa=1.6e5;t.poisson=.27;
 t.densityKgM3=1100.;t.viscousSeconds=4.e-5;
 assert(fem.addTetrahedron(t));
 t.vertices={1,2,3,4};t.youngPa=4.2e5;t.poisson=.34;
 assert(fem.addTetrahedron(t));
 auto fg=fem.compile(48000.);assert(fg&&fg->degreesOfFreedom()==15);
 auto fv=fg->makeVoice();assert(fv.excite(1,{.15,.06,-.07}));
 for(int i=0;i<7000;i++)assert(fv.processSample());
 auto fl=fv.ledger();assert(fl.dissipated>0&&std::abs(fl.residual)<1.e-7);
 assert(std::abs(fv.displacement(4).x)+std::abs(fv.displacement(4).y)+std::abs(fv.displacement(4).z)>1.e-10);
 GraphBuilder degenerate;
 for(int i=0;i<4;i++){Body b;b.rest={double(i),0,0};assert(degenerate.addBody(b)==i);}
 Tetrahedron flat;flat.vertices={0,1,2,3};assert(degenerate.addTetrahedron(flat));
 assert(!degenerate.compile(48000.));
 // More than legacy 9-node engine; allocation only when constructing voices.
 auto big=model(24,1.);std::vector<StateVoice> voices;
 voices.reserve(16);for(int i=0;i<16;i++){
  voices.emplace_back(big->makeVoice());assert(voices.back().excite(i%24,{.12,0,0}));
 }
 for(int t=0;t<256;t++)for(auto& voice:voices)assert(voice.processSample());
 // Typed spectral -> mechanical transducer, explicitly accounted as external power.
 SpectralDrive s;s.energy[0]=1.;s.weights[0]=1.;s.phaseTurns[0]=.25;s.maxForceNewtons=11.;
 assert(spectralDriveForce(s)>0.);
 s.energy[0]=-1.;assert(spectralDriveForce(s)==0.);
 std::cout<<"PANORAMA 2.1 graph: conservation err="<<maxError
          <<", damping="<<e.dissipated<<", work="<<f.work
          <<", bodies=24, voices=16, typed ports PASS\n";
}
