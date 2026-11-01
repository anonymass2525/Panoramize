#include "RealtimeSafety52.h"
#include "SmoothGraphSwap52.h"
#include "UnifiedControls50.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
using namespace panorama;
int main(){
 using namespace panorama::safe52;
 CoefficientBank b({{.01,1.,.5},{20.,20000.,1000.}},48000,5.);
 assert(!b.publish(0,std::numeric_limits<double>::quiet_NaN()));
 assert(!b.publish(0,1.5));assert(!b.publish(2,.1));
 assert(!b.publishBatch({.7,-1.}));
 assert(b.publishBatch({.9,10000.}));
 double prev=b.current(0);
 for(int i=0;i<600;i++){double a=b.tick(0);assert(a>=prev&&a<=.9000001);prev=a;double c=b.tick(1);assert(c>0&&c<=10000);}
 assert(b.current(0)>.85);
 auto a=universal23::Instrument::prepare(universal23::directGraph(),24000);
 auto g=universal23::genuineNativeGraph();
 auto n=universal23::Instrument::prepare(g,24000);
 assert(a&&n);
 swap52::SmoothGraphSwap engine;
 engine.setSampleRate(24000);engine.beginStopped(a);
 engine.noteOn(57,.7f);
 for(int i=0;i<1000;i++){auto x=engine.processOne();assert(std::isfinite(x.left)&&std::isfinite(x.right));}
 engine.publish(n);
 double energy=0;
 for(int i=0;i<1500;i++){
  if(i==300)engine.noteOff(57);
  auto x=engine.processOne();assert(std::isfinite(x.left)&&std::isfinite(x.right));energy+=x.left*x.left;
 }
 assert(energy>0&&!engine.inTransition()&&engine.currentGraph()==n.get());
 engine.noteOn(60,.65f);engine.allNotesOff();
 auto next=universal23::Instrument::prepare(universal23::directGraph(),24000);
 assert(next);engine.publish(next);
 for(int i=0;i<900;i++)assert(std::isfinite(engine.processOne().left));
 assert(engine.currentGraph()==next.get()&&!engine.inTransition());
 std::cout<<"Safe52 validated coefficients and active-note dual-render patch transition PASS energy="<<energy<<"\n";
}
