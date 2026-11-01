#include "SparseHamiltonian42.h"
#include "UniversalInstrument23.h"
#include <algorithm>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
using namespace panorama;
using namespace panorama::sparse42;
static Region makeString(int points,int n){
 Region g;g.family=Family::String1D;g.density=.003;g.tension=60.;g.damping=.001;
 for(int i=0;i<points;i++)g.vertices.push_back({.55*i/(points-1),double(n)*.04,0});
 for(int i=1;i<points;i++)g.elements.push_back({i-1,i});
 return g;
}
static Region makeMembrane(int side){
 Region g;g.family=Family::Membrane2D;g.density=.16;g.tension=110;g.damping=.003;
 for(int y=0;y<side;y++)for(int x=0;x<side;x++)g.vertices.push_back({.16*x/(side-1),.16*y/(side-1),0});
 for(int y=0;y<side-1;y++)for(int x=0;x<side-1;x++){
  int a=x+side*y;g.elements.push_back({a,a+1,a+1+side});g.elements.push_back({a,a+1+side,a+side});
 }return g;
}
int main(){
 std::string why;
 auto r=example();assert(valid(r,&why));
 auto blob=serialize(r);Recipe loaded;assert(deserialize(blob,loaded,&why));assert(blob==serialize(loaded));
 assert(!deserialize(blob+"EXTRA\n",loaded,&why));
 auto invalid=r;invalid.ports[0].terms[0].localDof=999;assert(!valid(invalid,&why));
 invalid=r;invalid.links.back().unit=PortUnit::Metres;assert(!valid(invalid,&why));
 invalid=r;invalid.regions[2].elements[0]={0,0,1,2};assert(!valid(invalid,&why));
 invalid=r;invalid.regions[2].vertices[0].x=std::numeric_limits<double>::quiet_NaN();assert(!valid(invalid,&why));
 invalid=r;invalid.regions[1].vertices[1]=invalid.regions[1].vertices[0]; // some triangles become degenerate
 assert(!Instrument::prepare(invalid,48000,&why));
 auto ins=Instrument::prepare(r,48000,&why);if(!ins){std::cerr<<why<<'\n';return 2;}
 const auto n=ins->dofCount();assert(n>46);
 ins->noteOn(57,.8f);double sum=0,body=0,cavity=0;
 for(int i=0;i<6000;i++){
  auto y=ins->processOne();assert(std::isfinite(y));sum+=std::abs(y);
  body=std::max(body,ins->regionEnergy(2));cavity=std::max(cavity,ins->regionEnergy(3));
 }
 auto L=ins->ledger();auto S=ins->stats();
 std::cout<<"4.2 dof="<<S.dofs<<" nnz="<<S.nnz<<" failures="<<S.failed<<" sample_peak="<<sum
          <<" bodyE="<<body<<" cavityE="<<cavity<<" active="<<L.active<<" loss="<<L.dissipated<<" residual="<<L.residual<<'\n';
 assert(S.nnz<S.dofs*S.dofs/2&&S.failed==0&&sum>1e-8&&body>1e-20&&cavity>1e-25);
 assert(L.finalEnergy>=0&&L.dissipated>=0&&std::abs(L.residual)<1e-12+std::abs(L.active)*1e-5);
 // Decouple body and air; force must not leak through a nonexistent port.
 auto disconnected=r;disconnected.links.clear();auto iso=Instrument::prepare(disconnected,48000,&why);assert(iso);
 iso->noteOn(57,.8f);for(int i=0;i<3000;i++)iso->processOne();
 assert(iso->regionEnergy(1)==0&&iso->regionEnergy(2)==0&&iso->regionEnergy(3)==0);
 // Lossless network preserves midpoint quadratic energy; zero all material and link loss.
 auto ideal=r;for(auto&g:ideal.regions)g.damping=0;for(auto&l:ideal.links)l.damping=0;
 auto passive=Instrument::prepare(ideal,48000,&why);assert(passive);
 passive->noteOn(57,.9f);for(int i=0;i<6000;i++)passive->processOne();
 auto W=passive->ledger();assert(passive->stats().failed==0);
 assert(std::abs(W.residual)<1e-12+1e-5*std::abs(W.active));
 assert(std::abs(W.finalEnergy-W.active)<1e-12+1e-5*std::abs(W.active));
 // Scale past the historical 46 DOF with arbitrary region instances and mesh resolution.
 auto large=r;large.regions.push_back(makeString(210,5));large.regions.push_back(makeMembrane(13));
 const int big1=int(large.regions.size())-2,big2=int(large.regions.size())-1;
 large.ports.push_back({big1,PortUnit::Metres,{{100,1.}}});large.ports.push_back({big2,PortUnit::Metres,{{84,1.}}});
 large.links.push_back({0,4,PortUnit::Metres,1,1,100,.005});
 large.links.push_back({4,5,PortUnit::Metres,1,1,150,.003});
 large.relativeTolerance=1e-8;large.maxIterations=220;
 auto scalable=Instrument::prepare(large,48000,&why);if(!scalable){std::cerr<<"large: "<<why<<'\n';return 3;}
 assert(scalable->dofCount()>400);
 scalable->noteOn(57,.65f);for(int i=0;i<400;i++)assert(std::isfinite(scalable->processOne()));
 auto B=scalable->stats();auto BL=scalable->ledger();
 std::cout<<"expanded dof="<<B.dofs<<" sparseNnz="<<B.nnz<<" fraction="<<double(B.nnz)/(B.dofs*B.dofs)
          <<" failures="<<B.failed<<" work="<<BL.active<<" residual="<<BL.residual<<'\n';
 assert(B.failed==0&&B.nnz<B.dofs*B.dofs/50);
 // User-editable .pgraph can embed the mesh and play through the old Universal Patchbay.
 auto patch=universal23::sparseHamiltonianGraph();patch.sparse42=r;
 auto encoded=universal23::serialize(patch);universal23::Graph loadedGraph;
 assert(universal23::deserialize(encoded,loadedGraph,&why)&&encoded==universal23::serialize(loadedGraph));
 auto player=universal23::Instrument::prepare(loadedGraph,48000,&why);if(!player){std::cerr<<"player: "<<why<<'\n';return 4;}
 player->noteOn(57,.8f);double absolute=0;
 for(int i=0;i<1200;i++){auto p=player->processOne();assert(std::isfinite(p.left));absolute+=std::abs(p.left);}
 assert(absolute>1e-8);
 // 4.1 old patch remains round-trip compatible.
 auto old=universal23::boundaryGraph();auto oldText=universal23::serialize(old);universal23::Graph oldRound;
 assert(universal23::deserialize(oldText,oldRound,&why));
 std::cout<<"SPARSE42_ALL_PASS\n";
}
