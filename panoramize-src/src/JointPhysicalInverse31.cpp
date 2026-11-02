#include "JointPhysicalInverse31.h"
#include "GuitarInverse29.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <limits>
#include <numeric>
#include <sstream>
namespace panorama::joint31 {
namespace {
constexpr double pi=3.14159265358979323846;
double clip(double v,double a,double b){return std::max(a,std::min(b,v));}
struct Signature {std::vector<double> values; bool ok=false;};
// Goertzel: temporal multiband representation that includes relative harmonic
// envelope AND time-dependent overall level. Results not PCM phase-locked.
Signature describe(const float* x,int n,int sr,double pitchHz,double onset){
 Signature z;
 if(!x||n<sr/2||sr<8000||sr>48000||pitchHz<72||pitchHz>700)return z;
 constexpr double times[4]={.085,.245,.49,.88};
 const int length=std::max(256,int(.068*sr));
 for(const auto t:times){
  const int start=int((onset+t)*sr);
  if(start<0||start+length>n)continue;
  double power=0.;for(int i=0;i<length;i++){const double v=x[start+i];power+=v*v;}
  const double level=std::sqrt(power/length);
  std::array<double,9> mag{};
  std::vector<double> weighted(size_t(length),0.);
  for(int j=0;j<length;j++)weighted[size_t(j)]=
   double(x[start+j])*(.5-.5*std::cos(2*pi*j/double(length-1)));
  const int usable=std::min(8,int(.44*sr/pitchHz));
  if(usable<5)return {};
  for(int k=1;k<=usable;k++){
   const double freq=pitchHz*k;const double omega=2*pi*freq/sr;
   const double c=2*std::cos(omega);double prev=0.,prev2=0.;
   for(int j=0;j<length;j++){
    const double cur=weighted[size_t(j)]+c*prev-prev2;
    prev2=prev;prev=cur;
   }
   const double sq=std::max(0.,prev*prev+prev2*prev2-c*prev*prev2);
   mag[size_t(k)]=std::sqrt(sq)/length;
  }
  z.values.push_back(std::log(level+1.e-5));
  const double base=mag[1];
  for(int k=2;k<=usable;k++)z.values.push_back(clip(std::log((mag[size_t(k)]+1.e-5)/(base+1.e-5)),-9.,9.));
  for(int k=usable+1;k<=8;k++)z.values.push_back(-9.);
 }
 z.ok=z.values.size()>=18;
 return z;
}
double distance(const Signature&a,const Signature&b){
 if(!a.ok||!b.ok||a.values.size()!=b.values.size())return 1.e3;
 double loss=0.;for(size_t i=0;i<a.values.size();i++){
  const double d=clip(a.values[i]-b.values[i],-5.,5.);
  const double weight=i%8==0?.75:1.;
  loss+=weight*d*d;
 }
 return loss/a.values.size();
}
double detectOnset(const Take&t){
 const int block=std::max(16,t.sampleRate/500);const int blocks=t.frames/block;
 if(blocks<20)return .035;
 double peak=0.;std::vector<double> levels(size_t(blocks),0.);
 for(int b=0;b<blocks;b++){double s=0.;for(int k=0;k<block;k++)s+=double(t.audio[b*block+k])*t.audio[b*block+k];
  levels[size_t(b)]=std::sqrt(s/block);peak=std::max(peak,levels[size_t(b)]);}
 for(int b=0;b<blocks;b++)if(levels[size_t(b)]>std::max(peak*.075,2.e-5))
  return std::max(0.,double((b-1)*block)/t.sampleRate);
 return .035;
}
std::string name(const performance30::CoupledModel&m){return m.bridge&&m.body?"String+Bridge+Body":m.bridge?"String+Bridge":m.body?"String+Body":"StringOnly";}
struct Context {
 std::vector<Take> takes;
 std::vector<Signature> targets;
 std::vector<PerformanceParams> initials;
 performance30::CoupledModel prior;
 unsigned budget=0, evaluations=0;
 double one(const performance30::CoupledModel&st,const PerformanceParams&p,size_t index,bool budgeted=true){
  if(budgeted){if(evaluations>=budget)return 1.e3;++evaluations;}
  auto m=st;m.string.frequencyHz=prior.string.frequencyHz;
  m.string.pickPosition=p.pickPosition;m.string.decayBase=p.decayBase;m.string.gain=p.gain;
  const Take&t=takes[index];
  const auto wav=performance30::render(m,t.sampleRate,double(t.frames)/t.sampleRate,p.onsetSeconds,
                                     p.pitchHz/prior.string.frequencyHz);
  if(wav.size()!=size_t(t.frames))return 1.e3;
  return distance(targets[index],describe(wav.data(),int(wav.size()),t.sampleRate,p.pitchHz,p.onsetSeconds));
 }
 // Every structure candidate consumes the same train-only scoring objective.
 double train(const performance30::CoupledModel&st,const std::vector<PerformanceParams>&p){
  double s=0.;int count=0;
  for(size_t j=0;j<takes.size();j++)if(takes[j].training){
   if(evaluations>=budget)return 1.e3;
   s+=one(st,p[j],j);count++;
  }
  return count?s/count:1.e3;
 }
};
void updatePerTake(Context&c,Candidate&best){
 for(size_t i=0;i<c.takes.size();i++)if(c.takes[i].training){
  double score=std::numeric_limits<double>::infinity();
  if(c.evaluations<c.budget)score=c.one(best.structure,best.performances[i],i);
  auto test=[&](PerformanceParams candidate){
   if(c.evaluations>=c.budget)return;
   const auto value=c.one(best.structure,candidate,i);
   if(value+1.e-12<score){score=value;best.performances[i]=candidate;}
  };
  for(double dp:{-.065,-.025,.025,.065}){
   auto pp=best.performances[i];pp.pickPosition=clip(pp.pickPosition+dp,.08,.48);test(pp);
  }
  for(double ratio:{.66,.85,1.18,1.45}){
   auto pp=best.performances[i];pp.decayBase=clip(pp.decayBase*ratio,.25,30.);test(pp);
  }
  for(double gain:{.75,1.32}){
   auto pp=best.performances[i];pp.gain=clip(pp.gain*gain,.02,1.5);test(pp);
  }
 }
}
void updateStructure(Context&c,Candidate&b){
 if(c.evaluations>=c.budget)return;
 double score=c.train(b.structure,b.performances);
 auto probe=[&](performance30::CoupledModel altered){
  if(c.evaluations>=c.budget)return;
  const double v=c.train(altered,b.performances);
  if(v+1.e-12<score){score=v;b.structure=altered;}
 };
 if(b.structure.bridge){for(double offset:{-75.,-35.,35.,75.}){
  auto m=b.structure;m.bridgeHz=clip(m.bridgeHz+offset,100.,1200.);probe(m);}
  for(double offset:{-.12,.12}){auto m=b.structure;m.bridgeMix=clip(m.bridgeMix+offset,.05,.8);probe(m);}}
 if(b.structure.body){for(double offset:{-55.,-25.,25.,55.}){
  auto m=b.structure;m.bodyHz=clip(m.bodyHz+offset,100.,1200.);probe(m);}
  for(double offset:{-.12,.12}){auto m=b.structure;m.bodyMix=clip(m.bodyMix+offset,.05,.8);probe(m);}}
 for(double ratio:{.6,1.5}){auto m=b.structure;m.string.inharmonicity=clip(m.string.inharmonicity*ratio,0.,.002);probe(m);}
 for(double delta:{-.025,.025}){auto m=b.structure;m.string.decayHigh=clip(m.string.decayHigh+delta,0.,.18);probe(m);}
}
}
performance30::CoupledModel forTake(const Candidate&c,size_t j){
 auto model=c.structure;
 if(j<c.performances.size()){
  model.string.pickPosition=c.performances[j].pickPosition;
  model.string.decayBase=c.performances[j].decayBase;
  model.string.gain=c.performances[j].gain;
 }
 return model;
}
Result infer(const std::vector<Take>&takes,const performance30::CoupledModel&prior,
 unsigned maxEval,unsigned alternatives){
 Result result;
 performance30::CoupledModel checked;
 if(takes.size()<3||takes.size()>12||maxEval<50||maxEval>5000||alternatives<1||alternatives>4||
    prior.string.frequencyHz<75||prior.string.frequencyHz>600||
    !performance30::deserialize(performance30::serialize(prior),checked))return result;
 Context c;c.takes=takes;c.prior=prior;c.budget=maxEval;
 c.targets.reserve(takes.size());c.initials.reserve(takes.size());
 for(const auto&t:takes){
  if(!t.audio||t.sampleRate<8000||t.sampleRate>48000||
     t.frames<int(.95*t.sampleRate)||t.frames>int(12.*t.sampleRate))return result;
  for(int i=0;i<t.frames;i++)if(!std::isfinite(t.audio[i]))return result;
  performance30::Performance perf;
  double hz=t.knownPitchHz;
  if(hz<75){perf=performance30::analyze(t.audio,t.frames,t.sampleRate);hz=perf.medianPitchHz;}
  if(hz<75||hz>700)return result;
  const double onset=detectOnset(t);
  auto profile=describe(t.audio,t.frames,t.sampleRate,hz,onset);
  if(!profile.ok)return result;
  c.targets.push_back(std::move(profile));
  guitar29::Take single{t.audio,t.frames,t.sampleRate,hz,t.training};
  const auto measurement=guitar29::analyze(single,prior.string.pickupPosition);
  PerformanceParams params;params.pitchHz=hz;params.onsetSeconds=onset;
  if(measurement.success){
   params.pickPosition=clip(measurement.pickPosition,.08,.48);
   params.decayBase=clip(measurement.estimatedDecayBase,.3,25.);
  }else{params.pickPosition=prior.string.pickPosition;params.decayBase=prior.string.decayBase;}
  params.gain=prior.string.gain;
  c.initials.push_back(params);
  if(t.training)result.trainingTakes++;else result.holdoutTakes++;
 }
 if(result.trainingTakes<2)return result;
 std::vector<Candidate> seeds;
 const double bridgeFreqs[]={360.,480.,620.};
 const double bodyFreqs[]={230.,270.,380.};
 for(int type=0;type<4;type++){
  const int nbridge=(type==1||type==3)?3:1;
  const int nbody=type>=2?3:1;
  for(int br=0;br<nbridge;br++)for(int bo=0;bo<nbody;bo++){
   performance30::CoupledModel m=prior;m.bridge=(type==1||type==3);m.body=(type>=2);
   if(m.bridge)m.bridgeHz=bridgeFreqs[br];
   if(m.body)m.bodyHz=bodyFreqs[bo];
   Candidate test;test.structure=m;test.performances=c.initials;test.topology=name(m);
   if(c.evaluations+result.trainingTakes>c.budget)break;
   test.initialTrainingLoss=c.train(test.structure,test.performances);
   test.trainingLoss=test.initialTrainingLoss;
   seeds.push_back(std::move(test));
  }
 }
 std::vector<Candidate> selected;
 for(int type=0;type<4;type++){
  const std::string label=type==0?"StringOnly":type==1?"String+Bridge":type==2?"String+Body":"String+Bridge+Body";
  const Candidate*best=nullptr;
  for(const auto&seed:seeds)if(seed.topology==label && (!best||seed.trainingLoss<best->trainingLoss))best=&seed;
  if(best)selected.push_back(*best);
 }
 for(auto&candidate:selected){
  for(int pass=0;pass<2&&c.evaluations<c.budget;pass++){
   updatePerTake(c,candidate);
   updateStructure(c,candidate);
  }
  // A lower training objective after block-coordinate descent is not necessarily
  // a recovered physical truth; it is only a better fit in this model family.
  if(c.evaluations+result.trainingTakes<=c.budget)
   candidate.trainingLoss=c.train(candidate.structure,candidate.performances);
  else candidate.trainingLoss=1.e3;
  double error=0.;unsigned holdouts=0;
  for(size_t j=0;j<takes.size();j++)if(!takes[j].training){
   error+=c.one(candidate.structure,candidate.performances[j],j,false);holdouts++;
  }
  candidate.holdoutLoss=holdouts?error/holdouts:0.;
 }
 std::sort(selected.begin(),selected.end(),[](const auto&a,const auto&b){return a.trainingLoss<b.trainingLoss;});
 for(const auto&candidate:selected){
  if(!std::isfinite(candidate.trainingLoss)||candidate.trainingLoss>=1.e3)continue;
  result.proposals.push_back(candidate);
  if(result.proposals.size()>=alternatives)break;
 }
 result.evaluatedModels=c.evaluations;
 result.success=!result.proposals.empty();return result;
}
std::string serialize(const Candidate&c){
 std::ostringstream out;out<<std::setprecision(17)<<"PANORAMIZE-JOINT31-v1\n";
 out<<"topology "<<c.topology<<"\ninitial_train_loss "<<c.initialTrainingLoss<<"\ntrain_loss "<<c.trainingLoss<<"\nholdout_loss "<<c.holdoutLoss<<"\n";
 for(size_t i=0;i<c.performances.size();i++){
  const auto&p=c.performances[i];out<<"performance "<<i<<' '<<p.pitchHz<<' '<<p.onsetSeconds<<' '
   <<p.pickPosition<<' '<<p.decayBase<<' '<<p.gain<<'\n';
 }
 out<<"STRUCTURE_BEGIN\n"<<performance30::serialize(c.structure)<<"STRUCTURE_END\n";
 return out.str();
}
bool deserialize(const std::string&text,Candidate&out){
 if(text.size()>32768)return false;
 std::istringstream in(text);std::string line;
 if(!std::getline(in,line)||line!="PANORAMIZE-JOINT31-v1")return false;
 Candidate c;
 if(!std::getline(in,line)||line.rfind("topology ",0)!=0)return false;
 c.topology=line.substr(9);
 auto numeric=[&](const char*label,double&v){
  if(!std::getline(in,line))return false;
  std::istringstream stream(line);std::string tag,extra;
  return bool(stream>>tag>>v)&&tag==label&&std::isfinite(v)&&!(stream>>extra);
 };
 if(!numeric("initial_train_loss",c.initialTrainingLoss)||!numeric("train_loss",c.trainingLoss)||
    !numeric("holdout_loss",c.holdoutLoss)||c.initialTrainingLoss<0||
    c.trainingLoss<0||c.holdoutLoss<0)return false;
 while(std::getline(in,line)&&line!="STRUCTURE_BEGIN"){
  std::istringstream l(line);std::string tag,extra;size_t index=0;
  PerformanceParams p;
  if(!(l>>tag>>index>>p.pitchHz>>p.onsetSeconds>>p.pickPosition>>p.decayBase>>p.gain)||
     tag!="performance"||index!=c.performances.size()||index>=12||l>>extra)return false;
  if(!std::isfinite(p.pitchHz)||!std::isfinite(p.onsetSeconds)||
     !std::isfinite(p.pickPosition)||!std::isfinite(p.decayBase)||!std::isfinite(p.gain)||
     p.pitchHz<75||p.pitchHz>700||p.onsetSeconds<0||p.onsetSeconds>10||
     p.pickPosition<.08||p.pickPosition>.48||p.decayBase<.25||p.decayBase>30||
     p.gain<.02||p.gain>1.5)return false;
  c.performances.push_back(p);
 }
 if(line!="STRUCTURE_BEGIN"||c.performances.empty())return false;
 std::string physical;bool found=false;
 while(std::getline(in,line)){
  if(line=="STRUCTURE_END"){found=true;break;}
  physical+=line+'\n';
 }
 if(!found||!performance30::deserialize(physical,c.structure)||c.topology!=name(c.structure))return false;
 while(std::getline(in,line))if(!line.empty())return false;
 out=std::move(c);return true;
}
} // namespace panorama::joint31
