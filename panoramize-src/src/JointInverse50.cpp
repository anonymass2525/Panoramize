#include "JointInverse50.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>
namespace panorama::joint50 {
namespace {
constexpr double pi=3.14159265358979323846;
struct Descriptor {std::array<double,10> harmonics{};std::array<double,5> envelope{};double rms=0;};
// Explicit MIDI calibration is required to avoid the known octave/pitch confound
// for synthetic/inharmonic or noisy timbres. The observations can be real WAVs.
Descriptor descriptor(const std::vector<float>&a,int sr,int midi){
 Descriptor r;if(a.size()<2048||sr<22050)return r;
 double p=0;for(float v:a)p+=double(v)*v;
 r.rms=std::sqrt(p/a.size());
 const double hz=440.*std::pow(2.,(midi-69)/12.);
 const int length=std::min<int>(4096,int(a.size()));
 // analyze at 3 temporal positions; independent of unknown global amplitude
 for(int block=0;block<3;block++){
  const size_t off=std::min<size_t>(a.size()-size_t(length),size_t(block)*(a.size()-length)/2);
  for(int k=1;k<=10;k++){
   double re=0,im=0;
   for(int n=0;n<length;n+=2){
    const double w=.5-.5*std::cos(2*pi*n/(length-1));
    const double phase=2*pi*hz*k*n/sr;
    const double x=a[off+size_t(n)]*w;
    re+=x*std::cos(phase);im+=x*std::sin(phase);
   }
   r.harmonics[size_t(k-1)]+=std::hypot(re,im);
  }
 }
 double norm=0;for(double v:r.harmonics)norm+=v*v;
 norm=std::sqrt(norm)+1e-15;for(double&v:r.harmonics)v/=norm;
 for(int i=0;i<5;i++){
  const size_t b=size_t(i)*a.size()/5,e=size_t(i+1)*a.size()/5;
  double energy=0;for(size_t j=b;j<e;j++)energy+=double(a[j])*a[j];
  r.envelope[size_t(i)]=std::sqrt(energy/std::max<size_t>(1,e-b));
 }
 const double mx=*std::max_element(r.envelope.begin(),r.envelope.end())+1e-12;
 for(double&v:r.envelope)v/=mx;
 return r;
}
double loss(const Descriptor&a,const Descriptor&b){
 double err=0;
 for(size_t i=0;i<a.harmonics.size();i++){
  const double d=a.harmonics[i]-b.harmonics[i];err+=d*d/(1+.2*i);
 }
 double e=0;for(size_t i=0;i<a.envelope.size();i++){
  double d=a.envelope[i]-b.envelope[i];e+=d*d;
 }
 return err+.15*e;
}
bool allGood(const Take&t,int sr){
 if(t.audio.size()<size_t(sr)/5||t.audio.size()>size_t(sr)*10||t.midi<36||t.midi>96)return false;
 for(float a:t.audio)if(!std::isfinite(a))return false;
 return true;
}
} // namespace
Result identify(const std::vector<Take>&takes,int sr,int budget){
 Result out;
 if(sr<22050||sr>96000||takes.size()<2||takes.size()>12||budget<5||budget>96){out.error="requires 2-12 MIDI-annotated takes and 5-96 evaluations";return out;}
 int train=0,hold=0;
 for(const auto&t:takes){if(!allGood(t,sr)){out.error="bad, silent or out-of-range take";return out;}train+=t.training;hold+=!t.training;}
 if(!train||!hold){out.error="training and held-out takes both required";return out;}
 std::vector<Descriptor> references;references.reserve(takes.size());
 for(const auto&t:takes){auto d=descriptor(t.audio,sr,t.midi);if(d.rms<1e-6){out.error="silent take cannot identify a structure";return out;}references.push_back(d);}
 // The candidate set includes the actual original Native27 graph, not a modal
 // proxy. The vocal state is a mode within that graph, with excitation still active.
 struct Trial{std::string name;universal23::Graph graph;std::vector<std::string>params;};
 const double f0=440.*std::pow(2.,(takes[0].midi-69)/12.);
 std::vector<Trial> trials={
  {"physical_string",design49::editableGraph(design49::Family::PhysicalString,f0,.24),{"guitar29.pickPosition","guitar29.decayBase","guitar29.spectralTilt"}},
  {"native_va_fm",design49::editableGraph(design49::Family::NativeVA_FM,f0),{"genuine47.tone.cutoff","genuine47.tone.drive","genuine47.tone.wet"}},
  {"native_state",design49::editableGraph(design49::Family::NativeState,f0),{"native27.mode.0.ratio","native27.mode.0.damping"}},
  {"hybrid",design49::editableGraph(design49::Family::NativePhysicalHybrid,f0,.24),{"guitar29.pickPosition","genuine47.tone.cutoff"}},
  {"reduced_fem",design49::editableGraph(design49::Family::MultidimensionalFEM,f0),{}},
 };
 // A separate vocal-rich Native27 direct-state graph for voice candidate search.
 auto vocal=universal23::directGraph();
 std::string vocalRatio="native27.mode.0.ratio",vocalDamp="native27.mode.0.damping";
 for(size_t i=0;i<vocal.instrument.modes.size();i++){
  auto &m=vocal.instrument.modes[i];
  if(m.engine==native27::Engine::Vocal){
   m.excitation=std::max(m.excitation,0.25);m.pickup=std::max(.1,m.pickup);
   vocalRatio="native27.mode."+std::to_string(i)+".ratio";
   vocalDamp="native27.mode."+std::to_string(i)+".damping";
  }
 }
 trials.push_back({"native_vocal_state",std::move(vocal),{vocalRatio,vocalDamp}});
 const auto evaluate=[&](const universal23::Graph&g,bool validation,double&v){
  v=0;int count=0;
  for(size_t i=0;i<takes.size();i++)if(takes[i].training!=validation){
   std::string why;auto audio=design49::renderGraph(g,takes[i].midi,sr,.24,&why);
   if(audio.empty())return false;
   const auto feature=descriptor(audio,sr,takes[i].midi);
   v+=loss(references[i],feature);++count;
  }
  v/=std::max(1,count);return std::isfinite(v);
 };
 const int quota=budget/int(trials.size());
 for(auto&trial:trials){
  double best=1e10;universal23::Graph found=trial.graph;int count=0;
  // Coordinate search with bounded deterministic modifications, using actual
  // original sound synthesis for scoring. No gradient through FEM and no ML.
  auto controls=controls50::catalog(trial.graph);
  std::vector<controls50::Parameter> tuning;
  for(const auto&id:trial.params)for(const auto&c:controls)if(c.id==id){tuning.push_back(c);break;}
  for(int i=0;i<std::max(1,quota);i++){
   universal23::Graph candidate=(i==0)?trial.graph:found;
   if(i>0&&!tuning.empty()){
    const auto&c=tuning[size_t((i-1)%int(tuning.size()))];
    const double base=c.read(candidate);
    const double ratio=(i%2)?0.8:1.25;
    const double proposed=std::clamp(base*ratio,c.lo,c.hi);
    std::string reason;controls50::change(candidate,c.id,proposed,&reason);
   }
   double err=0;if(evaluate(candidate,false,err)){
    ++count;if(err<best){best=err;found=std::move(candidate);}
   }
  }
  out.totalEvaluations+=count;
  if(count){double holdout=0;evaluate(found,true,holdout);
   out.ranked.push_back({trial.name,std::move(found),best,holdout,count});}
 }
 std::sort(out.ranked.begin(),out.ranked.end(),[](const auto&a,const auto&b){return a.trainError<b.trainError;});
 if(out.ranked.empty()){out.error="No candidate synthesized";return out;}
 out.uncertaintyGap=out.ranked.size()>1?out.ranked[1].trainError-out.ranked[0].trainError:0;
 // Neither shape nor material nor vocal tract can be uniquely recovered from these
 // descriptors; never label candidates identifiable, regardless of score gap.
 out.identifiable=false;
 out.warning="Non-unique inverse: no recovery of unknown 3D shape, material, microphone position, articulatory vocal tract or a unique synthesis algorithm. MIDI notes are annotated; outputs are editable hypotheses, not a reconstructed ground truth.";
 return out;
}
} // namespace panorama::joint50
