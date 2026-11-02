#include "GenerativePhysics37.h"
#include <cassert>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
using namespace panorama::gen37;
int main(){
 for(int x=1;x<=3;x++)for(int y=1;y<=2;y++){
  Shape s;s.xCells=x;s.yCells=y;s.camber=.16;s.taper=-.17;
  if(!validShape(s))continue;
  panorama::fem36::Mesh mesh;std::string err;
  assert(buildTetraMesh(s,mesh,&err));assert(err.empty());
  assert(mesh.vertices.size()==size_t((x+1)*(y+1)*2));assert(mesh.tetrahedra.size()==size_t(6*x*y));
  panorama::fem36::Mesh restored;
  assert(panorama::fem36::deserializeMesh(panorama::fem36::serializeMesh(mesh),restored));
  Shape decoded;assert(decodeShape(encodeShape(s),decoded));assert(decoded.camber==s.camber);
  panorama::fem36::Model model;model.mesh=mesh;
  auto instrument=panorama::fem36::Instrument::prepare(model,22050,&err);
  assert(instrument);instrument->noteOn(40,.7f);
  double sum=0;for(int i=0;i<600;i++){float audio=instrument->processOne();assert(std::isfinite(audio));sum+=std::abs(audio);}assert(sum>1e-12);
 }
 Shape bad;bad.camber=std::numeric_limits<double>::quiet_NaN();
 panorama::fem36::Mesh mesh;assert(!buildTetraMesh(bad,mesh));
 auto renderTest=[](Family f){
  WaveRecipe r;r.family=f;r.nx=26;r.ny=18;r.fundamentalHz=145;
  WaveRecipe q;assert(decodeWaveRecipe(encodeWaveRecipe(r),q));
  assert(q.nx==r.nx&&q.ny==r.ny&&q.family==r.family);
  auto audio=renderWave(q,22050,2400);
  assert(audio.size()==2400);
  double sum=0;for(float v:audio){assert(std::isfinite(v));sum+=double(v)*v;}
  assert(sum>1e-14);return sum;
 };
 for(auto f:{Family::String1D,Family::Membrane2D,Family::Plate2D,Family::Tube1D}){
  auto power=renderTest(f);std::cout<<"Family="<<int(f)<<" power="<<power<<'\n';
 }
 WaveRecipe badWave;badWave.nx=5;assert(renderWave(badWave,22050,1000).empty());
 std::cout<<"GENERATIVE37 PASS\n";
}
