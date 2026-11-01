#pragma once
// v5.2: source graph is prepared off the audio thread, then published as a
// pending shared_ptr. The callback mirrors current MIDI voices into the newly
// prepared graph and crossfades old/new PCM for a bounded window. This is not
// physical-state transfer or a strict wait-free / hard-realtime ownership model.
#include "UniversalInstrument23.h"
#include <array>
#include <atomic>
#include <memory>
namespace panorama::swap52 {
class SmoothGraphSwap {
public:
 using Instrument=universal23::Instrument;
 void publish(std::shared_ptr<Instrument> prepared) noexcept;
 void collectRetired() noexcept; // invoke off the audio thread
 void beginStopped(std::shared_ptr<Instrument> initial) noexcept;
 void setSampleRate(double rate)noexcept;
 panorama::Sample processOne()noexcept;
 void noteOn(int midi,float velocity,int channel=1)noexcept;
 void noteOff(int midi,int channel=1)noexcept;
 void pitchBend(float semitones)noexcept;
 void modWheel(float value)noexcept;
 void sustain(bool down)noexcept;
 void allNotesOff()noexcept;
 void channelPressure(float value)noexcept;
 void polyPressure(int midi,float value)noexcept;
 void articulationControl(int controller,float value)noexcept;
 // UI updates for live controls must not call these directly on an active DSP.
 // Implement host controls in audio callback via currentGraph() only.
 Instrument* currentGraph() const noexcept{return current_.get();}
 Instrument* oldGraph()const noexcept{return fading_.get();}
 bool inTransition()const noexcept{return fading_!=nullptr;}
 int transitionSamplesLeft()const noexcept{return remaining_;}
private:
 struct Held {float velocity=0.f;bool on=false;};
 std::array<Held,16*128> keys_{};
 std::shared_ptr<Instrument> current_,fading_,pending_,retired_;
 int remaining_=0,total_=480;
 float bend_=0.f,wheel_=0.f,pressure_=0.f;
 bool sustain_=false;
 void poll()noexcept;
 static int key(int n,int c)noexcept{return (c-1)*128+n;}
};
}
