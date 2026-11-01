#include "CoupledPhysics38.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <limits>
#include <map>
#include <sstream>
#include <utility>
namespace panorama::coupled38 {
namespace {
bool in(double v,double a,double b){return std::isfinite(v)&&v>=a&&v<=b;}
bool validDomain(const Domain& d){
 if(d.family!=Family::String1D&&d.family!=Family::Membrane2D&&d.family!=Family::Plate2D&&
    d.family!=Family::Tube1D&&d.family!=Family::TetraResonator3D)return false;
 if(d.nx<8||d.nx>48||d.ny<8||d.ny>24||!in(d.fundamentalHz,40,1000)||
    !in(d.damping,0,100)||!in(d.exciter,.02,.98)||!in(d.pickup,.02,.98)||
    !in(d.amplitude,0,.01)||!in(d.young,1e6,2e9)||!in(d.density,1e3,1e6)||!in(d.poisson,0,.46))return false;
 for(double x:d.anisotropy)if(!in(x,.01,100))return false;
 return d.family!=Family::TetraResonator3D||gen37::validShape(d.shape);
}
bool validRecipe(const Recipe&r){
 if(r.domains.empty()||r.domains.size()>8||r.connections.size()>24||r.exciteDomain<0||
    r.exciteDomain>=int(r.domains.size())||!in(r.outputGain,.01,100000))return false;
 for(auto&d:r.domains)if(!validDomain(d))return false;
 for(auto&c:r.connections){
  if(c.from<0||c.to<0||c.from>=int(r.domains.size())||c.to>=int(r.domains.size())||c.from==c.to||
    !in(c.positionFrom,0,1)||!in(c.positionTo,0,1)||c.axisFrom<0||c.axisFrom>2||c.axisTo<0||c.axisTo>2||
    !in(c.stiffness,0,1e9)||!in(c.damping,0,1e5)||!in(c.ratio,.001,100))return false;
 }return true;
}
}
std::string serialize(const Recipe&r){
 std::ostringstream s;s<<std::setprecision(17)<<"PANORAMIZE-COUPLED38-v1\n";
 s<<r.domains.size()<<' '<<r.connections.size()<<' '<<r.exciteDomain<<' '<<r.outputGain<<'\n';
 for(auto&d:r.domains)s<<"node "<<int(d.family)<<' '<<d.nx<<' '<<d.ny<<' '<<d.fundamentalHz<<' '<<d.damping<<' '
   <<d.exciter<<' '<<d.pickup<<' '<<d.amplitude<<' '<<d.shape.xCells<<' '<<d.shape.yCells<<' '<<d.shape.zCells<<' '
   <<d.shape.length<<' '<<d.shape.width<<' '<<d.shape.thickness<<' '<<d.shape.camber<<' '<<d.shape.taper<<' '
   <<d.young<<' '<<d.density<<' '<<d.poisson
   <<' '<<d.anisotropy[0]<<' '<<d.anisotropy[1]<<' '<<d.anisotropy[2]
   <<' '<<d.anisotropy[3]<<' '<<d.anisotropy[4]<<' '<<d.anisotropy[5]<<'\n';
 for(auto&c:r.connections)s<<"link "<<c.from<<' '<<c.to<<' '<<c.positionFrom<<' '<<c.positionTo<<' '
  <<c.axisFrom<<' '<<c.axisTo<<' '<<c.stiffness<<' '<<c.damping<<' '<<c.ratio<<'\n';
 return s.str();
}
bool deserialize(const std::string&src,Recipe&out,std::string*error){
 auto fail=[&](const char*m){if(error)*error=m;return false;};
 if(src.size()>16000)return fail("oversized graph");
 std::istringstream s(src);std::string tag,extra;int n=0,m=0;Recipe r;
 if(!(s>>tag)||tag!="PANORAMIZE-COUPLED38-v1"||!(s>>n>>m>>r.exciteDomain>>r.outputGain)||
    n<1||n>8||m<0||m>24)return fail("invalid header");
 for(int i=0;i<n;i++){
  Domain d;int family=-1;
  if(!(s>>tag>>family>>d.nx>>d.ny>>d.fundamentalHz>>d.damping>>d.exciter>>d.pickup>>d.amplitude
        >>d.shape.xCells>>d.shape.yCells>>d.shape.zCells>>d.shape.length>>d.shape.width>>d.shape.thickness
        >>d.shape.camber>>d.shape.taper>>d.young>>d.density>>d.poisson
        >>d.anisotropy[0]>>d.anisotropy[1]>>d.anisotropy[2]>>d.anisotropy[3]>>d.anisotropy[4]>>d.anisotropy[5])||tag!="node"||family<0||family>4)
   return fail("invalid node");
  d.family=static_cast<Family>(family);r.domains.push_back(d);
 }
 for(int i=0;i<m;i++){
  Connection c;
  if(!(s>>tag>>c.from>>c.to>>c.positionFrom>>c.positionTo>>c.axisFrom>>c.axisTo>>c.stiffness>>c.damping>>c.ratio)||tag!="link")
   return fail("invalid link");
  r.connections.push_back(c);
 }
 if(s>>extra||!validRecipe(r))return fail("invalid topology or parameter bounds");
 out=std::move(r);if(error)error->clear();return true;
}
std::unique_ptr<Instrument> Instrument::prepare(const Recipe&r,int rate,std::string*error){
 auto fail=[&](const char*m)->std::unique_ptr<Instrument>{if(error)*error=m;return {};};
 if(!validRecipe(r)||rate<22050||rate>192000)return fail("invalid graph or sample rate");
 auto p=std::unique_ptr<Instrument>(new Instrument());p->outputGain_=r.outputGain;
 double bound=0.;
 std::map<std::pair<int,int>,double> femK;
 // construct physical mass / stiffness networks off the audio thread.
 auto allocate=[&](int count,int nX,int nY,const Domain&d){
  Region reg;reg.family=d.family;reg.offset=int(p->q_.size());reg.count=count;
  reg.nx=nX;reg.ny=nY;reg.damp=d.damping;
  reg.exciter=reg.offset;reg.pickup=reg.offset;
  p->q_.resize(p->q_.size()+size_t(count),0.);
  p->v_.resize(p->v_.size()+size_t(count),0.);
  p->mass_.resize(p->mass_.size()+size_t(count),1.);
  p->domains_.push_back(reg);
 };
 for(const auto&d:r.domains){
  if(d.family==Family::TetraResonator3D){
   fem36::Mesh mesh;std::string err;
   if(!gen37::buildTetraMesh(d.shape,mesh,&err))return fail("invalid tetrahedral shape");
   const int nv=int(mesh.vertices.size()),offset=int(p->q_.size());allocate(nv*3,nv,3,d);
   auto&reg=p->domains_.back();
   for(int i=offset;i<offset+nv*3;i++)p->mass_[size_t(i)]=0.;
   reg.exciter=offset+3*std::clamp(int(std::round(d.exciter*(nv-1))),0,nv-1)+1;
   reg.pickup=offset+3*std::clamp(int(std::round(d.pickup*(nv-1))),0,nv-1)+1;
   // Linear tetrahedral strain FEM. Isotropic D for 3D, same local B^T D B assembly as 3.6.
   const double mu=d.young/(2*(1+d.poisson));
   const double lam=d.young*d.poisson/((1+d.poisson)*(1-2*d.poisson));
   for(const auto&t:mesh.tetrahedra){
    double D[3][3]{};
    for(int a=0;a<3;a++)for(int b=0;b<3;b++)D[a][b]=mesh.vertices[size_t(t[b+1])][size_t(a)]-mesh.vertices[size_t(t[0])][size_t(a)];
    double det=D[0][0]*(D[1][1]*D[2][2]-D[1][2]*D[2][1])-D[0][1]*(D[1][0]*D[2][2]-D[1][2]*D[2][0])+D[0][2]*(D[1][0]*D[2][1]-D[1][1]*D[2][0]);
    if(std::abs(det)<1e-13)return fail("degenerate tetrahedron");
    double inv[3][3]{};
    for(int a=0;a<3;a++)for(int b=0;b<3;b++){
     int r0=(b+1)%3,r1=(b+2)%3,c0=(a+1)%3,c1=(a+2)%3;
     inv[a][b]=(D[r0][c0]*D[r1][c1]-D[r0][c1]*D[r1][c0])/det;
    }
    double grad[4][3]{};
    for(int a=1;a<4;a++)for(int k=0;k<3;k++)grad[a][k]=inv[a-1][k];
    for(int k=0;k<3;k++)grad[0][k]=-(grad[1][k]+grad[2][k]+grad[3][k]);
    double B[6][12]{};
    for(int a=0;a<4;a++){
     double x=grad[a][0],y=grad[a][1],z=grad[a][2];
     B[0][3*a]=x;B[1][3*a+1]=y;B[2][3*a+2]=z;
     B[3][3*a+1]=z;B[3][3*a+2]=y;B[4][3*a]=z;B[4][3*a+2]=x;B[5][3*a]=y;B[5][3*a+1]=x;
    }
    double C[6][6]{};
    for(int a=0;a<3;a++)for(int b=0;b<3;b++)C[a][b]=lam+(a==b?2*mu:0);
    for(int a=3;a<6;a++)C[a][a]=mu;
    for(int a=0;a<6;a++)for(int b=0;b<6;b++)
     C[a][b]*=std::sqrt(d.anisotropy[size_t(a)]*d.anisotropy[size_t(b)]);
    double vol=std::abs(det)/6.;
    for(int a=0;a<4;a++)for(int axis=0;axis<3;axis++)
     p->mass_[size_t(offset+3*t[size_t(a)]+axis)]+=vol*d.density/4.;
    for(int a=0;a<12;a++)for(int b=0;b<12;b++){
     double val=0;for(int u=0;u<6;u++)for(int v=0;v<6;v++)val+=B[u][a]*C[u][v]*B[v][b];
     val*=vol;
     if(std::abs(val)>1e-12)femK[{offset+3*t[size_t(a/3)]+a%3,offset+3*t[size_t(b/3)]+b%3}]+=val;
    }
   }
   // Positive anchor stiffness prevents unconstrained rigid-body drift.
   for(int ax=0;ax<3;ax++)p->springs_.push_back({offset+ax,-1,d.young*.0000001});

  }else {
   const bool is2d=d.family==Family::Membrane2D||d.family==Family::Plate2D;
   const int nx=d.nx,ny=is2d?d.ny:1,offset=int(p->q_.size());allocate(nx*ny,nx,ny,d);
   auto&reg=p->domains_.back();
   auto index=[&](int x,int y){return offset+x+nx*y;};
   const int ex=std::clamp(int(std::lround(d.exciter*(nx-1))),1,nx-2),ey=is2d?ny/2:0;
   const int px=std::clamp(int(std::lround(d.pickup*(nx-1))),1,nx-2),py=is2d?ny/2:0;
   reg.exciter=index(ex,ey);reg.pickup=index(px,py);
   // 1D tube uses acoustic longitudinal fluid displacement x; rho*A*dx is lumped
   // mass and rho*c^2*A/dx compressibility spring, so piston force=A*pressure.
   const double speed=2*d.fundamentalHz*(nx-1)/(is2d?std::sqrt(2.):1.);
   double mass=d.family==Family::Tube1D?1.18*.001*(1./(nx-1)):1.;
   double k=mass*speed*speed;
   for(int y=0;y<ny;y++)for(int x=0;x<nx;x++){
    int a=index(x,y);p->mass_[size_t(a)]=mass;
    if(x+1<nx)p->springs_.push_back({a,index(x+1,y),k});
    if(is2d&&y+1<ny)p->springs_.push_back({a,index(x,y+1),k});
    // Fixed displacement edges, tube ends use compliant reflecting termination.
    if(x==0||x==nx-1||(is2d&&(y==0||y==ny-1)))p->springs_.push_back({a,-1,k});
    // plate bending: next-neighbor curvature penalty (approximation; not Kirchhoff plate).
    if(d.family==Family::Plate2D){
     if(x+2<nx)p->springs_.push_back({a,index(x+2,y),k*.06});
     if(y+2<ny)p->springs_.push_back({a,index(x,y+2),k*.06});
    }
   }
  }
 }
 // locate attachment dofs. Link connects mechanical boundary velocities with
 // a unitless lever ratio. Linear K,D are reciprocal and passive under this mapping.
 auto port=[&](int i,double fraction,int axis){
  const auto&reg=p->domains_[size_t(i)];
  if(reg.family==Family::TetraResonator3D)
   return reg.offset+3*std::clamp(int(std::round(fraction*(reg.nx-1))),0,reg.nx-1)+axis;
  int x=std::clamp(int(std::round(fraction*(reg.nx-1))),0,reg.nx-1);
  return reg.offset+x+(reg.ny>1?reg.nx*(reg.ny/2):0);
 };
 for(auto&c:r.connections)p->couplers_.push_back({port(c.from,c.positionFrom,c.axisFrom),port(c.to,c.positionTo,c.axisTo),c.stiffness,c.damping,c.ratio});
 for(auto&entry:femK)if(std::abs(entry.second)>1e-13)
  p->elastic_.push_back({entry.first.first,entry.first.second,entry.second});
 p->f_.assign(p->q_.size(),0.);p->external_.assign(p->q_.size(),0.);
 // Conservative stability bound based on symmetric positive definite spring sums.
 std::vector<double> row(p->q_.size(),0.), rowD(p->q_.size(),0.);
 for(auto&s:p->springs_){row[size_t(s.a)]+=s.k;if(s.b>=0)row[size_t(s.b)]+=s.k;}
 for(auto&t:p->elastic_)row[size_t(t.a)]+=std::abs(t.k);
 for(auto&c:p->couplers_){
  row[size_t(c.a)]+=c.k;row[size_t(c.b)]+=c.k*c.ratio*c.ratio;
  rowD[size_t(c.a)]+=c.d;rowD[size_t(c.b)]+=c.d*c.ratio*c.ratio;
 }
 for(auto&reg:p->domains_)for(int j=0;j<reg.count;j++){size_t id=size_t(reg.offset+j);rowD[id]+=p->mass_[id]*reg.damp;}
 for(size_t i=0;i<row.size();i++){
  if(!(p->mass_[i]>0)||!std::isfinite(row[i]))return fail("invalid mass and spring matrix");
  bound=std::max(bound,2*row[i]/p->mass_[i]);
 }
 double dampingBound=0.;
 for(size_t i=0;i<rowD.size();i++)dampingBound=std::max(dampingBound,rowD[i]/p->mass_[i]);
 int sub=std::max(int(std::ceil(std::sqrt(bound)/(double(rate)*.28))),int(std::ceil(4*dampingBound/rate)));
 if(sub>128)return fail("stiffness or damping requires more than 128 substeps");
 p->substeps_=std::max(4,sub);p->dt_=1./(rate*p->substeps_);
 if(error)error->clear();
 p->excite(r.exciteDomain,r.domains[size_t(r.exciteDomain)].amplitude);
 return p;
}
void Instrument::forces() noexcept {
 std::fill(f_.begin(),f_.end(),0.);
 for(auto&s:springs_){double delta=q_[size_t(s.a)]-(s.b>=0?q_[size_t(s.b)]:0.);
  const double force=s.k*delta;f_[size_t(s.a)]-=force;if(s.b>=0)f_[size_t(s.b)]+=force;
 }
 for(auto&t:elastic_)f_[size_t(t.a)]-=t.k*q_[size_t(t.b)];
 for(auto&c:couplers_){const double dx=q_[size_t(c.a)]-c.ratio*q_[size_t(c.b)];
  const double dv=v_[size_t(c.a)]-c.ratio*v_[size_t(c.b)];
  double force=c.k*dx+c.d*dv;f_[size_t(c.a)]-=force;f_[size_t(c.b)]+=c.ratio*force;
 }
}
float Instrument::processOne() noexcept {
 for(int sub=0;sub<substeps_;sub++){
  forces();
  for(size_t i=0;i<f_.size();i++)if(external_[i]!=0.){
    f_[i]+=external_[i];
  }
  for(auto&c:couplers_){double dv=v_[size_t(c.a)]-c.ratio*v_[size_t(c.b)];dissipated_+=c.d*dv*dv*dt_;}
  for(auto&reg:domains_)for(int j=0;j<reg.count;j++){size_t i=size_t(reg.offset+j);dissipated_+=reg.damp*mass_[i]*v_[i]*v_[i]*dt_;}
  for(auto&reg:domains_)for(int j=0;j<reg.count;j++){
   size_t i=size_t(reg.offset+j);
   const double oldVelocity=v_[i];
   v_[i]=(v_[i]+dt_*f_[i]/mass_[i])/(1+dt_*reg.damp);
   externalWork_+=.5*external_[i]*(oldVelocity+v_[i])*dt_;
   q_[i]+=dt_*v_[i];
  }
 }
 std::fill(external_.begin(),external_.end(),0.);
 double signal=0;for(int i=0;i<int(domains_.size());i++)signal+=domainPickup(i);
 signal*=outputGain_/std::max(1,int(domains_.size()));
 if(!std::isfinite(signal)||std::abs(signal)>1.e10){
  std::fill(q_.begin(),q_.end(),0.);std::fill(v_.begin(),v_.end(),0.);return 0;
 }
 return float(std::tanh(signal));
}
bool Instrument::addExternalForce(int domain,double newtons) noexcept {
 if(domain<0||domain>=int(domains_.size())||!std::isfinite(newtons)||std::abs(newtons)>1000.)return false;
 int index=domains_[size_t(domain)].exciter;
 double total=external_[size_t(index)]+newtons;
 if(!std::isfinite(total)||std::abs(total)>1000.)return false;
 external_[size_t(index)]=total;return true;
}
double Instrument::domainVelocity(int domain) const noexcept{
 if(domain<0||domain>=int(domains_.size()))return 0.;
 return v_[size_t(domains_[size_t(domain)].pickup)];
}
void Instrument::excite(int domain,double displacement) noexcept {
 if(domain<0||domain>=int(domains_.size())||!std::isfinite(displacement)||std::abs(displacement)>.01)return;
 int id=domains_[size_t(domain)].exciter;q_[size_t(id)]+=displacement;
}
double Instrument::domainPickup(int domain) const noexcept{
 if(domain<0||domain>=int(domains_.size()))return 0;
 return q_[size_t(domains_[size_t(domain)].pickup)];
}
double Instrument::couplingEnergy() const noexcept {
 double e=0;for(auto&c:couplers_){double dx=q_[size_t(c.a)]-c.ratio*q_[size_t(c.b)];e+=.5*c.k*dx*dx;}return e;
}
double Instrument::domainEnergy(int domain) const noexcept {
 if(domain<0||domain>=int(domains_.size()))return 0;
 const auto&reg=domains_[size_t(domain)];int lo=reg.offset,hi=lo+reg.count;
 double e=0;for(int j=lo;j<hi;j++)e+=.5*mass_[size_t(j)]*v_[size_t(j)]*v_[size_t(j)];
 for(auto&s:springs_){if(s.a<lo||s.a>=hi)continue;
  double dx=q_[size_t(s.a)]-(s.b>=0?q_[size_t(s.b)]:0.);e+=.5*s.k*dx*dx;
 }
 for(auto&t:elastic_)if(t.a>=lo&&t.a<hi)e+=.5*t.k*q_[size_t(t.a)]*q_[size_t(t.b)];
 return e;
}
double Instrument::totalEnergy() const noexcept{
 double e=couplingEnergy();for(int d=0;d<int(domains_.size());d++)e+=domainEnergy(d);return e;
}
}
