#include "UnifiedGraph22.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
namespace panorama::unified22 {
namespace {
constexpr double pi=3.1415926535897932384626433832795;
constexpr int maxModes=128;
bool validNumber(double x){return std::isfinite(x);}
// SPD Cholesky in preallocated buffers (n <= maxModes).
bool factor(const std::vector<double>& A,std::vector<double>& L,int n){
 std::fill(L.begin(),L.end(),0.);
 for(int i=0;i<n;i++) for(int j=0;j<=i;j++){
  double s=A[size_t(i)*n+j];
  for(int k=0;k<j;k++)s-=L[size_t(i)*n+k]*L[size_t(j)*n+k];
  if(i==j){if(!(s>1.e-20)||!validNumber(s))return false;L[size_t(i)*n+i]=std::sqrt(s);}
  else L[size_t(i)*n+j]=s/L[size_t(j)*n+j];
 }
 return true;
}
void solve(const std::vector<double>& L,const std::vector<double>& rhs,std::vector<double>& result,std::vector<double>& scratch,int n){
 for(int i=0;i<n;i++){
  double s=rhs[size_t(i)];for(int j=0;j<i;j++)s-=L[size_t(i)*n+j]*scratch[size_t(j)];
  scratch[size_t(i)]=s/L[size_t(i)*n+i];
 }
 for(int i=n-1;i>=0;i--){
  double s=scratch[size_t(i)];for(int j=i+1;j<n;j++)s-=L[size_t(j)*n+i]*result[size_t(j)];
  result[size_t(i)]=s/L[size_t(i)*n+i];
 }
}
}
Unit nativeUnit(Quantity q) noexcept {
 switch(q){
  case Quantity::NormalizedCoordinate:return Unit::RootJouleSeconds;
  case Quantity::NormalizedFlow:return Unit::RootJoule;
  case Quantity::Effort:return Unit::RootJoulePerSecond;
  case Quantity::Audio:return Unit::NormalizedAudio;
  case Quantity::Displacement:return Unit::Metre;
  case Quantity::Velocity:return Unit::MetrePerSecond;
  case Quantity::Pressure:return Unit::Pascal;
  case Quantity::SpectralAmplitude:return Unit::RelativeAmplitude;
  case Quantity::PhaseRadians:return Unit::Radians;
 }return Unit::NormalizedAudio;
}
int Builder::addMode(Mode m){
 if(modes_.size()>=maxModes||m.name.empty()||!validNumber(m.frequencyHz)||m.frequencyHz<.01||m.frequencyHz>20000.||!validNumber(m.dampingPerSecond)||m.dampingPerSecond<0.||!validNumber(m.quartic)||m.quartic<0.||!validNumber(m.displacementMetresPerQ)||!validNumber(m.pressurePascalsPerQ)||!validNumber(m.spectralAmplitudePerQ)||!validNumber(m.phaseRadiansPerQ))return -1;
 modes_.push_back(std::move(m));return int(modes_.size()-1);
}
bool Builder::connect(const Link& l){
 if(l.a<0||l.b<0||l.a==l.b||size_t(l.a)>=modes_.size()||size_t(l.b)>=modes_.size()||!validNumber(l.strength)||l.strength<0.||!validNumber(l.ratio)||std::abs(l.ratio)>64.)return false;
 links_.push_back(l);return true;
}
bool Builder::connectTyped(const Port& a,const Port& b,LinkKind kind,double strength,double ratio){
 if(a.quantity!=Quantity::NormalizedCoordinate || b.quantity!=Quantity::NormalizedCoordinate ||
    a.unit!=Unit::RootJouleSeconds || b.unit!=Unit::RootJouleSeconds) return false;
 return connect({a.mode,b.mode,kind,strength,ratio});
}
std::shared_ptr<const PreparedGraph> Builder::compile(double sr,std::string* error) const {
 auto fail=[&](const char* s)->std::shared_ptr<const PreparedGraph>{if(error)*error=s;return {};};
 if(modes_.empty()||modes_.size()>maxModes||!validNumber(sr)||sr<8000.||sr>384000.)return fail("invalid graph/sample rate");
 auto g=std::make_shared<PreparedGraph>();g->modes_=modes_;g->sr_=sr;
 const int n=int(modes_.size());const size_t nn=size_t(n)*n;
 g->K_.assign(nn,0.);g->D_.assign(nn,0.);g->A_.assign(nn,0.);g->factor_.assign(nn,0.);
 for(int i=0;i<n;i++){
  const auto& m=modes_[size_t(i)];const double w=2.*pi*m.frequencyHz;
  g->K_[size_t(i)*n+i]=w*w;
  g->D_[size_t(i)*n+i]=m.dampingPerSecond;
  if(m.quartic>0.)g->nonlinear_=true;
 }
 for(const auto& l:links_){
  auto& M=l.kind==LinkKind::Conservative?g->K_:g->D_;
  const int a=l.a,b=l.b;double k=l.strength,r=l.ratio;
  M[size_t(a)*n+a]+=k; M[size_t(b)*n+b]+=k*r*r;
  M[size_t(a)*n+b]-=k*r; M[size_t(b)*n+a]-=k*r;
 }
 const double dt=1./sr;
 for(int i=0;i<n;i++)for(int j=0;j<n;j++){
  const size_t idx=size_t(i)*n+j;
  g->A_[idx]=dt*.5*g->D_[idx]+dt*dt*.25*g->K_[idx]+(i==j?1.:0.);
 }
 if(!factor(g->A_,g->factor_,n))return fail("matrix is not SPD");
 if(error)error->clear();
 return g;
}
Voice PreparedGraph::makeVoice() const{return Voice(shared_from_this());}
Voice::Voice(std::shared_ptr<const PreparedGraph> g):graph_(std::move(g)){
 const int n=int(graph_->modes_.size());const size_t nn=size_t(n)*n;
 q_.assign(n,0.);p_.assign(n,0.);forces_.assign(n,0.);source_.assign(n,0.);rhs_.assign(n,0.);tmp_.assign(n,0.);solution_.assign(n,0.);qnext_.assign(n,0.);
 matrix_.assign(nn,0.);chol_.assign(nn,0.);
}
void Voice::reset() noexcept {std::fill(q_.begin(),q_.end(),0.);std::fill(p_.begin(),p_.end(),0.);std::fill(forces_.begin(),forces_.end(),0.);initial_=active_=loss_=0.;processed_=false;}
bool Voice::excite(int m,double v) noexcept {
 if(m<0||size_t(m)>=p_.size()||!validNumber(v))return false;
 // Expected before sample 0; if called mid-note, treat it as an impulse from an active source.
 const double before=storedEnergy();p_[size_t(m)]+=v;
 const double delta=storedEnergy()-before;
 if(!processed_)initial_+=delta;
 else active_+=delta;
 return true;
}
bool Voice::addActiveForce(int m,double f) noexcept{
 if(m<0||size_t(m)>=forces_.size()||!validNumber(f))return false;
 forces_[size_t(m)]+=f;return true;
}
double Voice::storedEnergy() const noexcept{
 const int n=int(q_.size());double e=0.;
 for(int i=0;i<n;i++){
  double kq=0.;for(int j=0;j<n;j++)kq+=graph_->K_[size_t(i)*n+j]*q_[size_t(j)];
  const double x=q_[size_t(i)],v=p_[size_t(i)];
  e+=.5*v*v+.5*x*kq+graph_->modes_[size_t(i)].quartic*.25*x*x*x*x;
 }
 return e;
}
bool Voice::processSample() noexcept {
 const int n=int(q_.size());const double dt=1./graph_->sr_;
 source_=forces_;
 // midpoint algorithm: [I+dt D/2+dt² K/4] p1 + dt*grad V_nl(q0,q1) =
 // (I-dt D/2-dt² K/4)p0 - dt K q0 + dt u.
 for(int i=0;i<n;i++){
  double kq=0.,aExtra=0.;
  for(int j=0;j<n;j++){
   const size_t idx=size_t(i)*n+j;
   kq+=graph_->K_[idx]*q_[size_t(j)];
   aExtra+=(graph_->A_[idx]-(i==j?1.:0.))*p_[size_t(j)];
  }
  rhs_[size_t(i)]=p_[size_t(i)]-aExtra-dt*kq+dt*forces_[size_t(i)];
 }
 // Linear solution is initial iterate; nonlinear ports are solved by discrete-gradient Newton.
 solve(graph_->factor_,rhs_,solution_,tmp_,n);
 if(graph_->nonlinear_){
  bool converged=false;
  for(int iteration=0;iteration<14;iteration++){
   matrix_=graph_->A_;
   for(int i=0;i<n;i++){
    const auto& m=graph_->modes_[size_t(i)];
    const double q0=q_[size_t(i)],q1=q0+dt*.5*(p_[size_t(i)]+solution_[size_t(i)]);
    qnext_[size_t(i)]=q1;
    const double grad=m.quartic*.25*(q1*q1*q1+q1*q1*q0+q1*q0*q0+q0*q0*q0);
    forces_[size_t(i)]=grad; // temporary nonlinear force (original active forces must be saved separately)
    matrix_[size_t(i)*n+i]+=dt*dt*.125*m.quartic*(3.*q1*q1+2.*q1*q0+q0*q0);
   }
   // Compute residual in rhs_ scratch without corrupting original rhs: tmp = rhs - A*p - dt*grad.
   double maxResidual=0.;
   for(int i=0;i<n;i++){
    double s=rhs_[size_t(i)]-dt*forces_[size_t(i)];
    for(int j=0;j<n;j++)s-=graph_->A_[size_t(i)*n+j]*solution_[size_t(j)];
    tmp_[size_t(i)]=s;maxResidual=std::max(maxResidual,std::abs(s));
   }
   if(!validNumber(maxResidual))return false;
   if(maxResidual<1.e-12){converged=true;break;}
   if(!factor(matrix_,chol_,n))return false;
   // Newton update. Keep original rhs in rhs_, so use qnext_ as solve scratch output.
   // solve() scratch must be independent of input rhs; use forces_ as temporary.
   solve(chol_,tmp_,qnext_,forces_,n);
   double maxStep=0.;for(int i=0;i<n;i++){solution_[size_t(i)]+=qnext_[size_t(i)];maxStep=std::max(maxStep,std::abs(qnext_[size_t(i)]));}
   if(!validNumber(maxStep)||maxStep>1.e9)return false;
   if(maxStep<1.e-12){converged=true;break;}
  }
  if(!converged)return false;
 }
 // Work from the actual active port, not a residual reconstructed by subtraction.
 for(int i=0;i<n;i++){
  const double mid=(p_[size_t(i)]+solution_[size_t(i)])*.5;
  active_+=dt*source_[size_t(i)]*mid;
  for(int j=0;j<n;j++)loss_+=dt*mid*.5*graph_->D_[size_t(i)*n+j]*(p_[size_t(j)]+solution_[size_t(j)]);
 }
 for(int i=0;i<n;i++){
  q_[size_t(i)]+=dt*.5*(p_[size_t(i)]+solution_[size_t(i)]);
  p_[size_t(i)]=solution_[size_t(i)];
 }
 std::fill(forces_.begin(),forces_.end(),0.);
 processed_=true;
 return finite();
}
bool Voice::finite() const noexcept {for(double x:q_)if(!std::isfinite(x))return false;for(double x:p_)if(!std::isfinite(x))return false;return std::isfinite(ledger().residual);}
double Voice::read(const Port& p) const noexcept {
 if(p.mode<0||size_t(p.mode)>=q_.size()||p.unit!=nativeUnit(p.quantity))return 0.;
 const size_t i=size_t(p.mode);const auto& m=graph_->modes_[i];
 switch(p.quantity){
  case Quantity::NormalizedCoordinate:return q_[i];
  case Quantity::NormalizedFlow:return p_[i];
  case Quantity::Effort:{double e=0.;for(size_t j=0;j<q_.size();j++)e+=graph_->K_[i*q_.size()+j]*q_[j];return e+m.quartic*q_[i]*q_[i]*q_[i];}
  case Quantity::Audio:return p_[i];
  case Quantity::Displacement:return m.domain==Domain::Physical?q_[i]*m.displacementMetresPerQ:0.;
  case Quantity::Velocity:return m.domain==Domain::Physical?p_[i]*m.displacementMetresPerQ:0.;
  case Quantity::Pressure:return m.domain==Domain::VocalFormant?q_[i]*m.pressurePascalsPerQ:0.;
  case Quantity::SpectralAmplitude:return m.domain==Domain::Spectral?q_[i]*m.spectralAmplitudePerQ:0.;
  case Quantity::PhaseRadians:return m.domain==Domain::Phase?q_[i]*m.phaseRadiansPerQ:0.;
 }return 0.;
}
double Voice::audioSample(int m,double gain) const noexcept{
 if(m<0||size_t(m)>=p_.size()||!validNumber(gain))return 0.;return p_[size_t(m)]*gain;
}
EnergyLedger Voice::ledger() const noexcept{
 EnergyLedger e;e.stored=storedEnergy();e.initial=initial_;e.activeWork=active_;e.dissipated=loss_;e.residual=e.stored+e.dissipated-e.initial-e.activeWork;return e;
}
} // namespace panorama::unified22
