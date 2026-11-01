#pragma once
// 4.7: preserve native27 polyBLEP VA, sinusoidal FM, nonlinear quartic,
// spectral/phase/resonance/vocal normalized physical modes; no proxy replacement.
// Real native27 + spatial coupled38 are run by unified39; their active work is
// measured separately from conservative and dissipative modal coupling.
// The audio tone stage is NOT a passive physical power port: it operates on PCM.
#include "UnifiedAcousticPhysical39.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
namespace panorama::genuine47 {
enum class Source:int {Macro1,Macro2,Macro3,Macro4,Wheel,Pressure,Velocity,KeyTrack,Lfo};
enum class Target:int {Cutoff,Resonance,Drive,Output,PhysicalMix,AcousticMix};
struct Route {Source source=Source::Wheel;Target target=Target::Cutoff;double depth=.2;};
struct Tone {double cutoff=7000.,resonance=.5,drive=1.,output=.9,wet=.8,lfoHz=4.;};
struct Recipe {
 unified39::Recipe original=unified39::example();
 Tone tone;
 std::array<double,4> macros{{.5,.5,.5,.5}};
 std::vector<Route> routes;
};
Recipe example();
bool valid(const Recipe&,std::string*reason=nullptr);
std::string serialize(const Recipe&);
bool deserialize(const std::string&,Recipe&,std::string*reason=nullptr);
struct Meter {double nativeActiveWork=0,physicalActiveWork=0,nativeLedgerResidual=0;double peak=0;std::uint64_t samples=0;};
class Instrument {
public:
 static std::unique_ptr<Instrument> prepare(const Recipe&,int sampleRate,std::string*why=nullptr);
 void noteOn(int midi,float velocity) noexcept;
 void noteOff(int midi) noexcept;
 void allNotesOff() noexcept;
 void pitchBend(float semitones) noexcept;
 void modWheel(float value) noexcept;
 void channelPressure(float value) noexcept;
 void setMacro(int index,float value) noexcept; // CC-style macros, no preparation
 // Stable 4.8 DAW controls: 0..4 tone, 5..8 macros. Only updates existing DSP, no recompile.
 bool setHostParameterNormalized(int index,double normalized) noexcept;
 void clearHostAutomation() noexcept;
 bool setNativeModeNormalized(int mode,bool excitation,double normalized) noexcept;
 void clearNativeModeAutomation() noexcept;
 float processOne() noexcept;
 Meter meter() const noexcept;
 const Recipe& recipe()const noexcept{return recipe_;}
private:
 Instrument()=default;
 Recipe recipe_;
 std::unique_ptr<unified39::Instrument> original_;
 double sr_=48000.,wheel_=0,pressure_=0,velocity_=0,key_=0,lfoPhase_=0;
 double low_=0,band_=0,filteredCutoff_=7000.,filteredQ_=.5,filteredDrive_=1.,filteredOutput_=.9;
 double smoothedPhysical_=.8,smoothedAcoustic_=.18,smoothedWet_=.8;
 std::array<std::atomic<double>,5> toneTargets_{};
 std::atomic<bool> useHostTone_{false};
 std::array<std::atomic<double>,4> macros_{};
 double peak_=0;std::uint64_t frames_=0;
};
} // namespace panorama::genuine47
