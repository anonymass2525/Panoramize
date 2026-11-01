#pragma once
#include <array>
#include <cstddef>

namespace panorama {
// A bounded real-time-safe discretization of an arbitrary user-defined 3-D
// spring topology. The node positions are rest geometry; degrees of freedom
// are scalar transverse displacements (NOT full deformable-solid FEM).
constexpr int kMeshMasses=9;
constexpr int kMeshEdges=24;
struct MeshPoint { double x=0, y=0, z=0, mass=1; };
struct MeshEdge { int a=0,b=0; double spring=0; };
struct MeshTopology {
 int count=0, edges=0;
 std::array<MeshPoint,kMeshMasses> points{};
 std::array<MeshEdge,kMeshEdges> links{};
 double anchorSpring=0.; // positive ground stiffness stabilizes rigid motion
};
// Chain, closed ring, square grid; also accepts callers' own mesh topology.
MeshTopology makeMeshTopology(int shape,double baseHz,double tension=1.) noexcept;
class MaterialMesh {
public:
 bool configure(const MeshTopology&,double sampleRate) noexcept;
 void clear() noexcept;
 void excite(int massIndex,double velocity) noexcept;
 // Input force is in N, masses are kg. Outputs are in metres.
 // Implicit midpoint exactly preserves *quadratic mechanical energy* for
 // constant linear conservative springs up to floating-point error.
 double step(double force=0.,int drivenIndex=0) noexcept;
 double displacement(int index) const noexcept;
 double velocity(int index) const noexcept;
 double energy() const noexcept;
 double cumulativeWork() const noexcept {return work_;}
 int size() const noexcept {return n_;}
private:
 int n_=0; double h_=1./48000.,work_=0.;
 MeshTopology topology_{};
 std::array<std::array<double,kMeshMasses>,kMeshMasses> stiffness_{},chol_{};
 std::array<double,kMeshMasses> x_{},v_{};
};
// v1.7: Passivity-accounted mechanical coupling network. All springs
// (including inter-mass couplings) store energy; dashpots dissipate it.
// M and K are constant and positive (semi)definite; C is PSD. With an
// implicit-midpoint step, E_after + cumulativeDissipation - cumulativeWork
// equals the initially stored energy up to floating-point roundoff.
// IMPORTANT: This ledger covers THIS linear mechanical subsystem, not DSP
// waveshaping, envelope, voice mixer or output stages.
struct PassiveMeshSettings {
 MeshTopology topology{};
 std::array<double,kMeshMasses> groundDamping{}; // N s / m
 std::array<double,kMeshEdges> edgeDamping{}; // N s / m
};
class PassiveMesh {
public:
 bool configure(const PassiveMeshSettings&,double sampleRate) noexcept;
 void clear() noexcept;
 void excite(int index,double velocity) noexcept;
 double step(double force=0.,int drivenIndex=0) noexcept;
 double displacement(int index) const noexcept;
 double velocity(int index) const noexcept;
 double energy() const noexcept;
 double externalWork() const noexcept {return work_;}
 double dissipatedEnergy() const noexcept {return dissipation_;}
 int size() const noexcept {return n_;}
private:
 int n_=0; double h_=1./48000.,work_=0.,dissipation_=0.;
 PassiveMeshSettings settings_{};
 std::array<std::array<double,kMeshMasses>,kMeshMasses> stiffness_{},damping_{},chol_{};
 std::array<double,kMeshMasses> x_{},v_{};
};
struct InverseCandidate {
 int shape=0;
 double fundamental=220.,tension=1.,error=0.,scale=0.;
};
struct InverseResult {
 std::array<InverseCandidate,3> best{};
 int evaluated=0;
};
// Offline restricted model search; an ill-posed inverse is NOT unique.
// `samples` must be the isolated onset of a percussive source, float PCM.
InverseResult estimateMeshFromAudio(const float* samples,int count,double sampleRate) noexcept;
} // namespace panorama
