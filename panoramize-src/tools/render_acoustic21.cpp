#include "AcousticGraph21.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
using namespace panorama::acoustic21;
static void w16(std::ostream& o,unsigned v){char p[2]={char(v),char(v>>8)};o.write(p,2);}
static void w32(std::ostream& o,unsigned v){char p[4]={char(v),char(v>>8),char(v>>16),char(v>>24)};o.write(p,4);}
static Material3 iso(double k,double d){Material3 m;m.stiffness=Tensor3::isotropic(k);m.damping=Tensor3::isotropic(d);return m;}
static std::shared_ptr<const PreparedGraph> makeInstrument(int variant,int midi){
 GraphBuilder b;
 const double hz=440.*std::exp2((midi-69.)/12.);
 const int count=variant==0?8:variant==1?12:5;
 for(int i=0;i<count;i++){
  Body body;body.name="resonator_"+std::to_string(i);
  double angle=6.283185307179586*i/count;
  body.rest=(variant==2 ? std::vector<Vec3>{{0,0,0},{.1,0,0},{0,.1,0},{0,0,.1},{.1,.1,.1}}[size_t(i)] : Vec3{std::cos(angle),std::sin(angle),.15*std::sin(angle*3)});
  body.mass=(variant==2?.04:.9+.13*(i%3));
  // Each body gets 3D modes; calibrated near note frequency for this demo.
  body.anchor=(variant==2?iso(30.,.5):iso(std::pow(6.283185307179586*hz,2)*body.mass*(.9+.12*i),variant==0?7.:18.));
  b.addBody(body);
 }
 if(variant==2){
  Tetrahedron a;a.vertices={0,1,2,3};a.youngPa=1.6e5;a.poisson=.25;a.densityKgM3=1000.;a.viscousSeconds=.0002;b.addTetrahedron(a);
  Tetrahedron c;c.vertices={1,2,3,4};c.youngPa=3.5e5;c.poisson=.32;c.densityKgM3=1200.;c.viscousSeconds=.0002;b.addTetrahedron(c);
  return b.compile(48000.);
 }
 for(int i=0;i<count;i++){
  Link l;l.a=i;l.b=(i+1)%count;l.coupling=iso(hz*hz*(variant==0?.38:.9),variant==0?4.:10.);
  b.addLink(l);
 }
 if(variant==1)for(int i=0;i<count/2;i++){
  Link l;l.a=i;l.b=i+count/2;l.coupling=iso(hz*hz*.24,2.);b.addLink(l);
 }
 return b.compile(48000.);
}
static bool render(const std::string& file,int variant){
 auto prepared=makeInstrument(variant,48);if(!prepared)return false;
 auto voice=prepared->makeVoice();
 voice.excite(0,{0.,0.,.13});voice.excite(1,{.06,0.,.05});
 const unsigned sr=48000,frames=sr*3;
 std::vector<int16_t> pcm;pcm.reserve(size_t(frames)*2);
 const double pi=3.141592653589793;
 double maxAmp=0.;std::vector<double> raw;raw.reserve(size_t(frames)*2);
 for(unsigned i=0;i<frames;i++){
  if(i%12000==100){const double f=variant==0?4.5:6.;voice.addForce(0,{0.,0.,f});}
  if(i%12000==99){SpectralDrive d;d.energy[0]=.8;d.weights[0]=1;d.phaseTurns[0]=double(i%480)/480.;d.maxForceNewtons=8.;voice.addForce(2,{spectralDriveForce(d),0.,0.});}
  if(!voice.processSample())return false;
  const double l=voice.audioVelocity(1,{.3,0,.7});
  const double r=voice.audioVelocity(variant==0?4:variant==1?8:4,{0,.35,.65});
  raw.push_back(l);raw.push_back(r);maxAmp=std::max({maxAmp,std::abs(l),std::abs(r)});
 }
 const double gain=maxAmp>1.e-10?.65/maxAmp:0.;
 for(double v:raw)pcm.push_back(int16_t(std::lround(std::clamp(v*gain,-1.,1.)*32767)));
 std::ofstream out(file,std::ios::binary);if(!out)return false;
 out.write("RIFF",4);w32(out,36+unsigned(pcm.size()*2));out.write("WAVEfmt ",8);
 w32(out,16);w16(out,1);w16(out,2);w32(out,sr);w32(out,sr*4);w16(out,4);w16(out,16);
 out.write("data",4);w32(out,unsigned(pcm.size()*2));for(auto v:pcm)w16(out,uint16_t(v));
 const auto e=voice.ledger();
 std::cout<<file<<" peak(before normalization)="<<maxAmp<<" energyResidual="<<e.residual<<"\n";
 return bool(out)&&maxAmp>1.e-7;
}
int main(int argc,char** argv){
 if(argc!=4){std::cerr<<"usage: panorama_acoustic21_demo OUT_RING.wav OUT_CROSS.wav OUT_TETRA.wav\n";return 2;}
 return render(argv[1],0)&&render(argv[2],1)&&render(argv[3],2)?0:1;
}
