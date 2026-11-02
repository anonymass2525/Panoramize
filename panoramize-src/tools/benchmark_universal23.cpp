#include "UniversalInstrument23.h"
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
using namespace panorama::universal23;
int main(){
 for(const int voices:{1,4,8,16})for(const int state:{0,1}){
  Graph g=state?defaultGraph():Graph{"Legacy only",{{Kind::Legacy,-1,-1,52,.8f},{Kind::Output,0,-1,0,.8f}}};
  std::string error;
  auto instrument=Instrument::prepare(g,48000,&error);
  if(!instrument){std::cerr<<error<<'\n';return 1;}
  for(int i=0;i<voices;i++)instrument->noteOn(48+i,.8f);
  for(int i=0;i<128;i++)instrument->processOne();
  constexpr int n=2048;
  auto start=std::chrono::steady_clock::now();
  volatile float sink=0;
  for(int i=0;i<n;i++)sink+=instrument->processOne().left;
  auto stop=std::chrono::steady_clock::now();
  const double ms=std::chrono::duration<double,std::milli>(stop-start).count();
  std::cout<<(state?"Horizon Matter":"Legacy only")<<","<<voices<<","<<std::fixed<<std::setprecision(3)<<ms<<","<<100*ms/(1000.*n/48000.)<<"%\n";
 }
}
