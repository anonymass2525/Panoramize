#include "NonlinearPassive45.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <utility>
namespace panorama::passive45 {
namespace {
constexpr double pi=3.1415926535897932384626;
bool good(double x,double lo,double hi){return std::isfinite(x)&&x>=lo&&x<=hi;}
double norm3(double x,double y,double z){return std::sqrt(x*x+y*y+z*z);}
}
Recipe example(){
 Recipe r;r.spatial=sparse43::example();r.native=native27::example();
 // Use a planar 3D FEM boundary dof for an illustrative compliant stop.
 r.contacts.push_back({2,1,0.,3200.,.18,1.e-5});
 r.stretchEdges.push_back({2,1,2,600.});
 r.modalTransformer=.01;r.modalCoupling=80.;r.nonlinearIterations=30;r.nonlinearTolerance=1.e-9;
 return r;
}
bool validate(const Recipe&r,std::string*why){
 auto fail=[&](const char*s){if(why)*why=s;return false;};
 if(!sparse43::valid(r.spatial,why))return false;
 if(r.native.modes.empty()||r.native.modes.size()>48||r.native.edges.size()>192||
    !good(r.modalTransformer,1.e-8,100)||!good(r.modalCoupling,0,1.e8)||
    r.nonlinearIterations<2||r.nonlinearIterations>48||r.linearIterations<8||r.linearIterations>700||
    !good(r.nonlinearTolerance,1.e-12,1.e-4)||r.contacts.size()>32||r.stretchEdges.size()>64)
  return fail("unsupported nonlinear solver settings");
 for(const auto&m:r.native.modes){
  if(int(m.engine)<0||int(m.engine)>7||!good(m.ratio,.05,16)||!good(m.damping,0,2000)||
     !good(m.nonlinear,0,1.e16)||!good(m.pickup,-10,10)||!good(m.excitation,0,1.e5))return fail("invalid Native27 mode");
  if(r.strictPassive&&m.excitation>0)return fail("powered Native27 oscillator cannot be silently declared passive");
 }
 for(const auto&l:r.native.edges)if(l.a<0||l.b<0||l.a>=int(r.native.modes.size())||l.b>=int(r.native.modes.size())||l.a==l.b||
    !good(l.strength,0,1.e8)||!good(l.ratio,1.e-4,100)||
    (l.kind!=native26::Coupling::Conservative&&l.kind!=native26::Coupling::Dissipative)||
    l.quantity!=unified22::Quantity::NormalizedCoordinate||l.unit!=unified22::Unit::RootJouleSeconds)
   return fail("Native27 edge requires certified normalized passive coupling");
 for(const auto&c:r.contacts)if(c.normalPort<0||c.tangentPort<0||c.normalPort>=int(r.spatial.ports.size())||
    c.tangentPort>=int(r.spatial.ports.size())||
    r.spatial.ports[size_t(c.normalPort)].unit!=sparse43::PortUnit::Metres||
    r.spatial.ports[size_t(c.tangentPort)].unit!=sparse43::PortUnit::Metres||
    !good(c.gap,-.1,.1)||!good(c.stiffness,0,1.e8)||!good(c.friction,0,2)||!good(c.slipSpeed,1.e-7,1))
   return fail("contact requires mechanical displacement/velocity ports");
 for(const auto&e:r.stretchEdges){
  if(e.region<0||e.region>=int(r.spatial.regions.size()))return fail("invalid deforming mesh region");
  const auto&g=r.spatial.regions[size_t(e.region)];
  if(g.family!=sparse43::Family::Elastic3D||e.vertexA<0||e.vertexB<0||e.vertexA>=int(g.vertices.size())||
    e.vertexB>=int(g.vertices.size())||e.vertexA==e.vertexB||!good(e.stiffness,0,1.e8))return fail("invalid finite-strain mesh edge");
 }
 if(why)why->clear();return true;
}
std::string serialize(const Recipe&r){
 std::ostringstream o;o<<std::setprecision(17)<<"PANORAMIZE-PASSIVE45|1\n";
 o<<"SET "<<r.modalTransformer<<' '<<r.modalCoupling<<' '<<r.strictPassive<<' '<<r.nonlinearIterations<<' '
  <<r.linearIterations<<' '<<r.nonlinearTolerance<<' '<<r.contacts.size()<<' '<<r.stretchEdges.size()<<'\n';
 for(const auto&c:r.contacts)o<<"C "<<c.normalPort<<' '<<c.tangentPort<<' '<<c.gap<<' '<<c.stiffness<<' '<<c.friction<<' '<<c.slipSpeed<<'\n';
 for(const auto&e:r.stretchEdges)o<<"G "<<e.region<<' '<<e.vertexA<<' '<<e.vertexB<<' '<<e.stiffness<<'\n';
 o<<"SPARSE43_BEGIN\n"<<sparse43::serialize(r.spatial)<<"SPARSE43_END\n";
 o<<"NATIVE27_BEGIN\n"<<native27::serialize(r.native)<<"NATIVE27_END\n";
 return o.str();
}
bool deserialize(const std::string&text,Recipe&out,std::string*why){
 auto fail=[&](const char*s){if(why)*why=s;return false;};
 if(text.size()>2100000)return fail("oversized 4.5 graph");
 std::istringstream in(text);std::string line,tag,extra;Recipe r;int strict=0,nc=-1,ng=-1;
 if(!std::getline(in,line)||line!="PANORAMIZE-PASSIVE45|1"||!std::getline(in,line))return fail("bad header");
 std::istringstream opts(line);
 if(!(opts>>tag>>r.modalTransformer>>r.modalCoupling>>strict>>r.nonlinearIterations>>r.linearIterations>>r.nonlinearTolerance>>nc>>ng)||
    tag!="SET"||strict<0||strict>1||nc<0||nc>32||ng<0||ng>64||(opts>>extra))return fail("bad configuration");
 r.strictPassive=bool(strict);
 for(int i=0;i<nc;i++){
  if(!std::getline(in,line))return fail("missing contact");std::istringstream row(line);Contact c;
  if(!(row>>tag>>c.normalPort>>c.tangentPort>>c.gap>>c.stiffness>>c.friction>>c.slipSpeed)||tag!="C"||(row>>extra))return fail("bad contact");
  r.contacts.push_back(c);
 }
 for(int i=0;i<ng;i++){
  if(!std::getline(in,line))return fail("missing stretch");std::istringstream row(line);StretchEdge e;
  if(!(row>>tag>>e.region>>e.vertexA>>e.vertexB>>e.stiffness)||tag!="G"||(row>>extra))return fail("bad stretch");
  r.stretchEdges.push_back(e);
 }
 auto block=[&](const char*begin,const char*end,std::string&target){
  if(!std::getline(in,line)||line!=begin)return false;
  while(std::getline(in,line)){
   if(line==end)return true;
   target+=line+'\n';if(target.size()>2050000)return false;
  }return false;
 };
 std::string spatial,native;
 if(!block("SPARSE43_BEGIN","SPARSE43_END",spatial)||!block("NATIVE27_BEGIN","NATIVE27_END",native)||std::getline(in,line))return fail("bad nested graph blocks");
 if(!sparse43::deserialize(spatial,r.spatial,why)||!native27::deserialize(native,r.native,why)||!validate(r,why))return false;
 out=std::move(r);return true;
}
void Instrument::apply(const std::vector<double>&a,const std::vector<double>&x,std::vector<double>&y)const noexcept{
 for(int i=0;i<full_.n;i++){
  double sum=0.;for(int j=full_.ptr[size_t(i)];j<full_.ptr[size_t(i+1)];j++)sum+=a[size_t(j)]*x[size_t(full_.index[size_t(j)])];
  y[size_t(i)]=sum;
 }
}
double Instrument::dot(const std::vector<double>&a,const std::vector<double>&b)const noexcept{
 double x=0;for(size_t i=0;i<a.size();i++)x+=a[i]*b[i];return x;
}
double Instrument::rowValue(const Row&r,const std::vector<double>&x)const noexcept{
 double value=0;for(auto&term:r.terms)value+=term.second*x[size_t(term.first)];return value;
}
std::unique_ptr<Instrument> Instrument::prepare(const Recipe&r,double rate,std::string*why){
 auto fail=[&](const char*s)->std::unique_ptr<Instrument>{if(why)*why=s;return {};};
 if(!validate(r,why)||!good(rate,22050,192000))return fail("invalid 4.5 recipe/rate");
 auto combined=r.spatial;
 const int nativeRegion=int(combined.regions.size());
 sparse43::Region modes;modes.family=sparse43::Family::SpectralModal;modes.damping=0.;
 for(const auto&m:r.native.modes)modes.modes.push_back({110.*m.ratio,m.damping});
 combined.regions.push_back(modes);
 const int basePort=int(combined.ports.size());
 for(int i=0;i<int(r.native.modes.size());i++)combined.ports.push_back({nativeRegion,sparse43::PortUnit::NormalizedModal,{{i,1.}}});
 for(int i=0;i<int(r.native.modes.size());i++){
  if(r.modalCoupling>0)combined.links.push_back({combined.excitePort,basePort+i,sparse43::PortUnit::Metres,1,1,
    r.modalCoupling,.02,1,r.modalTransformer});
 }
 for(const auto&e:r.native.edges)combined.links.push_back({basePort+e.a,basePort+e.b,sparse43::PortUnit::Metres,1,1,
    e.kind==native26::Coupling::Conservative?e.strength:0.,
    e.kind==native26::Coupling::Dissipative?e.strength:0.,r.modalTransformer,r.modalTransformer*e.ratio});
 if(!sparse43::valid(combined,why))return fail("invalid spatial/native normalized port combination");
 auto core=sparse43::Instrument::prepare(combined,rate,why);if(!core)return {};
 auto x=std::unique_ptr<Instrument>(new Instrument());x->recipe_=r;x->full_=core->exportAssembled();x->h_=1./rate;
 x->nativeOffset_=x->full_.regionOffset[size_t(nativeRegion)];x->excitePort_=combined.excitePort;x->outputPort_=combined.outputPort;
 for(const auto&p:x->full_.ports){Row row;row.terms=p.weights;x->ports_.push_back(std::move(row));}
 for(const auto&c:r.contacts)x->contacts_.push_back({x->ports_[size_t(c.normalPort)],x->ports_[size_t(c.tangentPort)],c.gap,c.stiffness,c.friction,c.slipSpeed});
 for(const auto&e:r.stretchEdges){
  const auto&g=r.spatial.regions[size_t(e.region)];const auto&a=g.vertices[size_t(e.vertexA)],&b=g.vertices[size_t(e.vertexB)];
  double len=norm3(a.x-b.x,a.y-b.y,a.z-b.z);if(len<1e-8)return fail("degenerate nonlinear mesh edge");
  int off=x->full_.regionOffset[size_t(e.region)];x->stretches_.push_back({off+3*e.vertexA,off+3*e.vertexB,len,e.stiffness,{a.x-b.x,a.y-b.y,a.z-b.z}});
 }
 x->stats_.dofs=x->full_.n;x->stats_.contacts=int(x->contacts_.size());x->stats_.stretches=int(x->stretches_.size());x->stats_.nativeModes=int(r.native.modes.size());
 for(const auto&m:r.native.modes)if(m.excitation>0)x->stats_.activeModes++;
 const size_t n=size_t(x->full_.n);auto alloc=[&](std::vector<double>&v){v.assign(n,0.);};
 for(auto*v:{&x->q_,&x->v_,&x->trial_,&x->vmid_,&x->rhs_,&x->res_,&x->dir_,&x->z_,&x->Ap_,&x->diag_,&x->grad_,&x->lossForce_,&x->force_,&x->scratchK_,&x->scratchR_,&x->scratchA_,&x->qnext_})alloc(*v);
 x->origModeStiffness_.resize(r.native.modes.size(),0.);
 x->origModeDamping_.resize(r.native.modes.size(),0.);
 for(size_t i=0;i<r.native.modes.size();i++){
  x->origModeStiffness_[i]=r.native.modes[i].nonlinear*std::pow(r.modalTransformer,4);
  x->origModeDamping_[i]=r.native.modes[i].damping;
 }
 for(int i=0;i<x->full_.n;i++){
  double diag=2.*x->full_.mass[size_t(i)]/x->h_;
  for(int k=x->full_.ptr[size_t(i)];k<x->full_.ptr[size_t(i+1)];k++)if(x->full_.index[size_t(k)]==i)
   diag+=x->full_.R[size_t(k)]+.5*x->h_*x->full_.K[size_t(k)];
  if(!good(diag,1e-15,1e30))return fail("nonpositive linearized diagonal");
  x->diag_[size_t(i)]=1./diag;
 }
 if(why)why->clear();return x;
}
// Both conservative gradients below use exact discrete gradients:
// Delta U = grad_DG dot (q_new-q_old) within each nonlinear element.
void Instrument::nonlinear(const std::vector<double>&next,const std::vector<double>&midVel)noexcept{
 std::fill(grad_.begin(),grad_.end(),0.);
 std::fill(lossForce_.begin(),lossForce_.end(),0.);
 for(const auto&c:contacts_){
  const double za=rowValue(c.normal,q_)-c.gap,zb=rowValue(c.normal,next)-c.gap;
  const auto u=[&](double z){double a=std::max(0.,z);return .5*c.k*a*a;};
  const double dg=std::abs(zb-za)>1e-16?(u(zb)-u(za))/(zb-za):c.k*std::max(0.,.5*(za+zb));
  const double vtan=rowValue(c.tangent,midVel);
  const double normal=std::max(0.,c.k*.5*(std::max(0.,za)+std::max(0.,zb)));
  const double friction=c.mu*normal*std::tanh(vtan/c.v0);
  for(const auto&t:c.normal.terms)grad_[size_t(t.first)]+=dg*t.second;
  for(const auto&t:c.tangent.terms)lossForce_[size_t(t.first)]+=friction*t.second;
 }
 for(const auto&e:stretches_){
  const auto&rg=recipe_.spatial.regions;
  (void)rg;
  // The reference edge vector is reconstructed from the original tetra geometry.
  // Cached at prepare in the full edge record through the region/vertex map below.
  const double old[3]={e.ref[0]+q_[size_t(e.a)]-q_[size_t(e.b)],e.ref[1]+q_[size_t(e.a+1)]-q_[size_t(e.b+1)],e.ref[2]+q_[size_t(e.a+2)]-q_[size_t(e.b+2)]};
  const double neu[3]={e.ref[0]+next[size_t(e.a)]-next[size_t(e.b)],e.ref[1]+next[size_t(e.a+1)]-next[size_t(e.b+1)],e.ref[2]+next[size_t(e.a+2)]-next[size_t(e.b+2)]};
  // This edge penalty currently uses nonlinear relative displacement length.
  // It is a passive geometric correction; not a complete large-strain tetra FEM law.
  const double la=norm3(old[0],old[1],old[2]);
  const double lb=norm3(neu[0],neu[1],neu[2]);
  const double scale=.5*e.k*(la+lb-2*e.rest)/(la+lb);
  for(int j=0;j<3;j++){
   const double g=scale*(old[j]+neu[j]);grad_[size_t(e.a+j)]+=g;grad_[size_t(e.b+j)]-=g;
  }
 }
 for(size_t i=0;i<origModeStiffness_.size();i++){
  const int dof=nativeOffset_+int(i);const double a=q_[size_t(dof)],b=next[size_t(dof)];
  grad_[size_t(dof)]+=.25*origModeStiffness_[i]*(a*a*a+a*a*b+a*b*b+b*b*b);
 }
}
bool Instrument::linearSolve(const std::vector<double>&b)noexcept{
 // Prepared CSR PCG, with lumped-mass + diagonal K/R preconditioner.
 const int n=full_.n;
 apply(full_.K,vmid_,scratchK_);apply(full_.R,vmid_,scratchR_);
 for(int i=0;i<n;i++){
  const size_t a=size_t(i);
  const double Ax=(2*full_.mass[a]/h_)*vmid_[a]+scratchR_[a]+.5*h_*scratchK_[a];
  res_[a]=b[a]-Ax;z_[a]=res_[a]*diag_[a];dir_[a]=z_[a];
 }
 double rz=dot(res_,z_),start=std::max(1e-24,std::sqrt(dot(b,b)));
 if(std::sqrt(dot(res_,res_))<=start*1e-10)return true;
 for(int iter=0;iter<recipe_.linearIterations;iter++){
  apply(full_.K,dir_,scratchK_);apply(full_.R,dir_,scratchR_);
  for(int i=0;i<n;i++){const size_t a=size_t(i);Ap_[a]=(2*full_.mass[a]/h_)*dir_[a]+scratchR_[a]+.5*h_*scratchK_[a];}
  const double denominator=dot(dir_,Ap_);
  if(!(denominator>0)||!std::isfinite(denominator))return false;
  const double alpha=rz/denominator;
  for(int i=0;i<n;i++){const size_t a=size_t(i);vmid_[a]+=alpha*dir_[a];res_[a]-=alpha*Ap_[a];}
  if(std::sqrt(dot(res_,res_))<=start*1e-10)return true;
  for(int i=0;i<n;i++)z_[size_t(i)]=res_[size_t(i)]*diag_[size_t(i)];
  const double rzNew=dot(res_,z_);
  if(!std::isfinite(rzNew)||rz==0)return false;
  const double beta=rzNew/rz;
  for(int i=0;i<n;i++)dir_[size_t(i)]=z_[size_t(i)]+beta*dir_[size_t(i)];
  rz=rzNew;
 }
 return false;
}
static double edgeEnergy(const double*a,const double*b,const double*ref,double L,double k){
 double d2=0;for(int j=0;j<3;j++){double d=ref[j]+a[j]-b[j];d2+=d*d;}
 return .5*k*std::pow(std::sqrt(d2)-L,2);
}
double Instrument::elasticEnergy(const std::vector<double>&q,const std::vector<double>&v)const noexcept{
 double total=0;for(int i=0;i<full_.n;i++)total+=.5*full_.mass[size_t(i)]*v[size_t(i)]*v[size_t(i)];
 for(int i=0;i<full_.n;i++)for(int k=full_.ptr[size_t(i)];k<full_.ptr[size_t(i+1)];k++)total+=.5*q[size_t(i)]*full_.K[size_t(k)]*q[size_t(full_.index[size_t(k)])];
 for(const auto&c:contacts_){double gap=rowValue(c.normal,q)-c.gap;total+=.5*c.k*std::pow(std::max(0.,gap),2);}
 for(const auto&e:stretches_)total+=edgeEnergy(q.data()+e.a,q.data()+e.b,e.ref,e.rest,e.k);
 for(size_t i=0;i<origModeStiffness_.size();i++){double x=q[size_t(nativeOffset_+int(i))];total+=.25*origModeStiffness_[i]*x*x*x*x;}
 return total;
}
double Instrument::contactPotential()const noexcept{double v=0;for(auto&c:contacts_){double z=rowValue(c.normal,q_)-c.gap;v+=.5*c.k*std::pow(std::max(0.,z),2);}return v;}
double Instrument::stretchPotential()const noexcept{double v=0;for(auto&e:stretches_)v+=edgeEnergy(q_.data()+e.a,q_.data()+e.b,e.ref,e.rest,e.k);return v;}
double Instrument::modalPotential()const noexcept{double v=0;for(size_t i=0;i<origModeStiffness_.size();i++){double x=q_[size_t(nativeOffset_+int(i))];v+=.25*origModeStiffness_[i]*x*x*x*x;}return v;}
double Instrument::energy()const noexcept{return elasticEnergy(q_,v_);}
Ledger Instrument::ledger()const noexcept{Ledger l;l.initial=initial_;l.active=active_;l.dissipation=diss_;l.finalEnergy=energy();l.residual=l.finalEnergy-l.initial-l.active+l.dissipation;return l;}
Stats Instrument::stats()const noexcept{return stats_;}
double Instrument::regionEnergy(int region)const noexcept{
 if(region<0||region>=int(full_.regionOffset.size()))return 0;
 int off=full_.regionOffset[size_t(region)],n=full_.regionSize[size_t(region)];double e=0;
 for(int i=off;i<off+n;i++)e+=.5*full_.mass[size_t(i)]*v_[size_t(i)]*v_[size_t(i)];return e;
}
std::array<double,3> Instrument::deformedVertex(int region,int vertex)const noexcept{
 if(region<0||region>=int(recipe_.spatial.regions.size()))return {0.,0.,0.};
 const auto&reg=recipe_.spatial.regions[size_t(region)];
 if(reg.family!=sparse43::Family::Elastic3D||vertex<0||vertex>=int(reg.vertices.size()))return {0.,0.,0.};
 const int base=full_.regionOffset[size_t(region)]+3*vertex;
 const auto&ref=reg.vertices[size_t(vertex)];
 return {ref.x+q_[size_t(base)],ref.y+q_[size_t(base+1)],ref.z+q_[size_t(base+2)]};
}
void Instrument::noteOn(int midi,float velocity)noexcept{if(midi<0||midi>127||!good(velocity,0,1))return;midi_=midi;pulse_+=double(velocity)*recipe_.spatial.impulseForce;}
void Instrument::noteOff(int midi)noexcept{if(midi_==midi)midi_=-1;}
void Instrument::allNotesOff()noexcept{midi_=-1;pulse_=0;}
float Instrument::processOne()noexcept{
 const int n=full_.n;
 std::fill(force_.begin(),force_.end(),0.);
 if(pulse_>1e-14){for(const auto&t:ports_[size_t(excitePort_)].terms)force_[size_t(t.first)]+=pulse_*t.second;pulse_*=.75;}
 // Explicitly POWERED Native27-style excitation. A modal proxy without this drive
 // is passive, including phase/spectral/vocal resonator coordinates.
 if(midi_>=0){
  const double hz=440*std::pow(2.,(midi_-69)/12.);
  for(size_t i=0;i<recipe_.native.modes.size();i++){
   const auto&m=recipe_.native.modes[i];if(m.excitation<=0)continue;
   const double phase=2*pi*phase_*m.ratio;
   const double w=m.engine==native27::Engine::VA?std::sin(phase):
                  m.engine==native27::Engine::FM?std::sin(phase+std::sin(1.6*phase)):
                  std::sin(phase);
   force_[size_t(nativeOffset_+int(i))]+=std::clamp(m.excitation*w,-10000.,10000.);
  }
  phase_+=hz*h_;phase_-=std::floor(phase_);
 }
 apply(full_.K,q_,scratchA_);
 for(int i=0;i<n;i++)trial_[size_t(i)]=v_[size_t(i)];
 double error=0;bool converged=false;
 for(int outer=0;outer<recipe_.nonlinearIterations;outer++){
  for(int i=0;i<n;i++)qnext_[size_t(i)]=q_[size_t(i)]+h_*trial_[size_t(i)];
  nonlinear(qnext_,trial_);
  for(int i=0;i<n;i++){
   const size_t a=size_t(i);rhs_[a]=2*full_.mass[a]/h_*v_[a]-scratchA_[a]+force_[a]-grad_[a]-lossForce_[a];
   vmid_[a]=trial_[a];
  }
  if(!linearSolve(rhs_)){stats_.failures++;stats_.lastError=1.e10;return 0;}
  double delta=0,scale=1;
  for(int i=0;i<n;i++){delta=std::max(delta,std::abs(vmid_[size_t(i)]-trial_[size_t(i)]));scale=std::max(scale,std::abs(vmid_[size_t(i)]));}
  error=delta/scale;
  if(error<recipe_.nonlinearTolerance){converged=true;break;}
  for(int i=0;i<n;i++)trial_[size_t(i)]=.5*trial_[size_t(i)]+.5*vmid_[size_t(i)];
 }
 if(!converged){stats_.failures++;stats_.lastError=error;return 0;}
 // Recompute exact discrete gradient and friction at the accepted midpoint;
 // only then commit the physical state and energy ledger.
 for(int i=0;i<n;i++)qnext_[size_t(i)]=q_[size_t(i)]+h_*vmid_[size_t(i)];
 nonlinear(qnext_,vmid_);
 apply(full_.R,vmid_,scratchR_);
 const double a=h_*dot(force_,vmid_);
 const double loss=h_*(dot(vmid_,scratchR_)+dot(vmid_,lossForce_));
 for(int i=0;i<n;i++){q_[size_t(i)]=qnext_[size_t(i)];v_[size_t(i)]=2*vmid_[size_t(i)]-v_[size_t(i)];}
 active_+=a;diss_+=std::max(0.,loss);stats_.frames++;stats_.lastError=error;
 for(const auto&c:contacts_)if(rowValue(c.normal,q_)>c.gap)stats_.contactsEngaged++;
 const double signal=rowValue(ports_[size_t(outputPort_)],v_)*recipe_.spatial.outputGain;
 return std::isfinite(signal)?float(std::tanh(signal)):0.f;
}
} // namespace panorama::passive45
