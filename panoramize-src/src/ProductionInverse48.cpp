#include "ProductionInverse48.h"
#include <algorithm>
#include <cmath>
#include <numeric>
namespace panorama::production48 {
namespace {
constexpr double pi=3.14159265358979323846;
bool finite(const std::vector<float>&a){return std::all_of(a.begin(),a.end(),[](float x){return std::isfinite(x);});}
double quantile(std::vector<double>&v,double q){if(v.empty())return 0.;auto k=size_t(std::clamp(q,0.,1.)*(v.size()-1));std::nth_element(v.begin(),v.begin()+k,v.end());return v[k];}
double volume6(const fem36::Mesh&m,const std::array<int,4>&t){
 const auto&a=m.vertices[size_t(t[0])],&b=m.vertices[size_t(t[1])],&c=m.vertices[size_t(t[2])],&d=m.vertices[size_t(t[3])];
 const double x=b[0]-a[0],y=b[1]-a[1],z=b[2]-a[2];
 const double cx=c[0]-a[0],cy=c[1]-a[1],cz=c[2]-a[2],dx=d[0]-a[0],dy=d[1]-a[1],dz=d[2]-a[2];
 return x*(cy*dz-cz*dy)-y*(cx*dz-cz*dx)+z*(cx*dy-cy*dx);
}
double fitLoss(const fem36::Model&m,const std::vector<inverse36::Take>&takes,int sr,bool train){
 double score=0;int count=0;
 for(const auto&t:takes)if(t.training==train){
  const auto generated=inverse36::render(m,sr,t.midi,t.velocity,t.audio.size());
  if(generated.size()!=t.audio.size())return 1e20;
  // Limited onset jitter and gain-invariant cosine similarity. This tolerates
  // small microphone onset errors, but not reverb tails, equalization or polyphony.
  double coherence=0.;
  for(int shift=-24;shift<=24;shift++){
   double aa=0.,bb=0.,ab=0.;
   const size_t lo=shift<0?size_t(-shift):0;
   const size_t hi=shift>0?t.audio.size()-size_t(shift):t.audio.size();
   for(size_t i=lo;i<hi;i++){
    const double a=t.audio[i],b=generated[size_t(int(i)+shift)];
    aa+=a*a;bb+=b*b;ab+=a*b;
   }
   if(aa>1e-12&&bb>1e-12)coherence=std::max(coherence,std::clamp(ab*ab/(aa*bb),0.,1.));
  }
  score+=1-coherence;count++;
 }
 return count?score/count:1e20;
}
}
Quality assess(const std::vector<float>&a,int sr){
 Quality q;q.duration=sr>0?double(a.size())/sr:0.;
 if(sr<8000||sr>192000||a.empty()||!finite(a)){q.questionable=true;return q;}
 double sq=0;size_t clip=0,silent=0;
 for(float x:a){sq+=double(x)*x;q.peak=std::max(q.peak,double(std::abs(x)));clip+=std::abs(x)>=.999f;silent+=std::abs(x)<1e-5f;}
 q.rms=std::sqrt(sq/a.size());q.clippedFraction=double(clip)/a.size();q.silentFraction=double(silent)/a.size();
 const size_t hop=std::max(size_t(1),size_t(sr/20));std::vector<double>levels;
 for(size_t at=0;at<a.size();at+=hop){double en=0;const auto end=std::min(a.size(),at+hop);for(size_t k=at;k<end;k++)en+=double(a[k])*a[k];levels.push_back(std::sqrt(en/(end-at)));}
 auto high=levels; q.noiseFloorRms=quantile(levels,.1);const double p95=quantile(high,.95);
 q.estimatedDynamicRangeDb=20*std::log10((p95+1e-10)/(q.noiseFloorRms+1e-10));
 q.questionable=q.clippedFraction>.001||q.rms<1e-5||q.silentFraction>.92;
 return q;
}
LongAnalysis analyzeLong(const std::vector<float>&audio,int rate,int hop){
 LongAnalysis out;out.quality=assess(audio,rate);
 if(rate<8000||rate>96000||audio.size()<size_t(rate/3)||audio.size()>size_t(rate)*600||!finite(audio)||hop<128||hop>1024){out.error="unsupported duration/rate/nonfinite/hop";return out;}
 const size_t chunk=std::max(size_t(2048),size_t(rate)*8);double weights=0,confidence=0;
 for(size_t at=0;at<audio.size();at+=chunk){
  const auto end=std::min(audio.size(),at+chunk);
  if(end-at<size_t(rate/3))break;
  std::vector<float> segment(audio.begin()+at,audio.begin()+end);
  const auto partial=deep46::analyze(segment,rate,hop);
  if(!partial.error.empty()){out.error=partial.error;return out;}
  const double shift=double(at)/rate;
  for(auto x:partial.sections){x.fromSec+=shift;x.toSec+=shift;out.sections.push_back(x);}
  for(auto x:partial.gestures){x.timeSec+=shift;out.gestures.push_back(x);}
  const double w=double(end-at);weights+=w;confidence+=w*partial.validPitchFraction;
 }
 out.validPitchFraction=weights>0?confidence/weights:0;
 return out;
}
bool buildFreeform(const Freeform&f,fem36::Mesh&out,std::string*err){
 auto fail=[&](const char*s){if(err)*err=s;return false;};
 for(double v:f.controls)if(!std::isfinite(v)||std::abs(v)>.28)return fail("warp parameter out of bounds");
 if(!gen37::buildTetraMesh(f.base,out,err))return false;
 const double L=f.base.length,W=f.base.width,H=f.base.thickness;
 // Smooth four-mode freeform warp. Vertices retain tetra connectivity, but surfaces
 // are no longer restricted to camber/taper lattice transformation.
 for(auto&v:out.vertices){
  const double u=std::clamp(v[0]/L,0.,1.),w=std::clamp(v[1]/W,0.,1.);
  const double s=std::sin(pi*u);
  const double height=H*(f.controls[0]*s + f.controls[1]*(2*u-1)*std::cos(pi*u)
                          + f.controls[2]*(2*w-1) + f.controls[3]*s*(2*w-1));
  v[2]+=height;
 }
 // Reject tetra inversion and reduced quality vs the unwarped mesh; never silently
 // produce invalid finite element volumes.
 fem36::Mesh reference;if(!gen37::buildTetraMesh(f.base,reference))return fail("base mesh invalid");
 for(size_t k=0;k<out.tetrahedra.size();k++){
  const double before=volume6(reference,reference.tetrahedra[k]);
  const double after=volume6(out,out.tetrahedra[k]);
  if(!std::isfinite(after)||before*after<=0||std::abs(after)<std::abs(before)*.18)return fail("warped tetrahedron inverted or collapsed");
 }
 if(err)err->clear();return true;
}
Inversion fitSurfaceMaterial(const std::vector<inverse36::Take>&takes,int sr,const fem36::Model&seed,int budget){
 Inversion out;
 if(sr<22050||sr>96000||budget<2||budget>128||takes.size()<3||takes.size()>12){out.error="invalid inverse session";return out;}
 int nTrain=0,nHold=0;for(const auto&t:takes){
  if(t.audio.size()<256||t.audio.size()>size_t(sr)*2||!finite(t.audio)||t.midi<0||t.midi>127||!std::isfinite(t.velocity)||t.velocity<=0||t.velocity>1){out.error="invalid take";return out;}
  (t.training?nTrain:nHold)++;
 }if(nTrain<2||nHold<1){out.error="2 training and 1 holdout take required";return out;}
 // For physically meaningful geometry, do not guess a base mesh from an arbitrary
 // unlabeled model. Calibrate 3D surface around the supplied documented seed.
 gen37::Shape shape;shape.xCells=2;shape.yCells=1;shape.zCells=1;
 fem36::Mesh baseline;
 if(!gen37::buildTetraMesh(shape,baseline)||seed.mesh.vertices.size()!=baseline.vertices.size()||
    seed.mesh.tetrahedra!=baseline.tetrahedra){out.error="seed topology does not match supported base lattice; use geometry prior";return out;}
 for(size_t i=0;i<baseline.vertices.size();i++)for(int axis=0;axis<3;axis++)
  if(std::abs(seed.mesh.vertices[i][size_t(axis)]-baseline.vertices[i][size_t(axis)])>1e-8){
   out.error="seed geometry differs from documented base; shape not identifiable with this search";return out;
  }
 const std::array<double,5> vars{{0.,-.15,.15,-.25,.25}};
 const std::array<double,3> materialMultipliers{{.7,1.,1.3}};
 std::vector<ShapeFit> candidates;
 auto append=[&](int parameter,double setting,double eFactor,double rhoFactor){
  if(out.evaluations>=budget)return;
  ShapeFit c;c.surface.base=shape;c.material=seed;
  if(parameter>=0)c.surface.controls[size_t(parameter)]=setting;
  c.material.meshYoung=seed.meshYoung*eFactor;c.material.meshDensity=seed.meshDensity*rhoFactor;
  if(!buildFreeform(c.surface,c.material.mesh))return;
  c.trainError=fitLoss(c.material,takes,sr,true);out.evaluations++;
  if(std::isfinite(c.trainError))candidates.push_back(std::move(c));
 };
 append(-1,0,1,1);
 // Interleave material and shape so small budgets do not skip calibration entirely.
 for(int i=0;i<4&&out.evaluations<budget;i++){
  if(i==0)append(-1,0,.7,1);
  if(i==1)append(-1,0,1.3,1);
  if(i==2)append(-1,0,1,.7);
  if(i==3)append(-1,0,1,1.3);
  if(out.evaluations<budget)append(i,.15,1,1);
 }
 for(int p=0;p<4&&out.evaluations<budget;p++)for(double val:vars)if(val!=0&&val!=.15)append(p,val,1,1);
 for(double e:materialMultipliers)for(double rho:materialMultipliers)if(out.evaluations<budget&& (e!=1||rho!=1))append(-1,0,e,rho);
 if(candidates.empty()){out.error="no finite candidates";return out;}
 std::sort(candidates.begin(),candidates.end(),[](const auto&a,const auto&b){return a.trainError<b.trainError;});
 out.ambiguityRatio=candidates.size()>1?candidates[1].trainError/(candidates[0].trainError+1e-7):0.;
 out.uniqueIdentification=false; // physical inverse is not uniquely identifiable from these takes.
 const size_t keep=std::min(size_t(4),candidates.size());
 for(size_t i=0;i<keep;i++){
  auto c=std::move(candidates[i]);c.holdoutError=fitLoss(c.material,takes,sr,false);out.ranked.push_back(std::move(c));
 }
 return out;
}
Inversion fitAnnotatedPerformance(const std::vector<float>&continuous,int sr,
          const std::vector<AnnotatedNote>&notes,const fem36::Model&seed,int budget){
 Inversion out;
 if(sr<22050||sr>96000||notes.size()<3||notes.size()>12||!finite(continuous)||
    continuous.size()>size_t(sr)*600){out.error="invalid performance or annotations";return out;}
 std::vector<inverse36::Take>takes;
 for(const auto&n:notes){
  if(!std::isfinite(n.onsetSeconds)||n.onsetSeconds<0||n.midi<0||n.midi>127||
     !std::isfinite(n.velocity)||n.velocity<=0||n.velocity>1){out.error="invalid note annotation";return out;}
  const auto begin=size_t(n.onsetSeconds*sr);
  const size_t count=std::min(size_t(1024),size_t(sr)/10);
  if(begin>=continuous.size()||continuous.size()-begin<count){out.error="note annotation exceeds recording";return out;}
  inverse36::Take t;t.midi=n.midi;t.velocity=n.velocity;t.training=n.training;
  t.audio.assign(continuous.begin()+begin,continuous.begin()+begin+count);
  takes.push_back(std::move(t));
 }
 return fitSurfaceMaterial(takes,sr,seed,budget);
}
std::vector<Parameter> parameterRegistry(const universal23::Graph&g){
 std::vector<Parameter> p;
 for(size_t i=0;i<g.nodes.size();i++){
  const auto id="node."+std::to_string(i);p.push_back({id+".level","Graph node output", "gain",0,1,false,true});
 }
 if(std::any_of(g.nodes.begin(),g.nodes.end(),[](const auto&n){return n.kind==universal23::Kind::GenuineNative47;})){
  p.push_back({"native47.cutoff","TPT cutoff","Hz",20,20000,true,false});
  p.push_back({"native47.resonance","Filter resonance","ratio",.08,2,true,false});
  p.push_back({"native47.drive","Post-filter drive","ratio",.01,20,true,false});
  p.push_back({"native47.output","Output level","gain",0,4,true,false});
  p.push_back({"native47.wet","Filter mix","ratio",0,1,true,false});
  for(int k=0;k<4;k++)p.push_back({"native47.macro."+std::to_string(k+1),"Macro", "normalized",0,1,true,false});
  for(size_t m=0;m<g.genuine47.original.acoustic.modes.size();m++){
   p.push_back({"native27.mode."+std::to_string(m)+".damping","Native modal damping","1/s",0,1e4,false,true});
   p.push_back({"native27.mode."+std::to_string(m)+".nonlinear","Native nonlinearity","unitless",0,1e3,false,true});
  }
 }
 return p;
}
} // namespace panorama::production48
