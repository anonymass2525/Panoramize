#include "GuitarInverse29.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <limits>
#include <numeric>
#include <sstream>
namespace panorama::guitar29 {
namespace {
constexpr double pi=3.14159265358979323846, twopi=2*pi;
constexpr int kPartial=14;
bool ok(double x){return std::isfinite(x);}
struct Envelope {std::array<double,kPartial> amplitude{},slope{};double frequency=0.,onset=0.,attack=0.,rms=0.;int count=0;};
double freqPartial(double f,double B,int n){return f*n*std::sqrt((1.+B*n*n)/(1.+B));}
double dcoeff(const Model& m,int k){return m.decayBase+m.decayHigh*(k-1.)*(k-1.);}
// Loop-free single note modal readout; body / pickup position is an explicit prior.
double shape(int n,double p,double q,double tilt){return std::sin(pi*n*p)*std::sin(pi*n*q)*std::exp(-tilt*(n-1.))/double(n);}
// Quadratic interpolation on autocorrelation peaks; input is downsampled to cap cost.
double f0(const Take& t,int begin){
 const int decim=std::max(1,t.sampleRate/8000);const int n=std::min(3000,(t.frames-begin)/decim);
 if(n<700)return 0;
 std::vector<double> x; x.reserve(size_t(n));
 for(int i=0;i<n;i++)x.push_back(t.samples[begin+i*decim]);
 const double sr=double(t.sampleRate)/decim;
 const int low=std::max(9,int(sr/1100.)),high=std::min(n/4,int(sr/73.));
 std::vector<double> ac(size_t(high+2),0.);
 for(int lag=low;lag<=high;lag++){
  double xy=0.,xx=0.,yy=0.;
  for(int i=0;i+lag<n;i++){const double a=x[size_t(i)],b=x[size_t(i+lag)];xy+=a*b;xx+=a*a;yy+=b*b;}
  ac[size_t(lag)]=xy/std::sqrt(xx*yy+1.e-30);
 }
 double best=0.;for(int lag=low;lag<=high;lag++)best=std::max(best,ac[size_t(lag)]);
 int lagBest=-1;
 for(int lag=low+1;lag<high;lag++)if(ac[size_t(lag)]>=.90*best&&ac[size_t(lag)]>ac[size_t(lag-1)]&&ac[size_t(lag)]>=ac[size_t(lag+1)]){lagBest=lag;break;}
 if(lagBest<0){lagBest=low;for(int lag=low+1;lag<=high;lag++)if(ac[size_t(lag)]>ac[size_t(lagBest)])lagBest=lag;}
 if(best<.35)return 0;
 double interp=0.;if(lagBest>low&&lagBest<high){const double a=ac[size_t(lagBest-1)],b=ac[size_t(lagBest)],c=ac[size_t(lagBest+1)];const double d=a-2*b+c;if(std::abs(d)>1.e-9)interp=std::clamp(.5*(a-c)/d,-.5,.5);}
 return sr/(lagBest+interp);
}
int findOnset(const Take& t){
 const int block=std::max(32,t.sampleRate/500);const int blocks=t.frames/block; if(blocks<12)return -1;
 std::vector<double> rms; rms.reserve(size_t(blocks));
 for(int b=0;b<blocks;b++){
  double e=0.;for(int i=b*block;i<(b+1)*block;i++){const auto x=double(t.samples[i]);e+=x*x;}
  rms.push_back(std::sqrt(e/block));
 }
 const double peak=*std::max_element(rms.begin(),rms.end());if(peak<2.e-5)return -1;
 // Ignore isolated strong noise clicks when there is no sustained pitched energy.
 const auto end=std::min(blocks,int(.4*t.sampleRate/block));
 for(int b=0;b<end;b++)if(rms[size_t(b)]>std::max(peak*.10,2.e-5))return std::max(0,b*block-block);
 return -1;
}
double measure(const Take& t,int pos,int length,double frequency){
 if(frequency<=0||frequency>=t.sampleRate*.49||pos<0||pos+length>t.frames||length<128)return 0;
 double re=0.,im=0.,wsum=0.;const double delta=twopi*frequency/t.sampleRate;
 for(int j=0;j<length;j++){
  const double w=.5-.5*std::cos(twopi*j/(length-1));const double x=t.samples[pos+j]*w;
  re+=x*std::cos(delta*j);im+=x*std::sin(delta*j);wsum+=w;
 }
 return 2.*std::hypot(re,im)/(wsum+1.e-20);
}
// Find an inharmonicity value by tracking actual spectral peak locations.
// Peak estimates are weak for short/noisy recordings, so keep B bounded.
double estimateB(const Take& t,double f,int onset,int size,int harmonics){
 double bestB=0.,best=-1;
 const int start=std::min(t.frames-size,onset+std::max(0,int(.07*t.sampleRate)));
 for(int bi=0;bi<=14;bi++){
  const double B=bi*.00005;double score=0.;
  for(int n=3;n<=harmonics;n++){
   const double target=freqPartial(f,B,n);if(target>=t.sampleRate*.43)break;
   const double amp=measure(t,start,size,target);
   score+=std::log1p(amp*100.)/(1.+.09*n);
  }
  // This diagnostic is intentionally not treated as a unique material measurement.
  if(score>best){best=score;bestB=B;}
 }
 return bestB;
}
struct Features {Envelope e;std::array<double,kPartial> early{},late{};};
Features extract(const Take& t){
 Features f;const auto& s=t.samples;
 if(!s||t.frames<t.sampleRate*.65||t.frames>t.sampleRate*12||t.sampleRate<8000||t.sampleRate>48000)return f;
 for(int i=0;i<t.frames;i++)if(!ok(s[i]))return f;
 const int onset=findOnset(t);if(onset<0)return f;
 double pitch=t.knownFrequencyHz;
 if(pitch<=0)pitch=f0(t,std::min(t.frames-1,onset+int(.035*t.sampleRate)));
 if(pitch<73||pitch>1100)return f;
 f.e.frequency=pitch;f.e.onset=double(onset)/t.sampleRate;
 const int length=std::max(512,std::min(int(.095*t.sampleRate),t.frames/7));
 const int harmonics=std::min(kPartial,std::max(1,int((t.sampleRate*.40)/pitch)));
 if(harmonics<5)return f;
 f.e.count=harmonics;
 const double earlyTime=.09;const int p0=std::min(t.frames-length,onset+int(earlyTime*t.sampleRate));
 const double lateTime=std::min(1.15,std::max(.4,(t.frames-onset)/double(t.sampleRate)-.18));
 const int p1=std::min(t.frames-length,onset+int(lateTime*t.sampleRate));
 f.e.attack=0.;
 const int attackLength=std::min(t.frames-onset,int(.06*t.sampleRate));
 double max=0.;for(int i=onset;i<onset+attackLength;i++)max=std::max(max,double(std::abs(s[i])));
 for(int i=onset;i<onset+attackLength;i++)if(std::abs(s[i])>.4*max){f.e.attack=double(i-onset)/t.sampleRate;break;}
 double rms=0.;for(int i=onset;i<t.frames;i++)rms+=double(s[i])*s[i];f.e.rms=std::sqrt(rms/(t.frames-onset));
 const auto B=estimateB(t,pitch,onset,length,harmonics);
 for(int n=1;n<=harmonics;n++){
  // Peak fitting allows small F0 error; special bounds to avoid choosing neighbor harmonics.
  const double center=freqPartial(pitch,B,n);
  const double shift=std::max(1.5,.007*center);
  double bestA=-1,bestHz=center;
  for(int k=-2;k<=2;k++){
   const double candidate=center+k*shift/2;const auto a=measure(t,p0,length,candidate);
   if(a>bestA){bestA=a;bestHz=candidate;}
  }
  f.early[size_t(n-1)]=std::max(1.e-9,bestA);
  f.late[size_t(n-1)]=std::max(1.e-9,measure(t,p1,length,bestHz));
  f.e.slope[size_t(n-1)]=std::clamp(std::log(f.early[size_t(n-1)]/f.late[size_t(n-1)])/
   std::max(.12,double(p1-p0)/t.sampleRate),0.,80.);
 }
 // The 15th slot is used for inharmonicity diagnostic to keep the public header small.
 f.e.amplitude[0]=B;
 return f;
}
struct Fit {double p=.25,tilt=0.,d0=2.,d2=.01,loss=1.e30;};
Fit fit(const Features&f,double pickup){
 const int nmax=f.e.count;if(nmax<5)return {};
 double ampmax=0.;for(int i=0;i<nmax;i++)ampmax=std::max(ampmax,f.early[size_t(i)]);
 Fit best;
 // Robust envelope regression with 2 parameter trend; floor excludes vanishing modes.
 double sx=0.,sy=0.,sxx=0.,sxy=0.,sw=0.;
 for(int i=0;i<nmax;i++)if(f.early[size_t(i)]>ampmax*.023 && f.late[size_t(i)]>1.e-8){
  const double x=double(i*i),y=f.e.slope[size_t(i)];
  const double w=std::sqrt(f.early[size_t(i)]/(ampmax+1.e-20));
  sw+=w;sx+=w*x;sy+=w*y;sxx+=w*x*x;sxy+=w*x*y;
 }
 const double denom=sw*sxx-sx*sx;
 if(sw>1&&denom>1.e-6){best.d2=std::clamp((sw*sxy-sx*sy)/denom,0.,.2);best.d0=std::clamp((sy-best.d2*sx)/sw,.1,60.);}
 std::array<double,kPartial> observed{};
 // Restrict to partials sufficiently above noise.
 for(int i=0;i<nmax;i++)observed[size_t(i)]=std::log(std::max(f.early[size_t(i)],ampmax*.006));
 for(int j=8;j<=48;j++){
  const double p=j*.01;
  for(int it=0;it<=18;it++){
   const double tilt=it*.0125;
   std::array<double,kPartial> predicted{};double diff=0.,weight=0.;
   for(int k=1;k<=nmax;k++){
    const double form=std::max(1.e-6,std::abs(shape(k,p,pickup,tilt)));
    predicted[size_t(k-1)]=std::log(form)-best.d0*.09-best.d2*(k-1.)*(k-1.)*.09;
    const double w=f.early[size_t(k-1)]>ampmax*.018?1.:.22;
    diff+=w*(observed[size_t(k-1)]-predicted[size_t(k-1)]);weight+=w;
   }
   const double offset=diff/weight;
   double loss=0.;for(int k=1;k<=nmax;k++){
    const double w=f.early[size_t(k-1)]>ampmax*.018?1.:.22;
    const double d=observed[size_t(k-1)]-predicted[size_t(k-1)]-offset;
    loss+=w*std::min(d*d,9.);
   }
   loss/=weight;
   if(loss<best.loss){best.p=p;best.tilt=tilt;best.loss=loss;}
  }
 }
 return best;
}
}
// Evaluate every take against ONE shared physical instrument (no independent
// re-fitting on holdout). Amplitude scaling is profiled out: mic gain is unknown.
double modelDistance(const Features& f,const Model& m){
 if(f.e.count<5)return 1.e6;
 const int nmax=f.e.count;
 double top=0.;for(int i=0;i<nmax;i++)top=std::max(top,f.early[size_t(i)]);
 double sum=0.,weight=0.,error=0.;
 for(int i=0;i<nmax;i++){
  const int k=i+1;
  const double predicted=std::log(std::max(1.e-6,std::abs(shape(k,m.pickPosition,m.pickupPosition,m.spectralTilt))))-
   dcoeff(m,k)*.09;
  const double observed=std::log(std::max(f.early[size_t(i)],top*.006));
  const double w=f.early[size_t(i)]>top*.018?1.:.22;
  sum+=w*(observed-predicted);weight+=w;
 }
 const double offset=sum/std::max(1.e-9,weight);
 for(int i=0;i<nmax;i++){
  const int k=i+1;
  const double predicted=std::log(std::max(1.e-6,std::abs(shape(k,m.pickPosition,m.pickupPosition,m.spectralTilt))))-
   dcoeff(m,k)*.09;
  const double observed=std::log(std::max(f.early[size_t(i)],top*.006));
  const double w=f.early[size_t(i)]>top*.018?1.:.22;
  const double difference=observed-predicted-offset;
  error+=w*std::min(difference*difference,9.);
  if(f.early[size_t(i)]>top*.035){
   const double decayError=(f.e.slope[size_t(i)]-dcoeff(m,k))*.07;
   error+=w*.35*std::min(decayError*decayError,12.);
  }
 }
 const double bError=(f.e.amplitude[0]-m.inharmonicity)*800.;
 return error/std::max(1.e-9,weight)+.1*bError*bError;
}
std::vector<float> render(const Model& m,int sr,double seconds,double onset,double pitch){
 if(!ok(seconds)||seconds<=0||seconds>12||sr<8000||sr>48000||!ok(pitch)||pitch<.1||pitch>5)return {};
 const int frames=int(std::lround(seconds*sr)),start=std::max(0,int(onset*sr));
 std::vector<float> pcm(size_t(frames),0.f);
 if(frames<=start)return pcm;
 struct Osc {double re,im,rotR,rotI,decay,weight;};
 std::vector<Osc> osc;osc.reserve(size_t(std::clamp(m.harmonics,1,24)));
 for(int n=1;n<=std::clamp(m.harmonics,1,24);n++){
  const auto frequency=freqPartial(m.frequencyHz*pitch,m.inharmonicity,n);
  if(!ok(frequency)||frequency>sr*.43)break;
  const double angle=twopi*frequency/sr;
  osc.push_back({1.,0.,std::cos(angle),std::sin(angle),std::exp(-dcoeff(m,n)/sr),shape(n,m.pickPosition,m.pickupPosition,m.spectralTilt)});
 }
 for(int i=start;i<frames;i++){
  double value=0.;for(auto& o:osc){
   const double nextRe=(o.re*o.rotR-o.im*o.rotI)*o.decay;
   const double nextIm=(o.im*o.rotR+o.re*o.rotI)*o.decay;
   o.re=nextRe;o.im=nextIm;value+=o.weight*o.im;
  }
  const double ramp=std::min(1.,double(i-start)/(sr*.002));
  pcm[size_t(i)]=float(std::clamp(m.gain*value*ramp,-1.,1.));
 }
 return pcm;
}
Analysis analyze(const Take&take,double pickup){
 Analysis a;if(!ok(pickup)||pickup<.1||pickup>.95)return a;
 const auto x=extract(take);if(x.e.count<5)return a;
 const auto fitted=fit(x,pickup);
 a.frequencyHz=x.e.frequency;a.onsetSeconds=x.e.onset;a.attackSeconds=x.e.attack;a.rms=x.e.rms;
 a.estimatedDecayBase=fitted.d0;a.estimatedDecayHigh=fitted.d2;
 a.pickPosition=fitted.p;a.spectralTilt=fitted.tilt;a.inharmonicity=x.e.amplitude[0];
 a.loss=fitted.loss;a.analyzedHarmonics=x.e.count;
 a.confidence=std::clamp(1./(1.+fitted.loss),0.,1.);
 a.success=ok(a.loss)&&ok(a.frequencyHz)&&a.rms>1.e-6;
 return a;
}
InferenceResult infer(const std::vector<Take>& takes,double pickup){
 InferenceResult r;if(takes.empty()||takes.size()>16)return r;
 double sw=0.,p=0.,tilt=0.,d0=0.,d2=0.,B=0.;
 for(const auto& take:takes){
  const auto a=analyze(take,pickup);if(!a.success)return {};
  r.takes.push_back(a);
  if(!take.training)continue;
  const double weight=1./(1.+a.loss);
  sw+=weight;p+=weight*a.pickPosition;tilt+=weight*a.spectralTilt;d0+=weight*a.estimatedDecayBase;
  d2+=weight*a.estimatedDecayHigh;B+=weight*a.inharmonicity;

  r.trainingLoss+=a.loss;
 }
 if(sw<=0)return {};
 r.model.frequencyHz=r.takes.front().frequencyHz;r.model.pickPosition=p/sw;r.model.pickupPosition=pickup;
 r.model.decayBase=d0/sw;r.model.decayHigh=d2/sw;r.model.inharmonicity=B/sw;
 r.model.spectralTilt=tilt/sw;
 // Normalize loudness using ONLY training recordings, avoiding microphones'
 // unknown absolute gain: the physical pickup/DI amplitude is not identifiable.
 double gainSum=0.,gainWeight=0.;
 for(size_t i=0;i<takes.size();i++)if(takes[i].training){
  const auto& clip=takes[i];const auto& a=r.takes[i];
  Model unit=r.model;unit.gain=1.;
  const auto synthesized=render(unit,clip.sampleRate,double(clip.frames)/clip.sampleRate,a.onsetSeconds,a.frequencyHz/unit.frequencyHz);
  double squared=0.,reference=0.;
  for(int n=0;n<clip.frames;n++){
   squared+=double(synthesized[size_t(n)])*synthesized[size_t(n)];
   reference+=double(clip.samples[n])*clip.samples[n];
  }
  if(squared>1.e-9){gainSum+=std::sqrt(reference/squared);gainWeight+=1.;}
 }
 r.model.gain=gainWeight>0?std::clamp(gainSum/gainWeight,.002,1.9):.45;
 int train=0,hold=0;r.trainingLoss=0.;r.holdoutLoss=0.;
 for(size_t i=0;i<takes.size();i++){
  const auto features=extract(takes[i]);const auto loss=modelDistance(features,r.model);
  if(takes[i].training){train++;r.trainingLoss+=loss;}
  else {hold++;r.holdoutLoss+=loss;}
  r.takes[i].loss=loss;
 }
 r.trainingLoss/=train;if(hold)r.holdoutLoss/=hold;
 r.success=true;return r;
}
std::string serialize(const Model&m){
 std::ostringstream s;s<<std::setprecision(17)<<"PANORAMIZE-GUITAR-29|1\n";
 s<<"FREQUENCY|"<<m.frequencyHz<<"\nPICK_POSITION|"<<m.pickPosition<<"\nPICKUP_POSITION|"<<m.pickupPosition
 <<"\nDECAY_BASE|"<<m.decayBase<<"\nDECAY_HIGH|"<<m.decayHigh<<"\nINHARMONICITY|"<<m.inharmonicity
 <<"\nSPECTRAL_TILT|"<<m.spectralTilt<<"\nGAIN|"<<m.gain<<"\nHARMONICS|"<<m.harmonics<<"\n";
 return s.str();
}
bool deserialize(const std::string&text,Model&m){
 if(text.size()>10000)return false;std::istringstream in(text);std::string s;
 if(!std::getline(in,s)||s!="PANORAMIZE-GUITAR-29|1")return false;
 const std::array<std::string,9> keys={"FREQUENCY","PICK_POSITION","PICKUP_POSITION","DECAY_BASE","DECAY_HIGH","INHARMONICITY","SPECTRAL_TILT","GAIN","HARMONICS"};
 std::array<double,9> v{};
 for(size_t i=0;i<keys.size();i++){
  if(!std::getline(in,s)||s.rfind(keys[i]+"|",0)!=0)return false;
  try{size_t consumed=0;v[i]=std::stod(s.substr(keys[i].size()+1),&consumed);
   if(consumed!=s.size()-keys[i].size()-1||!ok(v[i]))return false;}catch(...){return false;}
 }
 if(std::getline(in,s)&&!s.empty())return false;
 if(v[0]<73||v[0]>1100||v[1]<.04||v[1]>.5||v[2]<.1||v[2]>.95||v[3]<0||v[3]>60||v[4]<0||v[4]>.3||v[5]<0||v[5]>.001||v[6]<0||v[6]>.5||v[7]<=0||v[7]>2||v[8]<1||v[8]>24||v[8]!=std::floor(v[8]))return false;
 m={v[0],v[1],v[2],v[3],v[4],v[5],v[6],v[7],int(v[8])};return true;
}
} // namespace panorama::guitar29
