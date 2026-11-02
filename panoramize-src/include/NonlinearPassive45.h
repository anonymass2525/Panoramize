#pragma once
// Panoramize 4.5: combined spatial FEM + Native27 *passive modal surrogate*.
// DOES NOT claim arbitrary powered VA/FM/nonlinear Native27 implementations
// are passive. Such generators are explicitly active, supplied and metered.
// Discrete-gradient contact, friction and finite-strain tetrahedral edge energies.
#include "SparseHamiltonian43.h"
#include "NativeState27.h"
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
namespace panorama::passive45 {
struct Contact {
 int normalPort=0,tangentPort=0;
 double gap=1e-7,stiffness=2000,friction=0.05,slipSpeed=1e-4;
};
struct StretchEdge {int region=0,vertexA=0,vertexB=1;double stiffness=100.;};
struct Recipe {
 sparse43::Recipe spatial;
 native27::Graph native;
 std::vector<Contact> contacts;
 std::vector<StretchEdge> stretchEdges;
 double modalTransformer=.01,modalCoupling=50.;
 bool strictPassive=false; // reject native powered modes, rather than silently relabeling
 int nonlinearIterations=12,linearIterations=150;
 double nonlinearTolerance=1e-9;
};
Recipe example();
std::string serialize(const Recipe&);
bool deserialize(const std::string&,Recipe&,std::string*why=nullptr);
bool validate(const Recipe&,std::string*why=nullptr);
struct Ledger {double initial=0,active=0,dissipation=0,finalEnergy=0,residual=0;};
struct Stats {int dofs=0,contacts=0,stretches=0,nativeModes=0,activeModes=0;
 std::uint64_t frames=0,failures=0,contactsEngaged=0;
 double lastError=0;};
class Instrument {
public:
 static std::unique_ptr<Instrument> prepare(const Recipe&,double sampleRate,std::string*why=nullptr);
 void noteOn(int midi,float velocity)noexcept;
 void noteOff(int midi)noexcept;
 void allNotesOff()noexcept;
 float processOne()noexcept;
 Ledger ledger()const noexcept;
 Stats stats()const noexcept;
 double energy()const noexcept;
 double contactPotential()const noexcept;
 double stretchPotential()const noexcept;
 double modalPotential()const noexcept;
 double regionEnergy(int region)const noexcept;
 std::array<double,3> deformedVertex(int region,int vertex)const noexcept; // meters; fixed topology
private:
 Instrument()=default;
 struct Row {std::vector<std::pair<int,double>> terms;};
 struct ContactOp {Row normal,tangent;double gap,k,mu,v0;};
 struct StretchOp {int a,b;double rest,k;double ref[3];};
 Recipe recipe_;
 sparse43::Assembled full_;
 std::vector<Row> ports_;
 std::vector<ContactOp> contacts_;
 std::vector<StretchOp> stretches_;
 std::vector<double> q_,v_,trial_,vmid_,rhs_,res_,dir_,z_,Ap_,diag_,grad_,lossForce_,force_,scratchK_,scratchR_,scratchA_,qnext_;
 std::vector<double> origModeStiffness_,origModeDamping_;
 double h_=0,initial_=0,active_=0,diss_=0,pulse_=0,phase_=0;
 int midi_=-1,excitePort_=0,outputPort_=0,nativeOffset_=0;
 Stats stats_;
 void apply(const std::vector<double>&matrix,const std::vector<double>&x,std::vector<double>&y)const noexcept;
 double dot(const std::vector<double>&a,const std::vector<double>&b)const noexcept;
 double rowValue(const Row&,const std::vector<double>&x)const noexcept;
 void nonlinear(const std::vector<double>&next,const std::vector<double>&midVel)noexcept;
 bool linearSolve(const std::vector<double>&b)noexcept;
 double elasticEnergy(const std::vector<double>&q,const std::vector<double>&v)const noexcept;
};
} // namespace panorama::passive45
