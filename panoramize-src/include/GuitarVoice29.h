#pragma once
// MIDI-playable, allocation-free per sample modal plucked string source.
#include "GuitarInverse29.h"
#include <array>
#include <memory>
#include <string>
namespace panorama::guitar29 {
class Instrument {
public:
 static std::unique_ptr<Instrument> prepare(Model model,double sampleRate,std::string* error=nullptr);
 void noteOn(int note,float velocity) noexcept;
 void noteOff(int note) noexcept;
 void sustain(bool down) noexcept;
 void pitchBend(float semitones) noexcept;
 void allNotesOff() noexcept;
 float processOne() noexcept;
private:
 struct Partial {double re=1.,im=0.,rotR=1.,rotI=0.,decay=1.,weight=0.;};
 struct Voice {std::array<Partial,24> partial{};int count=0;int duration=0;bool active=false,held=false,keyDown=false;double level=0.;};
 Model m_;
 double sr_=48000., bend_=1.;bool pedal_=false;
 std::array<Voice,128> voices_{};
 void setRotation(int midi) noexcept;
};
}
