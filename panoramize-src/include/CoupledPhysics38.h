#pragma once
// Panoramize 3.8: physically coupled spatial 1D / 2D / tetrahedral 3D network.
// Offline research kernel, NOT calibrated full guitar, room acoustics or realtime VST3.
#include "GenerativePhysics37.h"
#include <cstddef>
#include <array>
#include <memory>
#include <string>
#include <vector>
namespace panorama::coupled38 {
using Family=gen37::Family;
struct Domain {
 Family family=Family::String1D;
 int nx=24,ny=12; // point counts for 1D / 2D
 double fundamentalHz=180., damping=1.0, exciter=.34, pickup=.64, amplitude=.00012;
 gen37::Shape shape{}; // tetrahedral 3D only
 double young=5.e8,density=1.e5,poisson=.3;
 std::array<double,6> anisotropy{{1,1,1,1,1,1}}; // xx,yy,zz,yz,xz,xy strain factors
};
struct Connection {
 int from=0,to=1;
 double positionFrom=.76,positionTo=.50; // fractional pickup/attachment location
 int axisFrom=1,axisTo=1; // tetra mesh: 0=x,1=y,2=z; others ignore
 double stiffness=1.e6,damping=30.,ratio=1.; // U=1/2*k*(xa-ratio*xb)^2, SI effort units
};
struct Recipe {
 std::vector<Domain> domains;
 std::vector<Connection> connections;
 int exciteDomain=0;
 double outputGain=1500.;
};
std::string serialize(const Recipe&);
bool deserialize(const std::string&,Recipe&,std::string*error=nullptr);
class Instrument {
public:
 static std::unique_ptr<Instrument> prepare(const Recipe&,int rate,std::string*error=nullptr);
 // Real-time update: no heap allocation, dynamic graph topology prepared beforehand.
 float processOne() noexcept;
 void excite(int domain,double displacement) noexcept;
 bool addExternalForce(int domain,double newtons) noexcept; // active effort, applied through next full audio sample
 double domainVelocity(int domain) const noexcept; // m/s (calibration approximate)
 double externalWork() const noexcept {return externalWork_;}
 double totalEnergy() const noexcept;
 double domainEnergy(int domain) const noexcept; // excludes cross-domain coupling energy
 double couplingEnergy() const noexcept;
 double dissipatedEnergy() const noexcept {return dissipated_;}
 int substeps() const noexcept {return substeps_;}
 double domainPickup(int index) const noexcept;
 int domainCount() const noexcept {return int(domains_.size());}
private:
 struct Spring {int a,b;double k;}; // b=-1 is ground
 struct ElasticTerm {int a,b;double k;}; // full symmetric linear tetrahedral FEM K entries
 struct Coupler {int a,b;double k,d,ratio;};
 struct Region {Family family;int offset=0,count=0,nx=0,ny=0,pickup=0,exciter=0;double damp=0;};
 Instrument()=default;
 void forces() noexcept;
 std::vector<Region> domains_;
 std::vector<Spring> springs_;
 std::vector<ElasticTerm> elastic_;
 std::vector<Coupler> couplers_;
 std::vector<double> q_,v_,mass_,f_,external_;
 double dt_=0,outputGain_=1000,dissipated_=0,externalWork_=0;
 int substeps_=1;
};
} // namespace panorama::coupled38
