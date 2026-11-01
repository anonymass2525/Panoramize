#include "BoundaryPorts41.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <utility>
namespace panorama::boundary41 {
namespace {
bool bound(double a,double low,double high){return std::isfinite(a)&&a>=low&&a<=high;}
constexpr double rho=1.20,cSound=343.;
using V3=std::array<double,3>;
V3 cross(const V3&a,const V3&b){return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
double dot(const V3&a,const V3&b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
V3 diff(const V3&a,const V3&b){return {a[0]-b[0],a[1]-b[1],a[2]-b[2]};}
}
Recipe example(){return {};}
bool valid(const Recipe&r,std::string*error){
 auto fail=[&](const char* msg){if(error)*error=msg;return false;};
 if(r.stringPoints<8||r.stringPoints>20||r.membranePoints<3||r.membranePoints>4||!gen37::validShape(r.bodyShape))return fail("bad spatial topology");
 // N <= 20+16+3*32+1; cap actual mesh below 24 vertices for dense realtime method.
 const int meshNodes=(r.bodyShape.xCells+1)*(r.bodyShape.yCells+1)*(r.bodyShape.zCells+1);
 if(meshNodes>12)return fail("4.1 dense solver supports <=12 body vertices");
 const std::array<std::pair<double,double>,21> checks{{
 {r.stringMass,1e-8},{r.stringEdgeStiffness,1e-8},{r.stringDamping,0.},
 {r.membraneMass,1e-8},{r.membraneEdgeStiffness,1e-8},{r.membraneDamping,0.},
 {r.bodyYoung,1e4},{r.bodyDensity,50.},{r.bodyPoisson,.01},{r.bodyDamping,0.},
 {r.stringMembraneK,0.},{r.stringMembraneD,0.},{r.membraneBodyK,0.},{r.membraneBodyD,0.},
 {r.pistonArea,1e-5},{r.cavityVolume,1e-4},{r.cavityLength,.01},
 {r.bodyCavityK,0.},{r.bodyCavityD,0.},{r.soundGain,0.},{r.exciteN,0.}}};
 for(const auto&p:checks)if(!std::isfinite(p.first)||p.first<p.second||p.first>1e9)return fail("invalid SI coefficient");
 if(r.bodyPoisson>=.46||r.pistonArea>.1||r.cavityVolume>2||r.cavityLength>2||r.soundGain>1e5||r.exciteN>1e4)return fail("coefficient out of bounds");
 if(!bound(r.velocityToEffort,-1e5,1e5)||!bound(r.flowToForce,-1e5,1e5)||!bound(r.activeForceLimit,1e-6,1e5)||!bound(r.activeEffortLimit,1e-6,1e5))return fail("invalid active transducer");
 if(r.nativeEnabled&&(r.nativeMode<0||r.nativeMode>=int(r.native.modes.size())||r.native.modes.size()>16))return fail("invalid native mode");
 if(error)error->clear();return true;
}
std::string serialize(const Recipe&r){
 std::ostringstream s;s<<std::setprecision(17)<<"PANORAMIZE-BOUNDARY41|1\n";
 s<<"DIM "<<r.stringPoints<<' '<<r.membranePoints<<' '<<r.bodyShape.xCells<<' '<<r.bodyShape.yCells<<' '<<r.bodyShape.zCells<<' '
 <<r.bodyShape.length<<' '<<r.bodyShape.width<<' '<<r.bodyShape.thickness<<' '<<r.bodyShape.camber<<' '<<r.bodyShape.taper<<'\n';
 s<<"SI "<<r.stringMass<<' '<<r.stringEdgeStiffness<<' '<<r.stringDamping<<' '
 <<r.membraneMass<<' '<<r.membraneEdgeStiffness<<' '<<r.membraneDamping<<' '
 <<r.bodyYoung<<' '<<r.bodyDensity<<' '<<r.bodyPoisson<<' '<<r.bodyDamping<<' '
 <<r.stringMembraneK<<' '<<r.stringMembraneD<<' '<<r.membraneBodyK<<' '<<r.membraneBodyD<<' '
 <<r.pistonArea<<' '<<r.cavityVolume<<' '<<r.cavityLength<<' '<<r.bodyCavityK<<' '<<r.bodyCavityD<<' '
 <<r.soundGain<<' '<<r.exciteN<<'\n';
 s<<"ACTIVE "<<int(r.nativeEnabled)<<' '<<r.nativeMode<<' '<<r.velocityToEffort<<' '<<r.flowToForce<<' '<<r.activeForceLimit<<' '<<r.activeEffortLimit<<'\n';
 if(r.nativeEnabled)s<<"NATIVE_BEGIN\n"<<native27::serialize(r.native)<<"NATIVE_END\n";
 return s.str();
}
bool deserialize(const std::string&text,Recipe&out,std::string*error){
 auto fail=[&](const char*e){if(error)*error=e;return false;};
 if(text.size()>40000)return fail("oversized patch");
 std::istringstream in(text);std::string line,tag,extra;Recipe r;
 if(!std::getline(in,line)||line!="PANORAMIZE-BOUNDARY41|1")return fail("wrong version");
 if(!std::getline(in,line))return fail("missing DIM");
 {std::istringstream s(line);
 if(!(s>>tag>>r.stringPoints>>r.membranePoints>>r.bodyShape.xCells>>r.bodyShape.yCells>>r.bodyShape.zCells>>r.bodyShape.length>>r.bodyShape.width>>r.bodyShape.thickness>>r.bodyShape.camber>>r.bodyShape.taper)||tag!="DIM"||(s>>extra))return fail("bad DIM");}
 if(!std::getline(in,line))return fail("missing SI");
 {std::istringstream s(line);
 if(!(s>>tag>>r.stringMass>>r.stringEdgeStiffness>>r.stringDamping>>r.membraneMass>>r.membraneEdgeStiffness>>r.membraneDamping>>r.bodyYoung>>r.bodyDensity>>r.bodyPoisson>>r.bodyDamping>>r.stringMembraneK>>r.stringMembraneD>>r.membraneBodyK>>r.membraneBodyD>>r.pistonArea>>r.cavityVolume>>r.cavityLength>>r.bodyCavityK>>r.bodyCavityD>>r.soundGain>>r.exciteN)||tag!="SI"||(s>>extra))return fail("bad SI");}
 if(!std::getline(in,line))return fail("missing ACTIVE");
 {std::istringstream s(line);int enabled=0;
 if(!(s>>tag>>enabled>>r.nativeMode>>r.velocityToEffort>>r.flowToForce>>r.activeForceLimit>>r.activeEffortLimit)||tag!="ACTIVE"||(enabled!=0&&enabled!=1)||(s>>extra))return fail("bad ACTIVE");r.nativeEnabled=enabled!=0;}
 if(r.nativeEnabled){
  if(!std::getline(in,line)||line!="NATIVE_BEGIN")return fail("missing native block");
  std::string payload;bool closed=false;
  while(std::getline(in,line)){if(line=="NATIVE_END"){closed=true;break;}payload+=line+'\n';if(payload.size()>35000)return fail("oversized native block");}
  if(!closed||!native27::deserialize(payload,r.native,error))return fail("invalid native block");
 }
 if(std::getline(in,line))return fail("trailing data");
 if(!valid(r,error))return false;
 out=std::move(r);return true;
}
void Instrument::addEntry(std::vector<double>&a,int i,int j,double val)noexcept{a[size_t(i*n_+j)]+=val;}
void Instrument::addRelative(const Port&a,const Port&b,double k,double d)noexcept{
 // Single relative extension delta=(w_a.q - w_b.q). Force distributed by transpose
 // of the interpolation Jacobian; port power is strictly reciprocal.
 for(const auto&u:a.weights)for(const auto&v:a.weights){addEntry(k_,u.first,v.first,k*u.second*v.second);addEntry(d_,u.first,v.first,d*u.second*v.second);}
 for(const auto&u:b.weights)for(const auto&v:b.weights){addEntry(k_,u.first,v.first,k*u.second*v.second);addEntry(d_,u.first,v.first,d*u.second*v.second);}
 for(const auto&u:a.weights)for(const auto&v:b.weights){addEntry(k_,u.first,v.first,-k*u.second*v.second);addEntry(k_,v.first,u.first,-k*u.second*v.second);addEntry(d_,u.first,v.first,-d*u.second*v.second);addEntry(d_,v.first,u.first,-d*u.second*v.second);}
}
std::unique_ptr<Instrument> Instrument::prepare(const Recipe&r,double sr,std::string*error){
 auto fail=[&](const char*msg)->std::unique_ptr<Instrument>{if(error)*error=msg;return {};};
 if(!valid(r,error)||!bound(sr,22050,192000))return fail("invalid recipe or rate");
 fem36::Mesh mesh;std::string why;
 if(!gen37::buildTetraMesh(r.bodyShape,mesh,&why))return fail("invalid tetra geometry");
 auto p=std::unique_ptr<Instrument>(new Instrument());p->recipe_=r;p->sr_=sr;p->dt_=1/sr;
 p->sizes_[0]=r.stringPoints;p->sizes_[1]=r.membranePoints*r.membranePoints;
 p->sizes_[2]=int(mesh.vertices.size())*3;p->sizes_[3]=1;
 p->offsets_[0]=0;for(int i=1;i<4;i++)p->offsets_[i]=p->offsets_[i-1]+p->sizes_[i-1];
 p->n_=p->offsets_[3]+1;const int n=p->n_;
 p->mass_.assign(size_t(n),0);p->q_.assign(size_t(n),0);p->v_.assign(size_t(n),0);
 p->external_.assign(size_t(n),0);p->k_.assign(size_t(n*n),0);p->d_.assign(size_t(n*n),0);
 p->lhs_.assign(size_t(n*n),0);p->chol_.assign(size_t(n*n),0);p->rhs_.assign(size_t(n),0);p->vmid_.assign(size_t(n),0);
 auto spring=[&](int a,int b,double k){p->addEntry(p->k_,a,a,k);if(b>=0){p->addEntry(p->k_,b,b,k);p->addEntry(p->k_,a,b,-k);p->addEntry(p->k_,b,a,-k);}};
 // 1D transverse displacement, fixed ends; distributed force is specified in N.
 for(int i=0;i<r.stringPoints;i++){
  int index=i;p->mass_[size_t(index)]=r.stringMass;
  p->addEntry(p->d_,index,index,r.stringDamping);
  if(i+1<r.stringPoints)spring(index,index+1,r.stringEdgeStiffness);
  if(i==0||i==r.stringPoints-1)spring(index,-1,r.stringEdgeStiffness*3);
 }
 // 2D membrane, 3x3/4x4 grid, symmetric energy quadratic form.
 const int nx=r.membranePoints,o=p->offsets_[1];
 for(int y=0;y<nx;y++)for(int x=0;x<nx;x++){
  int a=o+x+nx*y;p->mass_[size_t(a)]=r.membraneMass;
  p->addEntry(p->d_,a,a,r.membraneDamping);
  if(x+1<nx)spring(a,a+1,r.membraneEdgeStiffness);
  if(y+1<nx)spring(a,a+nx,r.membraneEdgeStiffness);
  if(x==0||y==0||x==nx-1||y==nx-1)spring(a,-1,r.membraneEdgeStiffness*.4);
 }
 // True small-deformation 3D tetrahedron: local B^T D B, with isotropic positive
 // elasticity and lumped mass. No invented nodal contact stiffness here.
 const int fo=p->offsets_[2];double E=r.bodyYoung,nu=r.bodyPoisson;
 const double mu=E/(2*(1+nu)),lam=E*nu/((1+nu)*(1-2*nu));
 for(const auto&t:mesh.tetrahedra){
  const auto&x0=mesh.vertices[size_t(t[0])];
  const V3 a=diff(mesh.vertices[size_t(t[1])],x0),b=diff(mesh.vertices[size_t(t[2])],x0),c=diff(mesh.vertices[size_t(t[3])],x0);
  const double det=dot(a,cross(b,c));if(!std::isfinite(det)||std::abs(det)<1e-14)return fail("degenerate tetrahedron");
  const double volume=std::abs(det)/6.;
  const V3 g1=cross(b,c),g2=cross(c,a),g3=cross(a,b);
  V3 g[4]{};for(int axis=0;axis<3;axis++){
   g[1][size_t(axis)]=g1[size_t(axis)]/det;g[2][size_t(axis)]=g2[size_t(axis)]/det;g[3][size_t(axis)]=g3[size_t(axis)]/det;
   g[0][size_t(axis)]=-g[1][size_t(axis)]-g[2][size_t(axis)]-g[3][size_t(axis)];
  }
  double B[6][12]{};
  for(int j=0;j<4;j++){
   const double gx=g[j][0],gy=g[j][1],gz=g[j][2];
   B[0][3*j]=gx;B[1][3*j+1]=gy;B[2][3*j+2]=gz;
   B[3][3*j+1]=gz;B[3][3*j+2]=gy;B[4][3*j]=gz;B[4][3*j+2]=gx;B[5][3*j]=gy;B[5][3*j+1]=gx;
  }
  double C[6][6]{};for(int j=0;j<3;j++)for(int k=0;k<3;k++)C[j][k]=lam+(j==k?2*mu:0);
  for(int j=3;j<6;j++)C[j][j]=mu;
  for(int j=0;j<4;j++)for(int axis=0;axis<3;axis++)p->mass_[size_t(fo+3*t[size_t(j)]+axis)]+=volume*r.bodyDensity/4.;
  for(int j=0;j<12;j++)for(int k=0;k<12;k++){
   double val=0;for(int u=0;u<6;u++)for(int v=0;v<6;v++)val+=B[u][j]*C[u][v]*B[v][k];
   p->addEntry(p->k_,fo+3*t[size_t(j/3)]+j%3,fo+3*t[size_t(k/3)]+k%3,volume*val);
  }
 }
 // Tiny pinning spring to remove the otherwise unconstrained rigid drift.
 for(int j=0;j<3;j++)spring(fo+j,-1,r.bodyYoung*1e-7);
 for(int i=fo;i<fo+p->sizes_[2];i++)p->addEntry(p->d_,i,i,r.bodyDamping);
 // One SI acoustic cavity generalized coordinate q=displaced volume [m^3], v=U [m^3/s].
 // Momentum effort is pressure [Pa], M=rho*length/area, K=rho*c^2/volume.
 const int air=p->offsets_[3];p->mass_[size_t(air)]=rho*r.cavityLength/r.pistonArea;
 spring(air,-1,rho*cSound*cSound/r.cavityVolume);
 p->ports_[0].weights={{r.stringPoints/2,1.}};
 p->ports_[1].weights={{o+(nx/2)+(nx/2)*nx,1.}};
 // Uniform two-point tetrahedral boundary patch; transpose weights distribute
 // exactly the opposite effort, preserving power at the discrete interface.
 const int nv=int(mesh.vertices.size());
 p->ports_[2].weights={{fo+3*(nv-1)+2,.5},{fo+3*(nv-2)+2,.5}};
 p->ports_[3].weights={{air,1.}};
 p->addRelative(p->ports_[0],p->ports_[1],r.stringMembraneK,r.stringMembraneD);
 p->addRelative(p->ports_[1],p->ports_[2],r.membraneBodyK,r.membraneBodyD);
 Port piston=p->ports_[2];for(auto&w:piston.weights)w.second*=r.pistonArea;
 p->addRelative(piston,p->ports_[3],r.bodyCavityK,r.bodyCavityD);
 // Implicit midpoint matrix prepared OFF the audio callback.
 for(int i=0;i<n;i++)for(int j=0;j<n;j++){
  p->lhs_[size_t(i*n+j)]=p->d_[size_t(i*n+j)]+.5*p->dt_*p->k_[size_t(i*n+j)];
  if(i==j)p->lhs_[size_t(i*n+j)]+=2*p->mass_[size_t(i)]/p->dt_;
 }
 if(!p->factorize())return fail("non-positive implicit matrix");
 if(r.nativeEnabled){p->native_=native27::Instrument::prepare(r.native,sr,&why);if(!p->native_)return fail("native state failed to prepare");}
 p->initial_=p->energy();if(error)error->clear();return p;
}
bool Instrument::factorize()noexcept{
 for(int i=0;i<n_;i++)for(int j=0;j<=i;j++){
  double s=lhs_[size_t(i*n_+j)];
  for(int k=0;k<j;k++)s-=chol_[size_t(i*n_+k)]*chol_[size_t(j*n_+k)];
  if(i==j){if(!(s>0)&&std::isfinite(s))return false;if(!std::isfinite(s)||s<=0)return false;chol_[size_t(i*n_+j)]=std::sqrt(s);}
  else chol_[size_t(i*n_+j)]=s/chol_[size_t(j*n_+j)];
 }return true;
}
void Instrument::solve()noexcept{
 for(int i=0;i<n_;i++){
  double s=rhs_[size_t(i)];for(int j=0;j<i;j++)s-=chol_[size_t(i*n_+j)]*vmid_[size_t(j)];
  vmid_[size_t(i)]=s/chol_[size_t(i*n_+i)];
 }
 for(int i=n_-1;i>=0;i--){
  double s=vmid_[size_t(i)];for(int j=i+1;j<n_;j++)s-=chol_[size_t(j*n_+i)]*vmid_[size_t(j)];
  vmid_[size_t(i)]=s/chol_[size_t(i*n_+i)];
 }
}
double Instrument::portQ(const Port&p)const noexcept{double s=0;for(auto&w:p.weights)s+=w.second*q_[size_t(w.first)];return s;}
double Instrument::portV(const Port&p)const noexcept{double s=0;for(auto&w:p.weights)s+=w.second*v_[size_t(w.first)];return s;}
double Instrument::boundaryDisplacement(int r)const noexcept{return r>=0&&r<4?portQ(ports_[r]):0;}
double Instrument::boundaryVelocity(int r)const noexcept{return r>=0&&r<4?portV(ports_[r]):0;}
double Instrument::cavityPressure()const noexcept{return (rho*cSound*cSound/recipe_.cavityVolume)*q_[size_t(offsets_[3])];}
double Instrument::cavityVolumeFlow()const noexcept{return v_[size_t(offsets_[3])];}
void Instrument::noteOn(int midi,float velocity)noexcept{
 if(midi<0||midi>127||!std::isfinite(velocity)||velocity<=0)return;
 midi_=midi;external_[size_t(recipe_.stringPoints/3)]+=recipe_.exciteN*std::clamp(double(velocity),0.,1.);
 if(native_)native_->noteOn(midi,velocity);
}
void Instrument::noteOff(int midi)noexcept{if(midi==midi_)midi_=-1;if(native_)native_->noteOff(midi);}
void Instrument::allNotesOff()noexcept{midi_=-1;if(native_)native_->allNotesOff();}
void Instrument::pitchBend(float semitones)noexcept{if(native_)native_->pitchBend(semitones);}
float Instrument::processOne()noexcept{
 // Delayed native state remains an ACTIVE port; SI spatial/cavity coordinates
 // below are nevertheless all solved implicitly in the same matrix and step.
 if(native_&&midi_>=0){
  const double f=std::clamp(delayedNative_*recipe_.flowToForce,-recipe_.activeForceLimit,recipe_.activeForceLimit);
  for(const auto&w:ports_[2].weights)external_[size_t(w.first)]+=f*w.second;
  const double effort=std::clamp(delayedMechanical_*recipe_.velocityToEffort,-recipe_.activeEffortLimit,recipe_.activeEffortLimit);
  native_->addExternalEffort(midi_,recipe_.nativeMode,effort);
 }
 for(int i=0;i<n_;i++){
  double kv=0;for(int j=0;j<n_;j++)kv+=k_[size_t(i*n_+j)]*q_[size_t(j)];
  rhs_[size_t(i)]=2*mass_[size_t(i)]/dt_*v_[size_t(i)]-kv+external_[size_t(i)];
 }
 solve();
 double extPower=0,lossPower=0;
 for(int i=0;i<n_;i++){
  extPower+=external_[size_t(i)]*vmid_[size_t(i)];
  for(int j=0;j<n_;j++)lossPower+=vmid_[size_t(i)]*d_[size_t(i*n_+j)]*vmid_[size_t(j)];
 }
 work_+=dt_*extPower;dissipated_+=dt_*std::max(0.,lossPower);
 for(int i=0;i<n_;i++){q_[size_t(i)]+=dt_*vmid_[size_t(i)];v_[size_t(i)]=2*vmid_[size_t(i)]-v_[size_t(i)];external_[size_t(i)]=0;}
 delayedMechanical_=portV(ports_[2]);
 double nativeAudio=0;if(native_){nativeAudio=native_->processMono();delayedNative_=midi_>=0?native_->modeFlow(midi_,recipe_.nativeMode):0;}
 frames_++;
 // The air cavity's pressure [Pa] is converted only at the OUTPUT stage.
 const double out=recipe_.soundGain*(.5*portV(ports_[0])+.22*portV(ports_[2])+.000015*cavityPressure())+.15*nativeAudio;
 return std::isfinite(out)?float(std::tanh(out)):0;
}
double Instrument::energy()const noexcept{
 double sum=0;for(int i=0;i<n_;i++){
  sum+=.5*mass_[size_t(i)]*v_[size_t(i)]*v_[size_t(i)];
  for(int j=0;j<n_;j++)sum+=.5*q_[size_t(i)]*k_[size_t(i*n_+j)]*q_[size_t(j)];
 }return sum;
}
double Instrument::regionEnergy(int region)const noexcept{
 if(region<0||region>=4)return 0;double sum=0;
 for(int i=offsets_[region];i<offsets_[region]+sizes_[region];i++){
  sum+=.5*mass_[size_t(i)]*v_[size_t(i)]*v_[size_t(i)];
  // share half of boundary potential to both neighboring sides (K q is global)
  for(int j=0;j<n_;j++)sum+=.5*q_[size_t(i)]*k_[size_t(i*n_+j)]*q_[size_t(j)];
 }return sum;
}
Ledger Instrument::ledger()const noexcept{
 Ledger l;l.initial=initial_;l.active=work_;l.dissipated=dissipated_;l.finalEnergy=energy();
 l.residual=l.finalEnergy-l.initial-l.active+l.dissipated;
 if(native_)l.nativeActiveWork=native_->activeWork();return l;
}
} // namespace panorama::boundary41
