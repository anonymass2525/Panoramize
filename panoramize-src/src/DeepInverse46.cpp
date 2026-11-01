#include "DeepInverse46.h"
#include <algorithm>
#include <cmath>
#include <complex>
#include <limits>
#include <numeric>
#include <utility>
namespace panorama::deep46 {
namespace {
constexpr double pi=3.14159265358979323846;
bool finiteAudio(const std::vector<float>&a){for(auto v:a)if(!std::isfinite(v))return false;return true;}
void fft(std::vector<std::complex<double>>&a){
 const size_t n=a.size();
 for(size_t i=1,j=0;i<n;i++){
  size_t bit=n>>1;while(j&bit){j^=bit;bit>>=1;}j^=bit;if(i<j)std::swap(a[i],a[j]);
 }
 for(size_t len=2;len<=n;len<<=1){
  const std::complex<double>wlen(std::cos(-2*pi/len),std::sin(-2*pi/len));
  for(size_t i=0;i<n;i+=len){auto w=std::complex<double>(1);for(size_t j=0;j<len/2;j++){
   auto u=a[i+j],v=a[i+j+len/2]*w;a[i+j]=u+v;a[i+j+len/2]=u-v;w*=wlen;
  }}
 }
}
struct Snap {double rms=0,centroid=0,brightness=0,flatness=0;};
Snap spectrum(const float*ptr,int n,int rate){
 constexpr int N=1024;std::vector<std::complex<double>> buffer(N);
 double energy=0;for(int j=0;j<n;j++)energy+=double(ptr[j])*ptr[j];
 for(int i=0;i<N;i++){
  const double x=i<n?ptr[i]:0.;
  buffer[size_t(i)]=x*(.5-.5*std::cos(2*pi*i/(N-1)));
 }
 fft(buffer);
 double sum=0,cent=0,hi=0,logs=0;
 for(int k=1;k<N/2;k++){
  const double v=std::abs(buffer[size_t(k)])+1e-12,hz=k*double(rate)/N;
  sum+=v;cent+=v*hz;if(hz>1500)hi+=v;logs+=std::log(v);
 }
 Snap out;out.rms=std::sqrt(energy/std::max(1,n));
 if(sum>1e-6){out.centroid=cent/sum;out.brightness=hi/sum;out.flatness=std::exp(logs/(N/2-1))/(sum/(N/2-1));}
 return out;
}
double median(std::vector<double>v){if(v.empty())return 0.;std::nth_element(v.begin(),v.begin()+v.size()/2,v.end());return v[v.size()/2];}
// Comparison intentionally phase-insensitive and gain-invariant, so recordings
// can be onset-aligned but do not need identical phase. Does not identify source.
double soundDistance(const std::vector<float>&target,const std::vector<float>&candidate,int rate){
 if(target.size()!=candidate.size()||target.size()<512)return 1e12;
 double loss=0;int blocks=0;
 for(size_t at=0;at+1024<=target.size();at+=512){
  auto x=spectrum(target.data()+at,1024,rate),y=spectrum(candidate.data()+at,1024,rate);
  const double level=std::log((x.rms+1e-8)/(y.rms+1e-8));
  const double centroid=std::log((x.centroid+20)/(y.centroid+20));
  loss+=.25*level*level+1.3*centroid*centroid+3*std::pow(x.brightness-y.brightness,2)+
        .15*std::pow(x.flatness-y.flatness,2);
  blocks++;
 }
double rr=0,ss=0,rs=0;
 for(size_t i=0;i<target.size();i++){
  const double a=target[i],b=candidate[i];rr+=a*a;ss+=b*b;rs+=a*b;
 }
 // Known-onset physical simulation: phase-coherent cue, limited to 20% of
 // the objective so imperfect microphone phase cannot dominate features.
 const double coherence=(rr>1e-15&&ss>1e-15)?std::clamp(rs*rs/(rr*ss),0.,1.):0.;
 return blocks?loss/blocks+.2*(1-coherence):1e12;
}
double scoreWave(const gen37::WaveRecipe&r,const std::vector<float>&ref,int rate){
 auto syn=gen37::renderWave(r,rate,ref.size());
 return syn.size()==ref.size()?soundDistance(ref,syn,rate):1e12;
}
double scoreFEM(const fem36::Model&m,const std::vector<inverse36::Take>&takes,int rate,bool training){
 double total=0;int count=0;
 for(auto&t:takes)if(t.training==training){
  auto syn=inverse36::render(m,rate,t.midi,t.velocity,t.audio.size());
  if(syn.size()!=t.audio.size())return 1e12;
  total+=soundDistance(t.audio,syn,rate);count++;
 }
 return count?total/count:0.;
}
}
const char* gestureName(Gesture::Kind k) noexcept{
 switch(k){case Gesture::Kind::Attack:return "attack_candidate";
 case Gesture::Kind::LegatoCandidate:return "legato_candidate";
 case Gesture::Kind::VibratoCandidate:return "vibrato_candidate";
 case Gesture::Kind::DampingChange:return "damping_candidate";
 case Gesture::Kind::PitchChange:return "pitch_transition_candidate";}
 return "unknown";
}
Analysis analyze(const std::vector<float>&audio,int rate,int hop){
 Analysis out;out.sampleRate=rate;out.durationSec=audio.size()/double(std::max(1,rate));
 if(rate<8000||rate>96000||audio.size()<size_t(rate/3)||audio.size()>size_t(rate)*30||
    hop<64||hop>2048||!finiteAudio(audio)){out.error="invalid audio duration, rate or nonfinite samples";return out;}
 if(audio.size()<1024){out.error="FFT window longer than recording";return out;}
 auto old=performance30::analyze(audio.data(),int(audio.size()),rate);
 size_t fi=0;double valid=0;
 for(size_t at=0;at+1024<=audio.size();at+=size_t(hop)){
  auto f=spectrum(audio.data()+at,1024,rate);
  Frame x;x.seconds=(at+512.)/rate;x.rms=f.rms;x.centroidHz=f.centroid;x.brightness=f.brightness;x.flatness=f.flatness;
  while(fi+1<old.frames.size()&&old.frames[fi+1].seconds<=x.seconds)fi++;
  if(!old.frames.empty()){
   const auto&pitch=old.frames[fi];x.pitchHz=pitch.pitchHz;x.pitchConfidence=pitch.pitchConfidence;
   if(x.pitchHz>0&&x.pitchConfidence>.55)valid++;
  }
  out.frames.push_back(x);
 }
 out.validPitchFraction=valid/std::max<size_t>(1,out.frames.size());
 // Long-duration sections: 0.5 sec chunks, including the attack and tail.
 const int each=std::max(1,int(.5*rate/hop));
 for(size_t start=0;start<out.frames.size();start+=size_t(each)){
  size_t end=std::min(out.frames.size(),start+each);
  if(end-start<size_t(std::max(2,each/2))&& !out.sections.empty()){
   // Avoid a one-frame tail being mistaken for a long-duration timbre trend.
   break;
  }Section s;s.fromSec=out.frames[start].seconds;
  s.toSec=out.frames[end-1].seconds;s.rms=s.centroidHz=s.brightness=0;
  for(size_t i=start;i<end;i++){
   s.rms+=out.frames[i].rms;s.centroidHz+=out.frames[i].centroidHz;s.brightness+=out.frames[i].brightness;
  }
  s.rms/=end-start;s.centroidHz/=end-start;s.brightness/=end-start;out.sections.push_back(s);
 }
 if(out.sections.size()>1){
  const auto a=out.sections.front().centroidHz,b=out.sections.back().centroidHz;
  out.centroidChange=b-a;
 }
 for(auto&ev:old.events){
  Gesture g;g.timeSec=ev.seconds;g.confidence=ev.confidence;
  switch(ev.kind){
   case performance30::EventKind::Pluck:g.kind=Gesture::Kind::Attack;break;
   case performance30::EventKind::LegatoCandidate:g.kind=Gesture::Kind::LegatoCandidate;break;
   case performance30::EventKind::VibratoCandidate:continue; // replaced by periodic, confidence-scored estimator
   case performance30::EventKind::PitchTransition:g.kind=Gesture::Kind::PitchChange;break;
   case performance30::EventKind::DampingChange:g.kind=Gesture::Kind::DampingChange;break;
  }
  out.gestures.push_back(g);
 }
 // Legato/portamento hypothesis: a substantial, continuous pitch transition
 // without a coincident attack. Use medians across windows to reject vibrato.
 if(old.frames.size()>50){
  double last=-1.;
  for(size_t i=30;i+6<old.frames.size();i+=5){
   std::vector<double>before,after;double rmsBefore=0,rmsAfter=0;
   for(size_t k=i-30;k<i-20;k++){
    auto&f=old.frames[k];if(f.pitchConfidence>.62&&f.pitchHz>50)before.push_back(f.pitchHz);
    rmsBefore+=f.rms;
   }
   for(size_t k=i-5;k<i+5;k++){
    auto&f=old.frames[k];if(f.pitchConfidence>.62&&f.pitchHz>50)after.push_back(f.pitchHz);
    rmsAfter+=f.rms;
   }
   if(before.size()<7||after.size()<7)continue;
   double semitones=12*std::log2(median(after)/median(before));
   const double ratio=(rmsAfter+1e-7)/(rmsBefore+1e-7);
   const double sec=old.frames[i].seconds;
   if(std::abs(semitones)<1.55||ratio<.58||ratio>1.8||sec-last<.35)continue;
   bool attackNear=false;
   for(const auto&ev:out.gestures)if(ev.kind==Gesture::Kind::Attack&&std::abs(ev.timeSec-sec)<.15)attackNear=true;
   if(!attackNear){out.gestures.push_back({Gesture::Kind::LegatoCandidate,sec,.35,std::abs(semitones),0.});last=sec;}
  }
 }
 // Vibrato: detect periodic pitch modulation with autocorrelation of detrended cents.
 // Report candidate rate rather than asserting actual performer technique.
 if(old.frames.size()>=50){
  std::vector<double> cents(old.frames.size(),0.);
  for(size_t i=0;i<cents.size();i++)if(old.frames[i].pitchConfidence>.62&&old.frames[i].pitchHz>50)
   cents[i]=1200*std::log2(old.frames[i].pitchHz/440.);
  const int win=100,step=50;
  for(int start=0;start+win<=int(cents.size());start+=step){
   std::vector<double> validP;
   for(int i=start;i<start+win;i++)if(cents[size_t(i)]!=0)validP.push_back(cents[size_t(i)]);
   if(validP.size()<win*.8)continue;
   double mid=median(validP);std::vector<double> d(size_t(win),0.);
   for(int i=0;i<win;i++)d[size_t(i)]=cents[size_t(start+i)]==0?0:cents[size_t(start+i)]-mid;
   // Remove slow legato trend by a centered 200ms running average.
   std::vector<double>r(d.size());
   for(int i=0;i<win;i++){
    int lo=std::max(0,i-10),hi=std::min(win-1,i+10);double mean=0;for(int j=lo;j<=hi;j++)mean+=d[size_t(j)];
    r[size_t(i)]=d[size_t(i)]-mean/(hi-lo+1);
   }
   double energy=0;for(auto v:r)energy+=v*v;
   const double rms=std::sqrt(energy/win);if(rms<8||rms>220)continue;
   double best=-1;int bestLag=0;
   for(int lag=11;lag<=33;lag++){
    double ab=0,aa=0,bb=0;
    for(int i=0;i+lag<win;i++){const double a=r[size_t(i)],b=r[size_t(i+lag)];ab+=a*b;aa+=a*a;bb+=b*b;}
    const double corr=ab/std::sqrt(aa*bb+1e-15);
    if(corr>best){best=corr;bestLag=lag;}
   }
   if(best>.52&&bestLag>0){
    double hopSec=old.frames.size()>1?old.frames[1].seconds-old.frames[0].seconds:.01;
    double rateHz=1./(bestLag*hopSec);
    if(rateHz>=3.&&rateHz<=9.)out.gestures.push_back({Gesture::Kind::VibratoCandidate,old.frames[size_t(start+win/2)].seconds,std::min(1.,best),rms,rateHz});
   }
  }
 }
 std::sort(out.gestures.begin(),out.gestures.end(),[](auto&a,auto&b){return a.timeSec<b.timeSec;});
 return out;
}
WaveFit fitWave(const std::vector<float>&train,const std::vector<float>&holdout,int rate,gen37::Family family,double pickup,int budget){
 WaveFit out;
 if((family!=gen37::Family::String1D&&family!=gen37::Family::Membrane2D)||rate<22050||rate>96000||
    train.size()<2048||train.size()>size_t(rate)*2||holdout.size()!=train.size()||
    !finiteAudio(train)||!finiteAudio(holdout)||!std::isfinite(pickup)||pickup<=.05||pickup>=.95||budget<4||budget>500){
  out.error="invalid wave fitting inputs";return out;
 }
 // Generate diversity across spatial resolution, fundamental and excitation locations.
 const int nxs[]={18,24,32},nys[]={12,16};const double fs[]={130,196,270};
 const double es[]={.16,.28,.42,.58,.74,.86};
 std::vector<WaveCandidate> candidates;
 for(int nx:nxs)for(int ny:nys)for(double hz:fs)for(double ex:es){
  gen37::WaveRecipe model;model.family=family;model.nx=nx;model.ny=ny;model.fundamentalHz=hz;
  model.pickup=pickup;model.exciter=ex;model.damping=2.;model.amplitude=.2;
  candidates.push_back({model});
 }
 // Calibrated initial family at nx=24,ny=16,hz=196; vary excitation
 // position first. Broader mesh/fundamental sweep uses remaining budget.
 std::vector<int> selected;
 for(int i=0;i<int(candidates.size());i++){
  const auto&m=candidates[size_t(i)].model;
  if(m.nx==24&&m.ny==16&&m.fundamentalHz==196)
   selected.push_back(i);
 }
 const int n=std::min(budget,int(candidates.size()));
 if(int(selected.size())>n)selected.resize(size_t(n));
 for(int i=0;int(selected.size())<n&&i<n*4;i++){
  int index=int((int64_t(i)*int(candidates.size()))/(n*4));
  if(index<int(candidates.size())&&std::find(selected.begin(),selected.end(),index)==selected.end())selected.push_back(index);
 }
 for(int i=0;int(selected.size())<n&&i<int(candidates.size());i++)
  if(std::find(selected.begin(),selected.end(),i)==selected.end())selected.push_back(i);
 for(int index:selected){
  auto c=candidates[size_t(index)];c.trainingLoss=scoreWave(c.model,train,rate);out.evaluations++;
  if(std::isfinite(c.trainingLoss))out.proposals.push_back(c);
 }
 if(out.proposals.empty()){out.error="all wave candidates failed";return out;}
 std::sort(out.proposals.begin(),out.proposals.end(),[](const auto&a,const auto&b){return a.trainingLoss<b.trainingLoss;});
 // refine best candidate within an explicitly limited parameter family
 if(out.evaluations<budget){
  auto best=out.proposals[0];
  for(double delta:{-.2,.2,-.08,.08})for(int par=0;par<3&&out.evaluations<budget;par++){
   auto x=best;
   if(par==0)x.model.exciter=std::clamp(x.model.exciter+delta*.5,.08,.92);
   if(par==1)x.model.fundamentalHz=std::clamp(x.model.fundamentalHz*(1+delta),40.,1600.);
   if(par==2)x.model.damping=std::clamp(x.model.damping*(1+delta),0.,400.);
   x.trainingLoss=scoreWave(x.model,train,rate);out.evaluations++;
   if(x.trainingLoss<best.trainingLoss)best=x;
  }
  out.proposals.push_back(best);
 }
 std::sort(out.proposals.begin(),out.proposals.end(),[](auto&a,auto&b){return a.trainingLoss<b.trainingLoss;});
 // Holdout never decides which model to keep. Report it separately.
 if(out.proposals.size()>4)out.proposals.resize(4);
 for(auto&c:out.proposals)c.holdoutLoss=scoreWave(c.model,holdout,rate);
 out.ambiguityRatio=out.proposals.size()>1?(out.proposals[1].trainingLoss+1e-9)/(out.proposals[0].trainingLoss+1e-9):0;
 return out;
}
FEMFit fit3D(const std::vector<inverse36::Take>&takes,int rate,int budget){
 FEMFit out;
 if(takes.size()<2||takes.size()>12||rate<22050||rate>96000||budget<12||budget>500){out.error="invalid 3D search settings";return out;}
 int train=0,holdout=0;
 for(auto&t:takes){if(t.training)train++;else holdout++;if(t.audio.size()<1024||t.audio.size()>size_t(rate)*2||!finiteAudio(t.audio)){out.error="invalid take";return out;}}
 if(train<1||holdout<1){out.error="need independent training and holdout takes";return out;}
 // ShapeInverse37 actually generates meshes; it does not require an input model library.
 auto prior=gen37::discover(takes,rate,std::max(12,budget-8),4);
 if(!prior.error.empty()){out.error=prior.error;return out;}
 out.proposals=prior.proposals;out.evaluations=prior.evaluations;
 // Refine material, excitation and effective stiffness while preserving
 // generated geometry. Strict training-only coordinate steps; no holdout leakage.
 for(auto&candidate:out.proposals){
  candidate.trainingLoss=scoreFEM(candidate.model,takes,rate,true);
  for(int axis=0;axis<6&&out.evaluations<budget;axis++){
   for(int direction:{-1,1}){
    if(out.evaluations>=budget)break;
    auto test=candidate.model;
    switch(axis){
     case 0:test.meshDensity=std::clamp(test.meshDensity*(direction>0?1.2:.8),1e3,1e6);break;
     case 1:test.meshYoung=std::clamp(test.meshYoung*(direction>0?1.16:.84),1e6,2e9);break;
     case 2:test.pluckPosition=std::clamp(test.pluckPosition+(direction>0?.06:-.06),.05,.95);break;
     case 3:test.bridgeCoupling=std::clamp(test.bridgeCoupling*(direction>0?1.18:.82),0.,.5);break;
     case 4:test.anisotropy[0]=std::clamp(test.anisotropy[0]*(direction>0?1.2:.8),.01,100.);break;
     case 5:test.anisotropy[1]=std::clamp(test.anisotropy[1]*(direction>0?1.2:.8),.01,100.);break;
    }
    double e=scoreFEM(test,takes,rate,true);out.evaluations++;
    if(e<candidate.trainingLoss){candidate.trainingLoss=e;candidate.model=test;}
   }
  }
  candidate.validationLoss=scoreFEM(candidate.model,takes,rate,false);
 }
 std::sort(out.proposals.begin(),out.proposals.end(),[](auto&a,auto&b){return a.trainingLoss<b.trainingLoss;});
 if(out.proposals.size()>1)out.ambiguityRatio=(out.proposals[1].trainingLoss+1e-9)/(out.proposals[0].trainingLoss+1e-9);
 return out;
}
}
