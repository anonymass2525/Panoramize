#include "GraphInverse.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <limits>
#include <sstream>
#include <unordered_set>
#include <vector>

namespace panorama {
namespace {
constexpr int kAudioFrames=4096,kWindow=768,kBins=28,kRegions=3;
constexpr double pi=3.1415926535897932384626433832795;
using Descriptor=std::array<double,kBins*kRegions+kRegions>;
struct Rng {
 std::uint32_t state;
 explicit Rng(std::uint32_t s):state(s?s:1u){}
 std::uint32_t next() noexcept {state^=state<<13;state^=state>>17;state^=state<<5;return state;}
 int index(int n) noexcept {return int(next()%std::uint32_t(n));}
 float unit() noexcept {return float(next()&0xffffu)/65535.f;}
};
constexpr NodeType kSources[]={NodeType::VA,NodeType::Harmonic,NodeType::PhaseWarp,
 NodeType::Digital,NodeType::Noise,NodeType::SpectralMatter,NodeType::PhaseEcology,
 NodeType::PhysicalBody,NodeType::MeshBody};
constexpr NodeType kEffects[]={NodeType::LowPass,NodeType::Fold,NodeType::Modal,
 NodeType::Gain,NodeType::Mix,NodeType::Ring,NodeType::CoupledModes,
 NodeType::ResonantExchange,NodeType::FormantField,NodeType::MatterWeave};
bool spectralNode(NodeType type) noexcept {
 return type==NodeType::SpectralMatter||type==NodeType::MatterWeave||
        type==NodeType::FormantField||type==NodeType::PhaseMatterBridge;
}
bool dualNode(NodeType type) noexcept {
 return type==NodeType::Mix||type==NodeType::Ring||
        type==NodeType::CoupledModes||type==NodeType::ResonantExchange;
}
// Create safe semantic graph nodes. Typed consumers are only installed when
// their source domain matches the compileGraph contract.
NodeType suitableEffect(Rng& rng,const GraphPatch& g,int a,int b) noexcept {
 for(int i=0;i<30;i++){
  const auto type=kEffects[rng.index(int(sizeof(kEffects)/sizeof(kEffects[0])))];
  if((type==NodeType::FormantField||type==NodeType::MatterWeave)&&
     (a<0||!spectralNode(g.nodes[(size_t)a].type)))continue;
  if(type==NodeType::MatterWeave&&b>=0&&!spectralNode(g.nodes[(size_t)b].type))continue;
  return type;
 }
 return NodeType::LowPass;
}
GraphPatch makeTemplate(int model,float color,float cutoff) noexcept {
 GraphPatch g{};
 const auto osc=[&](NodeType t,float c){g.nodes[0]={t,-1,-1,.82f,c};};
 const auto effect=[&](NodeType t,float c){g.nodes[1]={t,0,-1,.83f,c};};
 switch(model){
 case 0:osc(NodeType::VA,color);break;
 case 1:osc(NodeType::Harmonic,color);break;
 case 2:osc(NodeType::PhaseWarp,color);break;
 case 3:osc(NodeType::Digital,color);break;
 case 4:osc(NodeType::SpectralMatter,color);break;
 case 5:osc(NodeType::VA,color);effect(NodeType::LowPass,cutoff);break;
 case 6:osc(NodeType::Harmonic,color);effect(NodeType::Fold,cutoff);break;
 case 7:osc(NodeType::Noise,color);effect(NodeType::Modal,cutoff);break;
 case 8:osc(NodeType::SpectralMatter,color);effect(NodeType::FormantField,cutoff);break;
 default:break;
 }
 g.nodes[8]={NodeType::Output,model>=5?1:0,-1,.88f,0.f};
 return g;
}
Patch makeSearchPatch(int model,float color,float cutoff) noexcept {
 Patch p=factoryPresets()[0];
 p.name="Inverse Graph / generated";
 p.engine=Engine::Modular;
 p.attack=.01f;p.decay=.35f;p.sustain=.73f;p.release=.5f;
 p.gain=.62f;p.brightness=.7f;p.cutoff=.69f;p.motion=.35f;
 p.modRoutes={};
 p.graph=makeTemplate(model,color,cutoff);
 return p;
}
int activeNodes(const GraphPatch& g) noexcept {
 int n=0;
 for(int i=0;i<8;i++)if(g.nodes[(size_t)i].type!=NodeType::Off)n=i+1;
 return n;
}
// A canonical, easily editable path: occupied nodes are consecutive before
// Output. DAG topology can branch/rejoin and all connections are checked by DSP.
bool mutate(Patch& p,Rng& rng,int maxNodes) noexcept {
 auto& g=p.graph;
 int n=activeNodes(g);
 if(n<1||n>8)return false;
 const int operation=rng.index(9);
 if(operation==0){ // oscillator / processing node family
  const int k=rng.index(n);
  auto& node=g.nodes[(size_t)k];
  if(k==0 || node.a<0){
   node.type=kSources[rng.index(int(sizeof(kSources)/sizeof(kSources[0])))];
   node.a=node.b=-1;
  }else{
   node.type=suitableEffect(rng,g,node.a,node.b);
   if(!dualNode(node.type)&&node.type!=NodeType::MatterWeave)node.b=-1;
   if(node.type==NodeType::FormantField)node.b=-1;
  }
 }else if(operation==1){ // edge rewiring
  const int k=1+rng.index(n); // includes Output
  auto& node=g.nodes[(size_t)(k==n?8:k)];
  if(k==n || node.a>=0){node.a=rng.index(k);}
  else return false;
 }else if(operation==2){ // secondary edge: parallel interaction
  const int k=1+rng.index(n-1>0?n-1:1);
  if(k>=n)return false;
  auto& node=g.nodes[(size_t)k];
  if(!dualNode(node.type) && node.type!=NodeType::MatterWeave)return false;
  node.b=rng.index(k);
 }else if(operation==3){ // append serial processor
  if(n>=maxNodes)return false;
  const int parent=g.nodes[8].a;
  g.nodes[(size_t)n]={suitableEffect(rng,g,parent,-1),parent,-1,
                     .55f+.35f*rng.unit(),rng.unit()};
  g.nodes[8].a=n;
 }else if(operation==4){ // append source and mixer as parallel branch
  if(n+2>maxNodes)return false;
  const int prior=g.nodes[8].a;
  g.nodes[(size_t)n]={kSources[rng.index(int(sizeof(kSources)/sizeof(kSources[0])))],-1,-1,
                       .5f+.35f*rng.unit(),rng.unit()};
  g.nodes[(size_t)(n+1)]={NodeType::Mix,prior,n,.6f+.35f*rng.unit(),rng.unit()};
  g.nodes[8].a=n+1;
 }else if(operation==5){ // truncate tail / simplify
  if(n<2)return false;
  const int tail=n-1;
  // No dangling references to removed node allowed.
  for(int j=0;j<tail;j++)if(g.nodes[(size_t)j].a==tail||g.nodes[(size_t)j].b==tail)return false;
  g.nodes[(size_t)tail]=GraphNode{};
  for(int j=0;j<tail;j++){
   if(g.nodes[(size_t)j].a==tail)g.nodes[(size_t)j].a=-1;
   if(g.nodes[(size_t)j].b==tail)g.nodes[(size_t)j].b=-1;
  }
  g.nodes[8].a=tail-1;
 }else if(operation==6){ // level, color, independently of graph structure
  auto& node=g.nodes[(size_t)rng.index(n)];
  float& value=(rng.index(2)?node.color:node.level);
  value=std::clamp(value+(rng.unit()-.5f)*.42f,.05f,.98f);
 }else if(operation==7){ // output taps a different stage / shortcut
  g.nodes[8].a=rng.index(n);
 }else{ // synthesis patch parameters, not just graph connections
  float* params[]={&p.brightness,&p.cutoff,&p.motion,&p.fm,&p.fold,&p.resonance,
                   &p.resonanceBody,&p.damping,&p.attack,&p.sustain};
  float& param=*params[rng.index(int(sizeof(params)/sizeof(params[0])))];
  param=std::clamp(param+(rng.unit()-.5f)*.35f,.02f,.98f);
 }
 return true;
}
// Hash every audible explored property, avoiding repeated DSP evaluations.
std::uint64_t fingerprint(const Patch& p) noexcept {
 std::uint64_t h=14695981039346656037ull;
 const auto put=[&h](std::uint32_t v){for(int i=0;i<4;i++){h^=std::uint8_t(v>>(i*8));h*=1099511628211ull;}};
 const auto scalar=[&](float f){std::uint32_t b;std::memcpy(&b,&f,sizeof(b));put(b);};
 for(const auto& n:p.graph.nodes){put(std::uint32_t(n.type));put(std::uint32_t(n.a));put(std::uint32_t(n.b));scalar(n.level);scalar(n.color);}
 for(float f:{p.brightness,p.cutoff,p.motion,p.fm,p.fold,p.resonance,p.resonanceBody,p.damping,p.attack,p.sustain})scalar(f);
 return h;
}
// Goertzel features. Hann window is computed only once, not once per band.
Descriptor features(const float* pcm,int count) noexcept {
 Descriptor f{};
 constexpr int starts[kRegions]={0,1152,2688};
 double rms[kRegions]{};
 static const auto hann=[] {
  std::array<double,kWindow> w{};
  for(int i=0;i<kWindow;i++)w[(size_t)i]=.5-.5*std::cos(2*pi*i/(kWindow-1));
  return w;
 }();
 for(int block=0;block<kRegions;block++){
  const int start=std::min(starts[block],std::max(0,count-kWindow));
  double power=0.;
  for(int i=0;i<kWindow;i++){const double x=pcm[start+i];power+=x*x;}
  rms[block]=std::sqrt(power/kWindow);
  double total=0.;
  for(int k=1;k<=kBins;k++){
   const double coefficient=2*std::cos(2*pi*k/kWindow);
   double s0=0.,s1=0.,s2=0.;
   for(int i=0;i<kWindow;i++){
    s0=double(pcm[start+i])*hann[(size_t)i]+coefficient*s1-s2;
    s2=s1;s1=s0;
   }
   const double band=std::sqrt(std::max(0.,s1*s1+s2*s2-coefficient*s1*s2));
   f[(size_t)(block*kBins+k-1)]=band;
   total+=band;
  }
  if(total>1.e-14)for(int k=0;k<kBins;k++)f[(size_t)(block*kBins+k)]/=total;
 }
 const double sum=rms[0]+rms[1]+rms[2];
 if(sum>1.e-12)for(int i=0;i<kRegions;i++)f[(size_t)(kBins*kRegions+i)]=rms[i]/sum;
 return f;
}
double distance(const Descriptor&a,const Descriptor&b) noexcept {
 double e=0.;
 for(int i=0;i<kBins*kRegions;i++){
  const double d=a[(size_t)i]-b[(size_t)i];e+=d*d;
 }
 for(int i=kBins*kRegions;i<(int)a.size();i++){
  const double d=a[(size_t)i]-b[(size_t)i];e+=.18*d*d;
 }
 return e;
}
struct Candidate { Patch patch; double score; double cost; std::uint64_t topology; };
// Only nodes causally connected to the final output count as structural
// novelty. Otherwise unused/disconnected nodes would produce false novelty.
std::array<bool,kGraphNodes> reachable(const GraphPatch&g) noexcept {
 std::array<bool,kGraphNodes> used{};
 std::array<int,kGraphNodes> stack{};
 int size=0;stack[(size_t)size++]=8;used[8]=true;
 while(size>0){
  const int i=stack[(size_t)--size];
  const auto&node=g.nodes[(size_t)i];
  for(int source:{node.a,node.b}){
   if(source<0||source>=kGraphNodes||used[(size_t)source])continue;
   used[(size_t)source]=true;
   stack[(size_t)size++]=source;
  }
 }
 return used;
}
std::uint64_t audibleTopologyFingerprint(const GraphPatch&g) noexcept {
 const auto used=reachable(g);
 std::uint64_t h=14695981039346656037ull;
 const auto put=[&](std::uint32_t v){
  for(int shift=0;shift<32;shift+=8){h^=std::uint8_t(v>>shift);h*=1099511628211ull;}
 };
 for(int i=0;i<kGraphNodes;i++){
  if(!used[(size_t)i])continue;
  const auto&n=g.nodes[(size_t)i];
  put(std::uint32_t(i));put(std::uint32_t(n.type));
  put(std::uint32_t(n.a));put(std::uint32_t(n.b));
 }
 return h;
}
int reachableActive(const GraphPatch&g) noexcept {
 const auto r=reachable(g);int n=0;
 for(int i=0;i<8;i++)n+=int(r[(size_t)i]);
 return n;
}
double structuralDistance(const GraphPatch&a,const GraphPatch&b) noexcept {
 const auto ra=reachable(a),rb=reachable(b);
 double delta=0.,total=0.;
 for(int i=0;i<kGraphNodes;i++){
  if(!ra[(size_t)i]&&!rb[(size_t)i])continue;
  total+=3.0;
  if(ra[(size_t)i]!=rb[(size_t)i]){delta+=3.0;continue;}
  const auto&an=a.nodes[(size_t)i],&bn=b.nodes[(size_t)i];
  delta+=double(an.type!=bn.type);
  delta+=double(an.a!=bn.a);
  delta+=double(an.b!=bn.b);
 }
 return total>0?delta/total:0.;
}
} // namespace
GraphInversePortfolioResult inferGraphPortfolioFromAudioEx(const float*samples,int count,double sampleRate,
                                        int midiNote,GraphInverseSettings settings,
                                        int maxProposals,double maxRelativeError) noexcept {
 GraphInversePortfolioResult portfolio{};
 GraphInverseResult& result=portfolio.best;
 result.midiNote=midiNote;result.seed=settings.seed;
 if(!samples||count<kAudioFrames||midiNote<0||midiNote>127||
    !std::isfinite(sampleRate)||sampleRate<8000||sampleRate>192000||
    settings.maxEvaluations<1||settings.maxEvaluations>4096||
    settings.maxActiveNodes<1||settings.maxActiveNodes>8||
    maxProposals<1||maxProposals>8||!std::isfinite(maxRelativeError)||
    maxRelativeError<1.||maxRelativeError>100.)return portfolio;
 const int scan=std::min(count,int(sampleRate));
 double peak=0.;
 for(int i=0;i<scan;i++){
  if(!std::isfinite(samples[i]))return portfolio;
  peak=std::max(peak,std::abs(double(samples[i])));
 }
 if(peak<1.e-6)return portfolio;
 int onset=0;
 const double threshold=std::max(peak*.012,1.e-6);
 while(onset<scan&&std::abs(double(samples[onset]))<threshold)onset++;
 onset=std::max(0,onset-32);
 if(count-onset<kAudioFrames)return portfolio;
 const Descriptor target=features(samples+onset,kAudioFrames);
 std::array<float,kAudioFrames> left{},right{};
 std::array<int,kGraphNodes> order{};
 std::vector<Candidate> elites;elites.reserve(12);
 std::vector<Candidate> archive;archive.reserve(256);
 std::unordered_set<std::uint64_t> visited,topologies;
 visited.reserve(size_t(settings.maxEvaluations)*2);
 topologies.reserve(size_t(settings.maxEvaluations));
 Rng rng(settings.seed);
 constexpr float colors[]={.25f,.5f,.75f},filters[]={.35f,.7f};
 const auto evaluate=[&](const Patch& p,bool mutated) noexcept -> bool {
  if(result.evaluated>=settings.maxEvaluations)return false;
  if(activeNodes(p.graph)>settings.maxActiveNodes)return false;
  if(!compileGraph(p.graph,order))return false;
  if(!validateModRoutes(p.graph,p.modRoutes))return false;
  if(!visited.insert(fingerprint(p)).second)return false;
  Synth synth;synth.prepare(sampleRate);synth.setPatch(p);if(!synth.graphValid())return false;
  synth.noteOn(midiNote,.8f);synth.render(left.data(),right.data(),kAudioFrames);
  for(float x:left)if(!std::isfinite(x))return false;
  double rms=0.;for(float x:left)rms+=double(x)*x;
  if(rms<1.e-14)return false; // exclude silent candidates
  const double score=distance(features(left.data(),kAudioFrames),target);
  const double cost=score+1.e-4*double(activeNodes(p.graph)-1);
  ++result.evaluated;
  const auto topo=audibleTopologyFingerprint(p.graph);
  if(topologies.insert(topo).second)++result.novelTopologiesEvaluated;
  if(mutated)++result.mutatedEvaluated;
  if(score<result.distance){result.distance=score;result.patch=p;result.success=true;}
  const Candidate candidate{p,score,cost,topo};
  // Retain the best acoustic fit for each audible topology. Keep a bounded
  // archive; no waveform data or unbounded histories enter the audio callback.
  auto it=std::find_if(archive.begin(),archive.end(),
      [&](const Candidate&c){return c.topology==topo;});
  if(it!=archive.end()){
   if(score<it->score)*it=candidate;
  }else if(archive.size()<256){archive.push_back(candidate);}
  else{
   auto worst=std::max_element(archive.begin(),archive.end(),
      [](const Candidate&a,const Candidate&b){return a.score<b.score;});
   if(score<worst->score)*worst=candidate;
  }
  if(elites.size()<12||cost<elites.back().cost){
   Candidate c=candidate;
   const auto place=std::lower_bound(elites.begin(),elites.end(),cost,
       [](const Candidate& x,double value){return x.cost<value;});
   elites.insert(place,c);if(elites.size()>12)elites.pop_back();
  }
  return true;
 };
 // Retain v1.7's 54 concrete comparisons as reproducible warm starts.
 for(int model=0;model<9&&result.evaluated<settings.maxEvaluations;model++)
  for(float color:colors)for(float filter:filters)
   evaluate(makeSearchPatch(model,color,filter),false);
 // Directed evolutionary neighborhood: mutate TYPES, EDGES, number of nodes,
 // parallel branches and timbral controls. Deterministic seed + finite budget.
 int attempts=0;
 while(result.evaluated<settings.maxEvaluations&&attempts<settings.maxEvaluations*120){
  ++attempts;
  Patch child;
  if(!elites.empty()&&rng.index(8)!=0){
   const int focus=std::min(int(elites.size()),6);
   child=elites[(size_t)rng.index(focus)].patch;
  }else{
   child=makeSearchPatch(rng.index(9),colors[rng.index(3)],filters[rng.index(2)]);
  }
  const int edits=1+rng.index(4); // occasionally jump out of local optima
  bool changed=false;
  for(int e=0;e<edits;e++)changed|=mutate(child,rng,settings.maxActiveNodes);
  if(!changed)continue;
  evaluate(child,true);
 }
 // Keep the closest sound first. Later slots trade some feature accuracy
 // for genuinely different connected architectures, using deterministic
 // diversity-aware selection (not duplicate parameterizations).
 if(result.success){
  portfolio.distinctAcceptedStructures=int(archive.size());
  const double maximum=result.distance*maxRelativeError+0.035;
  std::vector<bool> selected(archive.size(),false);
  for(int slot=0;slot<maxProposals;slot++){
   int pick=-1;double bestUtility=-std::numeric_limits<double>::infinity();
   double chosenNovelty=0.;
   for(size_t i=0;i<archive.size();i++){
    if(selected[i]||archive[i].score>maximum+1.e-12)continue;
    double novelty=1.;
    for(const auto&prior:portfolio.proposals)
     novelty=std::min(novelty,structuralDistance(archive[i].patch.graph,prior.patch.graph));
    // first slot always best fit. Later slots reward topology difference
    // while imposing the maximum acoustic error constraint above.
    const double utility=(slot==0?-archive[i].score:
          1.4*novelty - 0.55*archive[i].score/(result.distance+.05));
    if(utility>bestUtility){bestUtility=utility;pick=int(i);chosenNovelty=novelty;}
   }
   if(pick<0)break;
   selected[(size_t)pick]=true;
   const auto&c=archive[(size_t)pick];
   portfolio.proposals.push_back({c.patch,c.score,slot?chosenNovelty:0.,
                                  c.topology,reachableActive(c.patch.graph)});
  }
 }
 return portfolio;
}
GraphInverseResult inferGraphFromAudioEx(const float*samples,int count,double sampleRate,
                                        int midiNote,GraphInverseSettings settings) noexcept {
 return inferGraphPortfolioFromAudioEx(samples,count,sampleRate,midiNote,settings,1).best;
}
GraphInverseResult inferGraphFromAudio(const float*samples,int count,double sampleRate,int midiNote) noexcept {
 return inferGraphFromAudioEx(samples,count,sampleRate,midiNote,GraphInverseSettings{});
}
std::string graphInverseJson(const GraphInverseResult&r){
 if(!r.success)return "{}\n";
 const auto&p=r.patch;
 std::ostringstream o;o<<std::setprecision(9);
 o<<"{\n  \"format\": \"PANORAMA-GraphInverse-v1\",\n"
  <<"  \"name\": \"Inverse Graph / generated\",\n"
  <<"  \"midiNote\": "<<r.midiNote<<",\n"
  <<"  \"searchCandidates\": "<<r.evaluated<<",\n"
  <<"  \"topologyMutationsEvaluated\": "<<r.mutatedEvaluated<<",\n"
  <<"  \"distinctTopologiesEvaluated\": "<<r.novelTopologiesEvaluated<<",\n"
  <<"  \"searchSeed\": "<<r.seed<<",\n"
  <<"  \"featureDistance\": "<<r.distance<<",\n"
  <<"  \"engine\": "<<int(p.engine)<<",\n"
  <<"  \"parameters\": {\"attack\": "<<p.attack<<", \"decay\": "<<p.decay
  <<", \"sustain\": "<<p.sustain<<", \"release\": "<<p.release
  <<", \"gain\": "<<p.gain<<", \"cutoff\": "<<p.cutoff
  <<", \"motion\": "<<p.motion<<", \"brightness\": "<<p.brightness
  <<", \"resonance\": "<<p.resonance<<", \"detune\": "<<p.detune
  <<", \"unison\": "<<p.unison<<", \"fm\": "<<p.fm
  <<", \"fold\": "<<p.fold<<", \"resonanceBody\": "<<p.resonanceBody
  <<", \"damping\": "<<p.damping<<", \"blend\": "<<p.blend
  <<", \"stereoResonance\": "<<p.stereoResonance<<", \"crossDepth\": "<<p.crossDepth
  <<", \"routing\": "<<p.routing<<"},\n"
  <<"  \"nodes\": [\n";
 for(int i=0;i<kGraphNodes;i++){
  const auto&n=p.graph.nodes[(size_t)i];
  o<<"    {\"index\": "<<i<<", \"type\": "<<int(n.type)
   <<", \"a\": "<<n.a<<", \"b\": "<<n.b
   <<", \"level\": "<<n.level<<", \"color\": "<<n.color<<"}"
   <<(i==kGraphNodes-1?"\n":",\n");
 }
 o<<"  ]\n}\n";
 return o.str();
}
} // namespace panorama


