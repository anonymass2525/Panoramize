#pragma once
// 3.9: calibrated *active* transducers from spatial mechanical domain velocity
// to native27 power-normalized effort, and back from acoustic mode momentum to
// a mechanical driving force. NOT a passive energy-preserving junction.
// Physical39 is prepared off the audio thread, sample processing is allocation-free.
#include "CoupledPhysics38.h"
#include "NativeState27.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
namespace panorama::unified39 {
enum class Sensor : int { PhysicalVelocityMS=0, AcousticNormalizedFlow=1 };
enum class Actuator : int { NormalizedEffort=0, MechanicalForceN=1 };
struct Transducer {
 int physicalDomain=0, acousticMode=0;
 Sensor physicalSensor=Sensor::PhysicalVelocityMS;
 Actuator acousticActuator=Actuator::NormalizedEffort;
 Sensor acousticSensor=Sensor::AcousticNormalizedFlow;
 Actuator physicalActuator=Actuator::MechanicalForceN;
 double physicalToEffort=100.; // sqrt(J)/s per m/s, active gain (NOT an SI calibration)
 double flowToForce=.1; // N per normalized acoustic flow; active gain
 double effortLimit=10000.,forceLimit=100.; // clip *before* injection
 bool mechanicalToAcoustic=true,acousticToMechanical=true;
};
struct Recipe {
 coupled38::Recipe physics;
 native27::Graph acoustic;
 std::vector<Transducer> links;
 double physicalMix=.7, acousticMix=.3; // output mix only; not mechanical power
};
Recipe example();
std::string serialize(const Recipe&);
bool deserialize(const std::string&,Recipe&,std::string* error=nullptr);
class Instrument {
public:
 static std::unique_ptr<Instrument> prepare(const Recipe&,int sampleRate,std::string*error=nullptr);
 void setOutputMix(double physical,double acoustic) noexcept; // audio-thread readout only; does not change port energetics
 void noteOn(int midi,float velocity) noexcept;
 void noteOff(int midi) noexcept;
 void allNotesOff() noexcept;
 void pitchBend(float semitones) noexcept;
 void modWheel(float value) noexcept;
 bool setNativeModeNormalized(int mode,bool excitation,double normalized) noexcept;
 void clearNativeModeAutomation() noexcept;
 float processOne() noexcept;
 double physicalEnergy() const noexcept;
 double physicalExternalWork() const noexcept;
 double acousticActiveWork() const noexcept;
 double energyResidual() const noexcept;
 double maximumEffort() const noexcept{return maxEffort_;}
 double maximumForce() const noexcept{return maxForce_;}
 std::uint64_t limitedCount() const noexcept{return limited_;}
 const Recipe& recipe() const noexcept{return recipe_;}
private:
 Instrument()=default;
 Recipe recipe_;
 std::unique_ptr<coupled38::Instrument> physical_;
 std::unique_ptr<native27::Instrument> acoustic_;
 int currentMidi_=-1;
 double maxEffort_=0.,maxForce_=0.;
 double physicalMix_=0.,acousticMix_=0.;
 std::uint64_t limited_=0;
};
} // namespace panorama::unified39
