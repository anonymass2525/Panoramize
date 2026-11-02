#include "CoupledPhysics38.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
using namespace panorama::coupled38;
static Recipe recipe(bool connected=true){
 Recipe r;
 const auto f=panorama::gen37::Family::String1D;
 Domain string;string.family=f;string.nx=20;string.fundamentalHz=125;string.damping=0.;
 Domain membrane=string;membrane.family=panorama::gen37::Family::Membrane2D;membrane.nx=12;membrane.ny=10;membrane.fundamentalHz=130;
 Domain plate=membrane;plate.family=panorama::gen37::Family::Plate2D;plate.fundamentalHz=180;
 Domain body=string;body.family=panorama::gen37::Family::TetraResonator3D;body.shape.xCells=1;body.shape.yCells=1;body.shape.zCells=1;
 Domain tube=string;tube.family=panorama::gen37::Family::Tube1D;tube.nx=20;tube.fundamentalHz=125;
 r.domains={string,membrane,plate,body,tube};
 if(connected){
  for(int i=0;i<4;i++){
   Connection c;c.from=i;c.to=i+1;c.stiffness=i==3?2000.:8000.;c.damping=0.;
   c.positionFrom=.65;c.positionTo=.45;c.axisFrom=1;c.axisTo=1;r.connections.push_back(c);
  }
 }
 r.outputGain=2000;
 return r;
}
int main(){
 auto r=recipe();std::string err;
 auto p=Instrument::prepare(r,48000,&err);
 if(!p){std::cerr<<"prepare: "<<err<<'\n';return 1;}
 assert(p->domainCount()==5);
 assert(p->substeps()>0);
 double energy=p->totalEnergy();assert(energy>0);
 double maxDrift=0;
 for(int k=0;k<16000;k++){
  float sample=p->processOne();assert(std::isfinite(sample));
  double e=p->totalEnergy();assert(std::isfinite(e));assert(e>=-1e-8);
  maxDrift=std::max(maxDrift,std::abs(e/energy-1));
 }
 std::cout<<"initialE="<<energy<<" finalE="<<p->totalEnergy()<<" drift="<<maxDrift<<" substeps="<<p->substeps()<<'\n';
 assert(maxDrift<.06); // symplectic Euler, energy bounded rather than exactly conserved.
 double sumTransfer=0;
 for(int j=1;j<p->domainCount();j++){sumTransfer+=p->domainEnergy(j);std::cout<<"domainE["<<j<<"]="<<p->domainEnergy(j)<<'\n';}
 assert(sumTransfer>1e-12);
 // Pairwise cross-dimensional energy propagation; even the 3D/tube interface
 // couples displacement and its power-conjugate force (not PCM audio mixing).
 for(int pair=0;pair<4;pair++){
  auto pairR=recipe();
  auto first=pairR.domains[size_t(pair)],second=pairR.domains[size_t(pair+1)];
  pairR.domains={first,second};pairR.connections.clear();
  Connection c;c.from=0;c.to=1;c.damping=0.;c.stiffness=pair==3?50.:1.e6;
  c.positionFrom=.35;c.positionTo=.35;pairR.connections.push_back(c);
  auto linked=Instrument::prepare(pairR,48000,&err);
  if(!linked){std::cerr<<"pair "<<pair<<" failed: "<<err<<'\n';return 1;}
  pairR.connections.clear();auto noLink=Instrument::prepare(pairR,48000,&err);assert(noLink);
  double gained=0;
  for(int k=0;k<6000;k++){
   linked->processOne();noLink->processOne();
   gained=std::max(gained,linked->domainEnergy(1));
  }
  std::cout<<"pair "<<pair<<" maxEnergyTransferred="<<gained<<"\n";
  assert(gained>1e-19);assert(noLink->domainEnergy(1)==0.);
 }
 auto isolated=Instrument::prepare(recipe(false),48000,&err);assert(isolated);
 for(int i=0;i<16000;i++)isolated->processOne();
 for(int j=1;j<5;j++)assert(isolated->domainEnergy(j)==0.);
 auto d=recipe();for(auto&dom:d.domains)dom.damping=4.;for(auto&link:d.connections)link.damping=35.;
 auto lossy=Instrument::prepare(d,48000,&err);assert(lossy);
 double before=lossy->totalEnergy();
 for(int i=0;i<16000;i++)lossy->processOne();
 std::cout<<"dissipated="<<lossy->dissipatedEnergy()<<" remaining="<<lossy->totalEnergy()<<'\n';
 assert(lossy->totalEnergy()<before*.9);assert(lossy->dissipatedEnergy()>0.);
 Recipe copy;assert(deserialize(serialize(r),copy,&err));assert(serialize(copy)==serialize(r));
 auto anis=r;anis.domains[3].anisotropy={{2.,.5,1.5,.7,1.2,1.}};
 auto anisInstrument=Instrument::prepare(anis,48000,&err);assert(anisInstrument);
 assert(serialize(anis)!=serialize(r));
 auto anisCopy=Recipe{};assert(deserialize(serialize(anis),anisCopy));assert(serialize(anisCopy)==serialize(anis));
 auto replacement=copy;assert(!deserialize("not a graph",replacement));assert(serialize(replacement)==serialize(copy));
 auto bad=r;bad.connections[0].to=9;assert(!Instrument::prepare(bad,48000));
 bad=r;bad.connections[0].ratio=std::numeric_limits<double>::quiet_NaN();assert(!Instrument::prepare(bad,48000));
 bad=r;bad.domains[3].anisotropy[1]=-1.;assert(!Instrument::prepare(bad,48000));
 bad=r;bad.domains[3].shape.xCells=20;assert(!Instrument::prepare(bad,48000));
 bad=r;bad.connections[0].stiffness=1.e9;bad.connections[0].to=4;assert(!Instrument::prepare(bad,22050));
 std::cout<<"COUPLED38_PASS\n";
}
