#include "NativeState26.h"
#include <chrono>
#include <iostream>
#include <string>
int main(){
 for(int variant=0;variant<6;variant++){
  std::string error;auto synth=panorama::native26::Instrument::prepare(panorama::native26::example(variant),48000,&error);
  if(!synth){std::cerr<<error;return 2;}
  for(int note=48;note<64;note++)synth->noteOn(note,.7f);
  const auto start=std::chrono::steady_clock::now();
  volatile float output=0;
  for(int n=0;n<4096;n++)output+=synth->processMono();
  const auto end=std::chrono::steady_clock::now();
  const double ms=std::chrono::duration<double,std::milli>(end-start).count();
  std::cout<<"program="<<variant<<" sixteen_voices_4096f_48k_ms="<<ms<<" budget_ms=85.333\n";
 }
}
