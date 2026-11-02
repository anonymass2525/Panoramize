#pragma once
// Panoramize 3.2 - MIDI expressive, limited modal-string/bridge/body instrument.
// A controllable physical proxy, not an accurately calibrated real guitar.
#include "ExpressivePhysical32.h"
#include <array>
#include <cstdint>
#include <memory>
#include <string>
namespace panorama::articulated33 {
struct Model : public expressive32::Model {
 double hammerImpact=0.22; // at fretted boundary, fraction of normalized impulse
 double pullImpact=0.24; // tangential release impulse
 double slideFriction=0.25; // fretboard contact-noise excitation
 double palmDamping=140.; // per-second, mode-dependent bridge-side loss
 double palmPosition=0.91; // fraction of string length from nut

 performance30::CoupledModel physical{};
 bool monoLegato=true;
 double glideMs=55., bendSmoothingMs=6.;
 double velocityPickRange=.12; // fraction of string length, around base pick position
 double pressureContactDamping=34.; // extra inverse seconds at full pressure
 double releaseContactDamping=11.; // extra inverse seconds on released string
 double velocityForceCurve=1.45;
};
std::string serialize(const Model&);
bool deserialize(const std::string&,Model&);
class Instrument {
public:
 static std::unique_ptr<Instrument> prepare(const Model&,double sampleRate,std::string* error=nullptr);
 void noteOn(int midi,float velocity) noexcept;
 void noteOff(int midi) noexcept;
 void pitchBend(float semitones) noexcept;
 void channelPressure(float pressure) noexcept;
 void polyPressure(int midi,float pressure) noexcept;
 void sustain(bool down) noexcept;
 // Physical approximations. All operations are real-time safe and allocation-free.
 enum class Transition { Legato=0, HammerOn=1, PullOff=2, Slide=3 };
 void setNextTransition(Transition mode,float intensity=1.f) noexcept;
 void setTransitionIntensity(float intensity) noexcept;
 void hammerOn(int midi,float intensity=1.f) noexcept;
 void pullOff(int midi,float intensity=1.f) noexcept;
 void slideTo(int midi,float friction=1.f) noexcept;
 void palmMute(float depth) noexcept;
 void setPalmPosition(float x) noexcept;
 void allNotesOff() noexcept;
 float processOne() noexcept;
 int attackCount() const noexcept{return attacks_;}
 int legatoCount() const noexcept{return legatos_;}
 double lastPickPosition() const noexcept{return lastPick_;}
 double currentNotePitch() const noexcept{return voice_[0].pitchMidi;}
 int activeVoiceCount() const noexcept;
 int hammerCount() const noexcept{return hammers_;}
 int pullCount() const noexcept{return pulls_;}
 int slideCount() const noexcept{return slides_;}
 double palmDepth() const noexcept{return palmDepth_;}
private:
 struct Partial {double re=1.,im=0.,rotR=1.,rotI=0.,baseWeight=0.;};
 struct Voice {std::array<Partial,24> partial{};int note=-1,count=0;bool active=false,held=false;
  double pitchMidi=60.,targetMidi=60.,level=0.,pick=0.,age=0.,slideFriction=0.;
  uint32_t rng=0x12345678u;};
 struct Filter {double a=0.,b=0.,c=0.,d=0.,scale=0.;
  double tick(double input) noexcept {const double y=c*a+d*b+scale*input;b=a;a=y;return y;}};
 struct Bank {std::array<Filter,3> modes{};double amount=0.;};
 Model config_{};double sr_=48000.,pitchBend_=0.,smoothBend_=0.,globalPressure_=0.;
 std::array<Voice,16> voice_{};
 std::array<bool,128> held_{};
 std::array<double,128> pressure_{};
 std::array<unsigned long long,128> order_{};
 unsigned long long eventCounter_=0;
 Bank bridge_,body_;
 bool sustain_=false;int attacks_=0,legatos_=0,hammers_=0,pulls_=0,slides_=0;
 Transition pending_=Transition::Legato;float pendingIntensity_=1.f;
 double palmDepth_=0.,palmSmooth_=0.,palmX_=0.91;
 double lastPick_=0.;unsigned tick_=0;
 void strike(Voice&,int midi,float velocity) noexcept;
 void articulate(Voice&,int note,float intensity,Transition mode) noexcept;
 void updateRotation(Voice&) noexcept;
 double bankTick(Bank&,double) noexcept;
};
}
