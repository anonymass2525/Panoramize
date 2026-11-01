#pragma once
// PANORAMA 2.1: generalized passive 3-D state graph research kernel.
// Dense, pre-factored implicit-midpoint integrator; no allocations in processSample.
// This is a SMALL-DEFORMATION linear 3-DOF mass/link model, NOT general nonlinear FEM.
#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace panorama::acoustic21 {
struct Vec3 { double x=0.,y=0.,z=0.; };
Vec3 operator+(Vec3 a,Vec3 b) noexcept;
Vec3 operator-(Vec3 a,Vec3 b) noexcept;
Vec3 operator*(Vec3 a,double k) noexcept;
enum class Quantity { Displacement, Velocity, Force, SpectralEnergy, PhaseTurns, Audio };
enum class Unit { Metres, MetresPerSecond, Newtons, Joules, Turns, NormalizedAudio };
struct Port { int body=-1; int axis=-1; Quantity quantity=Quantity::Displacement; Unit unit=Unit::Metres; };
Unit nativeUnit(Quantity q) noexcept;
bool validPort(const Port& p,int bodyCount) noexcept;
// Typed conversions MUST be explicit and cannot silently treat energy as force.
struct Conversion { Quantity from=Quantity::Displacement,to=Quantity::Displacement; Unit fromUnit=Unit::Metres,toUnit=Unit::Metres; double gain=1.; };
bool validateConversion(const Conversion& c) noexcept;
// Generic 3x3 anisotropic stiffness/damping tensors. Must be symmetric PSD.
struct Tensor3 { std::array<double,9> a{}; static Tensor3 isotropic(double k) noexcept; };
struct Material3 { Tensor3 stiffness{},damping{}; };
struct Body { std::string name; Vec3 rest{}; double mass=1.; Material3 anchor{}; };
struct Link { int a=-1,b=-1; Material3 coupling{}; };
// Linear small-deformation 4-node tetrahedral FEM (isotropic elastic material).
// Connect as many nondegenerate elements as budget permits. No contacts,
// plasticity, large-deformation geometry updates or nonlinear constitutive laws.
struct Tetrahedron {
 std::array<int,4> vertices{};
 double youngPa=1.e5, poisson=.3, densityKgM3=1000., viscousSeconds=0.;
};
struct EnergyLedger {double mechanical=0, initial=0, work=0, dissipated=0, residual=0;};
struct SpectralDrive { std::array<double,12> energy{}, phaseTurns{}, weights{}; double maxForceNewtons=0.; };
// Explicit non-passive transduction: spectral state -> external Newton force.
// Its mechanical work is accounted for in EnergyLedger.work; no claim that
// the origin spectral oscillator obeys the same energy conservation law.
double spectralDriveForce(const SpectralDrive&) noexcept;
class PreparedGraph;
class StateVoice;
class GraphBuilder {
public:
 int addBody(const Body& b);
 bool addLink(const Link& link);
 bool addTetrahedron(const Tetrahedron& tetra);
 std::shared_ptr<const PreparedGraph> compile(double sampleRate,std::string* error=nullptr) const;
 size_t bodyCount() const noexcept {return bodies_.size();}
private:
 std::vector<Body> bodies_{};
 std::vector<Link> links_{};
 std::vector<Tetrahedron> tetrahedra_{};
};
class PreparedGraph : public std::enable_shared_from_this<PreparedGraph> {
public:
 int bodyCount() const noexcept {return bodies_;}
 int degreesOfFreedom() const noexcept {return dof_;}
 double sampleRate() const noexcept {return sr_;}
 // Pre-allocates a real-time-safe voice; call OUTSIDE the audio callback.
 StateVoice makeVoice() const;
private:
 friend class GraphBuilder;
 friend bool addElasticTetra(PreparedGraph&,const Tetrahedron&,const std::vector<Body>&);
 friend class StateVoice;
 int bodies_=0,dof_=0;
 double sr_=48000.;
 std::vector<double> mass_,stiffness_,damping_,cholesky_;
};
class StateVoice {
public:
 explicit StateVoice(std::shared_ptr<const PreparedGraph> graph);
 void reset() noexcept;
 bool excite(int body,Vec3 velocity) noexcept;
 bool addForce(int body,Vec3 newtons) noexcept;
 bool addForceAtPort(const Port& port,double force) noexcept;
 // All mechanical links, including zero-delay cycles, are solved together.
 // Fixed work memory, no heap, locks, trigonometry or matrix factorization here.
 bool processSample() noexcept;
 Vec3 displacement(int body) const noexcept;
 Vec3 velocity(int body) const noexcept;
 // Safe typed state read. Invalid port yields 0 (check validPort first).
 double readPort(const Port& port) const noexcept;
 EnergyLedger ledger() const noexcept;
 double audioVelocity(int body,Vec3 pickup,double gain=1.) const noexcept;
private:
 std::shared_ptr<const PreparedGraph> graph_;
 std::vector<double> x_,v_,forces_,rhs_,intermediate_,temp_;
 double initial_=0.,work_=0.,dissipation_=0.;
 double mechanicalEnergy() const noexcept;
};
} // namespace panorama::acoustic21
