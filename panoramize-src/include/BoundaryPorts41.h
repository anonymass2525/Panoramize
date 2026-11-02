#pragma once
// Panoramize 4.1. SI-calibrated discrete boundary power ports.
// 1D string / 2D membrane / 3D tetrahedral linear elasticity + lumped acoustic cavity
// are assembled into ONE midpoint implicit solve. An optional native27 connection
// is EXPLICITLY ACTIVE, causally delayed and excluded from passive claims.
#include "GenerativePhysics37.h"
#include "NativeState27.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
namespace panorama::boundary41 {
struct Recipe {
 int stringPoints=12, membranePoints=3;
 double stringMass=.00036, stringEdgeStiffness=220., stringDamping=.001;
 double membraneMass=.0006, membraneEdgeStiffness=700., membraneDamping=.002;
 gen37::Shape bodyShape{1,1,1,.20,.13,.025,0.,0.};
 double bodyYoung=2.e6, bodyDensity=700., bodyPoisson=.29, bodyDamping=.018;
 double stringMembraneK=130.,stringMembraneD=.008;
 double membraneBodyK=250.,membraneBodyD=.012;
 double pistonArea=.003, cavityVolume=.014, cavityLength=.12;
 double bodyCavityK=1.e7,bodyCavityD=50.; // Pa / m^3, Pa s / m^3
 double soundGain=90., exciteN=.25;
 bool nativeEnabled=false;
 native27::Graph native=native27::example();
 int nativeMode=0;
 double velocityToEffort=0., flowToForce=0.; // active normalization, not SI passive
 double activeForceLimit=1.,activeEffortLimit=500.;
};
Recipe example();
bool valid(const Recipe&,std::string*error=nullptr);
std::string serialize(const Recipe&);
bool deserialize(const std::string&,Recipe&,std::string*error=nullptr);
struct Ledger {
 double initial=0,active=0,dissipated=0,finalEnergy=0,residual=0;
 double nativeActiveWork=0; // reported independently, not a passive conservation claim
};
class Instrument {
public:
 static std::unique_ptr<Instrument> prepare(const Recipe&,double sampleRate,std::string*error=nullptr);
 void noteOn(int midi,float velocity) noexcept;
 void noteOff(int midi) noexcept;
 void pitchBend(float semitones) noexcept;
 void allNotesOff() noexcept;
 float processOne() noexcept;
 double energy() const noexcept;
 Ledger ledger() const noexcept;
 double regionEnergy(int region) const noexcept;
 double boundaryDisplacement(int region) const noexcept;
 double boundaryVelocity(int region) const noexcept;
 double cavityPressure() const noexcept;
 double cavityVolumeFlow() const noexcept;
 int dofCount()const noexcept{return n_;}
 std::uint64_t frames()const noexcept{return frames_;}
 const Recipe& recipe()const noexcept{return recipe_;}
private:
 Instrument()=default;
 struct Port {std::vector<std::pair<int,double>> weights; double area=1.;};
 struct Coupling {Port a,b;double k=0,d=0;};
 Recipe recipe_;
 int n_=0, offsets_[4]{},sizes_[4]{};
 double sr_=48000,dt_=0;
 std::vector<double> mass_,q_,v_,k_,d_,lhs_,chol_,rhs_,vmid_,external_;
 Port ports_[4];
 std::unique_ptr<native27::Instrument> native_;
 int midi_=-1;double delayedMechanical_=0., delayedNative_=0.;
 double initial_=0.,work_=0.,dissipated_=0.;
 std::uint64_t frames_=0;
 void addEntry(std::vector<double>&,int i,int j,double value)noexcept;
 void addRelative(const Port&,const Port&,double k,double d)noexcept;
 bool factorize()noexcept;
 void solve()noexcept;
 double portQ(const Port&)const noexcept;
 double portV(const Port&)const noexcept;
};
} // namespace panorama::boundary41
