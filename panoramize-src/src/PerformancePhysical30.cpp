#include "PerformancePhysical30.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <numeric>
#include <sstream>
namespace panorama::performance30 {
namespace {
constexpr double pi=3.1415926535897932384626433832795;
double clamp(double x,double a,double b){return std::max(a,std::min(b,x));}
double median(std::vector<double> a){if(a.empty())return 0;auto it=a.begin()+a.size()/2;std::nth_element(a.begin(),it,a.end());return *it;}
struct Osc {double a=0.,b=0.,c=0.,d=0.,scale=0.;
 void configure(double hz,double decay,double sr){const double r=std::exp(-decay/sr),w=2*pi*hz/sr;
  c=2*r*std::cos(w);d=-r*r;scale=std::max(.00001,1-r);}
 double tick(double in){double v=c*a+d*b+scale*in;b=a;a=v;return v;}
};
// A bounded second order linear filter bank, one resonance per effective axis.
struct Stage {std::array<Osc,3> axes{};double mix=0.;
 void configure(double fundamental,double decay,double sr,double amount){
  static constexpr double ratios[3]={1.,.79,1.33};mix=amount;
  for(int i=0;i<3;i++)axes[size_t(i)].configure(fundamental*ratios[i],decay,sr);
 }
 double tick(double input,const CoupledModel& m){double y=0,weight=0;
  for(int k=0;k<3;k++){const double projection=m.excitation[size_t(k)]*m.pickup[size_t(k)];
   y+=projection*axes[size_t(k)].tick(input*m.excitation[size_t(k)]);
   weight+=std::abs(projection*m.excitation[size_t(k)]);
  }return (1-mix)*input + mix*y/std::max(.02,weight);
 }
};
struct Frequency {double hz=0.,confidence=0.;};
Frequency pitch(const float* x,int start,int n,int sr){
 // Mono, downsampled normalized autocorrelation; octave ambiguity remains.
 const int decim=std::max(1,int(std::lround(sr/4000.)));
 const int count=n/decim;
 if(count<110)return {};
 std::vector<double> v(size_t(count),0.);
 double mean=0.;for(int i=0;i<count;i++){v[size_t(i)]=x[start+i*decim];mean+=v[size_t(i)];}mean/=count;
 double total=0.;for(double& a:v){a-=mean;total+=a*a;}
 if(total/count<1.e-9)return {};
 const double fs=double(sr)/decim;
 const int lo=std::max(4,int(fs/950)),hi=std::min(count/4,int(fs/75));
 if(hi<=lo+2)return {};
 std::vector<double> scores(size_t(hi+1),0.);
 double best=0.;for(int lag=lo;lag<=hi;lag++){
  double ab=0.,aa=0.,bb=0.;for(int j=0;j+lag<count;j++){
   const double a=v[size_t(j)],b=v[size_t(j+lag)];ab+=a*b;aa+=a*a;bb+=b*b;
  }scores[size_t(lag)]=ab/std::sqrt(aa*bb+1.e-24);best=std::max(best,scores[size_t(lag)]);
 }
 if(best<.48)return {};
 // Choose the shortest confidently periodic lag to reduce octave-down errors.
 int choice=-1;
 for(int lag=lo+1;lag<hi;lag++)if(scores[size_t(lag)]>.89*best &&
    scores[size_t(lag)]>=scores[size_t(lag-1)] && scores[size_t(lag)]>=scores[size_t(lag+1)]){choice=lag;break;}
 if(choice<0)choice=int(std::max_element(scores.begin()+lo,scores.end())-scores.begin());
 double offset=0.;if(choice>lo&&choice<hi){const double a=scores[size_t(choice-1)],b=scores[size_t(choice)],c=scores[size_t(choice+1)];
  if(std::abs(a-2*b+c)>1.e-8)offset=clamp(.5*(a-c)/(a-2*b+c),-.5,.5);}
 return {fs/(choice+offset),clamp(scores[size_t(choice)],0.,1.)};
}
// Extracts a compact temporal harmonic magnitude profile, not phase-locked PCM.
std::vector<double> features(const float* p,int frames,int sr,double knownF0){
 std::vector<double> out;
 const double offsets[]={.12,.30,.65,1.05};
 const int length=std::min(int(sr*.09),std::max(512,int(sr*.075)));
 for(double t:offsets){
  const int start=int(t*sr);
  if(start+length>=frames)break;
  double f=knownF0;
  if(f<=75){auto est=pitch(p,start,length,sr);f=est.hz;}
  if(f<73||f>650)continue;
  std::vector<double> mags;
  for(int k=1;k<=10;k++){
   const double freq=f*k;
   if(freq>sr*.42)break;
   double best=0.;for(int j=-1;j<=1;j++){
    const double w=2*pi*(freq+j*1.5)/sr;
    double re=0.,im=0.;for(int n=0;n<length;n++){
     const double a=p[size_t(start+n)]*(.5-.5*std::cos(2*pi*n/(length-1)));
     re+=a*std::cos(w*n);im+=a*std::sin(w*n);
    }best=std::max(best,2*std::hypot(re,im)/length);
   }mags.push_back(std::log(best+1.e-5));
  }
  if(mags.size()<4)continue;
  const double base=mags.front();for(double a:mags)out.push_back(a-base);
 }
 return out;
}
double distance(const std::vector<double>& a,const std::vector<double>& b){
 if(a.size()<10||b.size()<10)return 1.e4;
 const auto n=std::min(a.size(),b.size());double err=0.;
 for(size_t i=0;i<n;i++){const double delta=clamp(a[i]-b[i],-3.,3.);err+=delta*delta;}
 return err/n+double(std::max(a.size(),b.size())-n)*.05;
}
std::string topology(const CoupledModel& m){return m.bridge&&m.body?"String+Bridge+Body":
 m.bridge?"String+Bridge":m.body?"String+Body":"StringOnly";}
}
const char* eventName(EventKind e) noexcept {switch(e){case EventKind::Pluck:return "pluck_candidate";
 case EventKind::PitchTransition:return "pitch_transition_candidate";
 case EventKind::DampingChange:return "damping_or_mute_candidate";
 case EventKind::VibratoCandidate:return "vibrato_candidate";
 case EventKind::LegatoCandidate:return "legato_or_slide_candidate";}return "unknown";}
Performance analyze(const float* a,int frames,int sr){
 Performance result;
 if(!a||sr<8000||sr>96000||frames<sr/3||frames>sr*30)return result;
 for(int i=0;i<frames;i++)if(!std::isfinite(a[i]))return result;
 const int hop=std::max(80,sr/100),win=std::max(hop*3,sr*9/100);
 if(frames<win)return result;
 double peak=0.;
 for(int pos=0;pos+win<=frames;pos+=hop){
  double energy=0.;for(int j=0;j<win;j++){const double v=a[pos+j];energy+=v*v;}
  const double rms=std::sqrt(energy/win);
  const auto f=pitch(a,pos,win,sr);
  result.frames.push_back({(pos+win*.5)/double(sr),f.hz,f.confidence,rms,0.});
  peak=std::max(peak,rms);
 }
 if(peak<.00005)return result;
 std::vector<double> pitched;
 for(const auto& f:result.frames)if(f.pitchConfidence>.58&&f.rms>.025*peak)pitched.push_back(f.pitchHz);
 result.medianPitchHz=median(std::move(pitched));
 if(result.medianPitchHz<60)return result;
 result.valid=true;
 const double floor=std::max(.00015,peak*.035);
 // An independent 4 ms energy tracker locates the first attack without
 // smearing it by the 90 ms F0 window. The event remains a candidate.
 const int shortHop=std::max(24,sr/250);
 std::vector<double> shortEnergy;
 for(int pos=0;pos+shortHop<=frames;pos+=shortHop){
  double energy=0.;for(int j=pos;j<pos+shortHop;j++)energy+=double(a[j])*a[j];
  shortEnergy.push_back(std::sqrt(energy/shortHop));
 }
 double shortPeak=0.;for(double v:shortEnergy)shortPeak=std::max(shortPeak,v);
 int lastPluckBlock=-1000;
 const double onsetThreshold=std::max(.0002,shortPeak*.045);
 for(size_t b=1;b<shortEnergy.size();b++){
  const double now=shortEnergy[b],prev=shortEnergy[b-1];
  const bool fresh=(now>onsetThreshold&&prev<=onsetThreshold&&now>shortPeak*.10);
  const bool reattack=(now>shortPeak*.12 && prev>onsetThreshold &&
     now/(prev+onsetThreshold) >1.55);
  if((fresh||reattack) && int(b)-lastPluckBlock>int(.085*sr/shortHop)){
   const double when=b*shortHop/double(sr);
   const auto index=size_t(std::min(int(result.frames.size()-1),std::max(0,int(when*100))));
   result.events.push_back({EventKind::Pluck,when,result.frames[index].pitchHz,
     fresh?.75:.4});lastPluckBlock=int(b);
  }
 }
 int lastPitch=-1000,lastMute=-1000;
 for(size_t i=1;i<result.frames.size();i++){
  auto& f=result.frames[i];const auto& p=result.frames[i-1];
  const double delta=std::log((f.rms+floor)/(p.rms+floor));f.flux=std::max(0.,delta);
  if(i>=6 && f.pitchHz>0 && result.frames[i-5].pitchHz>0 &&
      f.pitchConfidence>.60 && result.frames[i-5].pitchConfidence>.60){
   const double semitones=12*std::log2(f.pitchHz/result.frames[i-5].pitchHz);
   if(std::abs(semitones)>.65 && int(i)-lastPitch>8 && f.flux<.26 && f.rms>floor*2){
    result.events.push_back({EventKind::PitchTransition,f.seconds,f.pitchHz,
      clamp(std::abs(semitones)/4.,.2,1.)});lastPitch=int(i);
    bool rearticulated=false;
    for(const auto& earlier:result.events)if(earlier.kind==EventKind::Pluck &&
      std::abs(earlier.seconds-f.seconds)<.085)rearticulated=true;
    if(!rearticulated && std::abs(semitones)>.8)
     result.events.push_back({EventKind::LegatoCandidate,f.seconds,f.pitchHz,.25});
   }
  }
  if(i>=5&&f.rms<peak*.8&&p.rms>floor*3&&
     std::log((p.rms+floor)/(f.rms+floor))>.27 && int(i)-lastMute>12){
    result.events.push_back({EventKind::DampingChange,f.seconds,f.pitchHz,.4});lastMute=int(i);
  }
 }
 // Short-time vibrato cue: alternating pitch direction over 120..500 ms.
 // Requires a coherent ±0.1..2 semitone continuous variation.
 for(size_t i=18;i+2<result.frames.size();i+=12){
  int turning=0,lastDir=0,valid=0;double mini=1.e30,maxi=0.,reference=0.;
  const size_t begin=i-17;for(size_t j=begin+1;j<=i;j++){
   const auto& left=result.frames[j-1],&right=result.frames[j];
   if(left.pitchConfidence<.68||right.pitchConfidence<.68||left.pitchHz<=0||right.pitchHz<=0)continue;
   reference+=right.pitchHz;valid++;mini=std::min(mini,right.pitchHz);maxi=std::max(maxi,right.pitchHz);
   const double st=12*std::log2(right.pitchHz/left.pitchHz);
   const int dir=st>.03?1:(st<-.03?-1:0);
   if(dir&&lastDir&&dir!=lastDir)turning++;
   if(dir)lastDir=dir;
  }
  if(valid>=12&&turning>=2&&mini>0&&12*std::log2(maxi/mini)>.16&&
     12*std::log2(maxi/mini)<3.){
   result.events.push_back({EventKind::VibratoCandidate,result.frames[i].seconds,reference/valid,.35});
  }
 }
 std::sort(result.events.begin(),result.events.end(),[](auto&a,auto&b){return a.seconds<b.seconds;});
 return result;
}
std::vector<float> render(const CoupledModel& m,int sr,double seconds,double onset,double pitchFactor){
 if(sr<8000||sr>96000||seconds<=0||seconds>30||!std::isfinite(seconds)||
   !std::isfinite(pitchFactor)||pitchFactor<=0)return {};
 auto x=guitar29::render(m.string,sr,seconds,onset,pitchFactor);
 Stage bridge,body;
 bridge.configure(m.bridgeHz,m.bridgeDecay,sr,m.bridgeMix);
 body.configure(m.bodyHz,m.bodyDecay,sr,m.bodyMix);
 for(float& v:x){double y=v;
  if(m.bridge)y=bridge.tick(y,m);
  if(m.body)y=body.tick(y,m);
  v=float(clamp(y,-2.,2.));
 }return x;
}
Discovery discover(const std::vector<Take>& recordings,const guitar29::Model& prior,
 unsigned maxCandidates,unsigned maxProposals){
 Discovery out;
 if(recordings.empty()||recordings.size()>12||maxCandidates==0||maxCandidates>500||maxProposals==0||maxProposals>4)return out;
 unsigned training=0;
 std::vector<std::vector<double>> observations; observations.reserve(recordings.size());
 for(const auto&t:recordings){
  auto perf=analyze(t.audio,t.frames,t.sampleRate);
  if(!perf.valid)return out;
  double pitchHz=t.knownPitchHz>70?t.knownPitchHz:perf.medianPitchHz;
  auto f=features(t.audio,t.frames,t.sampleRate,pitchHz);
  if(f.size()<10)return out;
  observations.push_back(std::move(f));out.performances.push_back(std::move(perf));
  training+=t.training?1:0;
 }
 if(!training)return out;
 std::vector<Proposal> tested;
 const double bodyF[]={230.,270.,310.,380.,490.};
 const double bridgeF[]={350.,480.,620.};
 static constexpr double exProfiles[3][3]={{.85,.35,.10},{.24,.83,.34},{.10,.26,.88}};
 static constexpr double piProfiles[3][3]={{.70,.25,.35},{.28,.76,.35},{.32,.22,.81}};
 for(int type=0;type<4;type++){
  const int nbody=type>=2?5:1;
  const int nbridge=(type==1||type==3)?3:1;
  for(int bi=0;bi<nbody;bi++)for(int br=0;br<nbridge;br++)for(int axis=0;axis<(type==0?1:3);axis++){
   if(tested.size()>=maxCandidates)break;
   CoupledModel m; m.string=prior;
   m.bridge=type==1||type==3;m.body=type>=2;
   m.bodyHz=bodyF[bi];m.bridgeHz=bridgeF[br];
   for(int a=0;a<3;a++){m.excitation[size_t(a)]=exProfiles[axis][a];m.pickup[size_t(a)]=piProfiles[axis][a];}
   double trainLoss=0,holdLoss=0;unsigned ht=0;
   for(size_t j=0;j<recordings.size();j++){
    const auto&t=recordings[j];const double pitchHz=t.knownPitchHz>70?t.knownPitchHz:out.performances[j].medianPitchHz;
    const double factor=pitchHz/prior.frequencyHz;
    double onset=.035;
    for(const auto&event:out.performances[j].events)if(event.kind==EventKind::Pluck){onset=event.seconds;break;}
    auto synth=render(m,t.sampleRate,double(t.frames)/t.sampleRate,onset,factor);
    const auto generated=features(synth.data(),int(synth.size()),t.sampleRate,pitchHz);
    const double e=distance(observations[j],generated);
    if(t.training)trainLoss+=e;else{holdLoss+=e;ht++;}
   }
   tested.push_back({m,trainLoss/training,ht?holdLoss/ht:0.,topology(m)});
  }
 }
 if(tested.empty())return out;
 std::sort(tested.begin(),tested.end(),[](const auto&a,const auto&b){return a.trainingLoss<b.trainingLoss;});
 for(const auto&p:tested){
  bool used=false;for(const auto&chosen:out.proposals)if(chosen.topology==p.topology)used=true;
  if(!used){out.proposals.push_back(p);if(out.proposals.size()==maxProposals)break;}
 }
 out.success=!out.proposals.empty();return out;
}
std::unique_ptr<Instrument> Instrument::prepare(const CoupledModel&m,double sr,std::string*error){
 CoupledModel verified;
 if(sr<8000||sr>192000||!std::isfinite(sr)||!deserialize(serialize(m),verified)){
  if(error)*error="invalid physical30 model";
  return {};
 }
 auto ptr=std::unique_ptr<Instrument>(new Instrument);
 ptr->string_=guitar29::Instrument::prepare(m.string,sr,error);
 if(!ptr->string_)return {};
 ptr->model_=m;
 static constexpr double ratios[3]={1.,.79,1.33};
 for(int i=0;i<3;i++){
  auto prep=[&](Bank& bank,double hz,double decay,double mix){
   const double r=std::exp(-decay/sr),w=2*pi*hz*ratios[i]/sr;
   auto& f=bank.filters[size_t(i)];f.c=2*r*std::cos(w);f.d=-r*r;f.scale=std::max(.00001,1-r);
   bank.amount=mix;
  };
  prep(ptr->bridge_,m.bridgeHz,m.bridgeDecay,m.bridgeMix);
  prep(ptr->body_,m.bodyHz,m.bodyDecay,m.bodyMix);
 }
 if(error)error->clear();
 return ptr;
}
void Instrument::noteOn(int note,float vel) noexcept{string_->noteOn(note,vel);}
void Instrument::noteOff(int note) noexcept{string_->noteOff(note);}
void Instrument::pitchBend(float semis) noexcept{string_->pitchBend(semis);}
void Instrument::sustain(bool down) noexcept{string_->sustain(down);}
void Instrument::allNotesOff() noexcept{string_->allNotesOff();}
double Instrument::tick(Bank& bank,double input) noexcept{
 double y=0.,weight=0.;
 for(int i=0;i<3;i++){
  const double projection=model_.excitation[size_t(i)]*model_.pickup[size_t(i)];
  y+=projection*bank.filters[size_t(i)].tick(input*model_.excitation[size_t(i)]);
  weight+=std::abs(projection*model_.excitation[size_t(i)]);
 }
 return (1-bank.amount)*input + bank.amount*y/std::max(.02,weight);
}
float Instrument::processOne() noexcept{
 double y=string_->processOne();
 if(model_.bridge)y=tick(bridge_,y);
 if(model_.body)y=tick(body_,y);
 return float(clamp(y,-2.,2.));
}
std::string serialize(const CoupledModel&m){
 std::ostringstream s;s<<std::setprecision(17)<<"PANORAMIZE-PHYSICAL30-v1\n";
 s<<"bridge "<<int(m.bridge)<<' '<<m.bridgeHz<<' '<<m.bridgeDecay<<' '<<m.bridgeMix<<'\n';
 s<<"body "<<int(m.body)<<' '<<m.bodyHz<<' '<<m.bodyDecay<<' '<<m.bodyMix<<'\n';
 s<<"excitation "<<m.excitation[0]<<' '<<m.excitation[1]<<' '<<m.excitation[2]<<'\n';
 s<<"pickup "<<m.pickup[0]<<' '<<m.pickup[1]<<' '<<m.pickup[2]<<'\n';
 s<<"STRING29_BEGIN\n"<<guitar29::serialize(m.string)<<"STRING29_END\n";return s.str();
}
bool deserialize(const std::string&src,CoupledModel&out){
 if(src.size()>16384)return false;
 std::istringstream s(src);std::string line,tag;CoupledModel x;
 if(!std::getline(s,line)||line!="PANORAMIZE-PHYSICAL30-v1")return false;
 auto values=[&](const char*name,int n,double*data)->bool{
  if(!std::getline(s,line))return false;
  std::istringstream ls(line);
  if(!(ls>>tag)||tag!=name)return false;
  for(int i=0;i<n;i++)if(!(ls>>data[i])||!std::isfinite(data[i]))return false;
  std::string extra;return !(ls>>extra);
 };
 double a[4],b[4],c[3],d[3];
 if(!values("bridge",4,a)||!values("body",4,b)||!values("excitation",3,c)||!values("pickup",3,d))return false;
 if((a[0]!=0&&a[0]!=1)||(b[0]!=0&&b[0]!=1))return false;
 x.bridge=bool(a[0]);x.body=bool(b[0]);x.bridgeHz=a[1];x.bridgeDecay=a[2];x.bridgeMix=a[3];
 x.bodyHz=b[1];x.bodyDecay=b[2];x.bodyMix=b[3];
 for(int i=0;i<3;i++){x.excitation[size_t(i)]=c[i];x.pickup[size_t(i)]=d[i];}
 if(!std::getline(s,line)||line!="STRING29_BEGIN")return false;
 std::string stringText;while(std::getline(s,line)&&line!="STRING29_END")stringText+=line+'\n';
 if(line!="STRING29_END"||!guitar29::deserialize(stringText,x.string))return false;
 while(std::getline(s,line))if(!line.empty())return false;
 if(x.bridgeHz<70||x.bridgeHz>6000||x.bodyHz<70||x.bodyHz>6000||
    x.bridgeDecay<1||x.bridgeDecay>1000||x.bodyDecay<1||x.bodyDecay>1000||
    x.bridgeMix<0||x.bridgeMix>1||x.bodyMix<0||x.bodyMix>1)return false;
 for(int i=0;i<3;i++)if(std::abs(c[i])>1||std::abs(d[i])>1)return false;
 out=x;return true;
}
} // namespace panorama::performance30
