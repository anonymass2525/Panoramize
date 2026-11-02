#include "AdvancedAcoustics.h"
#include <algorithm>
#include <cmath>
#include <limits>
namespace panorama {
namespace {
constexpr double pi=3.1415926535897932384626433832795;
inline bool valid(double x){return std::isfinite(x);}
inline double clamp(double x,double a,double b){return std::max(a,std::min(x,b));}
}
MeshTopology makeMeshTopology(int shape,double hz,double tension) noexcept {
 MeshTopology m{};
 hz=clamp(hz,35.,4000.);tension=clamp(tension,.1,3.);
 const double s=std::pow(2*pi*hz,2)*tension;
 shape=((shape%3)+3)%3;
 if(shape==0){ // 7 point string
  m.count=7;m.anchorSpring=s*.16;
  for(int i=0;i<m.count;i++)m.points[i]={i/6.,0,0,1.};
  for(int i=0;i<m.count-1;i++)m.links[m.edges++]={i,i+1,s*.45};
 }else if(shape==1){ // 8 point closed ring: truly cyclic physical topology
  m.count=8;m.anchorSpring=s*.20;
  for(int i=0;i<m.count;i++)m.points[i]={std::cos(2*pi*i/8),std::sin(2*pi*i/8),0.,1.};
  for(int i=0;i<m.count;i++)m.links[m.edges++]={i,(i+1)%m.count,s*.42};
 }else{ // 3x3 surface lattice, with cycles
  m.count=9;m.anchorSpring=s*.14;
  for(int i=0;i<9;i++)m.points[i]={double(i%3),double(i/3),0.,1.};
  for(int y=0;y<3;y++)for(int x=0;x<3;x++){
   if(x<2)m.links[m.edges++]={y*3+x,y*3+x+1,s*.29};
   if(y<2)m.links[m.edges++]={y*3+x,(y+1)*3+x,s*.29};
  }
 }
 return m;
}
bool MaterialMesh::configure(const MeshTopology& t,double sampleRate) noexcept {
 n_=0;
 if(t.count<1||t.count>kMeshMasses||t.edges<0||t.edges>kMeshEdges||
    !valid(sampleRate)||sampleRate<8000||sampleRate>384000||
    !valid(t.anchorSpring)||t.anchorSpring<0||t.anchorSpring>1.e10)return false;
 for(int i=0;i<t.count;i++){
  const auto&p=t.points[i];
  if(!valid(p.x)||!valid(p.y)||!valid(p.z)||!valid(p.mass)||p.mass<.001||p.mass>1000.)return false;
 }
 for(int i=0;i<t.edges;i++){
  const auto&e=t.links[i];
  if(e.a<0||e.b<0||e.a>=t.count||e.b>=t.count||e.a==e.b||
     !valid(e.spring)||e.spring<0||e.spring>1.e10)return false;
 }
 topology_=t;n_=t.count;h_=1./sampleRate;work_=0; x_.fill(0);v_.fill(0);
 for(auto&r:stiffness_)r.fill(0);
 for(int i=0;i<n_;i++)stiffness_[i][i]=t.anchorSpring;
 for(int j=0;j<t.edges;j++){
  const auto&e=t.links[j];const auto& pa=t.points[e.a];const auto& pb=t.points[e.b];
  const double length=std::sqrt((pa.x-pb.x)*(pa.x-pb.x)+(pa.y-pb.y)*(pa.y-pb.y)+(pa.z-pb.z)*(pa.z-pb.z));
  const double k=e.spring/std::max(.1,length);
  stiffness_[e.a][e.a]+=k;stiffness_[e.b][e.b]+=k;
  stiffness_[e.a][e.b]-=k;stiffness_[e.b][e.a]-=k;
 }
 // SPD effective matrix: M + h^2 K/4. Cholesky is precomputed at note-on,
 // so there are no matrix factorizations on the audio callback.
 const double h2=h_*h_*.25;
 for(auto&r:chol_)r.fill(0);
 for(int i=0;i<n_;i++)for(int j=0;j<=i;j++){
  double v=h2*stiffness_[i][j]+(i==j?topology_.points[i].mass:0.);
  for(int k=0;k<j;k++)v-=chol_[i][k]*chol_[j][k];
  if(i==j){if(v<=0||!valid(v)){n_=0;return false;}chol_[i][j]=std::sqrt(v);}
  else chol_[i][j]=v/chol_[j][j];
 }
 return true;
}
void MaterialMesh::clear() noexcept {x_.fill(0.);v_.fill(0.);work_=0.;}
void MaterialMesh::excite(int index,double amount) noexcept {
 if(index>=0&&index<n_&&valid(amount))v_[index]+=clamp(amount,-100.,100.);
}
double MaterialMesh::step(double force,int drivenIndex) noexcept {
 if(n_<1)return 0.;
 force=valid(force)?clamp(force,-1.e7,1.e7):0.;
 if(drivenIndex<0||drivenIndex>=n_)drivenIndex=0;
 // Implicit midpoint:
 // (M + h²K/4) v' = (M - h²K/4)v - h Kx + h f.
 // x' = x + h(v+v')/2.
 std::array<double,kMeshMasses> rhs{},y{},nv{};
 for(int i=0;i<n_;i++){
  double kx=0.,kv=0.;
  for(int j=0;j<n_;j++){
   kx+=stiffness_[i][j]*x_[j];kv+=stiffness_[i][j]*v_[j];
  }
  rhs[i]=topology_.points[i].mass*v_[i]-h_*kx-h_*h_*.25*kv+(i==drivenIndex?h_*force:0.);
 }
 for(int i=0;i<n_;i++){
  double x=rhs[i];for(int j=0;j<i;j++)x-=chol_[i][j]*y[j];
  y[i]=x/chol_[i][i];
 }
 for(int i=n_-1;i>=0;i--){
  double x=y[i];for(int j=i+1;j<n_;j++)x-=chol_[j][i]*nv[j];
  nv[i]=x/chol_[i][i];
 }
 work_+=h_*force*.5*(v_[drivenIndex]+nv[drivenIndex]);
 for(int i=0;i<n_;i++){x_[i]+=.5*h_*(v_[i]+nv[i]);v_[i]=nv[i];}
 return x_[n_/2];
}
double MaterialMesh::displacement(int i) const noexcept {return (i>=0&&i<n_)?x_[i]:0.;}
double MaterialMesh::velocity(int i) const noexcept {return (i>=0&&i<n_)?v_[i]:0.;}
double MaterialMesh::energy() const noexcept {
 double e=0.;
 for(int i=0;i<n_;i++){
  e+=.5*topology_.points[i].mass*v_[i]*v_[i];
  for(int j=0;j<n_;j++)e+=.5*x_[i]*stiffness_[i][j]*x_[j];
 }
 return e;
}
// Passivity-constrained implicit midpoint with energy bookkeeping.
bool PassiveMesh::configure(const PassiveMeshSettings& s,double sampleRate) noexcept {
 n_=0;
 const auto&t=s.topology;
 if(t.count<1||t.count>kMeshMasses||t.edges<0||t.edges>kMeshEdges||
    !valid(sampleRate)||sampleRate<8000.||sampleRate>384000.||
    !valid(t.anchorSpring)||t.anchorSpring<0.||t.anchorSpring>1.e10)return false;
 for(int i=0;i<t.count;i++){
  const auto&p=t.points[i];
  if(!valid(p.x)||!valid(p.y)||!valid(p.z)||!valid(p.mass)||p.mass<.001||p.mass>1000.||
     !valid(s.groundDamping[(size_t)i])||s.groundDamping[(size_t)i]<0||s.groundDamping[(size_t)i]>1.e7)return false;
 }
 for(int j=0;j<t.edges;j++){
  const auto&e=t.links[j];
  if(e.a<0||e.b<0||e.a>=t.count||e.b>=t.count||e.a==e.b||
     !valid(e.spring)||e.spring<0||e.spring>1.e10||
     !valid(s.edgeDamping[(size_t)j])||s.edgeDamping[(size_t)j]<0||s.edgeDamping[(size_t)j]>1.e7)return false;
 }
 settings_=s;n_=t.count;h_=1./sampleRate;clear();
 for(auto& row:stiffness_)row.fill(0.);
 for(auto& row:damping_)row.fill(0.);
 for(auto& row:chol_)row.fill(0.);
 for(int i=0;i<n_;i++){
  stiffness_[i][i]=t.anchorSpring;
  damping_[i][i]=s.groundDamping[(size_t)i];
 }
 for(int j=0;j<t.edges;j++){
  const auto&e=t.links[j];const auto&a=t.points[e.a];const auto&b=t.points[e.b];
  const double len=std::sqrt((a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y)+(a.z-b.z)*(a.z-b.z));
  const double k=e.spring/std::max(.1,len),c=s.edgeDamping[(size_t)j]/std::max(.1,len);
  for(int sign=0;sign<2;sign++){
   const int i=(sign==0?e.a:e.b), other=(sign==0?e.b:e.a);
   stiffness_[i][i]+=k; stiffness_[i][other]-=k;
   damping_[i][i]+=c; damping_[i][other]-=c;
  }
 }
 // SPD effective mass matrix M + h C/2 + h^2 K/4.
 for(int i=0;i<n_;i++)for(int j=0;j<=i;j++){
  double a=(i==j?t.points[i].mass:0.)+.5*h_*damping_[i][j]+.25*h_*h_*stiffness_[i][j];
  for(int q=0;q<j;q++)a-=chol_[i][q]*chol_[j][q];
  if(i==j){if(a<=0.||!valid(a)){n_=0;return false;}chol_[i][j]=std::sqrt(a);}
  else chol_[i][j]=a/chol_[j][j];
 }
 return true;
}
void PassiveMesh::clear() noexcept {x_.fill(0.);v_.fill(0.);work_=dissipation_=0.;}
void PassiveMesh::excite(int i,double velocity) noexcept {
 if(i>=0&&i<n_&&valid(velocity))v_[(size_t)i]+=clamp(velocity,-100.,100.);
}
double PassiveMesh::step(double force,int drivenIndex) noexcept {
 if(n_<=0)return 0.;
 force=valid(force)?clamp(force,-1.e7,1.e7):0.;
 if(drivenIndex<0||drivenIndex>=n_)drivenIndex=0;
 std::array<double,kMeshMasses> rhs{},y{},nv{},vm{};
 for(int i=0;i<n_;i++){
  double kx=0.,kv=0.,cv=0.;
  for(int j=0;j<n_;j++){
   kx+=stiffness_[i][j]*x_[j];kv+=stiffness_[i][j]*v_[j];cv+=damping_[i][j]*v_[j];
  }
  rhs[i]=settings_.topology.points[(size_t)i].mass*v_[i]
   -.5*h_*cv-.25*h_*h_*kv-h_*kx+(i==drivenIndex?h_*force:0.);
 }
 for(int i=0;i<n_;i++){
  double q=rhs[i];for(int j=0;j<i;j++)q-=chol_[i][j]*y[j];
  y[i]=q/chol_[i][i];
 }
 for(int i=n_-1;i>=0;i--){
  double q=y[i];for(int j=i+1;j<n_;j++)q-=chol_[j][i]*nv[j];
  nv[i]=q/chol_[i][i];
 }
 for(int i=0;i<n_;i++)vm[i]=.5*(v_[i]+nv[i]);
 double lost=0.;
 for(int i=0;i<n_;i++)for(int j=0;j<n_;j++)lost+=h_*vm[i]*damping_[i][j]*vm[j];
 // Roundoff in v^T C v is allowed but a genuinely passive loss cannot be negative.
 dissipation_+=std::max(0.,lost);
 work_+=h_*force*vm[(size_t)drivenIndex];
 for(int i=0;i<n_;i++){x_[i]+=h_*vm[i];v_[i]=nv[i];}
 return x_[(size_t)(n_/2)];
}
double PassiveMesh::displacement(int i) const noexcept{return i>=0&&i<n_?x_[(size_t)i]:0.;}
double PassiveMesh::velocity(int i) const noexcept{return i>=0&&i<n_?v_[(size_t)i]:0.;}
double PassiveMesh::energy() const noexcept{
 double e=0.;
 for(int i=0;i<n_;i++){
  e+=.5*settings_.topology.points[(size_t)i].mass*v_[i]*v_[i];
  for(int j=0;j<n_;j++)e+=.5*x_[i]*stiffness_[i][j]*x_[j];
 }
 return e;
}
// Fit a constrained dictionary (3 discrete topologies x tuning scale) to
// normalized windowed absolute envelope plus normalized autocorrelation.
// The acoustic inverse is nonunique: ranking must not be interpreted as a
// physical-material identification or a general inverse-synthesis solution.
InverseResult estimateMeshFromAudio(const float* samples,int count,double sampleRate) noexcept {
 InverseResult result{};
 for(auto&c:result.best)c.error=std::numeric_limits<double>::infinity();
 if(!samples||count<256||!valid(sampleRate)||sampleRate<8000||sampleRate>192000)return result;
 const int frames=std::min(count,int(sampleRate*.18));
 // Extract amplitude-independent multi-lag autocorrelation shape.
 constexpr int lags=16;
 std::array<double,lags> ref{};
 double referencePower=0.;
 for(int i=0;i<frames;i++)referencePower+=double(samples[i])*samples[i];
 if(referencePower<1.e-15)return result;
 for(int l=0;l<lags;l++){
  const int lag=1+(l*l*5);
  double c=0.;for(int i=lag;i<frames;i++)c+=double(samples[i])*samples[i-lag];
  ref[l]=c/referencePower;
 }
 // Analyze short onset model with a fixed impulse, then optimize over pitch
 // and tension. CPU-expensive, intended for explicit OFFLINE use only.
 for(int shape=0;shape<3;shape++)for(int pitch=0;pitch<19;pitch++)for(int t=0;t<3;t++){
  const double hz=110.*std::pow(2.,pitch/12.);
  const double tension=.7+.3*t;
  MaterialMesh mesh;
  if(!mesh.configure(makeMeshTopology(shape,hz,tension),sampleRate))continue;
  mesh.excite(0,.8);
  std::array<double,lags> corr{};
  std::array<double,1200> delay{};
  double pow=0.;
  for(int i=0;i<frames;i++){
   double x=mesh.step();x=clamp(x*200.,-1.,1.);
   pow+=x*x;
   for(int l=0;l<lags;l++){const int lag=1+l*l*5;if(i>=lag)corr[l]+=x*delay[(size_t)(i-lag)%delay.size()];}
   delay[(size_t)i%delay.size()]=x;
  }
  double err=0.;if(pow<1.e-18)continue;
  for(int l=0;l<lags;l++){double d=corr[l]/pow-ref[l];err+=d*d;}
  InverseCandidate candidate{shape,hz,tension,err,1.};
  ++result.evaluated;
  for(int j=0;j<3;j++)if(err<result.best[(size_t)j].error){
   for(int k=2;k>j;k--)result.best[(size_t)k]=result.best[(size_t)(k-1)];
   result.best[(size_t)j]=candidate;break;
  }
 }
 return result;
}
} // namespace panorama
