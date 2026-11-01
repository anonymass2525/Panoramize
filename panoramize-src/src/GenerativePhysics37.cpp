#include "GenerativePhysics37.h"
#include "MeshInverse36.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <array>
#include <limits>
namespace panorama::gen37 {
namespace {
constexpr double pi=3.14159265358979323846;
}
bool validShape(const Shape&s) noexcept {
 if(s.xCells<1||s.yCells<1||s.zCells<1||s.xCells>6||s.yCells>6||s.zCells>4)return false;
 if((s.xCells+1)*(s.yCells+1)*(s.zCells+1)>fem36::kVertices ||
    6*s.xCells*s.yCells*s.zCells>96)return false;
 return std::isfinite(s.length)&&s.length>=.03&&s.length<=1.&&
        std::isfinite(s.width)&&s.width>=.02&&s.width<=1.&&
        std::isfinite(s.thickness)&&s.thickness>=.008&&s.thickness<=.5&&
        std::isfinite(s.camber)&&std::abs(s.camber)<=.30&&
        std::isfinite(s.taper)&&std::abs(s.taper)<=.35;
}
bool buildTetraMesh(const Shape&s,fem36::Mesh&out,std::string*error){
 auto fail=[&](const char*msg){if(error)*error=msg;return false;};
 if(!validShape(s))return fail("shape outside bounded mesh limits");
 fem36::Mesh m;
 const int nx=s.xCells+1,ny=s.yCells+1;
 auto idx=[&](int x,int y,int z){return x+nx*(y+ny*z);};
 for(int z=0;z<=s.zCells;z++)for(int y=0;y<=s.yCells;y++)for(int x=0;x<=s.xCells;x++){
  double u=double(x)/s.xCells,v=double(y)/s.yCells,w=double(z)/s.zCells;
  double taper=1.+s.taper*(u-.5);
  double xx=s.length*u,yy=s.width*(v-.5)*taper,zz=s.thickness*w+s.camber*s.thickness*std::sin(pi*u)*(.35+.65*std::sin(pi*v));
  m.vertices.push_back({xx,yy,zz});
 }
 for(int z=0;z<s.zCells;z++)for(int y=0;y<s.yCells;y++)for(int x=0;x<s.xCells;x++){
  const int a=idx(x,y,z),b=idx(x+1,y,z),c=idx(x+1,y+1,z),d=idx(x,y+1,z);
  const int e=idx(x,y,z+1),f=idx(x+1,y,z+1),g=idx(x+1,y+1,z+1),h=idx(x,y+1,z+1);
  // Six tetrahedra around the same a -> g diagonal, conforming across cells.
  for(const std::array<int,4>&t:std::array<std::array<int,4>,6>{{{{a,b,c,g}},{{a,c,d,g}},{{a,d,h,g}},{{a,h,e,g}},{{a,e,f,g}},{{a,f,b,g}}}})
   m.tetrahedra.push_back(t);
 }
 // Attach six strings to nearest lattice vertices on the far (+x) edge.
 for(int k=0;k<fem36::kStrings;k++){
  const double alpha=double(k)/5.,v=alpha*s.yCells;
  int yi=std::clamp(int(std::lround(v)),0,s.yCells);
  int zi=(k%2)?s.zCells:0;
  m.bridgeVertex[size_t(k)]=idx(s.xCells,yi,zi);
 }
 // Round-trip through FEM36's rigorous tetra validation rather than trusting generation.
 fem36::Mesh verified;
 if(!fem36::deserializeMesh(fem36::serializeMesh(m),verified,error))return false;
 out=std::move(verified);if(error)error->clear();return true;
}
std::string encodeShape(const Shape&s){
 std::ostringstream o;o<<std::setprecision(17)<<"PANORAMIZE-SHAPE37-v1\n"<<s.xCells<<' '<<s.yCells<<' '<<s.zCells<<' '<<s.length<<' '<<s.width<<' '<<s.thickness<<' '<<s.camber<<' '<<s.taper<<'\n';return o.str();
}
bool decodeShape(const std::string&text,Shape&out){
 if(text.size()>500)return false;
 std::istringstream in(text);std::string tag,extra;Shape s;
 if(!(in>>tag)||tag!="PANORAMIZE-SHAPE37-v1")return false;
 if(!(in>>s.xCells>>s.yCells>>s.zCells>>s.length>>s.width>>s.thickness>>s.camber>>s.taper)||in>>extra||!validShape(s))return false;
 out=s;return true;
}
std::string encodeWaveRecipe(const WaveRecipe&r){
 std::ostringstream s;s<<std::setprecision(17)<<"PANORAMIZE-WAVE37-v1\n";
 s<<int(r.family)<<' '<<r.nx<<' '<<r.ny<<' '<<r.fundamentalHz<<' '<<r.damping<<' '<<r.dispersion
  <<' '<<r.exciter<<' '<<r.pickup<<' '<<r.amplitude<<'\n';return s.str();
}
bool decodeWaveRecipe(const std::string&source,WaveRecipe&out){
 if(source.size()>500)return false;
 std::istringstream in(source);std::string header,extra;WaveRecipe r;int kind=-1;
 if(!(in>>header)||header!="PANORAMIZE-WAVE37-v1")return false;
 if(!(in>>kind>>r.nx>>r.ny>>r.fundamentalHz>>r.damping>>r.dispersion>>r.exciter>>r.pickup>>r.amplitude)||in>>extra||kind<0||kind>3)return false;
 r.family=static_cast<Family>(kind);
 if(renderWave(r,22050,32).size()!=32)return false;
 out=r;return true;
}
std::vector<float> renderTetra(const fem36::Model&m,int rate,int midi,float velocity,std::size_t frames){
 return inverse36::render(m,rate,midi,velocity,frames);
}
std::vector<float> renderWave(const WaveRecipe&r,int rate,std::size_t frames){
 if(rate<22050||rate>96000||frames<32||frames>std::size_t(rate)*10||
    r.nx<8||r.nx>64||r.ny<8||r.ny>48||
    !std::isfinite(r.fundamentalHz)||r.fundamentalHz<40||r.fundamentalHz>1600||
    !std::isfinite(r.damping)||r.damping<0||r.damping>400||
    !std::isfinite(r.dispersion)||r.dispersion<0||r.dispersion>.01||
    !std::isfinite(r.exciter)||r.exciter<=0||r.exciter>=1||
    !std::isfinite(r.pickup)||r.pickup<=0||r.pickup>=1||
    !std::isfinite(r.amplitude)||r.amplitude<0||r.amplitude>1||
    r.family==Family::TetraResonator3D)return {};
 bool is2D=r.family==Family::Membrane2D||r.family==Family::Plate2D;
 const int nx=r.nx,ny=is2D?r.ny:1,N=nx*ny;
 std::vector<double> u(size_t(N),0.),v(size_t(N),0.),a(size_t(N),0.),lap(size_t(N),0.);
 auto index=[&](int x,int y){return size_t(x+nx*y);};
 // Spatial units are grid coordinates; c is chosen to scale a low mode near f0.
 // Acceleration = c^2 Laplacian - 2 damping velocity - bending Laplacian^2.
 const double factor=is2D?std::sqrt(2.):1.;
 const double c=2*r.fundamentalHz*(nx-1)/factor;
 const int oversample=std::max(8,int(std::ceil(2*c*std::sqrt(is2D?2.:1.)/rate)));
 if(oversample>128)return {};
 const double dt=1./(rate*oversample),c2=c*c;
 // Plate bending is a discrete curvature penalty; not a calibrated thin-plate model.
 const double bending=r.family==Family::Plate2D?r.dispersion*c2:0.;
 const int ex=std::clamp(int(std::round(r.exciter*(nx-1))),1,nx-2);
 const int ey=is2D?std::clamp(ny/2,1,ny-2):0;
 const int px=std::clamp(int(std::round(r.pickup*(nx-1))),1,nx-2);
 const int py=is2D?std::clamp(ny/2+1,1,ny-2):0;
 // Tube: 1D staggered pressure-flow leapfrog system, absorbing loss & open ends.
 if(r.family==Family::Tube1D){
  std::vector<double> p(size_t(nx),0.),flow(size_t(nx-1),0.);
  p[size_t(ex)]=r.amplitude;
  std::vector<float> out(frames);
  double waveSpeed=2*r.fundamentalHz*(nx-1); // p/u nondimensional acoustics
  double alpha=waveSpeed*dt;
  if(alpha>=.5)return {};
  for(size_t n=0;n<frames;n++){
   for(int sub=0;sub<oversample;sub++){
    for(int i=0;i<nx-1;i++)flow[size_t(i)]=(flow[size_t(i)]-alpha*(p[size_t(i+1)]-p[size_t(i)]))/(1+r.damping*dt);
    for(int i=1;i<nx-1;i++)p[size_t(i)]=(p[size_t(i)]-alpha*(flow[size_t(i)]-flow[size_t(i-1)]))/(1+r.damping*dt);
    p.front()=p.back()=0.;
   }
   double value=p[size_t(px)];if(!std::isfinite(value)||std::abs(value)>1e4)return {};
   out[n]=float(std::clamp(value,-1.,1.));
  }return out;
 }
 // Nonzero initial local displacement; fixed boundaries represent clamped edges.
 u[index(ex,ey)]=r.amplitude;
 std::vector<float> out(frames);
 for(size_t n=0;n<frames;n++){
  for(int sub=0;sub<oversample;sub++){
   for(int y=0;y<ny;y++)for(int x=0;x<nx;x++){
    const size_t i=index(x,y);
    if(x==0||x==nx-1||(is2D&&(y==0||y==ny-1))){lap[i]=0;continue;}
    double L=u[index(x-1,y)]+u[index(x+1,y)]-2*u[i];
    if(is2D)L+=u[index(x,y-1)]+u[index(x,y+1)]-2*u[i];
    lap[i]=L;
   }
   for(int y=0;y<ny;y++)for(int x=0;x<nx;x++){
    const size_t i=index(x,y);
    if(x==0||x==nx-1||(is2D&&(y==0||y==ny-1))){v[i]=0;u[i]=0;continue;}
    double bih=0;
    if(bending>0 && x>1&&x<nx-2&&(!is2D||(y>1&&y<ny-2))){
     bih=lap[index(x-1,y)]+lap[index(x+1,y)]-2*lap[i];
     if(is2D)bih+=lap[index(x,y-1)]+lap[index(x,y+1)]-2*lap[i];
    }
    a[i]=c2*lap[i]-bending*bih;
   }
   for(int y=0;y<ny;y++)for(int x=1;x<nx-1;x++){
    if(is2D&&(y==0||y==ny-1))continue;
    const size_t i=index(x,y);
    v[i]=(v[i]+dt*a[i])/(1+r.damping*dt);
    u[i]+=dt*v[i];
   }
  }
  const double value=u[index(px,py)];if(!std::isfinite(value)||std::abs(value)>1e4)return {};
  out[n]=float(std::clamp(value,-1.,1.));
 }return out;
}
}
