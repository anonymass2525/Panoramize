// Panoramize 5.7: Original, Evolved 5.5, Evolved 5.6, Coupled and morph comparisons.
// 48 kHz / stereo 16-bit WAV. C5, velocity 0.82, note-on 0s,
// note-off 1s, render to 3.5s. No reverb, processing or normalization.
#include "PanoramaDSP.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void u16(std::ostream& f,std::uint16_t x){
 const char b[2]={char(x&255u),char((x>>8)&255u)};f.write(b,2);
}
void u32(std::ostream& f,std::uint32_t x){
 const char b[4]={char(x&255u),char((x>>8)&255u),
                 char((x>>16)&255u),char((x>>24)&255u)};f.write(b,4);
}
std::int16_t quantize(float x){
 const double clipped=std::max(-1.,std::min(1.,double(x)));
 return static_cast<std::int16_t>(std::lround(clipped*32767.));
}
void writePreview(const std::filesystem::path& output,panorama::Patch patch){
 constexpr int sr=48000,frames=sr*7/2;
 panorama::Synth synth;synth.prepare(sr);synth.setPatch(patch);synth.noteOn(72,.82f);
 std::ofstream wav(output,std::ios::binary);
 if(!wav)throw std::runtime_error("unable to open preview WAV");
 wav.write("RIFF",4);u32(wav,36u+std::uint32_t(frames)*4u);
 wav.write("WAVEfmt ",8);u32(wav,16);u16(wav,1);u16(wav,2);u32(wav,sr);
 u32(wav,sr*4);u16(wav,4);u16(wav,16);
 wav.write("data",4);u32(wav,std::uint32_t(frames)*4u);
 double peak=0;
 for(int i=0;i<frames;i++){
  if(i==sr)synth.noteOff(72);
  const auto sample=synth.processOne();
  if(!std::isfinite(sample.left)||!std::isfinite(sample.right))
   throw std::runtime_error("NaN or Inf during WAV rendering");
  peak=std::max(peak,std::max(std::abs(double(sample.left)),std::abs(double(sample.right))));
  u16(wav,static_cast<std::uint16_t>(quantize(sample.left)));
  u16(wav,static_cast<std::uint16_t>(quantize(sample.right)));
 }
 if(!wav)throw std::runtime_error("WAV write failed");
 std::cout<<output.string()<<" peak "<<peak<<"\n";
}
}
int main(int argc,char**argv){
 try{
  auto dir=std::filesystem::path(argc>1?argv[1]:"bell57-previews");
  std::filesystem::create_directories(dir);
  const auto bell57=panorama::factoryPresets()[4];
  auto evolved56=bell57;evolved56.bell57Morph=0.f;
  evolved56.bellDynamics=.58f;evolved56.bellCoupling=.34f;evolved56.bellClarity=.62f;
  auto evolved55=evolved56;evolved55.bellDynamics=0.f;
  auto original=evolved55;original.metalMix=0.f;
  auto coupledStrong=evolved56;coupledStrong.bellDynamics=.83f;
  coupledStrong.bellCoupling=.65f;coupledStrong.bellClarity=.83f;
  auto fastBloom=bell57;fastBloom.bell57Bloom=.12f;
  writePreview(dir/"bell05_original_dry.wav",original);
  writePreview(dir/"bell05_evolved55_dry.wav",evolved55);
  writePreview(dir/"bell05_evolved56_dry.wav",evolved56);
  writePreview(dir/"bell05_coupled56_dry.wav",coupledStrong);
  writePreview(dir/"bell05_original_coupled57_dry.wav",bell57);
  writePreview(dir/"bell05_fast_bloom57_dry.wav",fastBloom);
  std::ofstream readme(dir/"README.txt");
  readme<<"Panoramize 5.7 / dry six-way Algorithmic Bell comparison\n"
        <<"48kHz stereo 16-bit PCM, C5 MIDI 72, velocity 0.82.\n"
        <<"1-second held note, then 2.5-second release/tail. No FX/normalization.\n"
        <<"Original: no metalMix; original Fractal Digital carrier.\n"
        <<"Evolved55: 5.5 resonant body with bellDynamics=0 (exact legacy path).\n"
        <<"Evolved56: evolving companion body, energy-neutral inter-mode rotation.\n"
        <<"Coupled56: stronger coupled resonances and clarity; experimental.\n"
        <<"OriginalCoupled57: 5.7 Original-led attack and coupled modal bloom.\n"
        <<"FastBloom57: quicker resonance emergence; compare with 5.7 default.\n"
        <<"No claim of objective superiority; compare at matched loudness.\n";
  return 0;
 }catch(const std::exception&e){std::cerr<<"Bell57 preview failed: "<<e.what()<<"\n";return 1;}
}
