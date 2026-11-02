#include "RealtimeSafety52.h"
#include <cassert>
#include <cmath>
#include <limits>
#include <iostream>
int main(){
 using namespace panorama::safe52;
 CoefficientBank b({{0.,1.,.5},{100.,10000.,1000.}},48000.,5.);
 assert(!b.publish(4,.1));
 assert(!b.publish(0,std::numeric_limits<double>::infinity()));
 assert(!b.publishBatch({.4,10001.}));
 assert(b.publishBatch({1.,9000.}));
 double last=.5;
 for(int i=0;i<5000;++i){auto x=b.tick(0);assert(x>=last&&x<=1.);last=x; auto y=b.tick(1);assert(y>0&&y<=9000.);}
 assert(last>.999);
 std::cout<<"ASan/UBSan coefficient validation and smoothing PASS\n";
}
