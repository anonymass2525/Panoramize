#include "AcousticGraph21.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
namespace panorama::acoustic21 {
namespace {
bool finite(double x) noexcept {return std::isfinite(x);}
bool tensorPSD(const Tensor3& t) noexcept {
 for(double v:t.a)if(!finite(v)||std::abs(v)>1.e12)return false;
 const auto& a=t.a;
 const double tol=1.e-9;
 if(std::abs(a[1]-a[3])>tol || std::abs(a[2]-a[6])>tol || std::abs(a[5]-a[7])>tol)return false;
 if(a[0]<0 ||a[4]<0||a[8]<0)return false;
 const double minor01=a[0]*a[4]-a[1]*a[3];
 const double minor02=a[0]*a[8]-a[2]*a[6];
 const double minor12=a[4]*a[8]-a[5]*a[7];
 const double det=a[0]*(a[4]*a[8]-a[5]*a[7])-a[1]*(a[3]*a[8]-a[5]*a[6])+a[2]*(a[3]*a[7]-a[4]*a[6]);
 const double scale=1.+std::pow(std::max({a[0],a[4],a[8]}),3);
 return minor01>=-1.e-12*scale && minor02>=-1.e-12*scale && minor12>=-1.e-12*scale && det>=-1.e-12*scale;
}
bool materialOK(const Material3& m) noexcept {return tensorPSD(m.stiffness)&&tensorPSD(m.damping);}
void add3(std::vector<double>& matrix,int n,int row,int col,const Tensor3& tensor,double sign){
 for(int a=0;a<3;a++)for(int b=0;b<3;b++)matrix[size_t((row*3+a)*n+col*3+b)]+=sign*tensor.a[size_t(a*3+b)];
}

}
Vec3 operator+(Vec3 a,Vec3 b) noexcept {return {a.x+b.x,a.y+b.y,a.z+b.z};}
Vec3 operator-(Vec3 a,Vec3 b) noexcept {return {a.x-b.x,a.y-b.y,a.z-b.z};}
Vec3 operator*(Vec3 a,double k) noexcept {return {a.x*k,a.y*k,a.z*k};}
Tensor3 Tensor3::isotropic(double k) noexcept {Tensor3 t;t.a={k,0,0,0,k,0,0,0,k};return t;}
Unit nativeUnit(Quantity q) noexcept {switch(q){
 case Quantity::Displacement:return Unit::Metres;
 case Quantity::Velocity:return Unit::MetresPerSecond;
 case Quantity::Force:return Unit::Newtons;
 case Quantity::SpectralEnergy:return Unit::Joules;
 case Quantity::PhaseTurns:return Unit::Turns;
 default:return Unit::NormalizedAudio;}}
bool validPort(const Port& p,int bodyCount) noexcept {
 if(p.body<0||p.body>=bodyCount||p.axis<0||p.axis>2)return false;
 return p.unit==nativeUnit(p.quantity) && (p.quantity==Quantity::Displacement||p.quantity==Quantity::Velocity||p.quantity==Quantity::Force);
}
bool validateConversion(const Conversion& c) noexcept {
 if(!finite(c.gain)||std::abs(c.gain)>1.e9)return false;
 if(c.fromUnit!=nativeUnit(c.from)||c.toUnit!=nativeUnit(c.to))return false;
 // No automatic passivity claim for conversions that change physical quantity.
 return c.from==c.to;
}
double spectralDriveForce(const SpectralDrive& s) noexcept {
 if(!finite(s.maxForceNewtons)||s.maxForceNewtons<0||s.maxForceNewtons>1.e7)return 0.;
 double sum=0,weight=0;
 for(size_t j=0;j<12;j++){
  if(!finite(s.energy[j])||!finite(s.phaseTurns[j])||!finite(s.weights[j])||s.energy[j]<0)return 0.;
  const double w=std::abs(s.weights[j]);weight+=w;
  sum+=s.weights[j]*std::sqrt(std::min(s.energy[j],1.e12))*std::sin(6.2831853071795864769*s.phaseTurns[j]);
 }
 if(weight<=0)return 0.;
 return s.maxForceNewtons*std::tanh(sum/weight);
}
int GraphBuilder::addBody(const Body& body){
 if(bodies_.size()>=256||!finite(body.mass)||body.mass<=0||body.mass>1.e9||
 !finite(body.rest.x)||!finite(body.rest.y)||!finite(body.rest.z)||!materialOK(body.anchor))return -1;
 bodies_.push_back(body);return int(bodies_.size())-1;
}
bool GraphBuilder::addLink(const Link& l){
 if(l.a<0||l.b<0||l.a==l.b||l.a>=int(bodies_.size())||l.b>=int(bodies_.size())||!materialOK(l.coupling))return false;
 // Prevent intentionally pathological graph sizes from exhausting memory.
 if(links_.size()>65536)return false;
 links_.push_back(l);return true;
}
bool GraphBuilder::addTetrahedron(const Tetrahedron& t){
 if(tetrahedra_.size()>32768 || !(t.youngPa>0&&t.youngPa<1.e12) ||
    !(t.poisson>-.99&&t.poisson<.499) ||
    !(t.densityKgM3>=0&&t.densityKgM3<1.e9) ||
    !(t.viscousSeconds>=0&&t.viscousSeconds<1000.))return false;
 for(int j=0;j<4;j++){
  if(t.vertices[size_t(j)]<0||t.vertices[size_t(j)]>=int(bodies_.size()))return false;
  for(int k=0;k<j;k++)if(t.vertices[size_t(k)]==t.vertices[size_t(j)])return false;
 }
 tetrahedra_.push_back(t);return true;
}
// Computes a 4-tetra linear-elastic element in world coordinates.
// B is the strain/displacement matrix, D the isotropic Hooke tensor:
// K_e = |V| B^T D B; 3D displacement, 6 engineering strain components.
// The matrix is real symmetric PSD, even for flipped tetra orientations.
bool addElasticTetra(PreparedGraph& g,const Tetrahedron& t,
                            const std::vector<Body>& bodies){
 const auto& p0=bodies[size_t(t.vertices[0])].rest;
 const auto& p1=bodies[size_t(t.vertices[1])].rest;
 const auto& p2=bodies[size_t(t.vertices[2])].rest;
 const auto& p3=bodies[size_t(t.vertices[3])].rest;
 const double d[3][3]={{p1.x-p0.x,p2.x-p0.x,p3.x-p0.x},
                      {p1.y-p0.y,p2.y-p0.y,p3.y-p0.y},
                      {p1.z-p0.z,p2.z-p0.z,p3.z-p0.z}};
 const double det=d[0][0]*(d[1][1]*d[2][2]-d[1][2]*d[2][1])
                 -d[0][1]*(d[1][0]*d[2][2]-d[1][2]*d[2][0])
                 +d[0][2]*(d[1][0]*d[2][1]-d[1][1]*d[2][0]);
 if(!finite(det)||std::abs(det)<1.e-12)return false;
 const double invDet=1./det,volume=std::abs(det)/6.;
 double inv[3][3];
 for(int i=0;i<3;i++)for(int j=0;j<3;j++){
  const int r0=(j+1)%3,r1=(j+2)%3,c0=(i+1)%3,c1=(i+2)%3;
  inv[i][j]=(d[r0][c0]*d[r1][c1]-d[r0][c1]*d[r1][c0])*invDet;
 }
 // Inverse matrix is D^{-1}; gradients of barycentric shape functions
 // are columns of D^{-T}, i.e. rows of inverse D.
 double grad[4][3]{};
 for(int a=1;a<4;a++)for(int k=0;k<3;k++)grad[a][k]=inv[a-1][k];
 for(int k=0;k<3;k++)grad[0][k]=-(grad[1][k]+grad[2][k]+grad[3][k]);
 double B[6][12]{};
 for(int a=0;a<4;a++){
  const double gx=grad[a][0],gy=grad[a][1],gz=grad[a][2];
  B[0][3*a]=gx;B[1][3*a+1]=gy;B[2][3*a+2]=gz;
  B[3][3*a+1]=gz;B[3][3*a+2]=gy;  // yz
  B[4][3*a]=gz;B[4][3*a+2]=gx;    // xz
  B[5][3*a]=gy;B[5][3*a+1]=gx;    // xy
 }
 const double E=t.youngPa, nu=t.poisson;
 const double mu=E/(2.*(1.+nu));
 const double lambda=E*nu/((1.+nu)*(1.-2.*nu));
 double D[6][6]{};
 for(int i=0;i<3;i++)for(int j=0;j<3;j++)D[i][j]=lambda+(i==j?2.*mu:0.);
 for(int i=3;i<6;i++)D[i][i]=mu;
 double DB[6][12]{};
 for(int r=0;r<6;r++)for(int c=0;c<12;c++)for(int k=0;k<6;k++)DB[r][c]+=D[r][k]*B[k][c];
 for(int a=0;a<4;a++)for(int c=0;c<3;c++){
  const int row=t.vertices[size_t(a)]*3+c;
  g.mass_[size_t(row)]+=t.densityKgM3*volume/4.;
  for(int b=0;b<4;b++)for(int e=0;e<3;e++){
   const int col=t.vertices[size_t(b)]*3+e;
   double value=0.;for(int k=0;k<6;k++)value+=B[k][3*a+c]*DB[k][3*b+e];
   value*=volume;
   g.stiffness_[size_t(row)*g.dof_+col]+=value;
   g.damping_[size_t(row)*g.dof_+col]+=value*t.viscousSeconds;
  }
 }
 return true;
}
std::shared_ptr<const PreparedGraph> GraphBuilder::compile(double sampleRate,std::string* error) const {
 const auto fail=[&](const char* message)->std::shared_ptr<const PreparedGraph>{if(error)*error=message;return {};};
 if(!finite(sampleRate)||sampleRate<8000||sampleRate>384000)return fail("invalid sample rate");
 if(bodies_.empty())return fail("graph must have at least one body");
 auto g=std::make_shared<PreparedGraph>();
 g->bodies_=int(bodies_.size());g->dof_=g->bodies_*3;g->sr_=sampleRate;
 const int n=g->dof_;
 g->mass_.resize(size_t(n));g->stiffness_.assign(size_t(n)*n,0.);g->damping_.assign(size_t(n)*n,0.);
 for(int i=0;i<g->bodies_;i++){
  for(int k=0;k<3;k++)g->mass_[size_t(i*3+k)]=bodies_[size_t(i)].mass;
  add3(g->stiffness_,n,i,i,bodies_[size_t(i)].anchor.stiffness,1.);
  add3(g->damping_,n,i,i,bodies_[size_t(i)].anchor.damping,1.);
 }
 for(const auto& link:links_){
  add3(g->stiffness_,n,link.a,link.a,link.coupling.stiffness,1.);
  add3(g->stiffness_,n,link.b,link.b,link.coupling.stiffness,1.);
  add3(g->stiffness_,n,link.a,link.b,link.coupling.stiffness,-1.);
  add3(g->stiffness_,n,link.b,link.a,link.coupling.stiffness,-1.);
  add3(g->damping_,n,link.a,link.a,link.coupling.damping,1.);
  add3(g->damping_,n,link.b,link.b,link.coupling.damping,1.);
  add3(g->damping_,n,link.a,link.b,link.coupling.damping,-1.);
  add3(g->damping_,n,link.b,link.a,link.coupling.damping,-1.);
 }
 for(const auto& tetra:tetrahedra_)if(!addElasticTetra(*g,tetra,bodies_))return fail("invalid or degenerate tetrahedron");
 const double h=1./sampleRate;
 g->cholesky_.assign(size_t(n)*n,0.);
 // Cholesky(A), A=M+hC/2+h^2K/4. A is SPD if M>0 and C,K PSD.
 for(int i=0;i<n;i++)for(int j=0;j<=i;j++){
  double sum=(i==j?g->mass_[size_t(i)]:0.)+h*.5*g->damping_[size_t(i)*n+j]+h*h*.25*g->stiffness_[size_t(i)*n+j];
  for(int k=0;k<j;k++)sum-=g->cholesky_[size_t(i)*n+k]*g->cholesky_[size_t(j)*n+k];
  if(i==j){if(!finite(sum)||sum<=1.e-18)return fail("non-SPD solver matrix");g->cholesky_[size_t(i)*n+j]=std::sqrt(sum);}
  else g->cholesky_[size_t(i)*n+j]=sum/g->cholesky_[size_t(j)*n+j];
 }
 return g;
}
StateVoice PreparedGraph::makeVoice() const{return StateVoice(shared_from_this());}
StateVoice::StateVoice(std::shared_ptr<const PreparedGraph> graph):graph_(std::move(graph)){
 auto n=size_t(graph_->dof_);x_.assign(n,0.);v_.assign(n,0.);forces_.assign(n,0.);
 rhs_.assign(n,0.);intermediate_.assign(n,0.);temp_.assign(n,0.);
}
void StateVoice::reset() noexcept{std::fill(x_.begin(),x_.end(),0.);std::fill(v_.begin(),v_.end(),0.);
 std::fill(forces_.begin(),forces_.end(),0.);initial_=work_=dissipation_=0.;}
bool StateVoice::excite(int body,Vec3 velocity) noexcept {
 if(body<0||body>=graph_->bodies_||!finite(velocity.x)||!finite(velocity.y)||!finite(velocity.z))return false;
 // External impulse has already happened: use its resulting energy as initial state.
 const double values[]={velocity.x,velocity.y,velocity.z}; for(int k=0;k<3;k++)v_[size_t(body*3+k)]+=values[k];
 initial_=mechanicalEnergy()+dissipation_-work_;return true;
}
bool StateVoice::addForce(int body,Vec3 force) noexcept{
 if(body<0||body>=graph_->bodies_||!finite(force.x)||!finite(force.y)||!finite(force.z))return false;
 const double vals[]={force.x,force.y,force.z};
 for(int k=0;k<3;k++)forces_[size_t(body*3+k)]+=vals[k];return true;
}
bool StateVoice::addForceAtPort(const Port& port,double force) noexcept {
 if(!validPort(port,graph_->bodies_)||port.quantity!=Quantity::Force||!finite(force))return false;
 forces_[size_t(port.body*3+port.axis)]+=force;return true;
}
double StateVoice::mechanicalEnergy() const noexcept {
 const size_t n=x_.size();double e=0.;
 for(size_t i=0;i<n;i++){
  e+=.5*graph_->mass_[i]*v_[i]*v_[i];
  double kx=0.;for(size_t j=0;j<n;j++)kx+=graph_->stiffness_[i*n+j]*x_[j];
  e+=.5*x_[i]*kx;
 }return e;
}
bool StateVoice::processSample() noexcept {
 const int n=graph_->dof_;const double h=1./graph_->sr_;
 for(int i=0;i<n;i++){
  double kx=0.;for(int j=0;j<n;j++)kx+=graph_->stiffness_[size_t(i)*n+j]*x_[size_t(j)];
  rhs_[size_t(i)]=graph_->mass_[size_t(i)]*v_[size_t(i)]+.5*h*(forces_[size_t(i)]-kx);
 }
 // Forward and backward triangular solves L y=b, L^T v_mid=y.
 for(int i=0;i<n;i++){
  double value=rhs_[size_t(i)];for(int j=0;j<i;j++)value-=graph_->cholesky_[size_t(i)*n+j]*intermediate_[size_t(j)];
  intermediate_[size_t(i)]=value/graph_->cholesky_[size_t(i)*n+i];
 }
 for(int i=n-1;i>=0;i--){
  double value=intermediate_[size_t(i)];for(int j=i+1;j<n;j++)value-=graph_->cholesky_[size_t(j)*n+i]*temp_[size_t(j)];
  temp_[size_t(i)]=value/graph_->cholesky_[size_t(i)*n+i];
 }
 double powerIn=0,powerLoss=0;
 for(int i=0;i<n;i++){
  powerIn+=forces_[size_t(i)]*temp_[size_t(i)];
  double cv=0.;for(int j=0;j<n;j++)cv+=graph_->damping_[size_t(i)*n+j]*temp_[size_t(j)];
  powerLoss+=temp_[size_t(i)]*cv;
 }
 for(int i=0;i<n;i++){
  x_[size_t(i)]+=h*temp_[size_t(i)];
  v_[size_t(i)]=2.*temp_[size_t(i)]-v_[size_t(i)];
  forces_[size_t(i)]=0.;
  if(!finite(x_[size_t(i)])||!finite(v_[size_t(i)]))return false;
 }
 work_+=h*powerIn;dissipation_+=h*std::max(0.,powerLoss);
 return finite(work_)&&finite(dissipation_);
}
Vec3 StateVoice::displacement(int body) const noexcept{
 if(body<0||body>=graph_->bodies_)return {};const auto i=size_t(3*body);return {x_[i],x_[i+1],x_[i+2]};}
Vec3 StateVoice::velocity(int body) const noexcept{
 if(body<0||body>=graph_->bodies_)return {};const auto i=size_t(3*body);return {v_[i],v_[i+1],v_[i+2]};}
double StateVoice::readPort(const Port& port) const noexcept{
 if(!validPort(port,graph_->bodies_))return 0.;
 const size_t index=size_t(3*port.body+port.axis);
 if(port.quantity==Quantity::Displacement)return x_[index];
 if(port.quantity==Quantity::Velocity)return v_[index];
 return 0.;
}
EnergyLedger StateVoice::ledger() const noexcept{
 const double e=mechanicalEnergy();return {e,initial_,work_,dissipation_,e+dissipation_-work_-initial_};
}
double StateVoice::audioVelocity(int body,Vec3 pickup,double gain) const noexcept{
 const auto v=velocity(body);return gain*(v.x*pickup.x+v.y*pickup.y+v.z*pickup.z);
}
} // namespace panorama::acoustic21
