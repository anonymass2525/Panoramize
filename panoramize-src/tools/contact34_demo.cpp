#include "ContactPhysical34.h"
#include "UniversalInstrument23.h"
#include <chrono>
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
using namespace panorama::contact34;
static void wav(const std::string&file,const std::vector<float>&audio,int sr=48000){
 std::ofstream out(file,std::ios::binary);
 auto w16=[&](uint16_t n){out.put(char(n&255));out.put(char(n>>8));};
 auto w32=[&](uint32_t n){w16(uint16_t(n));w16(uint16_t(n>>16));};
 auto str=[&](const char*p){out.write(p,4);};
 const uint32_t bytes=uint32_t(audio.size()*3);
 str("RIFF");w32(bytes+36);str("WAVE");str("fmt ");w32(16);w16(1);w16(1);w32(sr);w32(sr*3);w16(3);w16(24);
 str("data");w32(bytes);
 for(float x:audio){const int32_t v=int32_t(std::lround(std::clamp(double(x),-.999999,.999999)*8388607.));
  out.put(char(v&255));out.put(char((v>>8)&255));out.put(char((v>>16)&255));}
}
int main(int argc,char**argv){
 if(argc<2){std::cerr<<"usage: panoramize_contact34_demo out_dir\n";return 2;}
 const std::string path=argv[1];const int rate=48000;
 const std::array<std::string,6> names={"01_Open_String", "02_Hammer_On", "03_Pull_Off", "04_Moving_Fret_Slide", "05_Palm_Mute", "06_Bridge_Body_Coupling"};
 for(int mode=0;mode<6;mode++){
  Model m; m.openMidi=45;m.openHz=110;
  if(mode==5){m.bodyCoupling=.45;m.bodyHzX=180;m.bodyHzY=310;m.bodyHzZ=530;}
  auto p=Instrument::prepare(m,rate);if(!p)return 3;
  std::vector<float> samples; samples.reserve(rate*2);
  for(int i=0;i<rate*2;i++){
   if(i==2400)p->noteOn(mode==2?57:45,.9f);
   if(i==rate/3){
    if(mode==1){p->nextGesture(Instrument::Gesture::Hammer,1.f);p->noteOn(52,.92f);}
    if(mode==2){p->nextGesture(Instrument::Gesture::Pull,1.f);p->noteOn(45,.92f);}
    if(mode==3){p->nextGesture(Instrument::Gesture::Slide,1.f);p->noteOn(57,.9f);}
    if(mode==4)p->palmMute(1.f);
    if(mode==5){p->channelPressure(.55f);}
   }
   if(i==rate+rate/3)p->noteOff(mode==1?52:mode==3?57:45);
   samples.push_back(p->processOne()*.8f);
  }
  wav(path+"/"+names[size_t(mode)]+".wav",samples);
  double rms=0;for(float x:samples)rms+=double(x)*x;rms=std::sqrt(rms/samples.size());
  std::cout<<names[size_t(mode)]<<" RMS "<<rms<<" contacts "<<p->fretContactCount()<<" energy "<<p->energy()<<"\n";
 }
 // Universal Patchbay patch for Studio One. The GUI loads it as .pgraph.
 const auto graph=panorama::universal23::contactGraph();
 std::ofstream f(path+"/Panoramize_Spatial_String_34.pgraph");f<<panorama::universal23::serialize(graph);
 auto m=graph.contact34;std::ofstream model(path+"/Panoramize_Contact_Settings_34.contact34");model<<serialize(m);
 // measure independent voices, including true spatial solver work (not DAW or GUI).
 std::ofstream bench(path+"/benchmark.csv");bench<<"voices,frames,sample_rate,elapsed_ms,buffer_budget_ms,ratio\n";
 for(int voices:{1,4,8,16}){
  std::vector<std::unique_ptr<Instrument>> v;
  for(int i=0;i<voices;i++){auto q=Instrument::prepare(m,rate);q->noteOn(45+i%5,.8f);v.emplace_back(std::move(q));}
  volatile float sink=0;const int frames=2048;
  auto t0=std::chrono::steady_clock::now();
  for(int i=0;i<frames;i++)for(auto& q:v)sink=sink+q->processOne();
  const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t0).count();
  bench<<voices<<','<<frames<<','<<rate<<','<<ms<<','<<frames*1000./rate<<','<<ms/(frames*1000./rate)<<'\n';
  std::cout<<"benchmark "<<voices<<" voices "<<ms<<" ms, ratio "<<ms/(frames*1000./rate)<<"\n";
 }
}
