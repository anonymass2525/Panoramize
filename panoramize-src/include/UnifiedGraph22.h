#pragma once
// PANORAMA 2.2: generalized, energy-normalized acoustic state graph research kernel.
// Power coordinates: q [sqrt(J)*s], p [sqrt(J)], E = .5 p'p + .5 q'Kq + sum lambda*q^4/4.
// Cross-domain connections are calibrated *normalized* couplings. They are NOT a claim
// that raw Pascal/metre/spectral magnitudes can be equated without a transducer model.
// A finite graph is compiled OFF the audio thread; a voice processes without allocation.
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
namespace panorama::unified22 {
enum class Domain { Physical, Spectral, Phase, Resonance, Nonlinear, VocalFormant };
enum class Quantity { NormalizedCoordinate, NormalizedFlow, Effort, Audio, Displacement, Velocity, Pressure, SpectralAmplitude, PhaseRadians };
enum class Unit { RootJouleSeconds, RootJoule, RootJoulePerSecond, NormalizedAudio, Metre, MetrePerSecond, Pascal, RelativeAmplitude, Radians };
enum class LinkKind { Conservative, Dissipative };
struct Port {int mode=-1; Quantity quantity=Quantity::NormalizedCoordinate; Unit unit=Unit::RootJouleSeconds;};
Unit nativeUnit(Quantity q) noexcept;
struct Mode {
 std::string name;
 Domain domain=Domain::Resonance;
 double frequencyHz=200., dampingPerSecond=1.;
 // Positive quartic potential coefficient in J/(sqrt(J)*s)^4. 0 = linear.
 double quartic=0.;
 // Dimensioned sensor mapping for non-energy-state physical/acoustic readouts.
 double displacementMetresPerQ=0., pressurePascalsPerQ=0.;
 double spectralAmplitudePerQ=0., phaseRadiansPerQ=0.;
};
struct Link {int a=-1,b=-1; LinkKind kind=LinkKind::Conservative;
 // Conservative: .5*k*(q_a-ratio*q_b)^2. Dissipative: c*(p_a-ratio*p_b)^2.
 double strength=0., ratio=1.;};
struct EnergyLedger {double stored=0., initial=0., activeWork=0., dissipated=0., residual=0.;};
class Voice;
class PreparedGraph : public std::enable_shared_from_this<PreparedGraph> {
public:
 size_t modeCount() const noexcept {return modes_.size();}
 double sampleRate() const noexcept {return sr_;}
 Voice makeVoice() const;
private:
 friend class Builder;
 friend class Voice;
 std::vector<Mode> modes_;
 std::vector<double> K_,D_,A_,factor_;
 double sr_=48000.;
 bool nonlinear_=false;
};
class Builder {
public:
 int addMode(Mode mode);
 bool connect(const Link& link);
 // Only power-normalized coordinate ports may form a conservative/dissipative link.
 // Raw SI physical and audio/spectral observables require calibrated transducers.
 bool connectTyped(const Port& a,const Port& b,LinkKind kind,double strength,double ratio=1.);
 std::shared_ptr<const PreparedGraph> compile(double sampleRate,std::string* error=nullptr) const;
 size_t modeCount() const noexcept {return modes_.size();}
private:
 std::vector<Mode> modes_;
 std::vector<Link> links_;
};
class Voice {
public:
 explicit Voice(std::shared_ptr<const PreparedGraph> prepared);
 void reset() noexcept;
 // Initial energy is established by exciting the voice before processing.
 bool excite(int mode,double velocity) noexcept;
 // Active force: [sqrt(J)/s] applied for one sample; accounted as work.
 bool addActiveForce(int mode,double force) noexcept;
 bool processSample() noexcept;
 bool finite() const noexcept;
 double read(const Port& port) const noexcept;
 double audioSample(int mode,double gain=1.) const noexcept;
 EnergyLedger ledger() const noexcept;
private:
 std::shared_ptr<const PreparedGraph> graph_;
 std::vector<double> q_,p_,forces_,source_,rhs_,tmp_,solution_,matrix_,chol_,qnext_;
 double initial_=0.,active_=0.,loss_=0.;
 bool processed_=false;
 double storedEnergy() const noexcept;
};
} // namespace panorama::unified22
