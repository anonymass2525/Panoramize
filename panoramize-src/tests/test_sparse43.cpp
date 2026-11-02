#include "SparseHamiltonian43.h"
#include "UniversalInstrument23.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
using namespace panorama;
using namespace panorama::sparse43;
int main(){
 std::string why;auto r=example();assert(valid(r,&why));
 assert(r.regions.size()==7&&r.ports.size()==7&&r.links.size()==6);
 auto blob=serialize(r);Recipe copy;assert(deserialize(blob,copy,&why)&&serialize(copy)==blob);
 auto bad=r;bad.links[4].modalScaleB=std::numeric_limits<double>::quiet_NaN();assert(!valid(bad,&why));
 bad=r;bad.ports[6].unit=PortUnit::CubicMetres;assert(!valid(bad,&why));
 bad=r;bad.regions[5].modes[0].frequencyHz=-1;assert(!valid(bad,&why));
 bad=r;bad.links[3].unit=PortUnit::Metres;assert(!valid(bad,&why));
 bad=r;bad.regions[4].vertices[0]=bad.regions[4].vertices[1];assert(!Instrument::prepare(bad,48000.,&why));
 bad=r;bad.preconditioner=3;assert(!valid(bad,&why));
 auto a=Instrument::prepare(r,48000,&why);if(!a){std::cerr<<why<<'\n';return 1;}
 assert(a->stats().dofs>60);a->noteOn(57,.85f);
 double total=0,modal=0,tube=0;
 for(int i=0;i<6500;i++){
  const float v=a->processOne();assert(std::isfinite(v));total+=std::abs(v);
  modal=std::max(modal,a->regionEnergy(5));tube=std::max(tube,a->regionEnergy(4));
 }
 auto l=a->ledger();auto t=a->stats();assert(!t.ic0);
 std::cout<<"dof="<<t.dofs<<" nnz="<<t.nnz<<" ic0="<<t.ic0<<" iterations="<<t.iterations<<" failed="<<t.failed
          <<" tube_E="<<tube<<" modal_E="<<modal<<" residual="<<l.residual<<'\n';
 assert(t.failed==0&&total>1e-7&&t.nnz<t.dofs*t.dofs/2);
 assert(std::abs(l.residual)<1e-11+std::abs(l.active)*1e-5);
 assert(modal>1e-26&&tube>1e-28);
 // Isolating all links must give exactly zero energy outside the excited region.
 auto iso=r;iso.links.clear();auto x=Instrument::prepare(iso,48000,&why);assert(x);x->noteOn(57,.85f);
 for(int i=0;i<5000;i++)assert(std::isfinite(x->processOne()));
 for(int i=1;i<int(iso.regions.size());i++)assert(x->regionEnergy(i)==0.);
 // Conservation with zero damping: conservative modal transformers cannot create energy.
 auto perfect=r;for(auto&rg:perfect.regions){rg.damping=0;for(auto&mode:rg.modes)mode.damping=0;}
 for(auto&link:perfect.links)link.damping=0;
 auto conservative=Instrument::prepare(perfect,48000,&why);assert(conservative);conservative->noteOn(57,.9f);
 for(int i=0;i<5500;i++)conservative->processOne();
 auto w=conservative->ledger();
 assert(conservative->stats().failed==0&&std::abs(w.residual)<1e-11+std::abs(w.active)*1e-5);
 // Jacobi fallback must produce a valid (possibly different tolerance) solution.
 auto j=r;j.preconditioner=0;auto xj=Instrument::prepare(j,48000,&why);assert(xj&&!xj->stats().ic0);
 xj->noteOn(57,.85f);for(int i=0;i<500;i++)assert(std::isfinite(xj->processOne()));assert(xj->stats().failed==0);
 // Scale the graph beyond 400 DOF; automatic preconditioner chooses IC(0).
 auto large=r;
 Region extra;extra.family=Family::String1D;extra.density=.003;extra.tension=65;extra.damping=.003;
 for(int i=0;i<210;i++)extra.vertices.push_back({.55*i/209.,.5,0});
 for(int i=1;i<210;i++)extra.elements.push_back({i-1,i});
 int extraId=int(large.regions.size());large.regions.push_back(extra);
 Region sheet;sheet.family=Family::Membrane2D;sheet.density=.16;sheet.tension=120;sheet.damping=.003;
 constexpr int side=13;
 for(int y=0;y<side;y++)for(int x=0;x<side;x++)sheet.vertices.push_back({.16*x/12.,.16*y/12.,.02});
 for(int y=0;y<12;y++)for(int x=0;x<12;x++){int a=x+side*y;sheet.elements.push_back({a,a+1,a+side+1});sheet.elements.push_back({a,a+side+1,a+side});}
 int sheetId=int(large.regions.size());large.regions.push_back(sheet);
 large.ports.push_back({extraId,PortUnit::Metres,{{100,1.}}});
 large.ports.push_back({sheetId,PortUnit::Metres,{{84,1.}}});
 large.links.push_back({0,int(large.ports.size())-2,PortUnit::Metres,1,1,100,.005});
 large.links.push_back({int(large.ports.size())-2,int(large.ports.size())-1,PortUnit::Metres,1,1,140,.003});
 large.relativeTolerance=1e-8;large.maxIterations=220;
 auto big=Instrument::prepare(large,48000,&why);
 if(!big){std::cerr<<"large43: "<<why<<'\n';return 5;}
 assert(big->stats().dofs>400&&big->stats().ic0);
 big->noteOn(57,.8f);for(int i=0;i<350;i++)assert(std::isfinite(big->processOne()));
 assert(big->stats().failed==0);
 auto q=big->ledger();assert(std::abs(q.residual)<1e-11+std::abs(q.active)*1e-5);
 std::cout<<"large43 dof="<<big->stats().dofs<<" nnz="<<big->stats().nnz<<" ic0="<<big->stats().ic0<<" residual="<<q.residual<<'\n';
 // Prepared Universal patch source, previous v4.2 patch unaffected.
 auto patch=universal23::sparseEnhancedGraph();patch.sparse43=r;
 auto pgraph=universal23::serialize(patch);universal23::Graph loaded;
 assert(universal23::deserialize(pgraph,loaded,&why)&&universal23::serialize(loaded)==pgraph);
 auto player=universal23::Instrument::prepare(loaded,48000,&why);if(!player){std::cerr<<why<<'\n';return 4;}
 player->noteOn(57,.7f);double energy=0;
 for(int i=0;i<1024;i++){auto s=player->processOne();assert(std::isfinite(s.left));energy+=std::abs(s.left);}
 assert(energy>1e-8);
 auto old=universal23::sparseHamiltonianGraph();auto oldText=universal23::serialize(old);
 assert(universal23::deserialize(oldText,loaded,&why)&&universal23::serialize(loaded)==oldText);
 std::cout<<"SPARSE43_PASS\n";
}
