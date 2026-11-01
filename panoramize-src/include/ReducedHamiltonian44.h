#pragma once
// Panoramize 4.4: offline congruent Galerkin model reduction of spatial FEM
// with exact (unreduced) boundary-port degrees of freedom, a prepared block
// preconditioner, and a prepared Schur-complement implicit multirate interface.
// Physical energy and passivity of linear positive M,K,R are retained by T^T A T.
// Interface effort uses a discrete gradient for energy-consistent coupling.
// Reduced interior modes remain approximate. Experimental core.
#include "SparseHamiltonian43.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
namespace panorama::reduced44 {
struct Recipe {
 sparse43::Recipe source;
 int modesPerRegion=6; // interior low-order analytic trial modes per region
 std::vector<int> rateFactor; // 1,2,4 per region, chosen by user; default all 1
 bool multirate=false; // prepared local subcycles + implicit Schur interface
 int maxIterations=100;double relativeTolerance=1e-10;
};
Recipe example();
bool valid(const Recipe&,std::string*reason=nullptr);
std::string serialize(const Recipe&);
bool deserialize(const std::string&,Recipe&,std::string*reason=nullptr);
struct Stats {
 int fullDofs=0,reducedDofs=0,boundaryDofs=0,nnz=0,iterations=0;
 int regions=0;bool multirate=false;std::uint64_t fails=0,frames=0;
 double residual=0;
};
struct Ledger {double initial=0,active=0,dissipated=0,finalEnergy=0,splitError=0; }; // splitError: accounting residual
class Instrument {
public:
 static std::unique_ptr<Instrument> prepare(const Recipe&,double rate,std::string*reason=nullptr);
 void noteOn(int midi,float velocity) noexcept;
 void noteOff(int midi) noexcept;
 void allNotesOff() noexcept;
 float processOne() noexcept;
 Stats stats()const noexcept;
 Ledger ledger()const noexcept;
 double energy()const noexcept;
 double portVelocity(int)const noexcept;
 double regionEnergy(int)const noexcept;
 const Recipe& recipe()const noexcept{return recipe_;}
private:
 Instrument()=default;
 struct Matrix{int n=0;std::vector<int>ptr,index;std::vector<double>val;void apply(const std::vector<double>&x,std::vector<double>&y)const noexcept;};
 struct Coupler{std::vector<std::pair<int,double>> row;double stiffness=0,damping=0;};
 struct Block{int begin=0,count=0,factor=1;std::vector<double>L,internalK,internalR,internalM,transitionQ,transitionV,responseQ,responseV,dissipKernel;};
 Recipe recipe_;
 double h_=0,initial_=0,active_=0,dissipated_=0,pulse_=0;
 int lastMidi_=-1,lastIterations_=0,fullDofs_=0,boundaryDofs_=0;
 std::uint64_t fails_=0,frames_=0;
 std::vector<double>mass_,Mdense_,q_,v_,rhs_,vmid_,res_,z_,dir_,Ap_,scratchK_,scratchR_,diagInverse_,force_;
 Matrix K_,R_,A_;
 std::vector<Block>blocks_;
 std::vector<Coupler>couplers_;
 std::vector<double>schurInverse_,qNext_,vNext_,knownQ_,knownV_,interfaceRhs_,interfaceLambda_,interfaceForce_,localInput_;
 std::vector<std::vector<std::pair<int,double>>> ports_;
 std::vector<double>directK_,directR_; // reduced symmetric dense buffers, setup only (for split local blocks)
 std::vector<std::vector<double>>basis_; // setup-only; drop after preparing (length original DOF)
 std::vector<std::vector<double>>blockFactor_; // prepared block-Jacobi for full implicit A
 bool solve(const Matrix&,const std::vector<double>&b,std::vector<double>&solution,bool blockPrecondition) noexcept;
 bool advanceFull(double duration) noexcept;
 bool advanceImplicitInterface() noexcept;
};
} // namespace panorama::reduced44
