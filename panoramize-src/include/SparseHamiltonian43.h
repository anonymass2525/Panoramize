#pragma once
// Panoramize 4.3: variable-topology 1D string / 2D scalar membrane /
// 3D vector tetrahedral elastic FEM / 0D acoustic cavity.
// Canonical port-Hamiltonian state (q,p): H = .5*p^T M^-1 p + .5*q^T K q.
// q'=M^-1 p; p'=-Kq-R M^-1 p+B u. Passive links are C^T(k,d)C.
// Implicit midpoint and sparse (CSR) PCG; no allocations in processOne().
// Not a full sparse-direct solver or a guaranteed real-time solution for all meshes.
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>
namespace panorama::sparse43 {
enum class Family:int { String1D=1, Membrane2D=2, Elastic3D=3, Acoustic0D=4, AcousticTube1D=5, ModalResonator=6, SpectralModal=7 };
enum class PortUnit:int { Metres=1, CubicMetres=2, NormalizedModal=3 };
struct Vertex {double x=0,y=0,z=0;};
struct Mode { double frequencyHz=440., damping=0.1; };
struct Region {
 Family family=Family::String1D;
 std::vector<Vertex> vertices;
 std::vector<std::vector<int>> elements; // string segments, triangles, tetrahedra
 double density=.003, tension=80., young=1.e7, poisson=.3, damping=.01;
 // acoustic: density [kg/m^3], soundSpeed [m/s], length [m], area [m²], volume [m³]
 double soundSpeed=343., length=.2, area=.003, volume=.02;
 double gain=1.;
 std::vector<Mode> modes; // resonator/spectral-modal proxy: q [sqrt(J)*s], v [sqrt(J)], unit mass
};
struct PortTerm {int localDof=0; double weight=1.;};
struct Port {int region=0;PortUnit unit=PortUnit::Metres;std::vector<PortTerm> terms;};
struct Link {
 int a=0,b=1;PortUnit unit=PortUnit::Metres;
 double areaA=1.,areaB=1.; // mechanics -> displaced volume when link unit is m^3
 double stiffness=120.,damping=.1;
 double modalScaleA=1., modalScaleB=1.; // (metres or m³)/(sqrt(J)*s) per normalized modal coordinate
};
struct Recipe {
 std::vector<Region> regions;
 std::vector<Port> ports;
 std::vector<Link> links;
 int excitePort=0,outputPort=0;
 double impulseForce=.2,outputGain=800.;
 int maxIterations=120;double relativeTolerance=1e-10;
 int preconditioner=2; // 0=Jacobi, 1=IC(0), 2=auto (>=128 DOF IC0); fallback Jacobi
};
Recipe example();
bool valid(const Recipe&,std::string*error=nullptr);
std::string serialize(const Recipe&);
bool deserialize(const std::string&,Recipe&,std::string*error=nullptr);
struct Ledger {double initial=0,active=0,dissipated=0,finalEnergy=0,residual=0;};
struct Stats {int dofs=0,nnz=0,iterations=0;double residual=0;std::uint64_t failed=0,frames=0;bool ic0=false;};
// Immutable export for offline port-preserving model reduction. M is lumped.
// K and R include passive link contributions and are symmetric positive semidefinite.
struct Assembled {
 int n=0;
 std::vector<int> regionOffset,regionSize,ptr,index;
 std::vector<double> mass,K,R;
 struct PortView {int region;PortUnit unit;std::vector<std::pair<int,double>> weights;};
 struct LinkView {std::vector<std::pair<int,double>> row;double k,d;};
 std::vector<PortView> ports;
 std::vector<LinkView> links;
};
class Instrument {
public:
 static std::unique_ptr<Instrument> prepare(const Recipe&,double sampleRate,std::string*error=nullptr);
 void noteOn(int midi,float velocity) noexcept;
 void noteOff(int midi) noexcept;
 void allNotesOff() noexcept;
 float processOne() noexcept;
 Ledger ledger() const noexcept;
 Stats stats() const noexcept;
 Assembled exportAssembled() const; // prepare thread only, allocates
 double energy() const noexcept;
 double portVelocity(int port) const noexcept;
 double regionEnergy(int region) const noexcept; // internal energy, excluding link energy
 int dofCount() const noexcept {return int(q_.size());}
 const Recipe& recipe() const noexcept{return recipe_;}
private:
 struct Sparse {int n=0;std::vector<int> ptr,index;std::vector<double> val;void apply(const std::vector<double>&x,std::vector<double>&y) const noexcept;};
 struct WeightedPort {PortUnit unit;int region=0;std::vector<std::pair<int,double>> weights;};
 struct RelativeLink {std::vector<std::pair<int,double>> row;double k,d;};
 Recipe recipe_;
 double h_=0,initial_=0,active_=0,dissipated_=0,pulse_=0;
 int lastMidi_=-1,lastIterations_=0;double lastResidual_=0;
 std::uint64_t failed_=0,frames_=0;
 std::vector<int> offsets_,sizes_;
 std::vector<double> mass_,q_,v_,f_,rhs_,vmid_,res_,dir_,z_,Ap_,scratchK_,scratchR_,scratchA_,invDiag_;
 Sparse K_,R_,A_,L_;
 std::vector<double> preBuf_;
 bool ic0_=false;
 void precondition(const std::vector<double>&r,std::vector<double>&out) noexcept;
 std::vector<WeightedPort> ports_;
 std::vector<RelativeLink> links_;
 bool solve() noexcept;
};
} // namespace panorama::sparse43
