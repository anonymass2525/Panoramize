#include "ReducedHamiltonian44.h"
#include "SparseHamiltonian43.h"
#include "reduced44_large.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <vector>
using namespace panorama;
int main(){
 std::cout<<"model,integrator,full_dofs,reduced_dofs,nnz,block64_median_us,block64_p95_us,failed,split_residual_J\n";
 volatile double checksum=0;
 for(int size=0;size<2;size++){
  auto recipe=size?reduced44::largeExample():reduced44::example();
  for(int mode=0;mode<3;mode++){
   auto benchmark=[&](auto&x){
    x->noteOn(57,.85f);for(int k=0;k<1024;k++)checksum+=x->processOne();
    std::vector<double>spans;spans.reserve(120);
    for(int k=0;k<120;k++){
     auto start=std::chrono::steady_clock::now();
     for(int i=0;i<64;i++)checksum+=x->processOne();
     spans.push_back(std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count());
    }
    std::sort(spans.begin(),spans.end());return std::pair<double,double>{spans[60],spans[114]};
   };
   if(mode==0){
    auto x=sparse43::Instrument::prepare(recipe.source,48000);if(!x){std::cerr<<"43 prepare failed\n";return 2;}
    auto times=benchmark(x);auto stats=x->stats();
    std::cout<<(size?"large":"base")<<",sparse43,"<<stats.dofs<<','<<stats.dofs<<','<<stats.nnz<<','<<times.first<<','<<times.second<<','<<stats.failed<<','<<x->ledger().residual<<'\n';
   }else{
    recipe.multirate=mode==2;
    auto x=reduced44::Instrument::prepare(recipe,48000);if(!x){std::cerr<<"44 prepare failed\n";return 3;}
    auto times=benchmark(x);auto stats=x->stats();
    std::cout<<(size?"large":"base")<<','<<(mode==1?"reduced_single":"reduced_multirate")<<','<<stats.fullDofs<<','<<stats.reducedDofs<<','<<stats.nnz<<','<<times.first<<','<<times.second<<','<<stats.fails<<','<<x->ledger().splitError<<'\n';
   }
  }
 }
 std::cerr<<"checksum="<<checksum<<'\n';
}