namespace panorama {
namespace {
constexpr int kInstrumentFrames=4096;
struct InstrumentGoal {
 InstrumentExample example;
 Descriptor descriptor{};
 double rms=0.;
};
bool validExample(const InstrumentExample& e) noexcept {
 if(!e.samples||e.count<kInstrumentFrames||e.count>2000000||
   e.midiNote<0||e.midiNote>127||e.velocity<.01f||e.velocity>1.f||
   e.noteOffFrame<1||e.noteOffFrame>kInstrumentFrames||
   !std::isfinite(e.sampleRate)||e.sampleRate<8000||e.sampleRate>192000)return false;
 for(int i=0;i<kInstrumentFrames;i++)if(!std::isfinite(e.samples[i]))return false;
 return true;
}
double rmsValue(const float* p) noexcept {
 double power=0.;for(int i=0;i<kInstrumentFrames;i++)power+=double(p[i])*p[i];
 return std::sqrt(power/kInstrumentFrames);
}
// Unlike a single-note spectral fingerprint, this loss examines independent
// note-off/velocity conditions, preserving relative level and decay differences.
double gestureLoss(const Descriptor& a,const Descriptor& b,double rmsA,double rmsB) noexcept {
 const double spectral=distance(a,b);
 const double loudness=std::log((rmsA+1.e-6)/(rmsB+1.e-6));
 return spectral+0.022*loudness*loudness;
}
std::vector<InstrumentGoal> prepareGoals(const InstrumentExample* ex,int n) {
 std::vector<InstrumentGoal> goals;goals.reserve(size_t(n));
 for(int i=0;i<n;i++){
  InstrumentGoal g{};g.example=ex[i];g.descriptor=features(ex[i].samples,kInstrumentFrames);
  g.rms=rmsValue(ex[i].samples);goals.push_back(g);
 }
 return goals;
}
// mean + worst-case guard: do not optimize one loud note while sacrificing
// soft attacks or short note-offs. Each example is weighted equally.
double multiGestureLoss(const Patch& patch,const std::vector<InstrumentGoal>& goals,
                        std::array<float,kInstrumentFrames>& pcm) noexcept {
 double total=0.,worst=0.;
 for(const auto&g:goals){
  if(!renderInstrumentExample(patch,g.example,pcm.data(),kInstrumentFrames))
   return std::numeric_limits<double>::infinity();
  const double r=rmsValue(pcm.data());
  if(!std::isfinite(r)||r<1.e-8)return std::numeric_limits<double>::infinity();
  const double d=gestureLoss(features(pcm.data(),kInstrumentFrames),g.descriptor,r,g.rms);
  if(!std::isfinite(d))return std::numeric_limits<double>::infinity();
  total+=d;worst=std::max(worst,d);
 }
 return total/double(goals.size())+0.20*worst;
}
struct InstrumentCandidate {Patch patch;double score;std::uint64_t topology;};
} // namespace
bool renderInstrumentExample(const Patch& patch,const InstrumentExample& gesture,
                             float* output,int frames) noexcept {
 if(!output||frames<=0||frames>kInstrumentFrames||!std::isfinite(gesture.sampleRate)||
    gesture.sampleRate<8000||gesture.sampleRate>192000||gesture.midiNote<0||
    gesture.midiNote>127||gesture.velocity<.01f||gesture.velocity>1.f||
    gesture.noteOffFrame<1||gesture.noteOffFrame>kInstrumentFrames)return false;
 Synth synth;synth.prepare(gesture.sampleRate);synth.setPatch(patch);
 if(!synth.graphValid())return false;
 synth.noteOn(gesture.midiNote,gesture.velocity);
 for(int i=0;i<frames;i++){
  if(i==gesture.noteOffFrame)synth.noteOff(gesture.midiNote);
  const auto out=synth.processOne();
  output[i]=.5f*(out.left+out.right);
  if(!std::isfinite(output[i]))return false;
 }
 return true;
}
PlayableInstrumentResult inferPlayableInstruments(
 const InstrumentExample* training,int trainingCount,
 const InstrumentExample* holdout,int holdoutCount,
 InstrumentInverseSettings settings) noexcept {
 PlayableInstrumentResult result{};result.seed=settings.seed;
 if(!training||trainingCount<2||trainingCount>6||holdoutCount<0||holdoutCount>3||
    (holdoutCount>0&&!holdout)||settings.maxEvaluations<1||settings.maxEvaluations>512||
    settings.maxActiveNodes<1||settings.maxActiveNodes>8||
    settings.maxProposals<1||settings.maxProposals>6)return result;
 bool varied=false;
 for(int i=0;i<trainingCount;i++){
  if(!validExample(training[i])||rmsValue(training[i].samples)<1.e-6)return result;
  if(i>0 && (training[i].midiNote!=training[0].midiNote ||
    training[i].velocity!=training[0].velocity ||
    training[i].noteOffFrame!=training[0].noteOffFrame))varied=true;
 }
 if(!varied)return result;
 for(int i=0;i<holdoutCount;i++)if(!validExample(holdout[i])||
    rmsValue(holdout[i].samples)<1.e-6)return result;
 result.trainingExamples=trainingCount;result.holdoutExamples=holdoutCount;
 const auto goals=prepareGoals(training,trainingCount);
 const auto validation=prepareGoals(holdout,holdoutCount);
 std::array<float,kInstrumentFrames> scratch{};
 std::array<int,kGraphNodes> order{};
 std::unordered_set<std::uint64_t> visited,topologies;
 visited.reserve(size_t(settings.maxEvaluations)*2);
 topologies.reserve(size_t(settings.maxEvaluations));
 std::vector<InstrumentCandidate> elite,archive;
 elite.reserve(10);archive.reserve(size_t(settings.maxEvaluations));
 Rng rng(settings.seed);
 const auto evaluate=[&](const Patch& p,bool mutated) noexcept {
  if(result.evaluated>=settings.maxEvaluations||activeNodes(p.graph)>settings.maxActiveNodes||
      !compileGraph(p.graph,order)||!validateModRoutes(p.graph,p.modRoutes)||
      !visited.insert(fingerprint(p)).second)return;
  const double score=multiGestureLoss(p,goals,scratch);
  if(!std::isfinite(score))return;
  result.evaluated++;
  if(mutated)result.mutatedEvaluated++;
  const auto topology=audibleTopologyFingerprint(p.graph);
  if(topologies.insert(topology).second)result.distinctTopologies++;
  InstrumentCandidate candidate{p,score,topology};
  const auto inArchive=std::find_if(archive.begin(),archive.end(),
     [&](const InstrumentCandidate& c){return c.topology==topology;});
  if(inArchive==archive.end())archive.push_back(candidate);
  else if(score<inArchive->score)*inArchive=candidate;
  const auto rankScore=[&](const InstrumentCandidate& c){
   return c.score+0.0001*reachableActive(c.patch.graph);
  };
  if(elite.size()<10 || rankScore(candidate)<rankScore(elite.back())){
   const auto it=std::lower_bound(elite.begin(),elite.end(),rankScore(candidate),
       [&](const InstrumentCandidate& c,double v){return rankScore(c)<v;});
   elite.insert(it,candidate);
   if(elite.size()>10)elite.pop_back();
  }
 };
 constexpr float colors[]={.25f,.5f,.75f},filters[]={.35f,.7f};
 for(int model=0;model<9&&result.evaluated<settings.maxEvaluations;model++)
  for(float color:colors)for(float filter:filters)
   evaluate(makeSearchPatch(model,color,filter),false);
 int attempts=0;
 while(result.evaluated<settings.maxEvaluations&&
       attempts<settings.maxEvaluations*120){
  ++attempts;Patch child{};
  if(!elite.empty()&&rng.index(8)!=0){
   const int focus=std::min(int(elite.size()),6);
   child=elite[(size_t)rng.index(focus)].patch;
  }else child=makeSearchPatch(rng.index(9),colors[rng.index(3)],filters[rng.index(2)]);
  bool changed=false;
  for(int e=0,edits=1+rng.index(4);e<edits;e++)
   changed|=mutate(child,rng,settings.maxActiveNodes);
  if(changed)evaluate(child,true);
 }
 std::stable_sort(archive.begin(),archive.end(),[](const auto&a,const auto&b){return a.score<b.score;});
 for(const auto& c:archive){
  if((int)result.proposals.size()>=settings.maxProposals)break;
  double novelty=1.;
  for(const auto& chosen:result.proposals)
   novelty=std::min(novelty,structuralDistance(c.patch.graph,chosen.patch.graph));
  if(!result.proposals.empty()&&novelty<.15)continue;
  const double held=validation.empty()?std::numeric_limits<double>::quiet_NaN():
                    multiGestureLoss(c.patch,validation,scratch);
  result.proposals.push_back({c.patch,c.score,held,result.proposals.empty()?0.:novelty,
                              c.topology,reachableActive(c.patch.graph)});
 }
 result.success=!result.proposals.empty();
 return result;
}
} // namespace panorama
