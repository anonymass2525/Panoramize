#pragma once
#include "ReducedHamiltonian44.h"
namespace panorama::reduced44 {
inline Recipe largeExample(){
 Recipe big=example();
 sparse43::Region string;string.family=sparse43::Family::String1D;string.density=.003;string.tension=65;string.damping=.003;
 for(int i=0;i<210;i++)string.vertices.push_back({.55*i/209.,.5,0});
 for(int i=1;i<210;i++)string.elements.push_back({i-1,i});
 int stringRegion=int(big.source.regions.size());big.source.regions.push_back(string);
 sparse43::Region membrane;membrane.family=sparse43::Family::Membrane2D;membrane.tension=120;membrane.density=.16;membrane.damping=.003;
 constexpr int side=13;
 for(int y=0;y<side;y++)for(int x=0;x<side;x++)membrane.vertices.push_back({.16*x/12.,.16*y/12.,.02});
 for(int y=0;y<12;y++)for(int x=0;x<12;x++){
  int a=x+side*y;membrane.elements.push_back({a,a+1,a+side+1});membrane.elements.push_back({a,a+side+1,a+side});
 }
 int membraneRegion=int(big.source.regions.size());big.source.regions.push_back(membrane);
 big.source.ports.push_back({stringRegion,sparse43::PortUnit::Metres,{{100,1.}}});
 big.source.ports.push_back({membraneRegion,sparse43::PortUnit::Metres,{{84,1.}}});
 big.source.links.push_back({0,int(big.source.ports.size())-2,sparse43::PortUnit::Metres,1,1,100,.002});
 big.source.links.push_back({int(big.source.ports.size())-2,int(big.source.ports.size())-1,sparse43::PortUnit::Metres,1,1,140,.003});
 big.rateFactor.resize(big.source.regions.size(),1);
 big.rateFactor[size_t(stringRegion)]=2;big.rateFactor[size_t(membraneRegion)]=4;
 big.source.relativeTolerance=1e-8;big.source.maxIterations=220;
 return big;
}
}
