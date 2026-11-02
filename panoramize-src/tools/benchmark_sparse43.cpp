#include "SparseHamiltonian43.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <numeric>
#include <vector>
using namespace panorama::sparse43;
Recipe big(){
 Recipe r=example();
 Region g;g.family=Family::String1D;g.density=.003;g.tension=65;g.damping=.003;
 for(int k=0;k<210;k++)g.vertices.push_back({.55*k/209.,.5,0});
 for(int k=1;k<210;k++)g.elements.push_back({k-1,k});r.regions.push_back(g);
 const int a=int(r.regions.size())-1;
 r.ports.push_back({a,PortUnit::Metres,{{100,1.}}});r.links.push_back({0,int(r.ports.size())-1,PortUnit::Metres,1,1,100,.002});
 Region membrane;membrane.family=Family::Membrane2D;membrane.tension=120;membrane.density=.16;membrane.damping=.003;
 constexpr int side=13;for(int y=0;y<side;y++)for(int x=0;x<side;x++)membrane.vertices.push_back({.16*x/12.,.16*y/12.,.02});
 for(int y=0;y<12;y++)for(int x=0;x<12;x++){int z=x+side*y;membrane.elements.push_back({z,z+1,z+side+1});membrane.elements.push_back({z,z+side+1,z+side});}
 r.regions.push_back(membrane);int b=int(r.regions.size())-1;r.ports.push_back({b,PortUnit::Metres,{{84,1.}}});
 r.links.push_back({int(r.ports.size())-2,int(r.ports.size())-1,PortUnit::Metres,1,1,140,.003});
 r.relativeTolerance=1e-8;r.maxIterations=220;
 return r;
}
int main(){
 std::cout<<"dof,nnz,block,median_us,p95_us,failed,iterations_last,preconditioner\n";
 volatile double sink=0;
 for(int scale=0;scale<2;scale++)for(int method=0;method<=1;method++){
  std::string err;auto recipe=scale?big():example();recipe.preconditioner=method;auto x=Instrument::prepare(recipe,48000,&err);if(!x){std::cerr<<err;return 1;}
  x->noteOn(57,.85f);for(int k=0;k<1024;k++)sink+=x->processOne();
  std::vector<double> spans;spans.reserve(200);
  for(int k=0;k<200;k++){
   auto st=std::chrono::steady_clock::now();for(int i=0;i<64;i++)sink+=x->processOne();
   spans.push_back(std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-st).count());
  }
  std::sort(spans.begin(),spans.end());auto S=x->stats();
  std::cout<<S.dofs<<','<<S.nnz<<",64,"<<spans[100]<<','<<spans[190]<<','<<S.failed<<','<<S.iterations<<','<<S.ic0<<'\n';
 }
 std::cerr<<"checksum="<<sink<<'\n';
}
