#include "TemporalInverse28.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <string>
#include <utility>
#include <vector>
namespace panorama::temporal28 {
namespace {
constexpr double pi=3.1415926535897932384626433832795;
constexpr int features=8;
struct RNG { std::uint32_t state; explicit RNG(std::uint32_t seed):state(seed?seed:1u){}
 std::uint32_t next(){auto x=state;x^=x<<13;x^=x>>17;x^=x<<5;return state=x;}
 int integer(int n){return int(next()%std::uint32_t(n));}
 double uniform(){return double(next())/4294967295.;}
};
bool good(double x){return std::isfinite(x);}
void error(std::string* e,const char* message){if(e)*e=message;}
// Timbral modes share a normalized physical state. VA/FM are active sources.
native27::Mode mode(native27::Engine e,double r=1.) {
 native27::Mode m;m.engine=e;m.ratio=r;m.damping=5.;m.pickup=.30;
 m.excitation=(e==native27::Engine::VA||e==native27::Engine::FM)?130.:0.;return m;
}
native27::Graph initial(int index){
 native27::Graph g;g.name="Temporal Instrument";g.strikeMode=0;
 using E=native27::Engine;using C=native26::Coupling;
 switch(index%8){
 case 0:g.modes={mode(E::VA),mode(E::Resonance,2.)};break;
 case 1:g.modes={mode(E::FM),mode(E::Vocal,2.5)};break;
 case 2:g.modes={mode(E::VA),mode(E::Spectral,1.5),mode(E::Physical,.6)};break;
 case 3:g.modes={mode(E::FM),mode(E::Phase,1.01),mode(E::Nonlinear,3.)};break;
 case 4:g.modes={mode(E::FM),mode(E::Resonance,1.5),mode(E::Vocal,2.5)};break;
 case 5:g.modes={mode(E::VA),mode(E::Physical,.75),mode(E::Resonance,2.1)};break;
 case 6:g.modes={mode(E::VA),mode(E::FM,1.5),mode(E::Vocal,3.)};break;
 default:g.modes={mode(E::FM),mode(E::Spectral,2.),mode(E::Nonlinear,1.25)};break;
 }
 for(int i=1;i<int(g.modes.size());i++)g.edges.push_back({i-1,i,C::Conservative,4.e4,.87});
 if(index%3==0)g.edges.push_back({0,int(g.modes.size())-1,C::Dissipative,1.,.8});
 return g;
}
void mutate(native27::Graph& g,RNG& rng){
 if(g.modes.empty())return;
 const int index=rng.integer(int(g.modes.size())), action=rng.integer(13);
 auto& m=g.modes[size_t(index)];
 switch(action){
 case 0:m.ratio=std::clamp(m.ratio*std::exp((rng.uniform()-.5)*.65),.1,12.);break;
 case 1:m.damping=std::clamp(m.damping*std::exp((rng.uniform()-.5)*1.5),.02,100.);break;
 case 2:m.pickup=std::clamp(m.pickup+(rng.uniform()-.5)*.30,-1.,1.);break;
 case 3: if(m.excitation>0)m.excitation=std::clamp(m.excitation*std::exp((rng.uniform()-.5)*1.2),5.,1500.);else m.excitation=0.;break;
 case 4:m.engine=static_cast<native27::Engine>(rng.integer(8));m.excitation=(m.engine==native27::Engine::VA||m.engine==native27::Engine::FM)?130.:0.;break;
 case 5:m.nonlinear=m.nonlinear==0?1.e10*(1+rng.integer(20)):0;break;
 case 6:if(g.modes.size()<7){g.modes.push_back(mode(static_cast<native27::Engine>(rng.integer(8)),.7+2.*rng.uniform()));g.edges.push_back({index,int(g.modes.size())-1,native26::Coupling::Conservative,3.e4,.8});}break;
 case 7:if(g.modes.size()>2){g.modes.erase(g.modes.begin()+index);g.edges.erase(std::remove_if(g.edges.begin(),g.edges.end(),[&](auto e){return e.a==index||e.b==index;}),g.edges.end());for(auto& e:g.edges){if(e.a>index)--e.a;if(e.b>index)--e.b;}g.strikeMode=0;}break;
 case 8:if(!g.edges.empty()){auto& e=g.edges[size_t(rng.integer(int(g.edges.size())))];e.strength=std::clamp(e.strength*std::exp((rng.uniform()-.5)*1.5),1.,1.e8);}break;
 case 9:if(!g.edges.empty()){auto& e=g.edges[size_t(rng.integer(int(g.edges.size())))];e.kind=e.kind==native26::Coupling::Conservative?native26::Coupling::Dissipative:native26::Coupling::Conservative;e.strength=e.kind==native26::Coupling::Dissipative?2.:4.e4;}break;
 case 10:if(g.modes.size()>1&&g.edges.size()<18){const int other=(index+1+rng.integer(int(g.modes.size())-1))%int(g.modes.size());g.edges.push_back({index,other,native26::Coupling::Conservative,2.e4,.9});}break;
 case 11:g.strikeMode=index;break;
 case 12:if(!g.edges.empty()){auto& e=g.edges[size_t(rng.integer(int(g.edges.size())))];e.ratio=std::clamp(e.ratio+(rng.uniform()-.5)*.3,.3,1.5);}break;
 }
}
std::uint64_t signature(const native27::Graph& g){
 std::uint64_t h=1469598103934665603ULL;
 auto add=[&](std::uint64_t x){h^=x;h*=1099511628211ULL;};
 add(g.modes.size());add(g.edges.size());add(std::uint64_t(g.strikeMode+1));
 for(auto m:g.modes)add(std::uint64_t(int(m.engine)+1));
 std::vector<std::array<int,3>> edges;edges.reserve(g.edges.size());
 for(const auto& e:g.edges)edges.push_back({std::min(e.a,e.b),std::max(e.a,e.b),int(e.kind)});
 std::sort(edges.begin(),edges.end());for(const auto& e:edges){add(e[0]+1);add(e[1]+1);add(e[2]+1);}
 return h;
}
std::string why;
bool renderPrepared(native27::Instrument& inst,const Example& clip,std::vector<float>& output){
 inst.allNotesOff();inst.pitchBend(0);inst.modWheel(0);
 output.resize(size_t(clip.frames));int next=0;
 for(int n=0;n<clip.frames;n++){
  while(next<clip.eventCount&&clip.events[next].frame==n){
   const auto& e=clip.events[next++];switch(e.type){
   case EventType::NoteOn:inst.noteOn(e.note,e.value);break;
   case EventType::NoteOff:inst.noteOff(e.note);break;
   case EventType::PitchBend:inst.pitchBend(e.value);break;
   case EventType::ModWheel:inst.modWheel(e.value);break;
   }
  }
  const float sample=inst.processMono();if(!std::isfinite(sample))return false;
  output[size_t(n)]=sample;
 }return true;
}
struct Entry{native27::Graph graph;double loss;std::uint64_t topology;};
}
bool validate(const Example& e,std::string* err) noexcept {
 if(e.frames<1024||e.sampleRate<8000||e.sampleRate>48000||e.frames>12*e.sampleRate||!e.events||e.eventCount<1||e.eventCount>1024){error(err,"invalid duration, sample rate or events");return false;}
 int last=-1;bool hasOn=false;
 for(int i=0;i<e.eventCount;i++){
  const auto& a=e.events[i];
  if(a.frame<last||a.frame<0||a.frame>=e.frames||int(a.type)<0||int(a.type)>3||!good(a.value)){error(err,"invalid event timeline");return false;}
  if(a.type==EventType::NoteOn){if(a.note<0||a.note>127||a.value<=0||a.value>1){error(err,"invalid note on");return false;}hasOn=true;}
  if(a.type==EventType::NoteOff&&(a.note<0||a.note>127)){error(err,"invalid note off");return false;}
  if(a.type==EventType::PitchBend&&std::abs(a.value)>24){error(err,"invalid bend");return false;}
  if(a.type==EventType::ModWheel&&(a.value<0||a.value>1)){error(err,"invalid modwheel");return false;}
  last=a.frame;
 }
 if(!hasOn){error(err,"requires a note onset");return false;}
 if(e.audio)for(int i=0;i<e.frames;i++)if(!std::isfinite(e.audio[i])){error(err,"nonfinite audio");return false;}
 if(err)err->clear();
 return true;
}
bool render(const native27::Graph& graph,const Example& clip,std::vector<float>& output,std::string* err){
 if(!validate(clip,err))return false;
 auto inst=native27::Instrument::prepare(graph,clip.sampleRate,err);if(!inst)return false;
 if(!renderPrepared(*inst,clip,output)){error(err,"unstable or invalid renderer");return false;}
 if(err)err->clear();
 return true;
}
std::vector<float> describe(const float* pcm,int frames,int rate){
 if(!pcm||frames<1024||rate<8000)return {};
 const int window=std::max(192,rate/40); // 25ms local detail, full time order retained
 std::vector<float> result;result.reserve(size_t((frames+window-1)/window)*features);
 constexpr std::array<double,4> frequencies{{160.,420.,1100.,2800.}};
 std::array<double,4> co{};for(int j=0;j<4;j++)co[size_t(j)]=2.*std::cos(2*pi*frequencies[size_t(j)]/rate);
 for(int start=0;start<frames;start+=window){
  const int count=std::min(window,frames-start);
  if(count<window/2)break;
  double sum=0.,diff=0.,crest=0.,zc=0.;float prev=pcm[start];
  std::array<double,4> s{},s1{};
  for(int i=0;i<count;i++){
   const float value=pcm[start+i];double x=value;
   sum+=x*x;diff+=double(value-prev)*double(value-prev);crest=std::max(crest,std::abs(x));
   if((value>=0)!=(prev>=0))zc++;
   for(int k=0;k<4;k++){const double z=x+co[size_t(k)]*s[size_t(k)]-s1[size_t(k)];s1[size_t(k)]=s[size_t(k)];s[size_t(k)]=z;}
   prev=value;
  }
  double rms=std::sqrt(sum/count), slope=std::sqrt(diff/count);
  result.push_back(float(std::log(1.e-5+rms)));
  result.push_back(float(std::log(1.e-5+slope)));
  result.push_back(float(zc/count));
  result.push_back(float(crest/(rms+1.e-4)));
  for(int k=0;k<4;k++){
   const double power=std::max(0.,s[size_t(k)]*s[size_t(k)]+s1[size_t(k)]*s1[size_t(k)]-co[size_t(k)]*s[size_t(k)]*s1[size_t(k)]);
   result.push_back(float(std::log(1.e-4+std::sqrt(power)/count)));
  }
 }
 return result;
}
double distance(const std::vector<float>& a,const std::vector<float>& b){
 if(a.empty()||a.size()!=b.size()||a.size()%features)return std::numeric_limits<double>::infinity();
 constexpr double weight[features]={3.5,1.4,.8,.15,.75,.75,.75,.75};
 double sum=0.;for(size_t i=0;i<a.size();i++){
  const double x=double(a[i])-b[i];
  // Clipped robust loss, prevents a few very quiet frames dominating.
  const double v=std::min(8.,std::abs(x));sum+=weight[i%features]*v*v;
 }
 return sum/(a.size()/features);
}
Result infer(const Example* train,int trainCount,const Example* hold,int holdCount,Settings settings){
 Result result;
 if(!train||trainCount<1||trainCount>8||holdCount<0||holdCount>8||(holdCount>0&&!hold)||settings.evaluations<1||settings.evaluations>128||settings.proposals<1||settings.proposals>5)return result;
 for(int i=0;i<trainCount;i++)if(!train[i].audio||!validate(train[i]))return result;
 for(int i=0;i<holdCount;i++)if(!hold[i].audio||!validate(hold[i]))return result;
 // Input audio descriptors are computed once. Holdout is NEVER evaluated during search.
 std::vector<std::vector<float>> reference;reference.reserve(size_t(trainCount));
 for(int i=0;i<trainCount;i++)reference.push_back(describe(train[i].audio,train[i].frames,train[i].sampleRate));
 RNG rng(settings.seed);std::vector<Entry> archive;
 std::vector<float> rendered;
 for(int iteration=0;iteration<settings.evaluations;iteration++){
  native27::Graph g;
  if(iteration<8)g=initial(iteration);
  else{
   // Balance exploration with exploitation; mutate best third, not only global best.
   if(iteration%5==0||archive.empty())g=initial(rng.integer(8));
   else g=archive[size_t(rng.integer(std::max(1,int(archive.size()/3))))].graph;
   for(int changes=1+rng.integer(3);changes>0;--changes)mutate(g,rng);
  }
  std::string why;std::map<int,std::unique_ptr<native27::Instrument>> inst;bool valid=true;
  for(int e=0;e<trainCount;e++){
   const int rate=train[e].sampleRate;
   if(inst.count(rate))continue;
   auto i=native27::Instrument::prepare(g,rate,&why);
   if(!i){valid=false;break;}inst.emplace(rate,std::move(i));
  }
  if(!valid)continue;
  double loss=0.;
  for(int e=0;e<trainCount;e++){
   if(!renderPrepared(*inst.at(train[e].sampleRate),train[e],rendered)){valid=false;break;}
   auto desc=describe(rendered.data(),int(rendered.size()),train[e].sampleRate);
   const double d=distance(reference[size_t(e)],desc);
   if(!good(d)){valid=false;break;}
   loss+=d/trainCount;
  }
  if(!valid)continue;
  ++result.evaluated;
  archive.push_back({std::move(g),loss,signature(g)});
  std::sort(archive.begin(),archive.end(),[](const Entry& a,const Entry& b){return a.loss<b.loss;});
  if(archive.size()>40)archive.resize(40);
 }
 if(archive.empty())return result;
 {std::vector<std::uint64_t> seen;for(const auto&e:archive)if(std::find(seen.begin(),seen.end(),e.topology)==seen.end())seen.push_back(e.topology);result.distinctTopologies=int(seen.size());}
 // Extract structurally different results. No holdout peek until selection is done.
 const double best=archive.front().loss;
 for(const auto&e:archive){
  if(int(result.proposals.size())>=settings.proposals)break;
  if(!result.proposals.empty()&&e.loss>std::max(best*2.5,best+1.5))continue;
  bool duplicate=false;for(const auto& p:result.proposals)if(p.topology==e.topology)duplicate=true;
  if(duplicate)continue;
  Proposal p;p.graph=e.graph;p.trainingLoss=e.loss;p.topology=e.topology;p.novelty=result.proposals.empty()?0.:1.;
  // Only now run the explicitly held-out performances.
  double holdLoss=0.;for(int j=0;j<holdCount;j++){
   if(!render(p.graph,hold[j],rendered)) {holdLoss=std::numeric_limits<double>::infinity();break;}
   holdLoss+=distance(describe(hold[j].audio,hold[j].frames,hold[j].sampleRate),describe(rendered.data(),int(rendered.size()),hold[j].sampleRate))/holdCount;
  }
  p.unseenLoss=holdCount>0?holdLoss:std::numeric_limits<double>::quiet_NaN();
  result.proposals.push_back(std::move(p));
 }
 result.success=!result.proposals.empty();result.trainCount=trainCount;result.holdoutCount=holdCount;
 return result;
}
} // namespace panorama::temporal28
