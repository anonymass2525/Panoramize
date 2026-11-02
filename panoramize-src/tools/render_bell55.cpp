// Panoramize 5.5: repeatable DRY factory-bell listening references.
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
  auto dir=std::filesystem::path(argc>1?argv[1]:"bell55-previews");
  std::filesystem::create_directories(dir);
  const auto evolved=panorama::factoryPresets()[4];
  auto original=evolved;original.metalMix=0.f;
  auto brighter=evolved;brighter.metalMix=.78f;
  brighter.metalColor=.92f;brighter.metalDecay=.90f;
  writePreview(dir/"bell05_original_dry.wav",original);
  writePreview(dir/"bell05_evolved55_dry.wav",evolved);
  writePreview(dir/"bell05_shimmer55_dry.wav",brighter);
  std::ofstream readme(dir/"README.txt");
  readme<<"Panoramize 5.5 / dry factory Algorithmic Bell A/B\n"
        <<"Three 48kHz stereo 16-bit PCM previews, C5 MIDI 72, velocity 0.82.\n"
        <<"1 second held note, then 2.5 seconds of note-off tail.\n"
        <<"No loudness normalization, no FX, no plate reverb.\n"
        <<"original: metalMix=0; DSP carrier matches previous Fractal Digital path.\n"
        <<"evolved55: metalMix=.57, metalColor=.66, metalDecay=.75.\n"
        <<"shimmer55: metalMix=.78, metalColor=.92, metalDecay=.90.\n"
        <<"Differences are source timbre design alternatives, not proof of superior quality.\n";
  return 0;
 }catch(const std::exception&e){std::cerr<<"Bell55 preview failed: "<<e.what()<<"\n";return 1;}
}
