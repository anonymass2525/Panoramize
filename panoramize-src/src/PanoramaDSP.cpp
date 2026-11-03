#include "PanoramaDSP.h"
#include "FastDSPMath.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <tuple>
namespace panorama {
namespace {
constexpr double pi=3.14159265358979323846, twoPi=2*pi;
inline double clamp(double x,double lo,double hi){return std::max(lo,std::min(hi,x));}
inline double fract(double x){return x-std::floor(x);}
inline double polyBlep(double phase,double dt){
 if(phase<dt){double t=phase/dt;return t+t-t*t-1;}
 if(phase>1-dt){double t=(phase-1)/dt;return t*t+t+t+1;}
 return 0.0;
}
inline double noteHz(int midi){return 440.0*std::pow(2.0,(midi-69)/12.0);}
inline double dbSmooth(double x){return std::tanh(x*1.25)*.85;}
inline uint32_t hashStep(uint32_t& state){state^=state<<13;state^=state>>17;state^=state<<5;return state;}
inline double randBi(uint32_t& state){return ((hashStep(state)&0xffffff)/8388608.0)-1.0;}
}
bool isUnconfiguredModularGraph(const GraphPatch& graph) noexcept {
 // Do not overwrite a patch that has any declared node or any audio wiring.
 for(int i=0;i<kGraphNodes;i++){
  if(i<kGraphNodes-1 && graph.nodes[i].type!=NodeType::Off)return false;
  if(graph.nodes[i].a!=-1 || graph.nodes[i].b!=-1)return false;
 }
 return true;
}
GraphPatch makeStarterModularGraph() noexcept {
 // A deliberately audible, editable first patch: two complementary oscillators,
 // digital cross-synthesis, wavefolding and a resonant parallel path.
 GraphPatch graph{};
 graph.nodes[0]={NodeType::Harmonic,-1,-1,.85f,.42f};
 graph.nodes[1]={NodeType::PhaseWarp,-1,-1,.73f,.65f};
 graph.nodes[2]={NodeType::Mix,0,1,.84f,.38f};
 graph.nodes[3]={NodeType::Digital,0,-1,.61f,.27f};
 graph.nodes[4]={NodeType::Ring,2,3,.55f,.50f};
 graph.nodes[5]={NodeType::Mix,2,4,.85f,.25f};
 graph.nodes[6]={NodeType::Modal,5,-1,.68f,.48f};
 graph.nodes[7]={NodeType::Mix,5,6,.88f,.30f};
 graph.nodes[8]={NodeType::Output,7,-1,.88f,.25f};
 return graph;
}
bool compileGraph(const GraphPatch& graph,std::array<int,kGraphNodes>& order) noexcept {
 // Cyclic connections are allowed only through explicit one-sample FeedbackDelay.
 // Algebraic zero-delay cycles remain invalid.
 if(graph.nodes[kGraphNodes-1].type!=NodeType::Output)return false;
 std::array<int,kGraphNodes> inDegree{};
 for(int i=0;i<kGraphNodes;i++) {
   const auto &n=graph.nodes[i];
   if(static_cast<int>(n.type)<0||static_cast<int>(n.type)>24)return false;
   // Check indices BEFORE dereferencing typed-source nodes (host automation
   // and saved sessions can contain malformed indices).
   for(int source:{n.a,n.b})if(source < -1 || source>=kGraphNodes || source==i && n.type!=NodeType::FeedbackDelay)return false;
   if(n.type==NodeType::MatterWeave || n.type==NodeType::FormantField || n.type==NodeType::PhaseMatterBridge || n.type==NodeType::UnifiedCoupler){
     const auto spectralSource=[&graph](int source){
       return source>=0 && (graph.nodes[source].type==NodeType::SpectralMatter ||
                            graph.nodes[source].type==NodeType::MatterWeave ||
                            graph.nodes[source].type==NodeType::FormantField ||
                            graph.nodes[source].type==NodeType::PhaseMatterBridge ||
                            graph.nodes[source].type==NodeType::UnifiedCoupler);
     };
     if(!spectralSource(n.a) || (n.type==NodeType::MatterWeave && n.b>=0 && !spectralSource(n.b)))return false;
     // A PhaseMatterBridge needs a spectral-energy source and an explicit
     // typed phase source. Audio-only B signals are not phase frames.
     if(n.type==NodeType::PhaseMatterBridge){
       if(n.b<0)return false;
       const auto t=graph.nodes[n.b].type;
       if(t!=NodeType::PhaseEcology && !spectralSource(n.b))return false;
     }
     if(n.type==NodeType::UnifiedCoupler){
       if(n.b<0)return false;
       const auto t=graph.nodes[n.b].type;
       if(t!=NodeType::PhysicalBody && t!=NodeType::MeshBody && t!=NodeType::PassiveBody && t!=NodeType::UnifiedCoupler &&
          t!=NodeType::ResonantExchange && t!=NodeType::CoupledModes)return false;
     }
   }
   if(n.type==NodeType::FeedbackDelay && n.b!=-1)return false;
   for(int source:{n.a,n.b}) {
     if(source< -1||source>=kGraphNodes)return false;
     if(source==i && n.type!=NodeType::FeedbackDelay)return false;
     // The input is read from the PREVIOUS sample, not this topological pass.
     if(source>=0 && n.type!=NodeType::FeedbackDelay)inDegree[i]++;
   }
 }
 std::array<bool,kGraphNodes> visited{};
 for(int k=0;k<kGraphNodes;k++) {
   int chosen=-1;
   for(int i=0;i<kGraphNodes;i++)if(!visited[i]&&inDegree[i]==0){chosen=i;break;}
   if(chosen<0)return false;
   order[k]=chosen;visited[chosen]=true;
   for(int i=0;i<kGraphNodes;i++)if(!visited[i]){
     if(graph.nodes[i].type!=NodeType::FeedbackDelay){
       if(graph.nodes[i].a==chosen)inDegree[i]--;
       if(graph.nodes[i].b==chosen)inDegree[i]--;
     }
   }
 }
 return true;
}
bool validateModRoutes(const GraphPatch& graph,const std::array<ModRoute,2>& routes) noexcept {
 std::array<int,kGraphNodes> audioOrder{};
 if(!compileGraph(graph,audioOrder))return false;
 // Audio modulation dependencies extend the DAG. Reject control edges that close a cycle,
 // including those involving audio edges and both modulation lanes.
 std::array<std::array<bool,kGraphNodes>,kGraphNodes> reachable{};
 for(int target=0;target<kGraphNodes;target++){
  for(int upstream:{graph.nodes[target].a,graph.nodes[target].b})
   if(upstream>=0 && graph.nodes[target].type!=NodeType::FeedbackDelay)reachable[upstream][target]=true;
 }
 for(const auto&route:routes){
  if(route.source<0||route.source>5+kGraphNodes||route.target<0||route.target>=kGraphNodes||route.destination<0||route.destination>3)return false;
  if(!std::isfinite(route.depth)||route.depth<-1.f||route.depth>1.f)return false;
  // Muted modulation lanes carry no audio dependency; permit editing them freely.
  if(route.source>=6 && route.depth!=0.f){const int upstream=route.source-6;
   if(upstream==route.target)return false;
   reachable[upstream][route.target]=true;
  }
 }
 for(int via=0;via<kGraphNodes;via++)
  for(int from=0;from<kGraphNodes;from++)
   for(int to=0;to<kGraphNodes;to++)
    if(reachable[from][via]&&reachable[via][to])reachable[from][to]=true;
 for(int i=0;i<kGraphNodes;i++)if(reachable[i][i])return false;
 return true;
}
// v1.2: Compare all audible patch settings without memcmp (which would read
// padding) and without heap allocation. The UI can avoid re-compiling
// immutable filter, spectral and routing coefficients on every audio block.
bool samePatchConfiguration(const Patch& a,const Patch& b) noexcept {
 if(std::tie(a.name,a.engine,a.cutoff,a.resonance,a.attack,a.decay,a.sustain,a.release,
             a.detune,a.unison,a.fm,a.fold,a.resonanceBody,a.damping,a.motion,
             a.blend,a.brightness,a.gain,a.routing,a.crossDepth,a.stereoResonance,
             a.metalMix,a.metalColor,a.metalDecay,a.bellDynamics,a.bellCoupling,a.bellClarity)
   !=std::tie(b.name,b.engine,b.cutoff,b.resonance,b.attack,b.decay,b.sustain,b.release,
             b.detune,b.unison,b.fm,b.fold,b.resonanceBody,b.damping,b.motion,
             b.blend,b.brightness,b.gain,b.routing,b.crossDepth,b.stereoResonance,
             b.metalMix,b.metalColor,b.metalDecay,b.bellDynamics,b.bellCoupling,b.bellClarity))return false;
 for(int i=0;i<kGraphNodes;i++){
  const auto& x=a.graph.nodes[(size_t)i];const auto& y=b.graph.nodes[(size_t)i];
  if(std::tie(x.type,x.a,x.b,x.level,x.color)!=std::tie(y.type,y.a,y.b,y.level,y.color))return false;
 }
 for(int i=0;i<2;i++){
  const auto& x=a.modRoutes[(size_t)i];const auto& y=b.modRoutes[(size_t)i];
  if(std::tie(x.source,x.target,x.destination,x.depth)!=std::tie(y.source,y.target,y.destination,y.depth))return false;
 }
 return true;
}
void Synth::Envelope::trigger(const Patch& p,double sr){
 stage=Stage::Attack;value=0;
 const auto alpha=[sr](double seconds){return 1.0-std::exp(-1.0/(sr*std::max(.001,seconds)));};
 coefA=alpha(p.attack);coefD=alpha(p.decay);coefR=alpha(p.release*.45);
}
void Synth::Envelope::release(){if(stage!=Stage::Off) stage=Stage::Release;}
double Synth::Envelope::step(const Patch& p,double sr){
 if(stage==Stage::Off)return 0;
 (void)sr;
 switch(stage){
 case Stage::Attack: value+=(1.0-value)*coefA;if(value>.999) {value=1;stage=Stage::Decay;}break;
 case Stage::Decay: value+=(p.sustain-value)*coefD;if(std::fabs(p.sustain-value)<.0005) stage=Stage::Sustain;break;
 case Stage::Sustain: value+=(p.sustain-value)*.005;break;
 case Stage::Release: value+=(0.0-value)*coefR;if(value<.00008){value=0;stage=Stage::Off;}break;
 case Stage::Off:break;
 }
 return value;
}
void Synth::Voice::start(int midi,float velocity,int ch,uint64_t count,double sr,const Patch& p){
 active=true;releasedByKey=false;note=midi;channel=ch;age=count;vel=clamp(velocity,0,1);baseHz=noteHz(midi);
 for(int i=0;i<5;i++)ph[i]=fract(double(i)*.174+double((midi*17)%59)/97.);
 modPhase=0;subphase=0;lfoPhase=0;filt1=filt2=lpPrev=feedback=smear=crossDelay=0;noiseEnv=1;random=0x9E3779B9u^(uint32_t(midi+5)*0x85ebca6bu)^(uint32_t(count)*3u);
 env.trigger(p,sr);
 graphLfo=graphLfoB=0;
 for(int node=0;node<kGraphNodes;node++){
   auto& st=graphStates[node];st=GraphState{};
   st.matterPole=1.-std::exp(-1./(sr*.003));
   st.formantPole=1.-std::exp(-1./(sr*.0018));
   st.rng=random^(0x9e3779b9u*static_cast<uint32_t>(node+1));
   if(p.graph.nodes[(size_t)node].type==NodeType::PhysicalBody){
     st.physicalDisplacement=.026; st.physicalVelocityNorm=.012;
   }
   if(p.graph.nodes[(size_t)node].type==NodeType::MeshBody){
     // Three editable material families. The public MaterialMesh API can also
     // accept custom 3D coordinates, masses and arbitrary connection topology.
     const int shape=std::min(2,int(p.graph.nodes[(size_t)node].color*3.));
     st.mesh.configure(makeMeshTopology(shape,baseHz,.8+1.2*p.resonanceBody),sr);
     st.mesh.excite(0,8.);
   }
   if(p.graph.nodes[(size_t)node].type==NodeType::PassiveBody){
     // Two interacting spring/ring families, with explicit positive damping.
     // The intrinsic color is sampled at note-on (not sample-rate automated).
     PassiveMeshSettings cfg{};
     const auto&def=p.graph.nodes[(size_t)node];
     cfg.topology=makeMeshTopology(def.color<.5f?0:1,baseHz*.68,1.0+p.resonanceBody*.5);
     for(int j=0;j<cfg.topology.count;j++)cfg.groundDamping[(size_t)j]=.03+double(p.damping)*6.;
     for(int j=0;j<cfg.topology.edges;j++)cfg.edgeDamping[(size_t)j]=.01+double(def.color)*3.;
     st.passiveMesh.configure(cfg,sr);
     st.passiveMesh.excite(0,6.);
   }
   for(int j=0;j<3;j++)st.unisonPhase[j]=fract(ph[j]+node*.13);
   for(int j=0;j<6;j++)st.ecologyPhases[(size_t)j]=fract(ph[j%5]+.1618034*j+.037*node);
   for(int j=0;j<12;j++){
     st.matterPhases[(size_t)j]=fract(ph[j%5]+.0871*j+.041*node);
     st.matterEnergy[(size_t)j]=.58*std::exp(-.23*j);
   }
   for(int j=0;j<8;j++){
     auto&m=st.modal[j];
     constexpr double ratios[]={1.,1.493,2.11,2.98,4.21,5.37,6.94,9.43};
     const auto&def=p.graph.nodes[node];
     double tuning=.64+def.color*.85;
     double hz=clamp(baseHz*ratios[j]*tuning,25.,sr*.44);
     m.cosw=2.*std::cos(twoPi*hz/sr);
     m.r=clamp(1.-(0.00018+j*.00011)*(1.5-.93*def.color)*48000./sr,.90,.99997);
     m.energy=(1./(1.+j*.46))*((j%3==0)?1.:-.83);
   }
 }
 noiseDecay=std::exp(-1.0/(sr*(.014+.060*(1-p.damping))));
 // Inharmonic partial ratios deliberately tuned away from strict harmonic series.
 // The original eight-mode initialization remains exactly intact unless an
 // enhanced bell is expressly enabled for Fractal Digital.
 constexpr double ratios[]={1.,1.499,2.14,2.997,4.01,5.53,7.21,10.9};
 const bool enhancedBell=(p.engine==Engine::FractalDigital && p.metalMix>0.f);
 for(int i=0;i<8;i++){
   auto &m=modes[i];m.y1=m.y2=0;
   double shift=1.+.13*double(p.motion)*std::sin(i*2.5+midi*.21);
   if(enhancedBell){
     // Realistic-sounding plate partials are not constrained to harmonics.
     // This is an intentionally *stylized modal model*, not calibrated FEM.
     const double stretched=(ratios[i]-1.)*.015*(double(p.metalColor)-.5);
     shift*=1.+stretched;
   }
   const double hz=clamp(baseHz*ratios[i]*shift,40.,sr*.43);
   double q=clamp(.997-((.00008+i*.00018)+p.damping*(.00025+i*.0007))*48000./sr,.90,.99993);
   if(enhancedBell){
     // Per-partial T60: high metal partials fade faster than body modes.
     // A stable pole radius < 1 guarantees exponential decay without drive.
     const double t60=(.30+3.0*clamp(p.metalDecay,0.,1.))*std::exp(-.13*i);
     q=clamp(std::exp(-6.907755278982137/(sr*t60)),.90,.99998);
   }
   m.r=q; m.cosw=2.*std::cos(twoPi*hz/sr);
   m.energy=(1./(1.+i*.8))*((i%3==0)?1.:-.74);
 }
 // Bell 5.6 adds a companion modal family, so legacy 5.5 resonances remain
 // unmodified and all newer timbres can be bypassed without approximations.
 bellStrike=1.;
 bellDynamicsSmoothed=clamp(p.bellDynamics,0.,1.);
 bellSmoothCoef=1.-std::exp(-1./(sr*.012));
 bellStrikePole=std::exp(-1./(sr*(.0045+.006*bellDynamicsSmoothed)));
 const double angle=.00065*clamp(p.bellCoupling,0.,1.)*48000./sr;
 bellCouplingCos=std::cos(angle);bellCouplingSin=std::sin(angle);
 for(int i=0;i<8;i++){
   auto& b=companion[(size_t)i];b=BellMode{};
   const double iRatio=ratios[i]*(1.+.0035*std::sin(double(i)*2.17+.39));
   const double hz=baseHz*iRatio*(1.+.012*(p.metalColor-.5f));
   // Mute out-of-band companions rather than clamping them all to Nyquist.
   if(hz>sr*.40){b.strike=0.;b.pickup=0.;b.r=.995;continue;}
   const double w=twoPi*hz/sr;
   b.cw=std::cos(w);b.sw=std::sin(w);
   const double t60=(.35+3.5*clamp(p.metalDecay,0.,1.))
                  *std::exp(-double(i)*(.10+.19*clamp(p.bellClarity,0.,1.)));
   b.r=std::exp(-6.907755278982137/(sr*t60));
   b.strike=(1./(1.+i*.65))*((i%3==1)?-.78:1.);
   b.pickup=(1./(1.+i*.48))*((i%3==1)?-.80:1.);
 }
}
double Synth::Voice::process(double sr,const Patch& p,double bendMultiplier,double baseCutoff,float wheel,Sample& stereo,
    const std::array<int,kGraphNodes>& order,int renderCount,
    const std::array<double,kGraphNodes>& filterCoeffs,
    const std::array<std::array<double,12>,kGraphNodes>& weaveRatios,
    const std::array<std::array<double,12>,kGraphNodes>& matterFactors){
 if(!active)return 0;
 if(p.engine==Engine::Modular){ stereo=processGraph(sr,p,bendMultiplier,wheel,order,renderCount,filterCoeffs,weaveRatios,matterFactors);return .5*(stereo.left+stereo.right); }
 const double envVal=env.step(p,sr);
 if(!env.active()){active=false;return 0;}
 const double lfoRate=.11+2.3*p.motion + 3.5*wheel;
 lfoPhase=fract(lfoPhase+lfoRate/sr);
 double lfo=std::sin(twoPi*lfoPhase);
 const double freq=clamp(baseHz*bendMultiplier*(1.0+.00103972*p.motion*lfo),10.,sr*.43);
 const double dt=freq/sr;
 // Independent, band-limited VA oscillators; five unison oscillators per MIDI voice.
 double vaL=0,vaR=0;
 const int n=(p.unison>.34?5:3);
 for(int i=0;i<n;i++){
   double d=(i-(n-1)*.5)*p.detune*.011;
   const double di=clamp(dt*(1+d),.0000001,.48);
   double phase=ph[i];
   double saw=2*phase-1-polyBlep(phase,di);
   const double pulseWidth=.5+.16*std::sin(twoPi*(lfoPhase*.35+i*.067));
   double pulse=(phase<pulseWidth?1.:-1.)+polyBlep(phase,di)-polyBlep(fract(phase-pulseWidth),di);
   double osc=(.68*saw+.20*pulse+.12*std::sin(twoPi*phase));
   double pan=(i-(n-1)*.5)/((n-1)*.5);
   vaL+=osc*(.5-.22*pan);vaR+=osc*(.5+.22*pan);
   ph[i]=fract(phase+di);
 }
 vaL/=n*.5;vaR/=n*.5;
 const double va=.5*(vaL+vaR);
 // Nested modulator waveform -> phase modulation + iterative fold; bandwidth limited by 2x base.
 modPhase=fract(modPhase+clamp(dt*(1.50+2.5*p.fm),0.,.46));
 const double mod=std::sin(twoPi*modPhase)+.24*std::sin(twoPi*modPhase*2);
 // Routed feedback is explicitly delayed by one sample and hard-clamped.
 double phaseFeedback=(p.routing==3 && p.engine==Engine::Hybrid)
     ? crossDelay*(0.025+0.21*clamp(p.crossDepth,0.,1.))
     : feedback*.025;
 double carrier=std::sin(twoPi*(subphase+p.fm*.58*mod+phaseFeedback));
 double fractal=carrier;
 const int folds=1+(int)std::floor(p.fold*3.2);
 for(int i=0;i<folds;i++)fractal=std::sin((1.3+.42*p.fold)*fractal+(.22+.06*i)*carrier);
 // Soft blend preserves pitched fundamental and avoids excess inharmonic fold energy.
 const double fmOut=(.53*carrier+.47*fractal)*.94;
 subphase=fract(subphase+dt);
 // Modal excitation receives either impulse-noise, VA, algorithmic signal or their mixture.
 double trans=randBi(random)*noiseEnv;
 noiseEnv*=noiseDecay;
 const double drive=clamp(.57*va+.36*fmOut,-1.7,1.7);
 double excitation=trans*(.05+.44*p.brightness) + .09*drive;
 const int route=(p.engine==Engine::Hybrid)?std::clamp(p.routing,0,4):0;
 const double depth=clamp(p.crossDepth,0.,1.);
 const double rawRing=va*fmOut;
 // Audio-rate routing is real, not a post-mix selector. Only the exciter signal
 // changes here; all modal modes receive it *before* their recursive filter.
 if(route==1) excitation=trans*(.05+.44*p.brightness)*(.15+.6*(1-depth)) +
          (.09+.34*depth)*va;
 else if(route==2) excitation=trans*(.05+.44*p.brightness)*(.15+.6*(1-depth)) +
          (.09+.40*depth)*fmOut;
 else if(route==3) excitation=trans*(.05+.44*p.brightness)*(.3-.2*depth) +
          (.09+.66*depth)*rawRing;
 else if(route==4) excitation=trans*(.05+.44*p.brightness)*(.35-.15*depth) +
          (.09+.29*depth)*va;
 double modal=0, modalL=0, modalR=0;
 for(int i=0;i<8;i++){
  auto &m=modes[i];
  double y=m.r*m.cosw*m.y1-m.r*m.r*m.y2+excitation*m.energy*.09;
  m.y2=m.y1;m.y1=y;
  double contribution=y/(1.+i*.23);
  if(p.engine==Engine::FractalDigital && p.metalMix>0.f){
    // Live tone color: tilt the inharmonic partials, without retuning
    // resonators already struck during the current note.
    const double tilt=1.+.55*(double(p.metalColor)-.5)*(double(i)-3.5)/3.5;
    contribution*=tilt;
  }
  modal+=contribution;
  // Alternate modal partials spatially; zero width retains the original mono sound.
  const double width=clamp(p.stereoResonance,0.,1.);
  const double pan=(i%2==0?1.:-1.)*(.32+.065*i)*width;
  modalL+=contribution*(1.+pan);
  modalR+=contribution*(1.-pan);
 }
 // nonlinear, controlled soft saturation; no unstable feedback in prototype
 modal=std::tanh(modal*.70);
 const double physical=std::tanh((modal+.18*drive)*(.75+1.4*p.resonanceBody));
 const double physicalL=std::tanh((modalL+.18*drive)*(.75+1.4*p.resonanceBody));
 const double physicalR=std::tanh((modalR+.18*drive)*(.75+1.4*p.resonanceBody));
 double outL=0,outR=0;
 // 5.6 companion body is routed separately so its natural tail can outlive
 // the direct envelope without changing the established 5.5 audio path.
 double bellExtraL=0.,bellExtraR=0.;
 switch(p.engine){
 case Engine::CrystalVA:outL=vaL;outR=vaR;break;
 case Engine::FractalDigital:{
  if(p.metalMix<=0.f){
    // Exact original Factory Bell / Fractal Digital output when disabled.
    outL=outR=fmOut;
  }else{
    // New metallic body runs in parallel with the distinctive original
    // PM/fold carrier; conservative direct gain preserves attack clarity.
    const double wet=clamp(p.metalMix,0.,1.);
    const double bodyGain=.68+.26*clamp(p.resonanceBody,0.,1.);
    outL=(1.-.30*wet)*fmOut+wet*bodyGain*physicalL;
    outR=(1.-.30*wet)*fmOut+wet*bodyGain*physicalR;
    // Optional energy-normalized resonator network: independently excited
    // nearby partials create density and time-varying beat structures.
    // Adjacent complex-oscillator pairs are mixed by orthogonal rotations,
    // preserving total x^2+v^2 in absence of excitation and damping.
    const double target=clamp(p.bellDynamics,0.,1.);
    bellDynamicsSmoothed+=(target-bellDynamicsSmoothed)*bellSmoothCoef;
    // Evolve the strike clock even while bypassed: switching on mid-note
    // must not replay a brand-new metallic attack.
    const double strike=bellStrike*(.62*trans+.38*fmOut);
    bellStrike*=bellStrikePole;
    if(bellDynamicsSmoothed>1.e-7){
      for(int i=0;i<8;i++){
        auto& b=companion[(size_t)i];
        b.v+=strike*b.strike*.095;
        const double x=(b.x*b.cw+b.v*b.sw)*b.r;
        const double v=(b.v*b.cw-b.x*b.sw)*b.r;
        b.x=x;b.v=v;
      }
      if(bellCouplingSin>1.e-12){
        for(int i=0;i<7;i++){
          auto& a=companion[(size_t)i];
          auto& b=companion[(size_t)i+1];
          const double ax=a.x,av=a.v;
          a.x=bellCouplingCos*ax+bellCouplingSin*b.x;
          a.v=bellCouplingCos*av+bellCouplingSin*b.v;
          b.x=bellCouplingCos*b.x-bellCouplingSin*ax;
          b.v=bellCouplingCos*b.v-bellCouplingSin*av;
        }
      }
      double rawL=0.,rawR=0.;
      for(int i=0;i<8;i++){
        const auto& b=companion[(size_t)i];
        const double partial=b.x*b.pickup;
        const double pan=((i&1)?-1.:1.)*(.15+.065*i)*p.stereoResonance;
        rawL+=partial*(1.+pan);
        rawR+=partial*(1.-pan);
      }
      const double amount=.48*wet*bellDynamicsSmoothed;
      bellExtraL=amount*std::tanh(rawL*.85);
      bellExtraR=amount*std::tanh(rawR*.85);
    }
  }
  break;
 }
 case Engine::PhysicalResonance:outL=outR=physical;break;
 case Engine::Modular: break; // Handled by processGraph() before the legacy switch.
 case Engine::Hybrid:{
  // Cross-synthesis: algorithmic + VA excitation drives resonators; resonant energy affects PM next sample.
  feedback=clamp(modal,-1.,1.);
  // Explicit one-sample state: previous modal output feeds the next sample's PM.
  crossDelay=feedback;
  if(route==0){
    const double x=(1-p.blend)*(.55*va+.45*fmOut)+p.blend*physical;
    outL=x+.10*(vaL-va);outR=x+.10*(vaR-va);
  }else if(route==4){
    // The resonator modulates the VA amplitude (audio-rate cross-synthesis).
    const double amL=vaL*(.70+depth*.60*physicalL);
    const double amR=vaR*(.70+depth*.60*physicalR);
    outL=(1-p.blend)*amL+p.blend*physicalL;
    outR=(1-p.blend)*amR+p.blend*physicalR;
  }else{
    // Modes are the central instrument; oscillator feed-through retains pitch clarity.
    const double direct=(route==1?va:(route==2?fmOut:rawRing));
    const double amt=(1-p.blend)*direct;
    outL=amt+p.blend*physicalL+.05*(vaL-va);
    outR=amt+p.blend*physicalR+.05*(vaR-va);
  }
  break;
 }
 }
 // Low-pass nonlinear TPT-style approximation via 2 one-poles with coefficient smoothing.
 double cf=clamp(baseCutoff+freq*(.25*p.brightness),30.,sr*.44);
 cf*=1.+.08*p.motion*lfo; cf=clamp(cf,30.,sr*.44);
 const double g=1-std::exp(-twoPi*cf/sr);
 auto lp=[&](double x){filt1+=(std::tanh(x*(1+.8*p.resonance)) - filt1)*g;
     filt2+=(filt1-filt2)*g;
     return filt2;};
 // one common filter preserves unique voice articulation; stereo differences preserved pre-filter
 double mono=lp(.5*(outL+outR));
 double spread=.5*(outL-outR)*(.6+p.unison*.5);
 double gain=(.15+.70*vel)*envVal*p.gain;
 // Body decay retains its own modal T60; sqrt release attenuation avoids
 // forcing every metal mode to follow the much faster direct-sound envelope.
 const double bodyGate=std::sqrt(std::max(0.,envVal));
 const double bodyGain=(.15+.70*vel)*p.gain*bodyGate;
 stereo.left=float(dbSmooth((mono+spread)*gain+bellExtraL*bodyGain));
 stereo.right=float(dbSmooth((mono-spread)*gain+bellExtraR*bodyGain));
 return mono;
}
// v0.3 Modular: independent per-voice, per-node stereo DSP with no allocation or lock.
// Input dependency order is pre-compiled in Synth::setPatch. Graph state resets on note-on.
Sample Synth::Voice::processGraph(double sr,const Patch& p,double bendMultiplier,float wheel,
    const std::array<int,kGraphNodes>& order,int renderCount,
    const std::array<double,kGraphNodes>& filterCoeffs,
    const std::array<std::array<double,12>,kGraphNodes>& weaveRatios,
    const std::array<std::array<double,12>,kGraphNodes>& matterFactors){
 const double amp=env.step(p,sr);
 if(!env.active()){active=false;return {};}
 const double freq=clamp(baseHz*bendMultiplier,10.,sr*.43);
 const double dt=freq/sr;
 graphLfo=fract(graphLfo+(0.11+p.motion*1.6+wheel*4.0)/sr);
 graphLfoB=fract(graphLfoB+(0.37+p.motion*2.9+wheel*.8)/sr);
 const double lfo=std::sin(twoPi*graphLfo);
 const double lfoB=std::sin(twoPi*graphLfoB);
 // v0.9: typed spectral-state bus. Only SpectralMatter and MatterWeave
 // produce frames. Audio outputs are independent and never used as a proxy.
 // Fixed-size automatic storage; no RT allocations or mutable shared data.
 // Graph topological order and typed-source validation guarantee that every
 // referenced upstream frame and PCM sample is written before it is read.
 // This avoids clearing the full 9x12 spectral frame cache per voice/sample.
 struct SpectralFrame {std::array<double,12> energy;};
 struct PhaseFrame {std::array<double,12> cycle;};
 std::array<SpectralFrame,kGraphNodes> spectral;
 // Phase frames are a typed state channel (not reconstructed from PCM).
 // Graph compilation guarantees that all bridge inputs have run before use.
 std::array<PhaseFrame,kGraphNodes> phaseFrames;
 // v1.5: one typed frame carries spectrum, phase and unit-mass mechanics.
 // Frames are published by producer nodes before any consumer executes.
 std::array<AcousticStateFrame,kGraphNodes> acoustic{};
 std::array<Sample,kGraphNodes> values;
 const auto source=[&](int n)->Sample{return n<0?Sample{}:values[n];};
 for(int slot=0;slot<renderCount;slot++){
   const int i=order[slot];
   const auto&node=p.graph.nodes[i];auto& st=graphStates[i];
   const auto a=(node.type==NodeType::FeedbackDelay)?Sample{}:source(node.a);
   const auto b=(node.type==NodeType::FeedbackDelay)?Sample{}:source(node.b);
   // Modulation is evaluated once per sample from the graph's current sample.
   // Graph order additionally incorporates audio-rate modulation dependencies.
   double color=node.color,level=node.level,pitch=0.,phaseOffset=0.;
   for(const auto&route:p.modRoutes){
     if(route.source==0||route.target!=i||route.depth==0.f)continue;
     double value=0.;
     switch(route.source){
      case 1:value=lfo;break;
      case 2:value=lfoB;break;
      case 3:value=2.*amp-1.;break;
      case 4:value=2.*vel-1.;break;
      case 5:value=2.*wheel-1.;break;
      default:{const auto sample=source(route.source-6);value=clamp(.5*(sample.left+sample.right),-1.,1.);break;}
     }
     const double mod=route.depth*value;
     switch(route.destination){
      case 0:color=clamp(color+mod*.5,0.,1.);break;
      case 1:level=clamp(level+mod*.5,0.,1.);break;
      case 2:pitch+=mod*12.;break;
      case 3:phaseOffset+=mod*.22;break;
     }
   }
   const double oscillatorDt=(std::abs(pitch)<1.e-12)?dt:clamp(dt*std::exp2(clamp(pitch,-24.,24.)/12.),.0000001,.45);
   double l=0,r=0;
   switch(node.type){
   case NodeType::Off:break;
   case NodeType::VA:{
     const double rate=oscillatorDt*(.98+.13*color+.015*lfo*p.motion);
     for(int j=0;j<3;j++){
       const double detune=(j-1)*(.001+.023*p.detune)*p.unison;
       const double inc=clamp(rate*(1.+detune),.0000001,.46);
       const double ph=st.unisonPhase[j];
       const double saw=2*ph-1-polyBlep(ph,inc);
       double w=(.82*color+.10)*saw+(.84-.74*color)*std::sin(twoPi*(ph+phaseOffset));
       const double pan=(j-1)*(.08+.22*p.unison);
       l+=w*(1.-pan)/3.;r+=w*(1.+pan)/3.;
       st.unisonPhase[j]=fract(ph+inc);
     }
     break;
   }
   case NodeType::Digital:{
     const double mInc=clamp(oscillatorDt*(1.25+color*4.),.0000001,.45);
     const double mod=std::sin(twoPi*st.mod);
     const double inputMod=.5*(a.left+a.right);
     st.mod=fract(st.mod+mInc);
     const double wave=std::sin(twoPi*(st.phase+phaseOffset)+mod*(.3+3.8*color)+inputMod*(.2+2.*p.fm));
     st.phase=fract(st.phase+oscillatorDt);
     l=r=wave;break;
   }
   case NodeType::Noise:{
     // 0.0 means short impulse, 1.0 means almost sustained textured excitation.
     const double n=randBi(st.rng);
     const double decay=.92+.07999*color;
     st.noiseLevel*=decay;
     const double envelope=.25+.75*st.noiseLevel;
     l=r=n*envelope;break;
   }
   case NodeType::Ring:l=a.left*b.left*2.;r=a.right*b.right*2.;break;
   case NodeType::Mix:{
     const double mix=color;
     l=a.left*(1.-mix)+b.left*mix;
     r=a.right*(1.-mix)+b.right*mix;
     break;
   }
   case NodeType::Fold:{
     const double fold=1.1+4.5*color;
     l=std::sin(a.left*fold+(.25*color)*std::sin(2.3*a.left));
     r=std::sin(a.right*fold+(.25*color)*std::sin(2.3*a.right));
     break;
   }
   case NodeType::Modal:{
     const double excitationL=clamp(a.left,-2.,2.);
     const double excitationR=clamp(a.right,-2.,2.);
     // Modal bank shares oscillation modes between channels, but the individual
     // partials pan and resonate differently; zero-cost small stereo spread.
     for(int j=0;j<8;j++){
       auto&m=st.modal[j];
       const double x=.5*(excitationL+excitationR);
       double y=m.r*m.cosw*m.y1-m.r*m.r*m.y2 + x*m.energy*.019;
       m.y2=m.y1;m.y1=y;
       double pan=(j%2?-.23:.23)*(.3+.7*p.stereoResonance);
       l+=y*(1.-pan)/(1.+j*.3);r+=y*(1.+pan)/(1.+j*.3);
     }
     l=std::tanh(l*(.9+p.resonanceBody+color*.45));r=std::tanh(r*(.9+p.resonanceBody+color*.45));
     break;
   }
   case NodeType::LowPass:{
     const double c=clamp(filterCoeffs[i]*(.25+1.5*color),.00001,.99);
     st.filterL+=(a.left-st.filterL)*c;
     st.filterR+=(a.right-st.filterR)*c;
     l=st.filterL;r=st.filterR;break;
   }
   case NodeType::Gain:l=a.left;r=a.right;break;
   case NodeType::Harmonic:{
     // A sample-free additive oscillator: additive partials are dynamically tilted
     // towards high or low harmonics. Do not evaluate partials above 0.44*sr.
     double w=0.,norm=0.;
     const double hz=sr*oscillatorDt;
     // Interpolate two precomputed timbral spectral envelopes instead of
     // evaluating 12 expensive power functions for every sample.
     constexpr double rich[]={1.,.61557,.46346,.37893,.32413,.28529,.25613,.23326,.2148,.19953,.18665,.17562};
     constexpr double pure[]={1.,.15389,.0515,.02368,.01297,.00786,.00512,.00351,.00251,.00186,.00143,.00112};
     for(int harmonic=1;harmonic<=12&&harmonic*hz<.44*sr;harmonic++){
       const double weight=rich[harmonic-1]*(1.-color)+pure[harmonic-1]*color;
       w+=weight*std::sin(twoPi*(harmonic*(st.phase+phaseOffset)));
       norm+=weight;
     }
     const double wave=norm>0?w/norm:0.;
     l=wave*(1.-.12*p.unison);r=wave*(1.+.12*p.unison);
     st.phase=fract(st.phase+oscillatorDt);
     break;
   }
   case NodeType::PhaseWarp:{
     // Phase distortion using a continuous bend function. Extreme bends can alias;
     // limited distortion depth is intentional in this first prototype.
     const double phase=fract(st.phase+phaseOffset);
     const double bend=.18+.64*color;
     const double warped=(phase<bend)?(phase*.5/bend):(.5+(phase-bend)*.5/(1.-bend));
     const double wave=.72*std::sin(twoPi*warped)+.28*std::sin(twoPi*phase);
     l=r=wave;
     st.phase=fract(st.phase+oscillatorDt);
     break;
   }
   case NodeType::CoupledModes:{
     // v0.5: Exciter A transfers energy into four resonant quadrature states.
     // Input B *bends the resonant frequencies before energy is emitted*.
     // The rotation-damping representation has a bounded state norm even if
     // B changes at audio rate (unlike time-varying biquad coefficients).
     const double exciter=clamp(.5*(a.left+a.right),-1.,1.);
     const double control=clamp(.5*(b.left+b.right),-1.,1.);
     const double smoothing=1.-std::exp(-1.0/(sr*.00018));
     st.couplingSmooth+=smoothing*(control-st.couplingSmooth);
     const double bend=std::exp2(st.couplingSmooth*(.04+.55*color));
     const double hz0=sr*oscillatorDt;
     constexpr double ratios[]={1.,1.497,2.369,3.891};
     for(int j=0;j<4;j++){
       auto&m=st.modal[j];
       const double hz=clamp(hz0*ratios[j]*(.74+.46*color)*bend,25.,sr*.43);
       const double angle=twoPi*hz/sr;
       const double c=std::cos(angle),sn=std::sin(angle);
       // Uniform contraction guarantees bounded recursive energy for any bend.
       const double decay=(.00035+j*.00013+p.damping*.0012)*48000./sr;
       const double radius=clamp(1.-decay,.90,.99992);
       const double input=exciter*(.018/(1.+j*.40))*(.80+.20*st.couplingSmooth);
       const double real=radius*(m.y1*c-m.y2*sn)+input;
       const double imag=radius*(m.y1*sn+m.y2*c);
       m.y1=real;m.y2=imag;
       const double weight=1./(1.+j*.42);
       const double pan=((j%2)?-.28:.28)*(.35+.65*p.stereoResonance);
       l+=real*weight*(1.-pan);
       r+=real*weight*(1.+pan);
     }
     // Controlled gain, bounded output; existing per-node soft-clip follows.
     l=std::tanh(l*1.75);
     r=std::tanh(r*1.75);
     break;
   }
   case NodeType::ResonantExchange:{
     // v0.6: Resonant Exchange is *reciprocal* modal energy transfer, not
     // sample mixing and not frequency bending. Input A excites two modes;
     // input B changes the rate at which each pair exchanges internal state.
     // Every per-mode evolution is a contracting complex rotation. The
     // coupling is a real orthogonal rotation applied to BOTH real/imag parts,
     // so with A=0 total state energy cannot grow, even with audio-rate B.
     const double excitation=clamp(.5*(a.left+a.right),-1.,1.);
     const double control=clamp(.5*(b.left+b.right),-1.,1.);
     const double hz0=sr*oscillatorDt;
     constexpr double ratios[]={1.,1.497,2.017,3.091};
     // Cache only when the exact inputs are unchanged. Keying on hz0 also
     // covers sample-accurate pitch bend; keying on color covers automation.
     // No waveform approximation: use the same expressions and libm as v1.2.
     if(hz0!=st.cachedExchangeHz0 || color!=st.cachedExchangeColor){
       for(int j=0;j<4;j++){
         const double hz=clamp(hz0*ratios[j]*(.79+.45*color),25.,sr*.43);
         const double angle=twoPi*hz/sr;
         st.cachedExchangeCos[j]=std::cos(angle);
         st.cachedExchangeSin[j]=std::sin(angle);
       }
       st.cachedExchangeHz0=hz0;
       st.cachedExchangeColor=color;
     }
     for(int j=0;j<4;j++){
       auto& mode=st.modal[j];
       const double c=st.cachedExchangeCos[j],si=st.cachedExchangeSin[j];
       const double decay=(.00055+j*.00016+p.damping*.0014)*48000./sr;
       const double radius=clamp(1.-decay,.90,.99980);
       const double drive=((j==0)?1.0:((j==2)?.65:0.0));
       const double amount=excitation*(.012+.006*color)*drive;
       const double re=radius*(mode.y1*c-mode.y2*si)+amount;
       const double im=radius*(mode.y1*si+mode.y2*c);
       mode.y1=re;mode.y2=im;
     }
     // B is a time-varying *energy-exchange* control; an orthogonal map
     // cannot inject energy by itself. Thus silence at input A stays silent.
     const double exchangeAngle=(.012+.28*color)*(.6+.4*control);
     if(exchangeAngle!=st.cachedCouplingAngle){
       st.cachedCouplingCos=std::cos(exchangeAngle);
       st.cachedCouplingSin=std::sin(exchangeAngle);
       st.cachedCouplingAngle=exchangeAngle;
     }
     const double ec=st.cachedCouplingCos,es=st.cachedCouplingSin;
     for(int pair=0;pair<4;pair+=2){
       auto& x=st.modal[pair];auto& y=st.modal[pair+1];
       const double xr=ec*x.y1-es*y.y1;
       const double yr=es*x.y1+ec*y.y1;
       const double xi=ec*x.y2-es*y.y2;
       const double yi=es*x.y2+ec*y.y2;
       x.y1=xr;x.y2=xi;y.y1=yr;y.y2=yi;
     }
     for(int j=0;j<4;j++){
       const double weight=1./(1.+.48*j);
       const double pan=(j%2?-.30:.30)*(.25+.75*p.stereoResonance);
       l+=st.modal[j].y1*weight*(1.-pan);
       r+=st.modal[j].y1*weight*(1.+pan);
     }
     l=std::tanh(l*1.30);r=std::tanh(r*1.30);
     break;
   }
   case NodeType::PhaseEcology:{
     // v0.7: Phase Ecology: a population of six interacting partial oscillators.
     // Unlike a static additive spectrum or a resonator, oscillators exchange
     // *phase information*. A perturbs the running frequencies; B continuously
     // controls the attraction between neighbouring phase trajectories.
     // Frequencies and all network pulls are bounded and do not allocate.
     const double aDrive=clamp(.5*(a.left+a.right),-1.,1.);
     const double bControl=clamp(.5*(b.left+b.right),-1.,1.);
     const double hz0=sr*oscillatorDt;
     constexpr double coherent[]={1.,1.004,2.,2.008,3.,3.015};
     constexpr double dispersed[]={1.,1.088,1.617,1.743,2.431,2.597};
     const auto old=st.ecologyPhases;  // synchronous update; no order bias
     const double attraction=(.008+.20*color)*(.7+.65*bControl)*hz0;
     const double driveHz=hz0*(.05+.19*color)*aDrive;
     double norm=0.;
     for(int j=0;j<6;j++){
       const double ratio=coherent[j]*(1.-color)+dispersed[j]*color;
       const double nominalHz=hz0*ratio;
       // Even an inaudible upper oscillator publishes a valid phase frame.
       // Otherwise bridge readers could encounter an unwritten stack slot.
       phaseFrames[(size_t)i].cycle[(size_t)j]=old[(size_t)j];
       phaseFrames[(size_t)i].cycle[(size_t)(j+6)]=old[(size_t)j];
       if(nominalHz>=sr*.435)continue; // no partial above guarded Nyquist
       const int prev=(j+5)%6,next=(j+1)%6;
       const double pull=std::sin(twoPi*(old[(size_t)next]-old[(size_t)j]))
                        +std::sin(twoPi*(old[(size_t)prev]-old[(size_t)j]));
       const double hz=clamp(nominalHz+attraction*pull+driveHz,1.,sr*.44);
       const double phase=fract(old[(size_t)j]+hz/sr);
       st.ecologyPhases[(size_t)j]=phase;
       // The six-oscillator phase population is exposed to the typed bus.
       // Each oscillator drives two channels in the 12-bin bridge frame.
       phaseFrames[(size_t)i].cycle[(size_t)j]=phase;
       phaseFrames[(size_t)i].cycle[(size_t)(j+6)]=phase;
       const double weight=1./(1.+.27*j);
       const double partial=std::sin(twoPi*phase)*weight;
       const double pan=(j%2?-.24:.24)*(.35+.65*p.stereoResonance);
       l+=partial*(1.-pan);r+=partial*(1.+pan);
       norm+=weight;
     }
     if(norm>0.){l=l/norm*1.55;r=r/norm*1.55;}
     break;
   }
   case NodeType::SpectralMatter:{
     // v0.8: the waveform is no longer a fixed wavetable/oscillator shape.
     // Twelve individually phase-continuous partials carry positive spectral
     // energy. Neighbour-to-neighbour diffusion evolves the distribution, with
     // audio A controlling excitation and audio B changing the spectral target.
     // The graph exchanges the *internal spectrum*, not just a mixed signal.
     // Every partial is attenuated well before Nyquist; no allocations/locks.
     const double excite=clamp(.5*(a.left+a.right),-1.,1.);
     const double bval=clamp(.5*(b.left+b.right),-1.,1.);
     const double pole=st.matterPole;
     st.matterControl+=pole*(bval-st.matterControl);
     const double movement=clamp(color+.29*st.matterControl,0.,1.);
     const double drive=.32+.75*std::abs(excite);
     const double base=sr*oscillatorDt;
     double sumL=0.,sumR=0.,normalizer=0.;
     // Update energies synchronously to avoid transfer depending on bin order.
     const auto previous=st.matterEnergy;
     for(int j=0;j<12;j++){
       const double bin=j+1.;
       // Physical-style dispersion and inharmonicity, continuous across colors.
       const double stiff=.00009+.0016*movement*movement;
       // v1.2: preserve the original arithmetic evaluation order while
       // removing one sqrt per partial/sample for unmodulated spectral color.
       // Audio-rate modulation retains the precise dynamic expression.
       const double factor=(movement==static_cast<double>(node.color))
          ?matterFactors[(size_t)i][(size_t)j]
          :std::sqrt((1.+stiff*bin*bin)/(1.+stiff));
       const double hz=base*bin*factor;
       const double low=previous[(size_t)(j==0?0:j-1)];
       const double high=previous[(size_t)(j==11?11:j+1)];
       const double diffuse=(.002+.012*movement)*(low+high-2.*previous[(size_t)j]);
       // The target is a moving spectral formant, not just harmonic tilt.
       const double focus=1.5+movement*9.;
       const double dist=(bin-focus)/3.1;
       // Rational, smoothly decaying spectral kernel: avoids evaluating
       // millions of exponentials on the audio callback.
       const double dd=dist*dist;
       const double formant=1./(1.+.53*dd+.17*dd*dd);
       constexpr double decayShape[]={1.,.827,.684,.566,.468,.387,.320,.265,.219,.181,.150,.124};
       const double spectralTarget=(.14+.76*formant+.30*decayShape[j])*drive;
       const double tracking=.00055+.0010*std::abs(excite);
       const double energy=clamp(previous[(size_t)j]+diffuse+tracking*(spectralTarget-previous[(size_t)j]),0.,1.7);
       st.matterEnergy[(size_t)j]=energy;
       spectral[i].energy[(size_t)j]=energy;
       // Phase continuity is maintained even when a partial is suppressed.
       const double deviation=1.+clamp(excite,-1.,1.)*(.0005+.006*movement);
       const double inc=clamp(hz*deviation/sr,0.,.48);
       const double phase=fract(st.matterPhases[(size_t)j]+inc);
       st.matterPhases[(size_t)j]=phase;
       phaseFrames[(size_t)i].cycle[(size_t)j]=phase;
       // Smooth fade region below Nyquist; no hard partial on/off clicks.
       const double nyquistFade=clamp((.44*sr-hz)/(.09*sr),0.,1.);
       const double taper=nyquistFade*nyquistFade*(3.-2.*nyquistFade);
       if(taper<=0.)continue;
       constexpr double weightShape[]={1.,.949,.901,.855,.812,.771,.732,.695,.659,.626,.595,.565};
       const double weight=energy*taper*weightShape[j];
       const double partial=weight*detail::sineCycles(phase+phaseOffset);
       const double pan=((j%4)<2?-.17:.17)*(.30+.70*p.stereoResonance);
       sumL+=partial*(1.-pan);sumR+=partial*(1.+pan);
       normalizer+=weight*weight;
     }
     const double scale=.48/std::sqrt(std::max(.2,normalizer));
     l=sumL*scale;r=sumR*scale;
     break;
   }
   case NodeType::MatterWeave:{
     // v0.9: two *typed* spectral streams exchange their moving partial
     // energies through a local, smoothed 12-bin reservoir. No audio-waveform
     // sidechain is consulted here; sources may be completely audio-muted.
     // The topology validator enforces a spectral A and optional spectral B.
     const auto& inputA=spectral[(size_t)node.a];
     const bool hasB=node.b>=0;
     const auto& inputB=hasB?spectral[(size_t)node.b]:inputA;
     const double morph=.10+.80*color;
     // Color is constant for most patches; compute the exact one-pole
     // coefficient only when automation actually changes it.
     if (color != st.previousWeaveColor) {
       st.cachedWeaveSlew=1.-std::exp(-1./(sr*(.003+.032*(1.-color))));
       st.previousWeaveColor=color;
     }
     const double slew=st.cachedWeaveSlew;
     const double baseHz=sr*oscillatorDt;
     const auto previous=st.matterEnergy;
     double norm=0.;
     for(int j=0;j<12;j++){
       // The source bins are *independent state* observations.  Opposing
       // indices give an asymmetric spectral transmutation, not a crossfade
       // between two ordinary audio signals.
       const double sourceA=inputA.energy[(size_t)j];
       const double sourceB=inputB.energy[(size_t)(11-j)];
       const double target=(1.-morph)*sourceA+morph*sourceB;
       // Discrete diffusion in a bounded energy reservoir.
       const double neighbourL=previous[(size_t)(j==0?0:j-1)];
       const double neighbourR=previous[(size_t)(j==11?11:j+1)];
       const double diffusion=.0015*(neighbourL+neighbourR-2.*previous[(size_t)j]);
       const double energy=clamp(previous[(size_t)j]+slew*(target-previous[(size_t)j])+diffusion,0.,1.7);
       st.matterEnergy[(size_t)j]=energy;
       spectral[i].energy[(size_t)j]=energy;
       const double bin=j+1.;
       // Smoothly dispersive tuning.  The two state frames control the
       // spectral envelope while keyboard pitch retains musical coherence.
       const double hz=baseHz*((color==node.color)
           ?weaveRatios[(size_t)i][(size_t)j]
           :bin*std::sqrt((1.+(.00005+.00065*color*color)*bin*bin)/
                          (1.+(.00005+.00065*color*color))));
       const double phase=fract(st.matterPhases[(size_t)j]+clamp(hz/sr,0.,.48));
       st.matterPhases[(size_t)j]=phase;
       phaseFrames[(size_t)i].cycle[(size_t)j]=phase;
       const double fade=clamp((.43*sr-hz)/(.11*sr),0.,1.);
       const double taper=fade*fade*(3.-2.*fade);
       const double weight=energy*taper/(1.+.15*j);
       norm+=weight*weight;
       const double partial=weight*detail::sineCycles(phase+phaseOffset);
       const double pan=(j%2?-.16:.16)*(.3+.7*p.stereoResonance);
       l+=partial*(1.-pan);r+=partial*(1.+pan);
     }
     const double scale=.42/std::sqrt(std::max(.25,norm));
     l*=scale;r*=scale;
     break;
   }
   case NodeType::FormantField:{
     // v1.0: a *state-domain formant shaper*.  Input A must be a typed spectral
     // state; input B is an independent audio-rate articulatory gesture.
     // No reliance on A's audio output, and no unbounded feedback.
     const auto& parent=spectral[(size_t)node.a];
     const double gesture=clamp(.5*(b.left+b.right),-1.,1.);
     // Vowel-like trajectories between two sets of *absolute Hz* resonances:
     // formants do not track note pitch, unlike ordinary keyboard filters.
     const double articulation=clamp(color+.19*gesture,0.,1.);
     // Three poles travel through a continuously variable 'vocal space'.
     const double f1=300.+510.*articulation;
     const double f2=2050.-1020.*articulation;
     const double f3=2900.-430.*articulation;
     const double baseHz=sr*oscillatorDt;
     const double smoothing=st.formantPole;
     double norm=0.;
     for(int j=0;j<12;j++){
       const double bin=j+1.;
       const double hz=baseHz*bin;
       const double prev=st.matterEnergy[(size_t)j];
       // Formant peaks use a smooth rational envelope with a weak
       // fundamental floor to retain musical pitch and avoid normalization
       // explosions at high notes.
       const double x1=(hz-f1)/190.;
       const double x2=(hz-f2)/310.;
       const double x3=(hz-f3)/410.;
       const double envelope=.20+1.55/(1.+x1*x1)+1.30/(1.+x2*x2)+.80/(1.+x3*x3);
       const double target=clamp(parent.energy[(size_t)j]*envelope*.67,0.,1.7);
       const double energy=clamp(prev+smoothing*(target-prev),0.,1.7);
       st.matterEnergy[(size_t)j]=energy;
       spectral[i].energy[(size_t)j]=energy;
       const double next=fract(st.matterPhases[(size_t)j]+clamp(hz/sr,0.,.48));
       st.matterPhases[(size_t)j]=next;
       phaseFrames[(size_t)i].cycle[(size_t)j]=next;
       const double fade=clamp((.44*sr-hz)/(.08*sr),0.,1.);
       const double taper=fade*fade*(3.-2.*fade);
       const double weight=energy*taper/(1.+j*.10);
       norm+=weight*weight;
       const double partial=weight*detail::sineCycles(next+phaseOffset);
       const double width=(j%3==0?-.18:.18)*p.stereoResonance;
       l+=partial*(1.-width);r+=partial*(1.+width);
     }
     const double scale=.47/std::sqrt(std::max(.27,norm));
     l*=scale;r*=scale;
     break;
   }
   case NodeType::PhaseMatterBridge:{
     // v1.4: the *internal* spectral-energy and oscillator-phase states
     // meet directly, independent of upstream audio gain or stereo PCM.
     // A = typed 12-band energy state; B = typed phase state (either
     // PhaseEcology or any typed spectral generator). Both are graph-bound.
     const auto& in=spectral[(size_t)node.a];
     const auto& phaseA=phaseFrames[(size_t)node.a];
     const auto& phaseB=phaseFrames[(size_t)node.b];
     const double hz0=sr*oscillatorDt;
     const double slew=st.formantPole; // sample-rate-derived, note-on cache
     const double depth=clamp(color,0.,1.);
     double norm=0.;
     for(int j=0;j<12;j++){
       const double hz=hz0*(j+1.);
       const double ref=phaseB.cycle[(size_t)j];
       const double own=st.matterPhases[(size_t)j];
       // B alters the running phase velocity, not the output mixer.
       // Clamp excursions so the oscillator cannot jump past Nyquist.
       const double error=detail::sineCycles(ref-own);
       const double inc=clamp((hz/sr)*(1.+.72*depth*error),0.,.43);
       const double next=fract(own+inc);
       st.matterPhases[(size_t)j]=next;
       phaseFrames[(size_t)i].cycle[(size_t)j]=next;
       // A's spectral reservoir is shaped by the *relative* phase of
       // the two source systems. State follows smoothly across samples.
       const double coherence=.5+.5*detail::sineCycles(ref-phaseA.cycle[(size_t)j]+.25);
       const double target=clamp(in.energy[(size_t)j]*(1.-.52*depth*(1.-coherence)),0.,1.7);
       const double energy=clamp(st.matterEnergy[(size_t)j]+slew*(target-st.matterEnergy[(size_t)j]),0.,1.7);
       st.matterEnergy[(size_t)j]=energy;
       spectral[(size_t)i].energy[(size_t)j]=energy;
       // Raised-cosine guard tapers upper partials before Nyquist.
       const double fade=clamp((sr*.44-hz)/(sr*.10),0.,1.);
       const double taper=fade*fade*(3.-2.*fade);
       const double weight=energy*taper/(1.+.13*j);
       const double partial=weight*detail::sineCycles(next+phaseOffset);
       const double pan=(j%2?-.15:.15)*(.35+.65*p.stereoResonance);
       l+=partial*(1.-pan);r+=partial*(1.+pan);
       norm+=weight*weight;
     }
     const double scale=.48/std::sqrt(std::max(.25,norm));
     l*=scale;r*=scale;
     break;
   }
   case NodeType::PhysicalBody:{
     // Direct model of x'=v, v'=F - omega^2*x - damping*v (unit mass).
     // Stable damped rotation for each sample; bounded external excitation.
     // This publishes true typed x,v,F,E even if node PCM Level == 0.
     const double hz=clamp(sr*oscillatorDt*(.55+1.5*color),35.,sr*.12);
     const double omega=twoPi*hz;
     const double angle=omega/sr;
     const double radius=std::exp(-(.00035+.0024*p.damping));
     const double ca=std::cos(angle),sa=std::sin(angle);
     const double exc=clamp(.5*(a.left+a.right)+.4*(b.left+b.right),-1.,1.);
     // Unit mass: F [N] = m [kg] * acceleration [m/s^2], m = 1kg.
     // The exact harmonic rotation is followed by an impulse on velocity.
     const double externalForce=omega*omega*.075*exc;
     const double deltaVNorm=externalForce/(omega*sr);
     const double oldX=st.physicalDisplacement,oldV=st.physicalVelocityNorm;
     const double nx=radius*(oldX*ca+oldV*sa);
     const double nv=radius*(oldV*ca-oldX*sa)+deltaVNorm;
     st.physicalDisplacement=clamp(nx,-1.5,1.5);
     st.physicalVelocityNorm=clamp(nv,-1.5,1.5);
     auto& frame=acoustic[(size_t)i];
     frame.domains=AcousticStateFrame::Mechanical;
     frame.displacement=st.physicalDisplacement;
     frame.velocity=omega*st.physicalVelocityNorm;
     frame.force=externalForce;
     frame.mechanicalEnergy=.5*(frame.velocity*frame.velocity+omega*omega*frame.displacement*frame.displacement);
     l=st.physicalDisplacement*2.0;r=st.physicalDisplacement*1.8;
     break;
   }
   case NodeType::UnifiedCoupler:{
     // Typed A: moving spectral energy + phase; typed B: x, v, F, E.
     // A local delay-state makes the coupling reciprocal *inside this node*.
     // There is deliberately no unrestricted zero-delay graph feedback.
     const auto& spec=acoustic[(size_t)node.a];
     const auto& mech=acoustic[(size_t)node.b];
     const double fundamental=sr*oscillatorDt;
     const double hzBody=clamp(fundamental*(.55+1.35*color),35.,sr*.12);
     const double omega=twoPi*hzBody;
     const double angle=omega/sr;
     const double c=std::cos(angle),sn=std::sin(angle);
     const double damping=std::exp(-(.00050+.0018*p.damping));
     double excitation=0.;
     for(int j=0;j<12;j++){
       excitation+=spec.spectralEnergy[(size_t)j]*
              detail::sineCycles(spec.phaseCycle[(size_t)j])/(1.+j*.42);
     }
     excitation=clamp(excitation*.07,-.30,.30);
     // Convert mechanical force to normalized displacement drive; preserve
     // dimensional meaning instead of adding N directly to meters.
     const double forceMeters=clamp(mech.force/(omega*omega),-.2,.2);
     const double driveMeters=clamp(.065*excitation+.08*mech.displacement+.035*forceMeters,-.15,.15);
     const double externalForce=omega*omega*driveMeters;
     const double oldX=st.physicalDisplacement,oldV=st.physicalVelocityNorm;
     const double nextX=damping*(oldX*c+oldV*sn);
     const double nextV=damping*(oldV*c-oldX*sn)+externalForce/(omega*sr);
     st.physicalDisplacement=clamp(nextX,-1.5,1.5);
     st.physicalVelocityNorm=clamp(nextV,-1.5,1.5);
     const double displacement=st.physicalDisplacement;
     auto& frame=acoustic[(size_t)i];
     frame.domains=AcousticStateFrame::Spectral|AcousticStateFrame::Phase|AcousticStateFrame::Mechanical;
     frame.displacement=displacement;
     frame.velocity=omega*st.physicalVelocityNorm;
     frame.force=externalForce;
     frame.mechanicalEnergy=.5*(frame.velocity*frame.velocity+omega*omega*displacement*displacement);
     double norm=0.;
     for(int j=0;j<12;j++){
       const double base=spec.spectralEnergy[(size_t)j];
       const double relative=detail::sineCycles(spec.phaseCycle[(size_t)j]-st.matterPhases[(size_t)j]);
       const double physicalMod=clamp(displacement*.48+st.physicalVelocityNorm*.14,-.65,.65);
       const double target=clamp(base*(.83+.13*relative*color)+.11*std::abs(physicalMod),0.,1.7);
       const double updated=clamp(st.matterEnergy[(size_t)j]+.008*(target-st.matterEnergy[(size_t)j]),0.,1.7);
       st.matterEnergy[(size_t)j]=updated;
       spectral[(size_t)i].energy[(size_t)j]=updated;
       frame.spectralEnergy[(size_t)j]=updated;
       const double partialHz=fundamental*(j+1.);
       const double increment=clamp(partialHz/sr*(1.+physicalMod*color*.27),0.,.43);
       const double nextPhase=fract(st.matterPhases[(size_t)j]+increment);
       st.matterPhases[(size_t)j]=nextPhase;
       phaseFrames[(size_t)i].cycle[(size_t)j]=nextPhase;
       frame.phaseCycle[(size_t)j]=nextPhase;
       const double taper=clamp((sr*.44-partialHz)/(sr*.08),0.,1.);
       const double window=taper*taper*(3.-2.*taper);
       const double weight=updated*window/(1.+j*.20);
       const double partial=weight*detail::sineCycles(nextPhase+phaseOffset);
       const double pan=(j%2?-.14:.14)*p.stereoResonance;
       l+=partial*(1.-pan);r+=partial*(1.+pan);
       norm+=weight*weight;
     }
     const double scale=.42/std::sqrt(std::max(.3,norm));
     l=std::tanh(l*scale+displacement*.18);
     r=std::tanh(r*scale+displacement*.16);
     break;
   }
   case NodeType::MeshBody:{
     // The mechanical frame is published independent of PCM node gain.
     const double force=1200.*std::tanh(.5*(a.left+a.right+b.left+b.right));
     const double x=st.mesh.step(force);
     const int pick=st.mesh.size()/2;
     auto& frame=acoustic[(size_t)i];
     frame.domains=AcousticStateFrame::Mechanical;
     frame.displacement=x;frame.velocity=st.mesh.velocity(pick);
     frame.force=force;frame.mechanicalEnergy=st.mesh.energy();
     l=std::tanh(x*18.);r=std::tanh((x*.76+st.mesh.displacement(0)*.24)*18.);
     break;
   }
   case NodeType::PassiveBody:{
     // Causal mechanical port: external audio is a FORCING signal, not a
     // claim that the full digital graph preserves physical energy. Work and
     // dissipated energy are tracked exactly within the mechanical subsystem.
     const double force=900.*std::tanh(.5*(a.left+a.right+b.left+b.right));
     const double x=st.passiveMesh.step(force);
     const auto&mesh=st.passiveMesh;
     auto& frame=acoustic[(size_t)i];
     frame.domains=AcousticStateFrame::Mechanical;
     frame.displacement=x;
     frame.velocity=mesh.velocity(mesh.size()/2);
     frame.force=force;
     frame.mechanicalEnergy=mesh.energy();
     l=std::tanh(x*22.);r=std::tanh((x*.78+mesh.displacement(0)*.22)*22.);
     break;
   }
   case NodeType::FeedbackDelay:{
     // Explicit z^-1 feedback: no current-sample recursion. tanh/gain
     // bounds the audible path, even when the user creates a loop.
     l=st.delayed.left;r=st.delayed.right;
     break;
   }
   case NodeType::Output:{
     // Stereo output is a final two-input sum; Level controls total output.
     l=a.left + b.left*color;
     r=a.right + b.right*color;
     break;
   }
   }
   // Legacy spectral/phase node adapters publish their independent typed
   // state on the same acoustic bus, irrespective of PCM Level/gain.
   if(node.type==NodeType::SpectralMatter || node.type==NodeType::MatterWeave ||
      node.type==NodeType::FormantField || node.type==NodeType::PhaseMatterBridge){
     auto& frame=acoustic[(size_t)i];
     frame.domains=AcousticStateFrame::Spectral|AcousticStateFrame::Phase;
     frame.spectralEnergy=spectral[(size_t)i].energy;
     frame.phaseCycle=phaseFrames[(size_t)i].cycle;
   }else if(node.type==NodeType::PhaseEcology){
     auto& frame=acoustic[(size_t)i];
     frame.domains=AcousticStateFrame::Phase;
     frame.phaseCycle=phaseFrames[(size_t)i].cycle;
   }else if(node.type==NodeType::ResonantExchange || node.type==NodeType::CoupledModes){
     // Existing complex modal states are a quadrature oscillator. Under the
     // chosen convention, real part = x and imaginary part = -v / omega.
     // Publish an *equivalent* unit-mass mechanical reference frame without
     // changing legacy audio. Original mode input force is not directly
     // observable here, so leave external force at zero (not fabricated).
     const double omega=twoPi*clamp(sr*oscillatorDt*(.79+.45*color),25.,sr*.43);
     const auto& mode=st.modal[0];
     auto& frame=acoustic[(size_t)i];
     frame.domains=AcousticStateFrame::Mechanical;
     frame.displacement=mode.y1;
     frame.velocity=-omega*mode.y2;
     frame.force=0.;
     frame.mechanicalEnergy=.5*(frame.velocity*frame.velocity+omega*omega*frame.displacement*frame.displacement);
   }
   // Per-node gain is deliberately applied after each operation.
   values[i]={float(std::tanh(l*level*1.2)),float(std::tanh(r*level*1.2))};
 }
 // A delayed edge may originate from ANY node, including the Output, itself,
 // or a node processed later. Snapshots are always read one sample later.
 for(int i=0;i<kGraphNodes;i++)if(p.graph.nodes[(size_t)i].type==NodeType::FeedbackDelay){
   const int src=p.graph.nodes[(size_t)i].a;
   graphStates[(size_t)i].delayed=src<0?Sample{}:values[(size_t)src];
 }
 // Publish internal domain frames independent of node output gain. This is
 // an observational adapter; the quantities are reference-model states, not
 // measurements of a physical instrument.
 publishedAcoustic={};
 for(int slot=0;slot<renderCount;slot++){
  const auto& f=acoustic[size_t(order[size_t(slot)])];
  if(f.domains&AcousticStateFrame::Spectral){publishedAcoustic.spectralEnergy=f.spectralEnergy;publishedAcoustic.domains|=AcousticStateFrame::Spectral;}
  if(f.domains&AcousticStateFrame::Phase){publishedAcoustic.phaseCycle=f.phaseCycle;publishedAcoustic.domains|=AcousticStateFrame::Phase;}
  if(f.domains&AcousticStateFrame::Mechanical){
   publishedAcoustic.displacement=f.displacement;publishedAcoustic.velocity=f.velocity;
   publishedAcoustic.force=f.force;publishedAcoustic.mechanicalEnergy=f.mechanicalEnergy;
   publishedAcoustic.domains|=AcousticStateFrame::Mechanical;
  }
 }
 const auto last=values[kGraphNodes-1];
 const double gain=(.15+.7*vel)*amp*p.gain;
 return {float(dbSmooth(last.left*gain)),float(dbSmooth(last.right*gain))};
}
Synth::Synth(){patch_=factoryPresets()[0];prepare(48000);}
void Synth::prepare(double rate){
 // Changing host sample rate must rebuild all rate-dependent graph and filter
 // coefficients even when the host does not re-send the current patch.
 // Reset voice state first; the patch and modulation wiring remain intact.
 sr_=clamp(rate,8000.,192000.);
 reset();
 setPatch(patch_);
}
void Synth::reset(){for(auto &v:voices_)v=Voice{};nextAge_=0;bend_=wheel_=0;bendMultiplier_=1.;sustain_=false;dcLX_=dcLY_=dcRX_=dcRY_=softL_=softR_=0;}
void Synth::setPatch(const Patch& p){
 patch_=p;
 // If an old non-modular factory patch is switched to Modular, its nine node
 // slots were never configured. Seed an editable, valid sound path, rather than
 // treating the unconnected graph as a user-created silent patch.
 if(patch_.engine==Engine::Modular && isUnconfiguredModularGraph(patch_.graph))
  patch_.graph=makeStarterModularGraph();
 patch_.cutoff=clamp(p.cutoff,.01,1);patch_.gain=clamp(p.gain,0,1);
 patch_.routing=std::clamp(p.routing,0,4);
 patch_.crossDepth=clamp(p.crossDepth,0,1);
 patch_.stereoResonance=clamp(p.stereoResonance,0,1);
 patch_.metalMix=clamp(p.metalMix,0,1);
 patch_.metalColor=clamp(p.metalColor,0,1);
 patch_.metalDecay=clamp(p.metalDecay,0,1);
 patch_.bellDynamics=clamp(p.bellDynamics,0,1);
 patch_.bellCoupling=clamp(p.bellCoupling,0,1);
 patch_.bellClarity=clamp(p.bellClarity,0,1);
 for(int i=0;i<kGraphNodes;i++) {
   auto &node=patch_.graph.nodes[i];
   node.level=float(clamp(node.level,0,1));node.color=float(clamp(node.color,0,1));
 }
 graphValid_=compileGraph(patch_.graph,graphOrder_);
 for(auto &m:patch_.modRoutes){
   m.source=std::clamp(m.source,0,5+kGraphNodes);
   m.target=std::clamp(m.target,0,kGraphNodes-1);
   m.destination=std::clamp(m.destination,0,3);
   m.depth=std::isfinite(m.depth)?float(clamp(m.depth,-1.,1.)):0.f;
 }
 graphValid_=graphValid_&&validateModRoutes(patch_.graph,patch_.modRoutes);
 // Sorting includes both audio and audio-rate modulation dependencies.
 // Re-run a deterministic Kahn sort of the combined dependency graph.
 if(graphValid_){
  std::array<bool,kGraphNodes> seen{};
  for(int slot=0;slot<kGraphNodes;slot++){
    int chosen=-1;
    for(int i=0;i<kGraphNodes;i++)if(!seen[i]){
      const auto &node=patch_.graph.nodes[i];
      bool ready=(node.type==NodeType::FeedbackDelay) ||
        ((node.a<0||seen[node.a])&&(node.b<0||seen[node.b]));
      for(const auto&m:patch_.modRoutes)if(m.source>=6&&m.depth!=0.f&&m.target==i&&!seen[m.source-6])ready=false;
      if(ready){chosen=i;break;}
    }
    if(chosen<0){graphValid_=false;break;}
    graphOrder_[slot]=chosen;seen[chosen]=true;
  }
 }
 // v1.3: compile the *audible* subgraph. Even if a patch has unused
 // SpectralMatter/Modal nodes, they must not consume CPU every voice/sample.
 // Preserve the existing full-graph cycle/type/modulation validation above.
 // Reverse-walk the combined topological order from the final Output.
 // Both PCM connections and live audio-rate modulation sources count.
 graphRenderCount_=0;
 const bool hasFeedback=std::any_of(patch_.graph.nodes.begin(),patch_.graph.nodes.end(),
     [](const GraphNode& n){return n.type==NodeType::FeedbackDelay;});
 if(graphValid_){
  const auto fullOrder=graphOrder_;
  std::array<bool,kGraphNodes> live{};
  live[kGraphNodes-1]=true;
  for(int slot=kGraphNodes-1;slot>=0;--slot){
   const int target=fullOrder[(size_t)slot];
   if(!live[(size_t)target])continue;
   const auto&node=patch_.graph.nodes[(size_t)target];
   if(node.a>=0)live[(size_t)node.a]=true;
   if(node.b>=0)live[(size_t)node.b]=true;
   for(const auto&route:patch_.modRoutes){
    if(route.target==target&&route.depth!=0.f&&route.source>=6)
     live[(size_t)(route.source-6)]=true;
   }
  }
  for(int slot=0;slot<kGraphNodes;slot++){
   const int node=fullOrder[(size_t)slot];
   // Explicit feedback requires the update source even if its source is a
   // forward reference; retain the entire graph for such patches.
   if(hasFeedback||live[(size_t)node])graphOrder_[(size_t)graphRenderCount_++]=node;
  }
 }
 cutoffHz_=80.0*std::pow((sr_*.42)/80.,patch_.cutoff);
 for(int i=0;i<kGraphNodes;i++){
   auto& node=patch_.graph.nodes[i];
   double freq=clamp(cutoffHz_*(.09+2.5*node.color),40.,sr_*.43);
   graphFilterCoeffs_[i]=1.-std::exp(-twoPi*freq/sr_);
   // v1.1: one cache per graph patch, shared by all voices. No note-on
   // sqrt work, no per-sample sqrt while Color remains unmodulated.
   if (node.type==NodeType::SpectralMatter) {
     const double stiff=.00009+.0016*double(node.color)*double(node.color);
     for(int j=0;j<12;j++){
       const double bin=j+1.;
       graphMatterFactors_[(size_t)i][(size_t)j]=
           std::sqrt((1.+stiff*bin*bin)/(1.+stiff));
     }
   }
   if (node.type==NodeType::MatterWeave) {
     const double dispersion=.00005+.00065*node.color*node.color;
     for (int j=0;j<12;j++) {
       const double bin=j+1.;
       graphWeaveRatios_[(size_t)i][(size_t)j]=
           bin*std::sqrt((1.+dispersion*bin*bin)/(1.+dispersion));
     }
   }
 }
}
void Synth::noteOn(int note,float velocity,int channel){
 if(note<0||note>127)return;
 if(velocity<=0){noteOff(note,channel);return;}
 Voice* candidate=nullptr;
 for(auto &v:voices_)if(!v.active){candidate=&v;break;}
 if(!candidate)candidate=&*std::min_element(voices_.begin(),voices_.end(),[](const Voice&a,const Voice&b){return a.age<b.age;});
 candidate->start(note,velocity,channel,++nextAge_,sr_,patch_);
}
void Synth::noteOff(int note,int channel){for(auto &v:voices_)if(v.active&&v.note==note&&v.channel==channel&&!v.releasedByKey){v.releasedByKey=true;if(!sustain_)v.env.release();}}
void Synth::sustain(bool down){if(sustain_&&!down)for(auto &v:voices_)if(v.active&&v.releasedByKey)v.env.release();sustain_=down;}
void Synth::allNotesOff(){sustain_=false;for(auto&v:voices_)v.env.release();}
void Synth::pitchBend(float semitones){bend_=float(clamp(semitones,-24,24));bendMultiplier_=std::exp2(bend_/12.0);}
void Synth::modWheel(float w){wheel_=float(clamp(w,0,1));}
AcousticStateFrame Synth::acousticStateForNote(int note) const noexcept {
 AcousticStateFrame result{};
 if(note<0||note>127)return result;
 // No allocation or resynthesis: read only actual voice oscillator/graph state.
 for(const auto& v:voices_){
  if(!v.active||v.note!=note)continue;
  if(patch_.engine==Engine::Modular){
   if(v.publishedAcoustic.domains)return v.publishedAcoustic;
  }else{
   result.domains=AcousticStateFrame::Phase;
   const double cycle=v.ph[0]-std::floor(v.ph[0]);
   result.phaseCycle[0]=std::isfinite(cycle)?cycle:0.;
   if(patch_.engine==Engine::PhysicalResonance || patch_.engine==Engine::Hybrid){
    // Modal displacement and velocity in the synth's internal modal convention.
    // NOT SI displacement; publish only phase for such factory modes until
    // explicitly calibrated. Avoid fabricating a metre-valued physical port.
   }
  }
  break;
 }
 return result;
}
bool Synth::injectStateResponseForNote(int note,int port,double normalizedDrive) noexcept {
 if(note<0||note>127||port<0||port>3||!std::isfinite(normalizedDrive))return false;
 const double u=clamp(normalizedDrive,-1.,1.);
 bool applied=false;
 for(auto& v:voices_){
  if(!v.active||v.note!=note)continue;
  if(patch_.engine!=Engine::Modular){
   // A legacy non-modular VA/FM/Hybrid voice has an oscillator phase;
   // do not fabricate a spectral/physical reservoir where none exists.
   if(port==0){
    if(patch_.engine==Engine::FractalDigital)v.subphase=fract(v.subphase+u*.0075);
    else {v.ph[0]=fract(v.ph[0]+u*.0075);
     if(patch_.engine==Engine::Hybrid)v.subphase=fract(v.subphase+u*.003);}
    applied=true;
   }
   continue;
  }
  for(int slot=0;slot<graphRenderCount_;slot++){
   const int i=graphOrder_[size_t(slot)];
   const auto type=patch_.graph.nodes[(size_t)i].type;
   auto& st=v.graphStates[(size_t)i];
   if(port==0){
    if(type==NodeType::VA||type==NodeType::Harmonic||type==NodeType::Digital||type==NodeType::PhaseWarp){
     st.phase=fract(st.phase+u*.0075);applied=true;
    }else if(type==NodeType::PhaseEcology){
     for(auto& phase:st.ecologyPhases)phase=fract(phase+u*.0075);
     applied=true;
    }else if(type==NodeType::SpectralMatter||type==NodeType::MatterWeave||
             type==NodeType::FormantField||type==NodeType::PhaseMatterBridge||type==NodeType::UnifiedCoupler){
     for(auto& phase:st.matterPhases)phase=fract(phase+u*.0075);
     applied=true;
    }
   }else if(port==1){
    if(type==NodeType::SpectralMatter||type==NodeType::MatterWeave||
       type==NodeType::FormantField||type==NodeType::PhaseMatterBridge||type==NodeType::UnifiedCoupler){
     for(auto& energy:st.matterEnergy)energy=clamp(energy+u*.012,0.,1.7);
     applied=true;
    }
   }else if(port==2||port==3){
    if(type==NodeType::PhysicalBody||type==NodeType::UnifiedCoupler){
     if(port==2)st.physicalDisplacement=clamp(st.physicalDisplacement+u*.004,-1.5,1.5);
     else st.physicalVelocityNorm=clamp(st.physicalVelocityNorm+u*.004,-1.5,1.5);
     applied=true;
    }
   }
  }
 }
 return applied;
}
int Synth::activeVoiceCount()const{int n=0;for(const auto&v:voices_)if(v.active)n++;return n;}
Sample Synth::processOne(){
 if(patch_.engine==Engine::Modular&&!graphValid_)return {};
 double l=0,r=0;
 for(auto &v:voices_)if(v.active){Sample s;v.process(sr_,patch_,bendMultiplier_,cutoffHz_,wheel_,s,graphOrder_,graphRenderCount_,graphFilterCoeffs_,graphWeaveRatios_,graphMatterFactors_);l+=s.left;r+=s.right;}
 // harmless voice headroom, smooth clip and DC blocker.
 l=std::tanh(l*.8);r=std::tanh(r*.8);
 double outL=l-dcLX_+.995*dcLY_;dcLX_=l;dcLY_=outL;
 double outR=r-dcRX_+.995*dcRY_;dcRX_=r;dcRY_=outR;
 return {float(outL),float(outR)};
}
void Synth::render(float* left,float* right,int n,const MidiEvent* events,size_t eventCount){
 if(n<=0||!left||!right)return;
 size_t ei=0;
 for(int i=0;i<n;i++){
  while(events&&ei<eventCount&&events[ei].sample<=i){const auto&e=events[ei++];
   switch(e.type){
   case MidiEvent::Type::NoteOn:noteOn(e.note,e.value,e.channel);break;
   case MidiEvent::Type::NoteOff:noteOff(e.note,e.channel);break;
   case MidiEvent::Type::PitchBend:pitchBend(e.value);break;
   case MidiEvent::Type::Sustain:sustain(e.value>=.5);break;
   case MidiEvent::Type::ModWheel:modWheel(e.value);break;
   case MidiEvent::Type::AllNotesOff:allNotesOff();break;
   }
  }
  const Sample s=processOne();left[i]=s.left;right[i]=s.right;
 }
}
}
