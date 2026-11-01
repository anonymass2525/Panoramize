#include "GeneralizedFEM36.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <set>
namespace panorama::fem36 {
namespace {
constexpr double pi=3.14159265358979323846;
constexpr int pitches[kStrings]={40,45,50,55,59,64}; // E2 A2 D3 G3 B3 E4
// Geometry and bridge attachments are now stored in the per-instrument Mesh.
inline bool valid(double v,double lo,double hi){return std::isfinite(v)&&v>=lo&&v<=hi;}
inline double clamp(double x,double lo,double hi){return std::clamp(x,lo,hi);}
inline double hz(int midi){return 440.*std::exp2((midi-69)/12.);}
// Symmetric linear Hookean stiffness K = sum_t V B^T D B, assembled off audio thread.
bool buildMesh(std::array<std::array<double,kDof>,kDof>& K,
               std::array<double,kDof>& mass,const Model&m){
 for(auto&r:K)r.fill(0);
 mass.fill(0);
 for(const auto&t:m.mesh.tetrahedra){
  double d[3][3]{};
  for(int a=0;a<3;a++)for(int b=0;b<3;b++)d[a][b]=m.mesh.vertices[size_t(t[b+1])][size_t(a)]-m.mesh.vertices[size_t(t[0])][size_t(a)];
  const double det=d[0][0]*(d[1][1]*d[2][2]-d[1][2]*d[2][1])
                  -d[0][1]*(d[1][0]*d[2][2]-d[1][2]*d[2][0])
                  +d[0][2]*(d[1][0]*d[2][1]-d[1][1]*d[2][0]);
  if(std::abs(det)<1.e-12||!std::isfinite(det))return false;
  double inv[3][3]{};
  for(int a=0;a<3;a++)for(int b=0;b<3;b++){
   const int r0=(b+1)%3,r1=(b+2)%3,c0=(a+1)%3,c1=(a+2)%3;
   inv[a][b]=(d[r0][c0]*d[r1][c1]-d[r0][c1]*d[r1][c0])/det;
  }
  double grad[4][3]{};
  for(int a=1;a<4;a++)for(int k=0;k<3;k++)grad[a][k]=inv[a-1][k];
  for(int k=0;k<3;k++)grad[0][k]=-(grad[1][k]+grad[2][k]+grad[3][k]);
  double B[6][12]{};
  for(int a=0;a<4;a++){
   const double x=grad[a][0],y=grad[a][1],z=grad[a][2];
   B[0][3*a]=x; B[1][3*a+1]=y; B[2][3*a+2]=z;
   B[3][3*a+1]=z;B[3][3*a+2]=y;
   B[4][3*a]=z;B[4][3*a+2]=x;
   B[5][3*a]=y;B[5][3*a+1]=x;
  }
  const double mu=m.meshYoung/(2*(1+m.meshPoisson));
  const double lam=m.meshYoung*m.meshPoisson/((1+m.meshPoisson)*(1-2*m.meshPoisson));
  double D[6][6]{};
  // D = S D_isotropic S, SPD for positive anisotropy scalings.
  for(int a=0;a<3;a++)for(int b=0;b<3;b++)D[a][b]=(lam+(a==b?2*mu:0))*std::sqrt(m.anisotropy[size_t(a)]*m.anisotropy[size_t(b)]);
  for(int a=3;a<6;a++)D[a][a]=mu*m.anisotropy[size_t(a)];
  const double vol=std::abs(det)/6.;
  for(int a=0;a<4;a++)for(int c=0;c<3;c++){
   const int row=3*t[size_t(a)]+c;mass[size_t(row)]+=vol*m.meshDensity/4;
   for(int b=0;b<4;b++)for(int e=0;e<3;e++){
    const int col=3*t[size_t(b)]+e;
    double v=0;
    for(int j=0;j<6;j++)for(int k=0;k<6;k++)v+=B[j][3*a+c]*D[j][k]*B[k][3*b+e];
    K[size_t(row)][size_t(col)]+=vol*v;
   }
  }
 }
 // Constrain one anchor point to remove rigid-body null modes.
 for(int ax=0;ax<3;ax++)K[size_t(ax)][size_t(ax)]+=m.meshYoung*.08;
 return true;
}
}
Mesh defaultMesh(){
 Mesh m;
 m.vertices={{{0,0,0}},{{.2,0,0}},{{.2,.12,0}},{{0,.12,0}},
             {{0,0,.08}},{{.2,0,.08}},{{.2,.12,.08}},{{0,.12,.08}}};
 m.tetrahedra={{{0,1,2,6}},{{0,2,3,6}},{{0,3,7,6}},
                {{0,7,4,6}},{{0,4,5,6}},{{0,5,1,6}}};
 return m;
}
std::string serializeMesh(const Mesh&m){
 std::ostringstream out;out<<std::setprecision(17)<<"PANORAMIZE-MESH36-v1 "<<m.vertices.size()<<' '<<m.tetrahedra.size()<<'\n';
 for(const auto&v:m.vertices)out<<"v "<<v[0]<<' '<<v[1]<<' '<<v[2]<<'\n';
 for(const auto&t:m.tetrahedra)out<<"t "<<t[0]<<' '<<t[1]<<' '<<t[2]<<' '<<t[3]<<'\n';
 out<<"bridge";for(int v:m.bridgeVertex)out<<' '<<v;out<<'\n';return out.str();
}
bool deserializeMesh(const std::string& text,Mesh&out,std::string*error){
 auto fail=[&](const char*why){if(error)*error=why;return false;};
 if(text.size()>30000)return fail("mesh text too large");
 std::istringstream in(text);std::string tag,extra;int n=0,k=0;Mesh m;
 if(!(in>>tag>>n>>k)||tag!="PANORAMIZE-MESH36-v1"||n<8||n>kVertices||k<1||k>96)return fail("invalid mesh header");
 for(int i=0;i<n;i++){
  std::array<double,3> v{};if(!(in>>tag>>v[0]>>v[1]>>v[2])||tag!="v")return fail("invalid vertex");
  for(double x:v)if(!std::isfinite(x)||std::abs(x)>10)return fail("nonfinite vertex");
  m.vertices.push_back(v);
 }
 for(int i=0;i<k;i++){
  std::array<int,4> t{};if(!(in>>tag>>t[0]>>t[1]>>t[2]>>t[3])||tag!="t")return fail("invalid tetra");
  std::set<int> unique(t.begin(),t.end());
  if(unique.size()!=4||*unique.begin()<0||*unique.rbegin()>=n)return fail("bad tetra index");
  m.tetrahedra.push_back(t);
 }
 if(!(in>>tag)||tag!="bridge")return fail("missing bridge");
 for(int& v:m.bridgeVertex)if(!(in>>v)||v<0||v>=n)return fail("invalid bridge vertex");
 if(in>>extra)return fail("unexpected mesh data");
 // Every vertex must be present in at least one tetrahedron (positive mass).
 std::vector<int> used(size_t(n),0);
 for(auto t:m.tetrahedra)for(int id:t)++used[size_t(id)];
 for(int v:used)if(!v)return fail("isolated vertex");
 // At least one nondegenerate tetra; reject ANY degenerate tetra early.
 for(const auto&t:m.tetrahedra){
  const auto&a=m.vertices[size_t(t[0])];const auto&b=m.vertices[size_t(t[1])];
  const auto&c=m.vertices[size_t(t[2])];const auto&d=m.vertices[size_t(t[3])];
  const double ux=b[0]-a[0],uy=b[1]-a[1],uz=b[2]-a[2];
  const double vx=c[0]-a[0],vy=c[1]-a[1],vz=c[2]-a[2];
  const double wx=d[0]-a[0],wy=d[1]-a[1],wz=d[2]-a[2];
  const double det=ux*(vy*wz-vz*wy)-uy*(vx*wz-vz*wx)+uz*(vx*wy-vy*wx);
  if(!std::isfinite(det)||std::abs(det)<1.e-12)return fail("degenerate tetra");
 }
 out=std::move(m);if(error)error->clear();return true;
}
std::string serialize(const Model&m){
 std::ostringstream s;s<<std::setprecision(17)<<"PANORAMIZE-FEM36-v1\n";
 s<<"string "<<m.stringLoss<<' '<<m.longitudinalRatio<<' '<<m.torsionRatio<<' '
  <<m.fretGap<<' '<<m.fretStiffness<<' '<<m.fretFriction<<' '<<m.fingerStiffness<<' '
  <<m.fingerDamping<<' '<<m.palmDamping<<'\n';
 s<<"mesh "<<m.meshYoung<<' '<<m.meshDensity<<' '<<m.meshPoisson<<' '
  <<m.meshLoss<<' '<<m.bridgeCoupling<<' '<<m.outputGain<<' '<<m.glideMs<<' '<<m.pluckPosition<<' '<<m.stringTensionScale<<'\n';
 s<<"anisotropy";for(double v:m.anisotropy)s<<' '<<v;s<<'\n';
 s<<serializeMesh(m.mesh);return s.str();
}
bool deserialize(const std::string&s,Model&out){
 if(s.size()>32768)return false;
 std::istringstream in(s);std::string line,tag,extra;Model m;
 if(!std::getline(in,line)||line!="PANORAMIZE-FEM36-v1")return false;
 if(!std::getline(in,line))return false;
 {std::istringstream q(line);
 if(!(q>>tag>>m.stringLoss>>m.longitudinalRatio>>m.torsionRatio>>m.fretGap>>m.fretStiffness
     >>m.fretFriction>>m.fingerStiffness>>m.fingerDamping>>m.palmDamping)||tag!="string"||(q>>extra))return false;}
 if(!std::getline(in,line))return false;
 {std::istringstream q(line);
 if(!(q>>tag>>m.meshYoung>>m.meshDensity>>m.meshPoisson>>m.meshLoss>>m.bridgeCoupling
     >>m.outputGain>>m.glideMs>>m.pluckPosition>>m.stringTensionScale)||tag!="mesh"||(q>>extra))return false;}
 if(!std::getline(in,line))return false;
 {std::istringstream q(line);
 if(!(q>>tag)||tag!="anisotropy")return false;
 for(double& v:m.anisotropy)if(!(q>>v))return false;
 if(q>>extra)return false;
 }
 std::string rest((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
 if(!deserializeMesh(rest,m.mesh))return false;
 for(double v:m.anisotropy)if(!valid(v,.01,100.))return false;
 if(!valid(m.stringLoss,0,100)||!valid(m.longitudinalRatio,.01,1)||!valid(m.torsionRatio,.0001,.1)||
    !valid(m.fretGap,.000001,.01)||!valid(m.fretStiffness,.01,2)||!valid(m.fretFriction,0,2)||
    !valid(m.fingerStiffness,.1,10)||!valid(m.fingerDamping,0,500)||!valid(m.palmDamping,0,1000)||
    !valid(m.meshYoung,1.e6,2.e9)||!valid(m.meshDensity,1.e3,1.e6)||!valid(m.meshPoisson,0,.46)||
    !valid(m.meshLoss,0,300)||!valid(m.bridgeCoupling,0,.5)||!valid(m.outputGain,.01,20)||
    !valid(m.glideMs,1,1000)||!valid(m.pluckPosition,.05,.95)||!valid(m.stringTensionScale,.25,4))return false;
 out=m;return true;
}
std::unique_ptr<Instrument> Instrument::prepare(const Model&m,double sr,std::string*err){
 auto fail=[&](const char*reason)->std::unique_ptr<Instrument>{if(err)*err=reason;return {};};
 Model chk;
 if(!valid(sr,22050,192000)||!deserialize(serialize(m),chk))return fail("invalid six-string physical configuration");
 auto p=std::unique_ptr<Instrument>(new Instrument());p->m_=m;p->sr_=sr;
 if(!buildMesh(p->K_,p->mass_,m))return fail("degenerate 3D soundboard mesh");
 p->meshVertices_=int(m.mesh.vertices.size());
 for(int i=0;i<p->meshVertices_*3;i++)for(int j=0;j<p->meshVertices_*3;j++){
  const double v=p->K_[size_t(i)][size_t(j)];
  if(std::abs(v)>1.e-15)p->sparseK_[size_t(i)].push_back({j,v});
 }
 double fastest=0;
 for(int n=0;n<kStrings;n++){
  auto& s=p->string_[size_t(n)];
  s.kt=std::pow(2*hz(pitches[n])*(kPoints+1),2)*m.stringTensionScale;
  s.kl=s.kt*m.longitudinalRatio;
  s.kr=s.kt*m.torsionRatio;
  s.bridgeK=s.kt*m.bridgeCoupling;
  fastest=std::max(fastest,s.kt*(4+m.fingerStiffness+m.fretStiffness));
 }
 // conservative explicit-step Courant bound incl. FEM row sum/mass estimate
 for(int i=0;i<p->meshVertices_*3;i++){
  double row=0;for(auto term:p->sparseK_[size_t(i)])row+=std::abs(term.second);
  if(p->mass_[size_t(i)]>1.e-12)fastest=std::max(fastest,row/p->mass_[size_t(i)]);
 }
 const double omega=std::sqrt(fastest);
 p->steps_=std::clamp(int(std::ceil(omega/(sr*.43))),3,24);
 if(omega/(sr*p->steps_)>.55)return fail("stability substep limit exceeded");
 p->glideCoeff_=1-std::exp(-1./(sr*.001*m.glideMs));
 if(err)err->clear();
 return p;
}
double Instrument::rand01() noexcept{
 rng_^=rng_<<13;rng_^=rng_>>17;rng_^=rng_<<5;return double(rng_&0xffffu)/65535.;
}
void Instrument::nextGesture(Gesture g,float f) noexcept{gesture_=g;gestureStrength_=std::isfinite(f)?float(clamp(f,0,1)):1.f;}
void Instrument::noteOn(int note,float velocity) noexcept{
 if(note<0||note>127||!std::isfinite(velocity)||velocity<=0)return;
 int choice=-1,best=10000;
 if(gesture_!=Gesture::Pluck&&lastPlayedString_>=0){
  const int fret=note-pitches[lastPlayedString_];
  if(fret>=0&&fret<=24)choice=lastPlayedString_;
 }
 const bool reuse=(choice>=0);
 for(int s=0;s<kStrings&&!reuse;s++){
  int fret=note-pitches[s];if(fret<0||fret>24)continue;
  int score=fret+(string_[size_t(s)].held?50:0);
  if(score<best){best=score;choice=s;}
 }
 if(choice<0)return;
 lastPlayedString_=choice;auto& s=string_[size_t(choice)];const double fret=clamp(double(note-pitches[choice])+bend_,0.,24.);
 const double contact=clamp(1-std::exp2(-fret/12.),0.,.77);
 s.target=contact;s.midi=note;s.held=true;
 const double intensity=std::pow(clamp(velocity,0,1),1.3)*.00023;
 if(gesture_==Gesture::Pluck||s.amp==0){
  const double p=clamp(m_.pluckPosition+(.5-velocity)*.07,.05,.95);
  for(int i=0;i<kPoints;i++){
   const double x=double(i+1)/(kPoints+1);
   s.y[size_t(i)]+=intensity*(x<p?x/p:(1-x)/(1-p));
   s.twist[size_t(i)]+=intensity*.015*std::sin(pi*x);
  }
 }else {
  const double impact=(gesture_==Gesture::Pull?-1.:1.)*velocity*gestureStrength_*.07;
  const int ix=std::clamp(int(contact*(kPoints+1))-1,0,kPoints-1);
  s.vy[size_t(ix)]+=impact;
  if(gesture_==Gesture::Slide)s.slide=gestureStrength_;
 }
 s.amp=1;gesture_=Gesture::Pluck;gestureStrength_=1;
}
void Instrument::noteOff(int midi) noexcept{
 for(auto&s:string_)if(s.midi==midi)s.held=false;
}
void Instrument::pitchBend(float semitones) noexcept{
 if(!std::isfinite(semitones))return;
 bend_=clamp(semitones,-2,2);
 for(int i=0;i<kStrings;i++){
  auto& s=string_[size_t(i)];if(s.midi>=0)s.target=clamp(1-std::exp2(-clamp(s.midi-pitches[i]+bend_,0.,24.)/12.),0.,.77);
 }
}
void Instrument::channelPressure(float x) noexcept{if(std::isfinite(x))pressure_=clamp(x,0,1);}
void Instrument::palmMute(float x) noexcept{if(std::isfinite(x))palm_=clamp(x,0,1);}
void Instrument::allNotesOff() noexcept{
 lastPlayedString_=-1;
 for(auto&s:string_)s=StringState{};
 for(int n=0;n<kStrings;n++){
  auto&s=string_[size_t(n)];s.kt=std::pow(2*hz(pitches[n])*(kPoints+1),2)*m_.stringTensionScale;s.kl=s.kt*m_.longitudinalRatio;s.kr=s.kt*m_.torsionRatio;s.bridgeK=s.kt*m_.bridgeCoupling;
 }
 meshX_.fill(0);meshV_.fill(0);
}
void Instrument::evaluateForces(std::array<std::array<double,kPoints>,kStrings>&fy,
 std::array<std::array<double,kPoints>,kStrings>&fa,
 std::array<std::array<double,kPoints>,kStrings>&ft,
 std::array<double,kDof>& mf) noexcept{
 for(int i=0;i<meshVertices_*3;i++){
  double f=0;for(const auto&term:sparseK_[size_t(i)])f-=term.second*meshX_[size_t(term.first)];
  mf[size_t(i)]=f;
 }
 for(int n=0;n<kStrings;n++){
  auto&s=string_[size_t(n)];fy[size_t(n)].fill(0);fa[size_t(n)].fill(0);ft[size_t(n)].fill(0);
  const int v=m_.mesh.bridgeVertex[size_t(n)],yCoord=v*3+1,xCoord=v*3,zCoord=v*3+2;
  // Fixed nut and 3D mesh-controlled bridge end. Each segment applies
  // equal-and-opposite forces through its potential gradient.
  for(int i=0;i<=kPoints;i++){
   const double yl=i==0?0:s.y[size_t(i-1)];
   const double yr=i==kPoints?meshX_[size_t(yCoord)]:s.y[size_t(i)];
   const double al=i==0?0:s.axial[size_t(i-1)];
   const double ar=i==kPoints?meshX_[size_t(xCoord)]:s.axial[size_t(i)];
   const double tl=i==0?0:s.twist[size_t(i-1)];
   const double tr=i==kPoints?meshX_[size_t(zCoord)]/.04:s.twist[size_t(i)];
   const double dy=yr-yl, da=ar-al, dtheta=tr-tl;
   // Axial coupling of transverse displacement via geometric stretching
   // of a string segment, energy .5*kl*(da+0.5*dy^2)^2.
   const double stretch=da+.5*dy*dy;
   const double link=(i==kPoints?m_.bridgeCoupling:1.);
   const double T=link*(s.kt*dy+s.kl*stretch*dy);
   const double A=link*s.kl*stretch;
   const double R=link*s.kr*dtheta;
   if(i>0){fy[size_t(n)][size_t(i-1)]+=T;fa[size_t(n)][size_t(i-1)]+=A;ft[size_t(n)][size_t(i-1)]+=R;}
   if(i<kPoints){fy[size_t(n)][size_t(i)]-=T;fa[size_t(n)][size_t(i)]-=A;ft[size_t(n)][size_t(i)]-=R;}
   else {mf[size_t(yCoord)]-=T;mf[size_t(xCoord)]-=A;mf[size_t(zCoord)]-=R/.04;}
  }
  // Local string-fret unilateral contact; dissipative Coulomb-like regularized
  // kinetic friction along the string and torsional surface speed.
  for(int i=0;i<kPoints;i++){
   const double yi=s.y[size_t(i)], vyi=s.vy[size_t(i)];
   if(yi< -m_.fretGap){
    const double normal=std::max(0.,m_.fretStiffness*s.kt*(-m_.fretGap-yi));
    fy[size_t(n)][size_t(i)]+=normal;
    if(vyi<0)fy[size_t(n)][size_t(i)]-=vyi*m_.fingerDamping;
    const double rel=s.vaxial[size_t(i)]+.002*s.vtwist[size_t(i)];
    const double friction=m_.fretFriction*normal*std::tanh(rel/.05);
    fa[size_t(n)][size_t(i)]-=friction;
    ft[size_t(n)][size_t(i)]-=friction*.002;
    ++impacts_;
   }
   if(s.held&&s.finger>.005){
    const double w=std::max(0.,1.-std::abs((double(i+1)/(kPoints+1)-s.finger)*(kPoints+1)));
    fy[size_t(n)][size_t(i)]-=m_.fingerStiffness*s.kt*(1.+pressure_)*w*yi;
   }
  }
 }
}
void Instrument::integrate(double dt) noexcept{
 // Damped velocity-Verlet: conservative internal forces evaluated twice;
 // shared bridge mesh couples all six strings during the SAME substep.
 const double half=dt*.5;
 auto damp=[&](){
  for(auto&s:string_){
   const double base=std::exp(-(m_.stringLoss+(s.held||sustain_?0:30))*half);
   for(int i=0;i<kPoints;i++){
    double local=0;
    const double x=double(i+1)/(kPoints+1);
    if(s.finger>.005&&s.held)local+=m_.fingerDamping*(.3+pressure_)*std::max(0.,1.-std::abs((x-s.finger)*(kPoints+1)));
    local+=m_.palmDamping*palm_*std::max(0.,1.-std::abs((x-.94)*(kPoints+1)*.5));
    const double factor=base*std::exp(-local*half);
    s.vy[size_t(i)]*=factor;s.vaxial[size_t(i)]*=factor;s.vtwist[size_t(i)]*=factor;
   }
  }
  const double f=std::exp(-m_.meshLoss*half);
  for(auto&v:meshV_)v*=f;
 };
 damp();
 std::array<std::array<double,kPoints>,kStrings> fy{},fa{},ft{};std::array<double,kDof> mf{};
 evaluateForces(fy,fa,ft,mf);
 for(int n=0;n<kStrings;n++)for(int i=0;i<kPoints;i++){
  auto&s=string_[size_t(n)];const size_t j=size_t(i);
  s.vy[j]+=half*fy[size_t(n)][j];s.vaxial[j]+=half*fa[size_t(n)][j];s.vtwist[j]+=half*ft[size_t(n)][j];
  s.y[j]+=dt*s.vy[j];s.axial[j]+=dt*s.vaxial[j];s.twist[j]+=dt*s.vtwist[j];
 }
 for(int i=0;i<kDof;i++){
  if(mass_[size_t(i)]<1.e-10)continue;
  meshV_[size_t(i)]+=half*mf[size_t(i)]/mass_[size_t(i)];
  meshX_[size_t(i)]+=dt*meshV_[size_t(i)];
 }
 evaluateForces(fy,fa,ft,mf);
 for(int n=0;n<kStrings;n++)for(int i=0;i<kPoints;i++){
  auto&s=string_[size_t(n)];const size_t j=size_t(i);
  s.vy[j]+=half*fy[size_t(n)][j];s.vaxial[j]+=half*fa[size_t(n)][j];s.vtwist[j]+=half*ft[size_t(n)][j];
 }
 for(int i=0;i<kDof;i++)if(mass_[size_t(i)]>1.e-10)meshV_[size_t(i)]+=half*mf[size_t(i)]/mass_[size_t(i)];
 damp();
}
float Instrument::processOne() noexcept{
 const double dt=1./(sr_*steps_);
 for(auto&s:string_){s.finger+=(s.target-s.finger)*glideCoeff_;}
 for(int t=0;t<steps_;t++)integrate(dt);
 double sum=0;
 for(int i=0;i<kStrings;i++){
  auto&s=string_[size_t(i)];
  sum+=s.vy[size_t(kPoints*3/4)]*(.12+.025*i)+s.vaxial[size_t(kPoints/2)]*.025+s.vtwist[size_t(kPoints/2)]*.006;
 }
 for(int i=0;i<kVertices;i++)sum+=meshV_[size_t(3*i+1)]*.10;
 return float(std::tanh(std::isfinite(sum)?sum*m_.outputGain:0.));
}
double Instrument::meshEnergy() const noexcept{
 double e=0;for(int i=0;i<kDof;i++){
  e+=.5*mass_[size_t(i)]*meshV_[size_t(i)]*meshV_[size_t(i)];
  for(int j=0;j<kDof;j++)e+=.5*meshX_[size_t(i)]*K_[size_t(i)][size_t(j)]*meshX_[size_t(j)];
 }return e;
}
double Instrument::lastStringEnergy(int n) const noexcept{
 if(n<0||n>=kStrings)return 0;
 const auto&s=string_[size_t(n)];double e=0;
 for(int i=0;i<kPoints;i++)e+=.5*(s.vy[size_t(i)]*s.vy[size_t(i)]+s.vaxial[size_t(i)]*s.vaxial[size_t(i)]+s.vtwist[size_t(i)]*s.vtwist[size_t(i)]);
 const int vertex=m_.mesh.bridgeVertex[size_t(n)];double prevY=0,prevA=0,prevT=0;
 for(int i=0;i<=kPoints;i++){
  const double y=i==kPoints?meshX_[size_t(vertex*3+1)]:s.y[size_t(i)];
  const double a=i==kPoints?meshX_[size_t(vertex*3)]:s.axial[size_t(i)];
  const double t=i==kPoints?meshX_[size_t(vertex*3+2)]/.04:s.twist[size_t(i)];
  const double dy=y-prevY,da=a-prevA,dt=t-prevT,stretch=da+.5*dy*dy;
  const double link=(i==kPoints?m_.bridgeCoupling:1.);
  e+=link*(.5*s.kt*dy*dy+.5*s.kl*stretch*stretch+.5*s.kr*dt*dt);
  prevY=y;prevA=a;prevT=t;
 }
 if(s.held&&s.finger>.005)for(int i=0;i<kPoints;i++){
  const double w=std::max(0.,1.-std::abs((double(i+1)/(kPoints+1)-s.finger)*(kPoints+1)));
  e+=.5*m_.fingerStiffness*s.kt*(1.+pressure_)*w*s.y[size_t(i)]*s.y[size_t(i)];
 }
 for(int i=0;i<kPoints;i++)if(s.y[size_t(i)]< -m_.fretGap){
  const double delta=s.y[size_t(i)]+m_.fretGap;
  e+=.5*m_.fretStiffness*s.kt*delta*delta;
 }
 return e;
}
double Instrument::axialKineticEnergy() const noexcept{
 double e=0;for(const auto&s:string_)for(double v:s.vaxial)e+=.5*v*v;return e;
}
double Instrument::torsionalKineticEnergy() const noexcept{
 double e=0;for(const auto&s:string_)for(double v:s.vtwist)e+=.5*v*v;return e;
}
double Instrument::mechanicalEnergy() const noexcept{
 double e=meshEnergy();for(int n=0;n<kStrings;n++)e+=lastStringEnergy(n);return e;
}
double Instrument::meshDisplacement() const noexcept{
 double v=0;for(double x:meshX_)v+=x*x;return std::sqrt(v);
}
int Instrument::activeStrings() const noexcept{
 int n=0;for(const auto&s:string_)if(s.held)++n;return n;
}
} // namespace panorama::fem36
