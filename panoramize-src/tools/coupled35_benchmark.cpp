#include "CoupledGuitar35.h"
#include <array>
#include <chrono>
#include <iostream>
#include <vector>
int main(){
 for(int n:{1,2,4,8}){
  std::vector<std::unique_ptr<panorama::coupled35::Instrument>> instruments;
  for(int j=0;j<n;j++){
   auto ins=panorama::coupled35::Instrument::prepare({},48000);
   for(int midi:{40,45,50,55,59,64})ins->noteOn(midi,.7f);
   instruments.emplace_back(std::move(ins));
  }
  volatile float sink=0;
  for(int i=0;i<512;i++)for(auto&ins:instruments)sink+=ins->processOne();
  std::array<double,3> wall{};
  for(int rep=0;rep<3;rep++){
   auto start=std::chrono::steady_clock::now();
   for(int i=0;i<2048;i++)for(auto&ins:instruments)sink+=ins->processOne();
   auto end=std::chrono::steady_clock::now();
   wall[size_t(rep)]=std::chrono::duration<double,std::milli>(end-start).count();
  }
  std::cout<<n<<" x six-string instruments: "<<(wall[0]+wall[1]+wall[2])/3<<" ms / 2048 @48kHz, budget 42.67 ms\n";
 }
}
