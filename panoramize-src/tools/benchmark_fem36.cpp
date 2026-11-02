#include "GeneralizedFEM36.h"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <iterator>
int main(int argc,char**argv){
 if(argc!=2)return 2;
 std::ifstream f(argv[1]);if(!f)return 3;
 panorama::fem36::Model m;
 if(!panorama::fem36::deserialize(std::string((std::istreambuf_iterator<char>(f)),{}),m))return 4;
 auto instrument=panorama::fem36::Instrument::prepare(m,48000);
 if(!instrument)return 5;
 for(int note:{40,45,50,55,59,64})instrument->noteOn(note,.75f);
 volatile double sink=0;
 // Warm up plus 10 repeated blocks.
 for(int i=0;i<2048;i++)sink+=instrument->processOne();
 auto start=std::chrono::steady_clock::now();
 for(int b=0;b<10;b++)for(int i=0;i<2048;i++)sink+=instrument->processOne();
 const auto end=std::chrono::steady_clock::now();
 const double elapsed=std::chrono::duration<double,std::milli>(end-start).count()/10;
 std::cout<<std::fixed<<std::setprecision(4)<<"FEM36 vertices="<<m.mesh.vertices.size()<<" tetra="<<m.mesh.tetrahedra.size()<<" substeps="<<instrument->stepsPerSample()<<" sixstring_ms_per_2048="<<elapsed<<" (budget 42.667ms) sink="<<sink<<'\n';
}
