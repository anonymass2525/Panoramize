#pragma once
// Panoramize 3.5: six discrete strings with transverse, longitudinal and
// torsional degrees of freedom, reciprocal attachment to an 8-vertex, six-
// tetrahedron linear FEM soundboard. Experimental small-deformation model.
// NOT a calibrated acoustic guitar / nonlinear large-strain contact solver.
#include <array>
#include <cstdint>
#include <memory>
#include <string>
namespace panorama::coupled35 {
constexpr int kStrings=6, kPoints=24, kVertices=8, kDof=kVertices*3;
struct Model {
 double stringLoss=3.2, longitudinalRatio=.4, torsionRatio=.015;
 double fretGap=.00006, fretStiffness=.6, fretFriction=.22;
 double fingerStiffness=3., fingerDamping=80., palmDamping=280.;
 double meshYoung=5.e8, meshDensity=1.e5, meshPoisson=.30, meshLoss=12.;
 double bridgeCoupling=.08, outputGain=2.0, glideMs=80.;
};
std::string serialize(const Model&);
bool deserialize(const std::string&,Model&);
class Instrument {
public:
 enum class Gesture { Pluck, Hammer, Pull, Slide };
 static std::unique_ptr<Instrument> prepare(const Model&,double sampleRate,std::string* err=nullptr);
 void noteOn(int midi,float velocity) noexcept;
 void noteOff(int midi) noexcept;
 void nextGesture(Gesture g,float strength=1.f) noexcept;
 void pitchBend(float semitones) noexcept;
 void channelPressure(float normalized) noexcept;
 void palmMute(float normalized) noexcept;
 void sustain(bool down) noexcept {sustain_=down;}
 void allNotesOff() noexcept;
 float processOne() noexcept;
 double mechanicalEnergy() const noexcept;
 double meshEnergy() const noexcept;
 double meshDisplacement() const noexcept;
 double lastStringEnergy(int index) const noexcept;
 double axialKineticEnergy() const noexcept;
 double torsionalKineticEnergy() const noexcept;
 std::uint64_t contactCount() const noexcept {return impacts_;}
 int activeStrings() const noexcept;
 int stepsPerSample() const noexcept {return steps_;}
private:
 Instrument()=default;
 struct StringState {
  std::array<double,kPoints> y{}, vy{}, axial{}, vaxial{}, twist{}, vtwist{};
  int midi=-1;bool held=false;double finger=0,target=0,palm=0;
  double kt=0, kl=0, kr=0,bridgeK=0,amp=0,slide=0;
 };
 void evaluateForces(std::array<std::array<double,kPoints>,kStrings>& fy,
  std::array<std::array<double,kPoints>,kStrings>& fa,
  std::array<std::array<double,kPoints>,kStrings>& ft,
  std::array<double,kDof>& meshF) noexcept;
 void integrate(double dt) noexcept;
 double rand01() noexcept;
 std::array<StringState,kStrings> string_{};
 std::array<double,kDof> meshX_{},meshV_{},mass_{},stiffness_{};
 std::array<std::array<double,kDof>,kDof> K_{};
 Model m_{};double sr_=48000,pressure_=0,palm_=0,bend_=0,glideCoeff_=0;
 int steps_=4,lastPlayedString_=-1;bool sustain_=false;
 Gesture gesture_=Gesture::Pluck;float gestureStrength_=1.f;
 std::uint32_t rng_=0x5aa52b17;std::uint64_t impacts_=0;
};
}
