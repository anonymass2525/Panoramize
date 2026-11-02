#include "NonlinearPassive45.h"
#include <chrono>
#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>
using namespace panorama;
int main(){
 std::cout<<"variant,dofs,native_modes,block_frames,measured_median_us,budget_us,utilization\n";
 for(int v=0;v<3;v++){
  auto r=passive45::example();
  if(v==1){for(auto&x:r.native.modes)x.excitation=0.;r.strictPassive=true;}
  if(v==2){r.contacts.clear();r.stretchEdges.clear();}
  std::string why;auto instrument=passive45::Instrument::prepare(r,48000,&why);
  if(!instrument){std::cerr<<why<<'\n';return 2;}
  instrument->noteOn(57,.7f);
  volatile double checksum=0;
  for(int i=0;i<160;i++)checksum+=instrument->processOne();
  std::vector<double>times;
  for(int b=0;b<90;b++){
   auto t=std::chrono::steady_clock::now();
   for(int i=0;i<64;i++)checksum+=instrument->processOne();
   auto e=std::chrono::steady_clock::now();
   times.push_back(std::chrono::duration<double,std::micro>(e-t).count());
  }
  std::sort(times.begin(),times.end());const double med=times[times.size()/2];
  auto st=instrument->stats();
  std::cout<<v<<','<<st.dofs<<','<<st.nativeModes<<",64,"<<med<<",1333.333,"<<med/1333.333<<'\n';
  std::cerr<<"checksum="<<checksum<<" failed="<<st.failures<<'\n';
 }
}
