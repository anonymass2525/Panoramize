#include "SparseHamiltonian42.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <limits>
#include <map>
#include <numeric>
#include <sstream>
#include <stdexcept>
namespace panorama::sparse42 {
namespace {
using V=std::array<double,3>;
bool in(double v,double lo,double hi){return std::isfinite(v)&&v>=lo&&v<=hi;}
V sub(V a,V b){return {a[0]-b[0],a[1]-b[1],a[2]-b[2]};}
V cross(V a,V b){return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
double dot(V a,V b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
V xyz(const Vertex& v){return {v.x,v.y,v.z};}
double length(V v){return std::sqrt(dot(v,v));}
struct Builder {
 int n=0;std::map<std::pair<int,int>,double> values;
 void add(int i,int j,double a){if(std::abs(a)>1e-25)values[{i,j}]+=a;}
 void spring(int a,int b,double k){add(a,a,k);if(b>=0){add(b,b,k);add(a,b,-k);add(b,a,-k);}}
 void outer(const std::vector<std::pair<int,double>>&terms,double k){if(k==0)return;for(auto&a:terms)for(auto&b:terms)add(a.first,b.first,k*a.second*b.second);}
};
}
bool valid(const Recipe&r,std::string*error){
 auto fail=[&](const char*s){if(error)*error=s;return false;};
 if(r.regions.empty()||r.regions.size()>48||r.ports.empty()||r.ports.size()>256||r.links.size()>512||r.excitePort<0||r.outputPort<0||r.excitePort>=int(r.ports.size())||r.outputPort>=int(r.ports.size())||!in(r.impulseForce,0,10000)||!in(r.outputGain,0,1e8)||r.maxIterations<2||r.maxIterations>1000||!in(r.relativeTolerance,1e-14,1e-3))return fail("invalid graph limits");
 int n=0;
 for(const auto&rg:r.regions){
  if(rg.family!=Family::String1D&&rg.family!=Family::Membrane2D&&rg.family!=Family::Elastic3D&&rg.family!=Family::Acoustic0D)return fail("unknown family");
  if(!in(rg.density,1e-10,1e8)||!in(rg.tension,0,1e10)||!in(rg.young,1e4,1e11)||!in(rg.poisson,0,.48)||!in(rg.damping,0,1e8)||!in(rg.soundSpeed,10,1000)||!in(rg.length,1e-4,100)||!in(rg.area,1e-8,1e3)||!in(rg.volume,1e-8,1e3)||!in(rg.gain,-1e6,1e6))return fail("bad material SI parameters");
  const int nv=int(rg.vertices.size());
  if(rg.family==Family::Acoustic0D){if(nv||!rg.elements.empty())return fail("cavity has no spatial mesh");n++;continue;}
  if(nv<2||nv>2048||rg.elements.empty()||rg.elements.size()>8192)return fail("mesh exceeds region limits");
  n+=nv*(rg.family==Family::Elastic3D?3:1);
  const int required=rg.family==Family::String1D?2:rg.family==Family::Membrane2D?3:4;
  for(auto&v:rg.vertices)if(!in(v.x,-1e4,1e4)||!in(v.y,-1e4,1e4)||!in(v.z,-1e4,1e4))return fail("nonfinite vertex");
  for(auto&el:rg.elements){if(int(el.size())!=required)return fail("element dimension mismatch");for(int i=0;i<required;i++){if(el[i]<0||el[i]>=nv)return fail("element index invalid");for(int j=0;j<i;j++)if(el[i]==el[j])return fail("repeated element vertex");}}
 }
 if(n>4096)return fail("over 4096 DOF research cap");
 for(const auto&p:r.ports){if(p.region<0||p.region>=int(r.regions.size())||p.terms.empty()||p.terms.size()>64)return fail("invalid port");
  const auto&rg=r.regions[size_t(p.region)];const bool acoustic=rg.family==Family::Acoustic0D;
  if((acoustic&&p.unit!=PortUnit::CubicMetres)||(!acoustic&&p.unit!=PortUnit::Metres))return fail("port units incompatible with region");
  int dofs=acoustic?1:int(rg.vertices.size())*(rg.family==Family::Elastic3D?3:1);
  for(auto&t:p.terms)if(t.localDof<0||t.localDof>=dofs||!in(t.weight,-100,100))return fail("invalid port weight/index");
 }
 for(const auto&l:r.links){if(l.a<0||l.b<0||l.a>=int(r.ports.size())||l.b>=int(r.ports.size())||l.a==l.b||!in(l.areaA,1e-8,100)||!in(l.areaB,1e-8,100)||!in(l.stiffness,0,1e13)||!in(l.damping,0,1e12))return fail("invalid link");
  if(l.unit!=PortUnit::Metres&&l.unit!=PortUnit::CubicMetres)return fail("invalid link unit");
  if(l.unit==PortUnit::Metres&&(r.ports[size_t(l.a)].unit!=PortUnit::Metres||r.ports[size_t(l.b)].unit!=PortUnit::Metres))return fail("mechanical link requires mechanical ports");
 }
 if(error)error->clear();return true;
}
Recipe example(){
 Recipe r;
 Region string;string.family=Family::String1D;string.density=.003;string.tension=72;string.damping=.002;string.gain=1.;
 for(int i=0;i<12;i++)string.vertices.push_back({.65*i/11.,0,0});
 for(int i=0;i<11;i++)string.elements.push_back({i,i+1});r.regions.push_back(string);
 Region membrane;membrane.family=Family::Membrane2D;membrane.density=.16;membrane.tension=130;membrane.damping=.014;
 for(int y=0;y<4;y++)for(int x=0;x<4;x++)membrane.vertices.push_back({.12*x/3.,.12*y/3.,0});
 for(int y=0;y<3;y++)for(int x=0;x<3;x++){int a=x+4*y;membrane.elements.push_back({a,a+1,a+5});membrane.elements.push_back({a,a+5,a+4});}
 r.regions.push_back(membrane);
 Region body;body.family=Family::Elastic3D;body.young=2.e6;body.density=700;body.poisson=.3;body.damping=.006;
 body.vertices={{0,0,0},{.2,0,0},{.2,.13,0},{0,.13,0},{0,0,.03},{.2,0,.03},{.2,.13,.03},{0,.13,.03}};
 body.elements={{0,1,2,6},{0,2,3,6},{0,3,7,6},{0,7,4,6},{0,4,5,6},{0,5,1,6}};r.regions.push_back(body);
 Region cavity;cavity.family=Family::Acoustic0D;cavity.density=1.2;cavity.soundSpeed=343;cavity.length=.12;cavity.area=.003;cavity.volume=.015;cavity.damping=12;r.regions.push_back(cavity);
 r.ports={ {0,PortUnit::Metres,{{4,1.}}}, {1,PortUnit::Metres,{{5,1.}}}, {2,PortUnit::Metres,{{20,1.}}}, {3,PortUnit::CubicMetres,{{0,1.}}} };
 r.links={{0,1,PortUnit::Metres,1,1,220,.005},{1,2,PortUnit::Metres,1,1,290,.01},{2,3,PortUnit::CubicMetres,.003,1.,1e7,90.}};
 r.excitePort=0;r.outputPort=2;r.impulseForce=.15;r.outputGain=350.;r.maxIterations=150;return r;
}
std::string serialize(const Recipe&r){
 std::ostringstream s;s<<std::setprecision(17)<<"PANORAMIZE-SPARSE42|1\n";
 s<<"SET "<<r.regions.size()<<' '<<r.ports.size()<<' '<<r.links.size()<<' '<<r.excitePort<<' '<<r.outputPort<<' '<<r.impulseForce<<' '<<r.outputGain<<' '<<r.maxIterations<<' '<<r.relativeTolerance<<'\n';
 for(const auto&g:r.regions){s<<"REG "<<int(g.family)<<' '<<g.density<<' '<<g.tension<<' '<<g.young<<' '<<g.poisson<<' '<<g.damping<<' '<<g.soundSpeed<<' '<<g.length<<' '<<g.area<<' '<<g.volume<<' '<<g.gain<<' '<<g.vertices.size()<<' '<<g.elements.size()<<'\n';
  for(auto&v:g.vertices)s<<"V "<<v.x<<' '<<v.y<<' '<<v.z<<'\n';
  for(auto&e:g.elements){s<<"E "<<e.size();for(int a:e)s<<' '<<a;s<<'\n';}
 }
 for(const auto&p:r.ports){s<<"P "<<p.region<<' '<<int(p.unit)<<' '<<p.terms.size();for(auto&t:p.terms)s<<' '<<t.localDof<<' '<<t.weight;s<<'\n';}
 for(const auto&l:r.links)s<<"L "<<l.a<<' '<<l.b<<' '<<int(l.unit)<<' '<<l.areaA<<' '<<l.areaB<<' '<<l.stiffness<<' '<<l.damping<<'\n';
 return s.str();
}
bool deserialize(const std::string&blob,Recipe&out,std::string*error){
 auto fail=[&](const char*s){if(error)*error=s;return false;};
 if(blob.size()>2000000)return fail("file too large");
 std::istringstream in(blob);std::string tag,extra;Recipe r;int nr,np,nl;
 if(!(in>>tag)||tag!="PANORAMIZE-SPARSE42|1")return fail("wrong header");
 if(!(in>>tag>>nr>>np>>nl>>r.excitePort>>r.outputPort>>r.impulseForce>>r.outputGain>>r.maxIterations>>r.relativeTolerance)||tag!="SET"||nr<1||nr>48||np<1||np>256||nl<0||nl>512)return fail("bad setup");
 for(int i=0;i<nr;i++){
  Region g;int fam=-1,nv=-1,ne=-1;
  if(!(in>>tag>>fam>>g.density>>g.tension>>g.young>>g.poisson>>g.damping>>g.soundSpeed>>g.length>>g.area>>g.volume>>g.gain>>nv>>ne)||tag!="REG"||fam<1||fam>4||nv<0||nv>2048||ne<0||ne>8192)return fail("bad region");
  g.family=Family(fam);
  for(int k=0;k<nv;k++){Vertex v;if(!(in>>tag>>v.x>>v.y>>v.z)||tag!="V")return fail("bad vertex");g.vertices.push_back(v);}
  for(int k=0;k<ne;k++){int count=-1;if(!(in>>tag>>count)||tag!="E"||count<2||count>4)return fail("bad element");std::vector<int> e;
   for(int a=0;a<count;a++){int id;if(!(in>>id))return fail("bad element index");e.push_back(id);}g.elements.push_back(e);
  }r.regions.push_back(std::move(g));
 }
 for(int i=0;i<np;i++){Port p;int kind,count;if(!(in>>tag>>p.region>>kind>>count)||tag!="P"||count<1||count>64)return fail("bad port");p.unit=PortUnit(kind);
  for(int j=0;j<count;j++){PortTerm t;if(!(in>>t.localDof>>t.weight))return fail("bad port term");p.terms.push_back(t);}r.ports.push_back(std::move(p));}
 for(int i=0;i<nl;i++){Link l;int kind;if(!(in>>tag>>l.a>>l.b>>kind>>l.areaA>>l.areaB>>l.stiffness>>l.damping)||tag!="L")return fail("bad link");l.unit=PortUnit(kind);r.links.push_back(l);}
 if(in>>extra||!valid(r,error))return fail("bad graph data/units");out=std::move(r);return true;
}
void Instrument::Sparse::apply(const std::vector<double>&x,std::vector<double>&y)const noexcept{
 std::fill(y.begin(),y.end(),0.);
 for(int i=0;i<n;i++)for(int k=ptr[size_t(i)];k<ptr[size_t(i+1)];k++)y[size_t(i)]+=val[size_t(k)]*x[size_t(index[size_t(k)])];
}
std::unique_ptr<Instrument> Instrument::prepare(const Recipe&r,double rate,std::string*error){
 auto fail=[&](const char*s)->std::unique_ptr<Instrument>{if(error)*error=s;return {};};
 if(!valid(r,error)||!in(rate,22050,192000))return fail("invalid topology/sample rate");
 auto x=std::unique_ptr<Instrument>(new Instrument());x->recipe_=r;x->h_=1./rate;
 int n=0;
 for(auto&g:r.regions){x->offsets_.push_back(n);const int sz=g.family==Family::Acoustic0D?1:int(g.vertices.size())*(g.family==Family::Elastic3D?3:1);
  x->sizes_.push_back(sz);n+=sz;
 }
 x->mass_.assign(size_t(n),0.);x->q_.assign(size_t(n),0.);x->v_.assign(size_t(n),0.);x->f_.assign(size_t(n),0.);
 x->rhs_.assign(size_t(n),0.);x->vmid_.assign(size_t(n),0.);x->res_.assign(size_t(n),0.);x->dir_.assign(size_t(n),0.);
 x->z_.assign(size_t(n),0.);x->Ap_.assign(size_t(n),0.);x->scratchK_.assign(size_t(n),0.);x->scratchR_.assign(size_t(n),0.);
 x->scratchA_.assign(size_t(n),0.);x->invDiag_.assign(size_t(n),0.);
 Builder kb,db;kb.n=db.n=n;
 for(size_t region=0;region<r.regions.size();region++){
  const auto&g=r.regions[region];int off=x->offsets_[region];
  if(g.family==Family::Acoustic0D){
   x->mass_[size_t(off)]=g.density*g.length/g.area;
   kb.add(off,off,g.density*g.soundSpeed*g.soundSpeed/g.volume);
   db.add(off,off,g.damping);continue;
  }
  for(const auto&e:g.elements){
   if(g.family==Family::String1D){
    V a=xyz(g.vertices[size_t(e[0])]),b=xyz(g.vertices[size_t(e[1])]);double len=length(sub(b,a));
    if(!in(len,1e-7,1e5))return fail("degenerate 1D segment");
    const int i=off+e[0],j=off+e[1];double m=.5*g.density*len;
    x->mass_[size_t(i)]+=m;x->mass_[size_t(j)]+=m;
    kb.spring(i,j,g.tension/len);
   }else if(g.family==Family::Membrane2D){
    V a=xyz(g.vertices[size_t(e[0])]),b=xyz(g.vertices[size_t(e[1])]),c=xyz(g.vertices[size_t(e[2])]);
    V u=sub(b,a),v=sub(c,a);
    double uu=dot(u,u),uv=dot(u,v),vv=dot(v,v),det=uu*vv-uv*uv;
    if(!in(det,1e-16,1e16))return fail("degenerate triangle");
    double area=.5*std::sqrt(det);V grad[3]{};
    for(int k=0;k<3;k++){
     grad[1][k]=(vv*u[k]-uv*v[k])/det;grad[2][k]=(uu*v[k]-uv*u[k])/det;
     grad[0][k]=-grad[1][k]-grad[2][k];
    }
    for(int i=0;i<3;i++){
     x->mass_[size_t(off+e[size_t(i)])]+=g.density*area/3.;
     for(int j=0;j<3;j++)kb.add(off+e[size_t(i)],off+e[size_t(j)],g.tension*area*dot(grad[i],grad[j]));
    }
   }else{ // Elastic3D linear isotropic FEM, real 3D nodal vector DOFs
    V a=xyz(g.vertices[size_t(e[0])]),b=xyz(g.vertices[size_t(e[1])]),c=xyz(g.vertices[size_t(e[2])]),d=xyz(g.vertices[size_t(e[3])]);
    V u=sub(b,a),v=sub(c,a),w=sub(d,a);double det=dot(u,cross(v,w));
    if(!std::isfinite(det)||std::abs(det)<1e-16)return fail("degenerate tetrahedron");
    const double vol=std::abs(det)/6.,mu=g.young/(2*(1+g.poisson)),la=g.young*g.poisson/((1+g.poisson)*(1-2*g.poisson));
    V grad[4]{};grad[1]=cross(v,w);grad[2]=cross(w,u);grad[3]=cross(u,v);
    for(int j=0;j<3;j++){for(int k=1;k<4;k++)grad[k][j]/=det;grad[0][j]=-grad[1][j]-grad[2][j]-grad[3][j];}
    double B[6][12]{};
    for(int k=0;k<4;k++){
     double gx=grad[k][0],gy=grad[k][1],gz=grad[k][2];
     B[0][3*k]=gx;B[1][3*k+1]=gy;B[2][3*k+2]=gz;
     B[3][3*k+1]=gz;B[3][3*k+2]=gy;B[4][3*k]=gz;B[4][3*k+2]=gx;B[5][3*k]=gy;B[5][3*k+1]=gx;
     for(int ax=0;ax<3;ax++)x->mass_[size_t(off+3*e[size_t(k)]+ax)]+=vol*g.density/4.;
    }
    double C[6][6]{};for(int j=0;j<3;j++)for(int k=0;k<3;k++)C[j][k]=la+(j==k?2*mu:0);
    for(int j=3;j<6;j++)C[j][j]=mu;
    for(int j=0;j<12;j++)for(int k=0;k<12;k++){
     double z=0.;for(int l=0;l<6;l++)for(int m=0;m<6;m++)z+=B[l][j]*C[l][m]*B[m][k];
     kb.add(off+3*e[size_t(j/3)]+j%3,off+3*e[size_t(k/3)]+k%3,vol*z);
    }
   }
  }
  for(int j=0;j<x->sizes_[region];j++)db.add(off+j,off+j,g.damping);
 }
 // Only reject unused mesh nodes, not physically valid free rigid-body DOFs.
 for(double m:x->mass_)if(!(m>1e-15)||!std::isfinite(m))return fail("unreferenced or non-positive mass DOF");
 for(const auto&p:r.ports){WeightedPort w;w.region=p.region;w.unit=p.unit;
  for(auto&t:p.terms)w.weights.push_back({x->offsets_[size_t(p.region)]+t.localDof,t.weight});
  x->ports_.push_back(std::move(w));
 }
 for(auto&l:r.links){
  RelativeLink s;s.k=l.stiffness;s.d=l.damping;
  auto add=[&](const WeightedPort&p,double area,double sign){
   const double factor=l.unit==PortUnit::CubicMetres&&p.unit==PortUnit::Metres?area:1.;
   for(auto&t:p.weights)s.row.push_back({t.first,sign*factor*t.second});
  };
  add(x->ports_[size_t(l.a)],l.areaA,+1.);add(x->ports_[size_t(l.b)],l.areaB,-1.);
  kb.outer(s.row,l.stiffness);db.outer(s.row,l.damping);x->links_.push_back(std::move(s));
 }
 auto makeCSR=[&](const Builder&b){Sparse s;s.n=n;s.ptr.resize(size_t(n+1));
  for(auto&entry:b.values)if(std::abs(entry.second)>1e-22)s.ptr[size_t(entry.first.first+1)]++;
  for(int i=0;i<n;i++)s.ptr[size_t(i+1)]+=s.ptr[size_t(i)];
  s.index.reserve(size_t(s.ptr.back()));s.val.reserve(size_t(s.ptr.back()));
  for(auto&entry:b.values)if(std::abs(entry.second)>1e-22){s.index.push_back(entry.first.second);s.val.push_back(entry.second);}return s;};
 x->K_=makeCSR(kb);x->R_=makeCSR(db);
 Builder ab;ab.n=n;
 for(auto&item:kb.values)ab.add(item.first.first,item.first.second,x->h_*.5*item.second);
 for(auto&item:db.values)ab.add(item.first.first,item.first.second,item.second);
 for(int i=0;i<n;i++)ab.add(i,i,2*x->mass_[size_t(i)]/x->h_);
 x->A_=makeCSR(ab);
 for(int i=0;i<n;i++){
  double diag=0;for(int k=x->A_.ptr[size_t(i)];k<x->A_.ptr[size_t(i+1)];k++)if(x->A_.index[size_t(k)]==i)diag=x->A_.val[size_t(k)];
  if(!(diag>0)||!std::isfinite(diag))return fail("non-SPD implicit diagonal");x->invDiag_[size_t(i)]=1./diag;
 }
 x->initial_=x->energy();if(error)error->clear();return x;
}
bool Instrument::solve() noexcept{
 A_.apply(vmid_,scratchA_);
 const size_t n=rhs_.size();
 double rr0=0.,rz=0.;for(size_t i=0;i<n;i++){
  res_[i]=rhs_[i]-scratchA_[i];z_[i]=invDiag_[i]*res_[i];dir_[i]=z_[i];
  rr0+=rhs_[i]*rhs_[i];rz+=res_[i]*z_[i];
 }
 const double tol=recipe_.relativeTolerance*std::sqrt(rr0)+1e-17;
 auto rn=[&](){double s=0.;for(double v:res_)s+=v*v;return std::sqrt(s);};
 lastResidual_=rn();lastIterations_=0;
 if(lastResidual_<=tol)return true;
 for(int k=0;k<recipe_.maxIterations;k++){
  A_.apply(dir_,Ap_);
  double pd=0.;for(size_t i=0;i<n;i++)pd+=dir_[i]*Ap_[i];
  if(!(pd>0.)||!std::isfinite(pd)||!std::isfinite(rz))break;
  const double alpha=rz/pd;
  for(size_t i=0;i<n;i++){vmid_[i]+=alpha*dir_[i];res_[i]-=alpha*Ap_[i];}
  lastIterations_=k+1;
  lastResidual_=rn();if(lastResidual_<=tol)return true;
  double next=0.;for(size_t i=0;i<n;i++){z_[i]=invDiag_[i]*res_[i];next+=res_[i]*z_[i];}
  if(!(next>0)||!std::isfinite(next))break;
  const double beta=next/rz;for(size_t i=0;i<n;i++)dir_[i]=z_[i]+beta*dir_[i];rz=next;
 }
 return false;
}
void Instrument::noteOn(int midi,float velocity)noexcept{
 if(midi<0||midi>127||!std::isfinite(velocity)||velocity<=0)return;
 lastMidi_=midi;pulse_+=std::clamp(double(velocity),0.,1.)*recipe_.impulseForce;
}
void Instrument::noteOff(int midi)noexcept{if(lastMidi_==midi)lastMidi_=-1;}
void Instrument::allNotesOff()noexcept{lastMidi_=-1;pulse_=0.;}
float Instrument::processOne()noexcept{
 // Drive in generalized SI efforts conjugate to the excitation port velocity.
 std::fill(f_.begin(),f_.end(),0.);
 for(auto&p:ports_[size_t(recipe_.excitePort)].weights)f_[size_t(p.first)]+=p.second*pulse_;
 K_.apply(q_,scratchK_);
 for(size_t i=0;i<q_.size();i++)rhs_[i]=2*mass_[i]/h_*v_[i]-scratchK_[i]+f_[i];
 if(!solve()){
  ++failed_;std::fill(vmid_.begin(),vmid_.end(),0.);pulse_=0.;++frames_;return 0.f;
 }
 R_.apply(vmid_,scratchR_);
 double power=0,loss=0;for(size_t i=0;i<q_.size();i++){power+=f_[i]*vmid_[i];loss+=vmid_[i]*scratchR_[i];}
 active_+=h_*power;dissipated_+=std::max(0.,h_*loss);
 for(size_t i=0;i<q_.size();i++){q_[i]+=h_*vmid_[i];v_[i]=2*vmid_[i]-v_[i];}
 pulse_*=.65;frames_++;
 const double out=portVelocity(recipe_.outputPort)*recipe_.outputGain;
 return std::isfinite(out)?float(std::tanh(out)):0.f;
}
double Instrument::portVelocity(int port)const noexcept{
 if(port<0||port>=int(ports_.size()))return 0.;double s=0.;
 for(auto&t:ports_[size_t(port)].weights)s+=t.second*v_[size_t(t.first)];
 return s;
}
double Instrument::energy()const noexcept{
 double e=0.;
 for(size_t i=0;i<q_.size();i++){
  double kq=0.;for(int j=K_.ptr[i];j<K_.ptr[i+1];j++)kq+=K_.val[size_t(j)]*q_[size_t(K_.index[size_t(j)])];
  e+=.5*mass_[i]*v_[i]*v_[i]+.5*q_[i]*kq;
 }
 return e;
}
double Instrument::regionEnergy(int region)const noexcept{
 if(region<0||region>=int(sizes_.size()))return 0.;double e=0.;
 const int off=offsets_[size_t(region)],count=sizes_[size_t(region)];
 for(int i=off;i<off+count;i++)e+=.5*mass_[size_t(i)]*v_[size_t(i)]*v_[size_t(i)];
 return e;
}
Ledger Instrument::ledger()const noexcept{
 Ledger l;l.initial=initial_;l.active=active_;l.dissipated=dissipated_;l.finalEnergy=energy();
 l.residual=l.finalEnergy-l.initial-l.active+l.dissipated;return l;
}
Stats Instrument::stats()const noexcept{
 Stats s;s.dofs=int(q_.size());s.nnz=int(A_.val.size());s.iterations=lastIterations_;s.residual=lastResidual_;s.failed=failed_;s.frames=frames_;return s;
}
} // namespace panorama::sparse42
