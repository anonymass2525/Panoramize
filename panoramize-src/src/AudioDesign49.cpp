#include "AudioDesign49.h"
#include "GuitarInverse29.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>
namespace panorama::design49 {
namespace {
constexpr double pi=3.14159265358979323846;
bool validRate(int r){return r>=22050&&r<=96000;}
double squared(double x){return x*x;}
// Weighted, time-averaged harmonic descriptor. Does not equate spectral similarity
// with geometric identifiability; sample phase and recording gain are ignored.
std::array<double,10> harmonics(const std::vector<float>&a,int sr,double f0){
 std::array<double,10> h{};if(a.size()<512||f0<35)return h;
 const size_t n=std::min<size_t>(4096,a.size());
 size_t begin=0;double eMax=-1;
 for(size_t i=0;i+n<=a.size();i+=n/4){double e=0;for(size_t j=0;j<n;j+=8)e+=squared(a[i+j]);if(e>eMax){eMax=e;begin=i;}}
 for(size_t k=1;k<=h.size();k++){
  const double hz=f0*double(k);if(hz>sr*.46)continue;
  double re=0,im=0;
  for(size_t j=0;j<n;j++){
   const double w=.5-.5*std::cos(2*pi*double(j)/double(n-1));
   const double phase=2*pi*hz*double(j)/sr;
   re+=a[begin+j]*w*std::cos(phase);im-=a[begin+j]*w*std::sin(phase);
  }
  h[k-1]=std::hypot(re,im);
 }
 double sum=0;for(double x:h)sum+=x*x;
 if(sum>0)for(double&x:h)x/=std::sqrt(sum);
 return h;
}
// Correlation-based coarse pitch estimator; F0 only and *not* polyphonic.
std::pair<double,double> pitch(const std::vector<float>&a,int sr){
 const int n=int(std::min<size_t>(4096,a.size()));if(n<600)return {0,0};
 size_t start=0;double maxE=-1;
 for(size_t t=0;t+size_t(n)<=a.size();t+=512){double e=0;for(int i=0;i<n;i+=8)e+=squared(a[t+size_t(i)]);if(e>maxE){maxE=e;start=t;}}
 if(maxE<1e-10)return {0,0};
 const int lo=std::max(1,sr/1100), hi=std::min(n/2,sr/55);
 double best=0,lag=0;
 // normalized difference auto-correlation discourages spurious silence peaks
 for(int k=lo;k<=hi;k++){
  double dot=0,ea=0,eb=0;
  for(int i=0;i<n-k;i+=2){double x=a[start+size_t(i)],y=a[start+size_t(i+k)];dot+=x*y;ea+=x*x;eb+=y*y;}
  const double v=dot/std::sqrt(ea*eb+1e-20);
  if(v>best){best=v;lag=k;}
 }
 if(best<.24||lag<1)return {0,best};
 return {double(sr)/lag,std::clamp(best,0.,1.)};
}
} // namespace
const char* name(Family f)noexcept{
 switch(f){case Family::NativeVA_FM:return "Native27 VA/FM and tone";
 case Family::NativeState:return "Native27 editable acoustic states";
 case Family::PhysicalString:return "Physical plucked string (modal)";
 case Family::NativePhysicalHybrid:return "Native27 + physical string hybrid";
 case Family::MultidimensionalFEM:return "Reduced 1D/2D/3D FEM structure";}return "Unknown";
}
Features extract(const std::vector<float>&a,int sr){
 Features d;if(!validRate(sr)||a.empty()||a.size()>size_t(sr)*600)return d;
 d.durationSeconds=double(a.size())/sr;
 double power=0,peak=0;for(float x:a){if(!std::isfinite(x))return {};power+=double(x)*x;peak=std::max(peak,std::abs(double(x)));}
 d.rms=std::sqrt(power/a.size());if(peak<1e-6)return d;
 auto [f0,conf]=pitch(a,sr);d.pitchHz=f0;d.pitchConfidence=conf;
 const auto hs=harmonics(a,sr,f0);double sum=0,weighted=0,logsum=0;
 for(size_t k=0;k<hs.size();k++){const double v=hs[k]*hs[k];sum+=v;weighted+=v*f0*(k+1);logsum+=std::log(1.e-9+v);}
 if(sum>1.e-14){d.centroidHz=weighted/sum;d.highHarmonicRatio=0;
  for(size_t k=3;k<hs.size();k++)d.highHarmonicRatio+=hs[k]*hs[k]/sum;
  d.flatness=std::exp(logsum/hs.size())/(sum/hs.size());}
 const double threshold=peak*.08;size_t onset=0;while(onset<a.size()&&std::abs(a[onset])<threshold)onset++;
 d.attackSeconds=double(onset)/sr;return d;
}
universal23::Graph editableGraph(Family family,double f0,double pickPosition){
 universal23::Graph g;
 if(family==Family::NativeVA_FM||family==Family::NativePhysicalHybrid)g=universal23::genuineNativeGraph();
 else if(family==Family::NativeState)g=universal23::directGraph();
 else if(family==Family::MultidimensionalFEM)g=universal23::reducedMultirateGraph();
 else {g.name="Panoramize / Physical string candidate";g.nodes={{universal23::Kind::GuitarString29,-1,-1,0,.84f},{universal23::Kind::Output,0,-1,0,.80f}};}
 if(family==Family::PhysicalString||family==Family::NativePhysicalHybrid){
  g.guitar.frequencyHz=std::clamp(f0,65.,1000.);
  g.guitar.pickPosition=std::clamp(pickPosition,.07,.48);
  g.guitar.harmonics=16;
 }
 if(family==Family::NativePhysicalHybrid){
  g.name="Panoramize / Suggested Hybrid (Native + Physical)";
  g.nodes={{universal23::Kind::GenuineNative47,-1,-1,0,.70f},
    {universal23::Kind::GuitarString29,-1,-1,0,.48f},
    {universal23::Kind::Mix,0,1,0,.65f},
    {universal23::Kind::Output,2,-1,0,.9f}};
  g.globalRoutes.push_back({genuine47::Source::Macro2,1,.2f});
 }
 // Adopt a shared four-macro / wheel modulation row for *any* generated graph.
 if(g.globalRoutes.empty())g.globalRoutes={{genuine47::Source::Macro1,0,.2f},
                                                 {genuine47::Source::Wheel,0,.15f}};
 if(family==Family::NativeState)g.name="Panoramize / Suggested editable acoustic-state synthesis";
 else if(family==Family::NativeVA_FM)g.name="Panoramize / Suggested Native27 synthesis";
 return g;
}
std::vector<float> renderGraph(const universal23::Graph&g,int midi,int rate,double sec,std::string*error){
 std::vector<float> a;if(!validRate(rate)||sec<=0||sec>5){if(error)*error="bad rendering settings";return a;}
 auto inst=universal23::Instrument::prepare(g,rate,error);if(!inst)return a;
 inst->noteOn(midi,.75f);const size_t n=size_t(sec*rate);a.resize(n);
 for(size_t i=0;i<n;i++){const auto s=inst->processOne();a[i]=.5f*(s.left+s.right);if(!std::isfinite(a[i])){if(error)*error="non-finite output";return {};}}
 if(error)error->clear();
 return a;
}
std::vector<production48::Parameter> unifiedParameterRegistry(const universal23::Graph&g){
 auto p=production48::parameterRegistry(g);
 // New v4.9 registry extends legacy behavior without altering saved 4.8 semantics.
 for(size_t i=0;i<g.nodes.size();i++){
  const auto id="node."+std::to_string(i)+".level";
  for(auto&d:p)if(d.id==id){d.live=i<8;d.requiresRebuild=i>=8;break;}
 }
 for(int k=0;k<4;k++)p.push_back({"graph.macro."+std::to_string(k+1),"Global macro", "normalized",0,1,true,false});
 const auto has=[&](universal23::Kind k){return std::any_of(g.nodes.begin(),g.nodes.end(),[k](const auto&n){return n.kind==k;});};
 if(has(universal23::Kind::GuitarString29)){
  p.push_back({"physical.string.frequency_hz","String fundamental", "Hz",40,1100,false,true});
  p.push_back({"physical.string.pluck_position","Pluck position", "normalized",.05,.48,false,true});
  p.push_back({"physical.string.pickup_position","Pickup position", "normalized",.05,.95,false,true});
  p.push_back({"physical.string.decay","String damping", "1/s",.1,15,false,true});
 }
 if(has(universal23::Kind::DirectState27))for(size_t i=0;i<g.instrument.modes.size();i++){
  p.push_back({"state27.mode."+std::to_string(i)+".ratio","Modal frequency ratio","ratio",.01,40,false,true});
  p.push_back({"state27.mode."+std::to_string(i)+".damping","Modal damping","1/s",0,1e4,false,true});
 }
 if(has(universal23::Kind::ReducedHamiltonian44))
  p.push_back({"reduced44.topology","FEM structure and boundary coupling","graph",0,1,false,true});
 return p;
}
Result infer(const std::vector<float>&a,int sr,int evaluations){
 Result r;
 if(!validRate(sr)||a.size()<size_t(sr)/4||a.size()>size_t(sr)*600){r.error="requires finite 0.25-600 s audio, 22050-96000 Hz";return r;}
 r.observation=extract(a,sr);
 if(!(r.observation.rms>1e-5)||r.observation.pitchConfidence<.27){r.error="No stable monophonic pitch; cannot infer a physical/algorithmic structure";return r;}
 const int midi=std::clamp(int(std::lround(69+12*std::log2(r.observation.pitchHz/440.))),36,96);
 std::vector<float> target;
 // Crop to highest-energy 0.2 s to avoid lengthy audio simulation.
 const size_t wanted=size_t(sr*.20);size_t bestAt=0;double maxPower=-1;
 for(size_t p=0;p+wanted<=a.size();p+=std::max<size_t>(1,wanted/2)){
  double e=0;for(size_t j=0;j<wanted;j+=16)e+=squared(a[p+j]);if(e>maxPower){maxPower=e;bestAt=p;}
 }
 target.assign(a.begin()+bestAt,a.begin()+bestAt+wanted);
 const auto targetH=harmonics(target,sr,r.observation.pitchHz);
 // A small bounded forward-model comparison of existing real synth algorithms.
 // Scores are descriptor distances, *not* calibrated posterior probabilities.
 auto families=std::array<Family,5>{Family::NativeVA_FM,Family::NativeState,Family::PhysicalString,Family::NativePhysicalHybrid,Family::MultidimensionalFEM};
 const int cap=std::clamp(evaluations,1,5);
 for(int i=0;i<cap;i++){
  const Family f=families[size_t(i)];
  double pick=.24;
  if(f==Family::PhysicalString||f==Family::NativePhysicalHybrid){
   guitar29::Take t{a.data(),int(std::min<size_t>(size_t(sr),a.size())),sr,r.observation.pitchHz,true};
   const auto estimated=guitar29::analyze(t);if(estimated.success&&std::isfinite(estimated.pickPosition))pick=std::clamp(estimated.pickPosition,.07,.48);
  }
  auto g=editableGraph(f,r.observation.pitchHz,pick);
  std::string why;auto synth=renderGraph(g,midi,sr,.20,&why);
  if(synth.empty()){r.warning+=std::string(name(f))+": "+why+"; ";continue;}
  const Features proposed=extract(synth,sr);
  // Compare harmonic magnitudes allowing gain, translation and relative phase.
  const auto synthH=harmonics(synth,sr,r.observation.pitchHz);
  double d=0;for(size_t k=0;k<targetH.size();k++)d+=squared(targetH[k]-synthH[k])/(1.+.11*k);
  const double roll=std::abs(std::log((proposed.highHarmonicRatio+.015)/(r.observation.highHarmonicRatio+.015)));
  const double pitchPenalty=(proposed.pitchConfidence>.27&&proposed.pitchHz>0)?std::min(2.,std::abs(std::log2(proposed.pitchHz/r.observation.pitchHz))):.8;
  const double temporal=.04*std::abs(std::log((proposed.rms+1e-6)/(r.observation.rms+1e-6)));
  Candidate c;c.family=f;c.distance=d+.10*roll+.1*pitchPenalty+.05*temporal;
  c.spectralDistance=d;c.temporalDistance=temporal;c.midi=midi;c.graph=std::move(g);
  r.ranked.push_back(std::move(c));
 }
 std::stable_sort(r.ranked.begin(),r.ranked.end(),[](const auto&x,const auto&y){return x.distance<y.distance;});
 if(r.ranked.empty()){r.error="No synthesis candidate could be prepared: "+r.warning;return r;}
 r.ambiguous=r.ranked.size()>1 && r.ranked[1].distance-r.ranked[0].distance<.12;
 r.warning+="Scores are uncalibrated; material, mesh, microphone, polyphony and technique cannot be uniquely inferred.";
 return r;
}
} // namespace panorama::design49
