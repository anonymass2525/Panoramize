#pragma once
// Panoramize 3.4 - spatially discrete transverse string + moving finger/fret
// unilateral contact + reciprocal bridge/3-axis body coupling.
// This is a physical model approximation, not a validated real guitar FEM.
#include <array>
#include <cstdint>
#include <memory>
#include <string>

namespace panorama::contact34 {
constexpr int kNodes=40;
struct Model {
 double openHz=110.;             // one open string, no artificially retuned partial bank
 int openMidi=45;               // A2 by default
 double stringLoss=2.8;          // velocity damping per second
 double fingerStiffness=4.;      // multiples of nearest-neighbor string spring
 double fingerDamping=180.;      // moving fretted clamp, 1/s
 double fretGap=.00011;          // unilateral fret-board gap (metres proxy)
 double fretStiffness=.5;        // multiples of string spring
 double fretDamping=25.;
 double bridgeHz=520.;
 double bridgeMass=3.;
 double bridgeLoss=24.;
 double bodyHzX=205.,bodyHzY=320.,bodyHzZ=480.;
 double bodyMass=6.;
 double bodyLoss=14.;
 double bodyCoupling=.18;       // dimensionless spring ratio
 double pickPosition=.78;        // near bridge
 double palmPosition=.94;
 double palmLoss=170.;
 double slideFriction=.12;
 double outputGain=1.8;
 double glideMs=85.;
};
std::string serialize(const Model&);
bool deserialize(const std::string&,Model&);

class Instrument {
public:
 static std::unique_ptr<Instrument> prepare(const Model&,double sampleRate,std::string* error=nullptr);
 enum class Gesture { Pluck, Hammer, Pull, Slide };
 void noteOn(int note,float velocity) noexcept;
 void noteOff(int note) noexcept;
 void pitchBend(float semitones) noexcept;
 void channelPressure(float amount) noexcept;
 void polyPressure(int midi,float amount) noexcept;
 void palmMute(float amount) noexcept;
 void palmPosition(float location) noexcept;
 void nextGesture(Gesture g,float force=1.f) noexcept;
 void gestureIntensity(float force) noexcept;
 void sustain(bool down) noexcept {sustain_=down;}
 void allNotesOff() noexcept;
 float processOne() noexcept;
 double energy() const noexcept;
 double bridgeVelocity() const noexcept {return bv_;}
 double bodyVelocity() const noexcept{return bodyV_[0];}
 double fingerPosition() const noexcept{return fingerX_;}
 double lastInputPosition() const noexcept{return lastPickX_;}
 double accumulatedDissipation() const noexcept{return lossEnergy_;}
 unsigned long long fretContactCount() const noexcept{return fretContacts_;}
 int stepsPerSample() const noexcept{return steps_;}
 int noteCount() const noexcept{return attacks_;}
 double outputRmsProxy() const noexcept{return std::abs(bodyV_[0]);}
private:
 Instrument()=default;
 void forces(std::array<double,kNodes>& f,double& fb,std::array<double,3>& fbody) const noexcept;
 void integrate(double dt) noexcept;
 void strike(float velocity) noexcept;
 double signedNoise() noexcept;
 static double location(int midi,int openMidi,double bend) noexcept;
 Model m_{};
 double sr_=48000.,stiffness_=0.,fretK_=0.,fingerK_=0.;
 double bridgeK_=0.,glideCoef_=0.,dampBase_=0.,dampRelease_=0.,dampFret_=0.,dampBridge_=0.,dampBody_=0.,halfStep_=0.;
 std::array<double,3> bodyK_{},couplingK_{};
 int steps_=6, note_=-1, attacks_=0;
 bool active_=false,held_=false,sustain_=false;
 Gesture gesture_=Gesture::Pluck;
 float gestureForce_=1.f;
 double pressure_=0.,palm_=0.,palmX_=0.94,bend_=0.,fingerX_=0.,targetX_=0.,slideIntensity_=0.;
 double lastPickX_=0.78, bridge_=0.,bv_=0.;
 std::array<double,kNodes> x_{},v_{};
 std::array<double,3> bodyX_{},bodyV_{};
 double lossEnergy_=0.;unsigned long long fretContacts_=0;uint32_t seed_=0x9e3779b9u;
};
}
