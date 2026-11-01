#include "GeneralizedFEM36.h"
#include <cassert>
#include <algorithm>
#include <cmath>
#include <iostream>
using namespace panorama::fem36;
int main(){
 Model m;std::string error;Model b;
 assert(deserialize(serialize(m),b));
 assert(serialize(m)==serialize(b));
 assert(m.mesh.vertices.size()==8 && m.mesh.tetrahedra.size()==6);
 auto inst=Instrument::prepare(m,48000,&error);
 if(!inst){std::cerr<<error<<'\n';return 1;}
 inst->noteOn(40,.7f);
 double sum=0;
 for(int i=0;i<1200;i++){double x=inst->processOne();assert(std::isfinite(x));sum+=x*x;}
 assert(sum>1e-12);assert(inst->meshEnergy()>0);
 auto custom=m;
 // Arbitrary tetrahedral topology: add one connected vertex and one tetrahedron.
 custom.mesh.vertices.push_back({.1,.06,.14});
 custom.mesh.tetrahedra.push_back({4,5,6,8});
 custom.mesh.bridgeVertex={1,2,3,5,6,8};
 custom.anisotropy={1.3,.65,1.8,.8,1.4,.55};
 assert(deserialize(serialize(custom),b));
 assert(serialize(custom)==serialize(b));
 auto other=Instrument::prepare(custom,48000,&error);
 if(!other){std::cerr<<error<<'\n';return 2;}
 other->noteOn(40,.7f);double delta=0;
 for(int i=0;i<1200;i++){const double x=other->processOne();assert(std::isfinite(x));delta+=std::abs(x);}
 assert(delta>1e-7);
 auto invalid=custom;invalid.mesh.tetrahedra.push_back({0,0,1,2});
 assert(!Instrument::prepare(invalid,48000));
 invalid=custom;invalid.anisotropy[0]=-1;assert(!Instrument::prepare(invalid,48000));
 invalid=custom;invalid.mesh.vertices[8]=invalid.mesh.vertices[4];assert(!Instrument::prepare(invalid,48000));
 // The old 3.5 engine is untouched; new 3.6 objects are independent.
 std::cout<<"FEM36 PASS default="<<sum<<" customized="<<delta<<" vertices="<<custom.mesh.vertices.size()<<"\n";
}
