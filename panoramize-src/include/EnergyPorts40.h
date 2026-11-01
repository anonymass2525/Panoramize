#pragma once
// 4.0: calibrated generalized power ports with SI conjugate pairs.
// Mechanical: (force N, velocity m/s); acoustic: (pressure Pa, volume flow m^3/s).
// Acoustic node q stores displaced volume (m^3), mass stores acoustic inertance.
// Passive connector uses transformer c=1 (mechanical) or 1/area (acoustic),
// potential = k/2*(c_i*q_i-c_j*q_j)^2. Same-step midpoint solve; no z^-1 link.
// This is a reduced-order lumped system, NOT the full spatial FEM or native27 solver.
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
namespace panorama::energy40 {
enum class Domain:int { Mechanical=0, Acoustic=1 };
enum class LinkKind:int { Conservative=0, Dissipative=1, Combined=2 };
struct Node {
 Domain domain=Domain::Mechanical;
 double mass=0.02, frequency=160., damping=.05, area=.02;
 double output=20., drive=0.;
 double x=.1,y=.5; // GUI normalized position only
};
struct Link {int from=0,to=1;LinkKind kind=LinkKind::Combined;double stiffness=120.,damping=.2;};
struct Recipe {
 std::vector<Node> nodes;std::vector<Link> links;
 int exciteNode=0;
 double masterGain=1.;
 double smoothingMs=20.;
};
Recipe example();
bool valid(const Recipe&,std::string* error=nullptr);
std::string serialize(const Recipe&);
bool deserialize(const std::string&,Recipe&,std::string* error=nullptr);
struct Ledger {double initial=0,active=0,parameter=0,dissipated=0,finalEnergy=0,residual=0;};
class Instrument {
public:
 static std::unique_ptr<Instrument> prepare(const Recipe&,double sampleRate,std::string* error=nullptr);
 void noteOn(int midi,float velocity) noexcept;
 void noteOff(int midi) noexcept;
 void pitchBend(float semitones) noexcept;
 void allNotesOff() noexcept;
 // Thread-safe numeric target update (atomic); no topology rebuild or audio-thread allocation.
 bool setLinkTarget(int index,double stiffness,double damping) noexcept;
 bool setNodeTarget(int index,double frequency,double damping) noexcept;
 float processOne() noexcept;
 Ledger ledger() const noexcept;
 double energy() const noexcept;
 double nodeEnergy(int index)const noexcept;
 double nodeVelocity(int index)const noexcept;
 std::uint64_t samples() const noexcept{return samples_;}
 const Recipe& recipe()const noexcept{return recipe_;}
private:
 Instrument()=default;
 static constexpr int MaxN=12;
 Recipe recipe_;
 int n_=0;double dt_=0,alpha_=0;
 std::array<double,MaxN> q_{},v_{},f_{},vmid_{},diagMass_{},nodeK_{},nodeD_{},transform_{};
 std::array<std::atomic<double>,MaxN> targetK_{},targetD_{},baseFrequency_{};
 std::array<double,MaxN*MaxN> matrix_{},chol_{};
 std::array<double,24> linkK_{},linkD_{};
 std::array<std::atomic<double>,24> targetLK_{},targetLD_{};
 double activeWork_=0,parameterWork_=0,dissipated_=0,initialEnergy_=0;
 double pulse_=0;std::atomic<double> bendSemitones_{0.};std::atomic<int> midi_{-1};
 void setNoteFrequencyTargets() noexcept;
 std::uint64_t samples_=0;
 void assemble(std::array<double,MaxN*MaxN>& K,std::array<double,MaxN*MaxN>& D)const noexcept;
 bool solve(const std::array<double,MaxN*MaxN>& lhs,const std::array<double,MaxN>& rhs)noexcept;
};
} // namespace panorama::energy40
