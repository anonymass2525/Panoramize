#include "PanoramaDSP.h"
#include <algorithm>
#ifdef NDEBUG
#undef NDEBUG // run all assertions even in Release builds
#endif
#include <cassert>
#include <cmath>
#include <iostream>
#include <numeric>
#include <vector>
using namespace panorama;
static std::vector<float> take(int preset,int note=60){
 Synth s;s.prepare(48000);s.setPatch(factoryPresets()[preset]);s.noteOn(note,.85f);
 std::vector<float> a(24000),b(24000);
 s.render(a.data(),b.data(),(int)a.size());
 s.noteOff(note);return a;
}
static double rms(const std::vector<float>&a){double s=0;for(float v:a){assert(std::isfinite(v));s+=double(v)*v;}return std::sqrt(s/a.size());}
int main(){
 Synth s;s.prepare(48000);assert(s.activeVoiceCount()==0);assert(kPresetCount==72);
 // v1.2: the host patch cache must detect EVERY meaningful parameter change,
 // including graph wiring, audio-rate modulation, and factory program names.
 {
  const auto original=factoryPresets()[52];
  auto edited=original;
  assert(samePatchConfiguration(original,edited));
  edited.cutoff=.42f;assert(!samePatchConfiguration(original,edited));edited=original;
  edited.motion=.42f;assert(!samePatchConfiguration(original,edited));edited=original;
  edited.graph.nodes[0].color=.42f;assert(!samePatchConfiguration(original,edited));edited=original;
  edited.graph.nodes[8].a=1;assert(!samePatchConfiguration(original,edited));edited=original;
  edited.modRoutes[0].depth=.91f;assert(!samePatchConfiguration(original,edited));edited=original;
  edited.modRoutes[1].source=5;assert(!samePatchConfiguration(original,edited));edited=original;
  edited.name="Different Program";assert(!samePatchConfiguration(original,edited));
 }

 // v1.3: dead expensive DSP nodes must be omitted while upstream
 // audio/modulation dependencies still run (including zero-depth edits).
 {
  auto patch=factoryPresets()[24];
  patch.engine=Engine::Modular;
  patch.modRoutes={};
  patch.graph=GraphPatch{};
  patch.graph.nodes[0]={NodeType::VA,-1,-1,.83f,.45f};
  patch.graph.nodes[1]={NodeType::SpectralMatter,-1,-1,.72f,.75f};
  patch.graph.nodes[2]={NodeType::FormantField,1,-1,.9f,.5f};
  patch.graph.nodes[3]={NodeType::MatterWeave,1,2,.8f,.5f};
  patch.graph.nodes[4]={NodeType::ResonantExchange,0,0,.8f,.5f};
  patch.graph.nodes[8]={NodeType::Output,0,-1,.83f,.4f};
  Synth pruned;pruned.prepare(48000);pruned.setPatch(patch);
  assert(pruned.graphValid());
  assert(pruned.renderedGraphNodeCount()==2); // VA + Output
  pruned.noteOn(60,.85f);double sum=0.;
  for(int i=0;i<2048;i++) {auto x=pruned.processOne();sum+=x.left*x.left;}
  assert(sum>1.e-6);
  // A muted audio-rate modulation lane is NOT a dependency.
  patch.modRoutes[0]={7,0,0,0.f};
  pruned.setPatch(patch);assert(pruned.renderedGraphNodeCount()==2);
  // When enabled the spectral source becomes live, even though its audio
  // is not mixed to Output. SpectralMatter's output modulates VA Color.
  patch.modRoutes[0].depth=.70f;
  pruned.setPatch(patch);
  assert(pruned.graphValid() && pruned.renderedGraphNodeCount()==3);
  // Subsequent patch automation that reconnects a heavy generator
  // must update the execution graph without losing its validation.
  patch.modRoutes[0].depth=0.f;
  patch.graph.nodes[8].b=3;
  pruned.setPatch(patch);
  assert(pruned.graphValid() && pruned.renderedGraphNodeCount()==5);
  patch.graph.nodes[0].a=3;
  patch.graph.nodes[3].a=0; // cycle, even if it were on a dead path
  pruned.setPatch(patch);assert(!pruned.graphValid());
 }
 // 1.4: Phase-Matter Bridge consumes typed internal states, never PCM.
 {
  auto patch=factoryPresets()[56];
  std::array<int,kGraphNodes> sorted{};
  assert(compileGraph(patch.graph,sorted));
  assert(patch.graph.nodes[2].type==NodeType::PhaseMatterBridge);
  // Directly output the bridge and remove modulation so no secondary
  // oscillator or audio route can explain the resulting sound.
  patch.graph.nodes[8]={NodeType::Output,2,-1,.9f,0.f};
  patch.modRoutes={};
  // Both source nodes are PCM-muted, but typed state transfer still sounds.
  patch.graph.nodes[0].level=0.f;
  patch.graph.nodes[1].level=0.f;
  auto synthRms=[](Patch x){Synth z;z.prepare(48000);z.setPatch(x);z.noteOn(60,.8f);
   double e=0.;for(int n=0;n<4096;n++){auto s=z.processOne();
    assert(std::isfinite(s.left)&&std::isfinite(s.right));e+=double(s.left)*s.left;}
   return e/4096.;};
  const double baseline=synthRms(patch);
  assert(baseline>1.e-7);
  // Replace the phase oscillator with a different phase network, while
  // keeping its PCM gain zero: output must still change.
  auto other=patch;other.graph.nodes[1].color=.09f;
  Synth x,y;x.prepare(48000);y.prepare(48000);
  x.setPatch(patch);y.setPatch(other);x.noteOn(60,.8f);y.noteOn(60,.8f);
  double delta=0.;for(int n=0;n<4096;n++){
   const auto a=x.processOne(),b=y.processOne();
   delta+=double(a.left-b.left)*(a.left-b.left);
  } assert(delta/4096.>1.e-9);
  // Typed-source validation forbids audio-only PCM driving the phase bus.
  other.graph.nodes[1].type=NodeType::Noise;
  assert(!compileGraph(other.graph,sorted));
  other=patch;other.graph.nodes[2].a=1;
  assert(!compileGraph(other.graph,sorted)); // phase-only A cannot provide energy
  // Phase-Matter nodes may feed existing typed spectral processing.
  auto next=factoryPresets()[58];
  assert(compileGraph(next.graph,sorted));
  // This graph contains both FormantField and the bridge.
  assert(next.graph.nodes[5].type==NodeType::FormantField);
  // No cycles, including one carried by the typed bus.
  next.graph.nodes[0].a=4;
  assert(!compileGraph(next.graph,sorted));
 }
 std::vector<std::vector<float>> captures;
 for(int i=0;i<kPresetCount;i++){
  auto x=take(i);const auto r=rms(x);std::cout<<i+1<<" "<<factoryPresets()[i].name<<" rms="<<r<<"\n";
  assert(r>.00015 && r<.95);
  captures.push_back(std::move(x));
 }
 // Every adjacent preset must differ as produced signals, not only in naming.
 for(int i=1;i<kPresetCount;i++){
  double mse=0;for(size_t j=0;j<captures[i].size();j++){const double d=captures[i][j]-captures[i-1][j];mse+=d*d;}
  mse/=captures[i].size(); assert(mse>1e-8);
 }
 s.setPatch(factoryPresets()[0]);
 s.noteOn(60,1);s.noteOn(64,1);s.noteOn(67,1);assert(s.activeVoiceCount()==3);
 s.allNotesOff();std::vector<float>l(48000*6),r(l.size());s.render(l.data(),r.data(),(int)l.size());
 assert(s.activeVoiceCount()==0);
 // event sample-accuracy: no sound before start note.
 s.prepare(48000);s.setPatch(factoryPresets()[0]);
 std::vector<float>a(8000),b(8000);
 MidiEvent es[]={ {MidiEvent::Type::NoteOn,1000,60,1,1},{MidiEvent::Type::NoteOff,4000,60,1,0} };
 s.render(a.data(),b.data(),8000,es,2);
 for(int i=0;i<1000;i++)assert(a[i]==0&&b[i]==0);
 assert(rms(a)>.0001);
 // effect of physical -> direct generated sound; hybrid should be different from other three.
 for(int i=0;i<4;i++)assert(rms(captures[i*3])>.00015);
 // Explicit cross-synthesis ablation: changing physical contribution must change actual samples.
 auto hybrid=factoryPresets()[10];
 hybrid.engine=Engine::Hybrid;
 Synth dry;dry.prepare(48000);hybrid.blend=0;dry.setPatch(hybrid);dry.noteOn(60,.8f);
 Synth wet;wet.prepare(48000);hybrid.blend=1;wet.setPatch(hybrid);wet.noteOn(60,.8f);
 double crossDifference=0;
 for(int i=0;i<4096;i++){
  const auto a=dry.processOne(),b=wet.processOne();
  crossDifference+=(a.left-b.left)*(a.left-b.left);
 }
 assert(crossDifference/4096.0>.000001);
 // v0.2: a change of cross-routing topology must alter audible PCM.
 // Also prove crossDepth is not merely a visual parameter.
 auto renderPatch=[](Patch p){ Synth synth;synth.prepare(48000);synth.setPatch(p);
  synth.noteOn(60,.9f);std::vector<float>v(12000);
  for(auto &sample:v)sample=synth.processOne().left;
  return v;};
 auto compare=[](const auto&a,const auto&b){double mse=0.;
  for(size_t i=0;i<a.size();i++){const double d=a[i]-b[i];mse+=d*d;}
  return mse/a.size();};
 auto baseline=factoryPresets()[12]; baseline.routing=0;
 const auto classic=renderPatch(baseline);
 for(int route=1;route<=4;route++){
   baseline.routing=route;baseline.crossDepth=.9f;baseline.stereoResonance=.83f;
   auto audio=renderPatch(baseline);
   assert(rms(audio)>.0001);
   assert(compare(classic,audio)>1.e-6);
   auto zeroDepth=baseline;zeroDepth.crossDepth=0;
   assert(compare(audio,renderPatch(zeroDepth))>1.e-8);
 }
 // Ensure changing routed stereo width affects channels for identical notes.
 auto wide=factoryPresets()[15];wide.routing=4;wide.stereoResonance=1;
 Synth wideSynth;wideSynth.prepare(48000);wideSynth.setPatch(wide);wideSynth.noteOn(58,.9f);
 double stereoDiff=0;
 for(int i=0;i<12000;i++){auto sm=wideSynth.processOne();
  assert(std::isfinite(sm.left)&&std::isfinite(sm.right));
  stereoDiff+=double(sm.left-sm.right)*(sm.left-sm.right);}
 assert(stereoDiff/12000>1.e-9);
 // polyphony ceiling + finite data at several sample rates
 for(int rate:{44100,48000,96000}){
  s.prepare(rate);s.setPatch(factoryPresets()[10]);for(int n=36;n<70;n++)s.noteOn(n,.8f);
  assert(s.activeVoiceCount()==kMaxVoices);
  std::vector<float>x(16384),y(16384);
  s.render(x.data(),y.data(),(int)x.size());assert(rms(x)>.0001);assert(rms(y)>.0001);
 }
 // Hotfix 2: selecting Modular on a legacy factory preset previously produced
 // complete silence because all node types and both OUT inputs were empty.
 // All original factory sounds must remain untouched; only the explicitly
 // selected Modular engine receives a starter graph.
 {
  const auto starter=makeStarterModularGraph();
  std::array<int,kGraphNodes> order{};
  assert(compileGraph(starter,order));
  assert(starter.nodes[8].type==NodeType::Output);
  assert(starter.nodes[8].a>=0);
  assert(!isUnconfiguredModularGraph(starter));
  for(int i=0;i<16;i++){
   auto patch=factoryPresets()[i];
   assert(isUnconfiguredModularGraph(patch.graph));
   patch.engine=Engine::Modular;
   Synth migrated;migrated.prepare(48000);migrated.setPatch(patch);
   assert(migrated.graphValid());
   assert(migrated.patch().graph.nodes[0].type==NodeType::Harmonic);
   auto signal=renderPatch(patch);
   assert(rms(signal)>0.00001);
  }
  // A deliberate zero-output graph is not silently replaced.
  auto intentional=factoryPresets()[0];intentional.engine=Engine::Modular;
  intentional.graph.nodes[0]={NodeType::Off,-1,-1,.8f,.5f};
  intentional.graph.nodes[8]={NodeType::Output,0,-1,.8f,.5f};
  assert(!isUnconfiguredModularGraph(intentional.graph));
  Synth quiet;quiet.prepare(48000);quiet.setPatch(intentional);
  assert(quiet.graphValid());
  assert(rms(renderPatch(intentional))==0.);
 }
 // v0.3 genuine graph tests: compiled order, edit effect, invalid cycles, state reset.
 for(int i=16;i<kPresetCount;i++){
   const auto patch=factoryPresets()[i];
   assert(patch.engine==Engine::Modular);
   std::array<int,kGraphNodes> order{};
   assert(compileGraph(patch.graph,order));
   assert(order[kGraphNodes-1]==8); // All factory graphs have the fixed OUT as final sink.
   const auto audio=renderPatch(patch);
   assert(rms(audio)>.001 && rms(audio)<.98);
   // Routing changes must alter generated PCM without replacing the engine.
   auto edited=patch;
   edited.graph.nodes[8].a=0;
   edited.graph.nodes[8].b=1;
   edited.graph.nodes[8].color=.66f;
   const auto different=renderPatch(edited);
   assert(compare(audio,different)>1e-7);
   // Knobs modify audio-rate processing for a connected source.
   auto colorEdited=patch;
   colorEdited.graph.nodes[8].level=.10f;
   assert(compare(audio,renderPatch(colorEdited))>1e-8);
 }
 // Out-of-index order is supported by real topological sorting.
 {
   auto patch=factoryPresets()[16];
   patch.graph.nodes[0].type=NodeType::Gain;
   patch.graph.nodes[0].a=7;
   patch.graph.nodes[7].type=NodeType::VA;
   patch.graph.nodes[7].a=-1;patch.graph.nodes[7].b=-1;
   patch.graph.nodes[8].a=0;patch.graph.nodes[8].b=-1;
   std::array<int,kGraphNodes> order{};
   assert(compileGraph(patch.graph,order));
   assert(std::find(order.begin(),order.end(),7)<std::find(order.begin(),order.end(),0));
   assert(rms(renderPatch(patch))>.0001);
 }
 {
   auto patch=factoryPresets()[16];
   patch.graph.nodes[0].a=4;
   patch.graph.nodes[4].a=0;
   std::array<int,kGraphNodes> order{};
   assert(!compileGraph(patch.graph,order));
   Synth noCycle;noCycle.prepare(48000);noCycle.setPatch(patch);assert(!noCycle.graphValid());
   noCycle.noteOn(60,1.f);
   for(int i=0;i<256;i++){const auto z=noCycle.processOne();assert(z.left==0.f&&z.right==0.f);}
   patch.graph.nodes[0].a=-1;
   noCycle.setPatch(patch);assert(noCycle.graphValid());
   noCycle.noteOn(60,1.f);
   double energy=0;
   for(int i=0;i<4096;i++){const auto z=noCycle.processOne();assert(std::isfinite(z.left)&&std::isfinite(z.right));energy+=z.left*z.left;}
   assert(energy>1e-5);
 }
 // Note sample-scheduling and multivoice stability also hold for modular patches.
 for(int rate:{44100,48000,96000}){
   Synth synth;synth.prepare(rate);synth.setPatch(factoryPresets()[23]);
   for(int n=40;n<76;n++)synth.noteOn(n,.92f);
   assert(synth.activeVoiceCount()==kMaxVoices);
   for(int i=0;i<1500;i++){
     auto sample=synth.processOne();
     assert(std::isfinite(sample.left)&&std::isfinite(sample.right));
     assert(std::fabs(sample.left)<1.01 && std::fabs(sample.right)<1.01);
   }
 }
 // v0.4.2: prepare() must rederive filter coefficients and graph order.
 // The very same patch rendered after 48k -> 96k reconfiguration must match
 // a fresh 96k instance, even without an intervening setPatch() call.
 for(int idx:{18,23,24,29,31}){
   const auto patch=factoryPresets()[idx];
   Synth reused;reused.prepare(48000);reused.setPatch(patch);reused.prepare(96000);
   Synth fresh;fresh.prepare(96000);fresh.setPatch(patch);
   assert(reused.graphValid()&&fresh.graphValid());
   reused.noteOn(57,.9f);fresh.noteOn(57,.9f);
   for(int i=0;i<2048;i++){
     const auto a=reused.processOne(),b=fresh.processOne();
     assert(std::isfinite(a.left)&&std::isfinite(a.right));
     assert(a.left==b.left&&a.right==b.right);
   }
 }
 // v0.4: modulation influences actual PCM (LFO, envelope, note signal), not just UI.
 for(int idx=24;idx<32;idx++){
   auto patch=factoryPresets()[idx];
   assert(validateModRoutes(patch.graph,patch.modRoutes));
   auto audio=renderPatch(patch);
   assert(rms(audio)>.0001 && rms(audio)<.98);
   auto ablated=patch;
   ablated.modRoutes[0].depth=0.f;
   assert(compare(audio,renderPatch(ablated))>1e-10);
   auto ablated2=patch;
   ablated2.modRoutes[1].depth=0.f;
   assert(compare(audio,renderPatch(ablated2))>1e-10);
 }
 // v0.4.1: an unconnected (zero-depth) audio modulation lane must never
 // silence a patch while a user edits source/target before raising its depth.
 {
   auto patch=factoryPresets()[24];
   patch.modRoutes[0]={6,0,0,0.f}; // node0->self, but disabled
   assert(validateModRoutes(patch.graph,patch.modRoutes));
   Synth synth;synth.prepare(48000);synth.setPatch(patch);
   assert(synth.graphValid());
   assert(rms(renderPatch(patch))>.0001);
   patch.modRoutes[0].depth=.7f;
   assert(!validateModRoutes(patch.graph,patch.modRoutes));
 }
 // The mod graph is independent of, but must remain acyclic with, the signal graph.
 {
  auto patch=factoryPresets()[24];
  patch.modRoutes[0]={6,0,0,.7f}; // node0 modulates itself
  assert(!validateModRoutes(patch.graph,patch.modRoutes));
  Synth invalid;invalid.prepare(48000);invalid.setPatch(patch);
  assert(!invalid.graphValid());
  invalid.noteOn(60,.8f);
  for(int n=0;n<512;n++){const auto x=invalid.processOne();assert(x.left==0.f&&x.right==0.f);}
  // If audio goes from node0 -> node3, node3 cannot audio-modulate node0.
  patch.modRoutes[0]={9,0,0,.6f};
  assert(!validateModRoutes(patch.graph,patch.modRoutes));
 }
 for(int rate:{44100,48000,96000}){
  for(int idx=24;idx<32;idx++){
   Synth synth;synth.prepare(rate);synth.setPatch(factoryPresets()[idx]);assert(synth.graphValid());
   for(int n=48;n<65;n+=4)synth.noteOn(n,.88f);
   for(int j=0;j<2400;j++){
    const auto z=synth.processOne();
    assert(std::isfinite(z.left)&&std::isfinite(z.right));
    assert(std::abs(z.left)<1.02f&&std::abs(z.right)<1.02f);
   }
  }
 }
 // v0.5: new coupled-resonance node must be audible, responsive to both
 // audio inputs, and safe under polyphony, retuning and sample-rate changes.
 for(int idx=32;idx<36;idx++){
  const auto patch=factoryPresets()[idx];
  std::array<int,kGraphNodes> order{};
  assert(patch.engine==Engine::Modular);
  assert(compileGraph(patch.graph,order));
  assert(validateModRoutes(patch.graph,patch.modRoutes));
  const auto base=renderPatch(patch);
  assert(rms(base)>.001 && rms(base)<.95);
  auto noB=patch;
  for(auto& node:noB.graph.nodes)if(node.type==NodeType::CoupledModes)node.b=-1;
  assert(validateModRoutes(noB.graph,noB.modRoutes));
  // Different spectral trajectories, not merely the same output times gain.
  const double diff=compare(base,renderPatch(noB));
  assert(diff>1.e-8);
  for(int rate:{44100,48000,96000}){
   Synth synth;synth.prepare(rate);synth.setPatch(patch);
   assert(synth.graphValid());
   for(int n=44;n<80;n+=3)synth.noteOn(n,.95f);
   for(int i=0;i<4096;i++){
    const auto x=synth.processOne();
    assert(std::isfinite(x.left)&&std::isfinite(x.right));
    assert(std::abs(x.left)<1.02 && std::abs(x.right)<1.02);
   }
   // Reinitialization at a different rate must not retain stale coupled state.
   synth.prepare(48000);synth.setPatch(patch);
   Synth fresh;fresh.prepare(48000);fresh.setPatch(patch);
   synth.noteOn(60,.8f);fresh.noteOn(60,.8f);
   for(int i=0;i<512;i++){
    const auto x=synth.processOne(),y=fresh.processOne();
    assert(x.left==y.left && x.right==y.right);
   }
  }
 }
 // A/B semantics: B is an acoustic control, not a sound mixed into the output.
 // Without A there must be silence; changing B with A held fixed must alter
 // waveform geometry after optimal static amplitude matching.
 {
  auto p=factoryPresets()[32];
  p.graph.nodes[8].a=2;p.graph.nodes[8].b=-1;
  p.modRoutes={};
  const auto wet=renderPatch(p);
  assert(rms(wet)>.0001);
  auto noExciter=p;noExciter.graph.nodes[0].type=NodeType::Off;
  assert(rms(renderPatch(noExciter))==0.);
  auto noControl=p;noControl.graph.nodes[1].type=NodeType::Off;
  const auto dry=renderPatch(noControl);
  double projection=0.,denom=0.;
  for(size_t i=0;i<wet.size();i++){
   projection+=double(wet[i])*dry[i];
   denom+=double(dry[i])*dry[i];
  }
  const double scale=projection/(denom+1.e-20);
  double residual=0.;
  for(size_t i=0;i<wet.size();i++){
   const double error=wet[i]-scale*dry[i];
   residual+=error*error;
  }
  assert(residual/wet.size()>1.e-7);
 }
 // Reject audio cycles involving a coupled resonator, even if a mod lane is muted.
 {
  auto p=factoryPresets()[32];
  p.graph.nodes[0].a=2; // coupled node 2 already depends on node 0
  std::array<int,kGraphNodes> order{};
  assert(!compileGraph(p.graph,order));
 }
 // v0.6: new presets must have valid DAGs, non-silent output,
 // deterministic sample-rate reinitialisation, bounded polyphonic output.
 for(int idx=36;idx<kPresetCount;idx++){
  const auto patch=factoryPresets()[idx];
  std::array<int,kGraphNodes> order{};
  assert(patch.engine==Engine::Modular);
  assert(compileGraph(patch.graph,order));
  assert(validateModRoutes(patch.graph,patch.modRoutes));
  assert(rms(renderPatch(patch))>.0004);
  for(int rate:{44100,48000,96000}){
   Synth synth;synth.prepare(rate);synth.setPatch(patch);
   assert(synth.graphValid());
   for(int n=45;n<80;n+=5)synth.noteOn(n,.88f);
   for(int t=0;t<4096;t++){
    auto z=synth.processOne();
    assert(std::isfinite(z.left)&&std::isfinite(z.right));
    assert(std::abs(z.left)<1.02f&&std::abs(z.right)<1.02f);
   }
   synth.prepare(48000);synth.setPatch(patch);
   Synth fresh;fresh.prepare(48000);fresh.setPatch(patch);
   synth.noteOn(60,.8f);fresh.noteOn(60,.8f);
   for(int t=0;t<512;t++){
    auto x=synth.processOne(),y=fresh.processOne();
    assert(x.left==y.left&&x.right==y.right);
   }
  }
 }
 // A provides energy. B changes *internal exchange* but cannot create
 // audio alone. Audio from B must change waveform beyond a constant gain.
 {
  auto patch=factoryPresets()[36];
  patch.graph.nodes[8].a=2;
  patch.graph.nodes[8].b=-1;
  patch.modRoutes={};
  const auto full=renderPatch(patch);
  assert(rms(full)>.0001);
  auto noA=patch;noA.graph.nodes[0].type=NodeType::Off;
  assert(rms(renderPatch(noA))==0.);
  auto noB=patch;noB.graph.nodes[1].type=NodeType::Off;
  const auto withoutB=renderPatch(noB);
  assert(rms(withoutB)>.0001);
  double dot=0.,sq=0.;
  for(size_t i=0;i<full.size();i++){
   dot+=double(full[i])*withoutB[i];
   sq+=double(withoutB[i])*withoutB[i];
  }
  const double scale=dot/(sq+1.e-20);
  double residual=0.;
  for(size_t i=0;i<full.size();i++){
   const auto err=full[i]-scale*withoutB[i];
   residual+=err*err;
  }
  assert(residual/full.size()>1.e-8);
  // Reject zero-delay graph feedback.
  auto cycle=patch;
  cycle.graph.nodes[0].a=2;
  std::array<int,kGraphNodes> loopOrder{};
  assert(!compileGraph(cycle.graph,loopOrder));
 }
 // v0.7: Phase Ecology is an autonomous network of six partial phases.
 // Both audio inputs affect its dynamics; changing them must change waveform
 // beyond a static gain scale. All sample rates and high-polyphony should be finite.
 for(int idx=40;idx<44;idx++){
  const auto patch=factoryPresets()[idx];
  assert(patch.engine==Engine::Modular);
  std::array<int,kGraphNodes> order{};
  assert(compileGraph(patch.graph,order));
  assert(validateModRoutes(patch.graph,patch.modRoutes));
  assert(rms(renderPatch(patch))>.00015);
  for(int rate:{44100,48000,96000}){
   Synth synth;synth.prepare(rate);synth.setPatch(patch);
   assert(synth.graphValid());
   for(int n=45;n<84;n+=4)synth.noteOn(n,.90f);
   for(int k=0;k<4096;k++){
    auto z=synth.processOne();
    assert(std::isfinite(z.left)&&std::isfinite(z.right));
    assert(std::abs(z.left)<1.02&&std::abs(z.right)<1.02);
   }
   synth.prepare(48000);
   Synth fresh;fresh.prepare(48000);fresh.setPatch(patch);
   synth.noteOn(60,.83f);fresh.noteOn(60,.83f);
   for(int t=0;t<256;t++){
    const auto a=synth.processOne(),b=fresh.processOne();
    assert(a.left==b.left&&a.right==b.right);
   }
  }
 }
 // Isolate the ecology network to test the causal contribution of each input.
 {
  auto base=factoryPresets()[40];
  base.graph.nodes[8].a=2;base.graph.nodes[8].b=-1;
  base.modRoutes={};
  const auto full=renderPatch(base);
  assert(rms(full)>.0001);
  auto withoutA=base;withoutA.graph.nodes[2].a=-1;
  auto withoutB=base;withoutB.graph.nodes[2].b=-1;
  const auto noA=renderPatch(withoutA),noB=renderPatch(withoutB);
  assert(compare(full,noA)>1.e-8);
  assert(compare(full,noB)>1.e-8);
  assert(rms(noA)>.0001&&rms(noB)>.0001); // oscillator is self-running
  // Audio modulation dependency cycles remain forbidden.
  auto cycle=base;cycle.graph.nodes[0].a=2;
  std::array<int,kGraphNodes> bad{};
  assert(!compileGraph(cycle.graph,bad));
 }
 // v0.8: Spectral Matter is a twelve-partial stateful oscillator, deliberately
 // NOT backward PCM-compatible with the v0.7 timbres it replaces.
 for(int idx=44;idx<48;idx++){
  const auto patch=factoryPresets()[(size_t)idx];
  std::array<int,kGraphNodes> compiled{};
  assert(patch.engine==Engine::Modular);
  assert(compileGraph(patch.graph,compiled));
  assert(validateModRoutes(patch.graph,patch.modRoutes));
  assert(rms(renderPatch(patch))>.001);
  for(int rate:{44100,48000,96000}){
   Synth synth;synth.prepare(rate);synth.setPatch(patch);
   assert(synth.graphValid());
   for(int note=42;note<=102;note+=4)synth.noteOn(note,.9f);
   for(int sample=0;sample<8192;sample++){
    const auto z=synth.processOne();
    assert(std::isfinite(z.left)&&std::isfinite(z.right));
    assert(std::abs(z.left)<=1.01f&&std::abs(z.right)<=1.01f);
   }
   // prepare() must derive the same signal as a fresh instance at new rate.
   synth.prepare(48000);
   Synth fresh;fresh.prepare(48000);fresh.setPatch(patch);
   synth.noteOn(60,.8f);fresh.noteOn(60,.8f);
   for(int sample=0;sample<512;sample++){
    const auto x=synth.processOne(),y=fresh.processOne();
    assert(x.left==y.left&&x.right==y.right);
   }
  }
 }
 // The two audio inputs of Spectral Matter must reshape its evolving
 // spectrum; ablation changes the signal, not just the level parameter.
 {
  auto patch=factoryPresets()[44];
  patch.graph.nodes[8].a=2;patch.graph.nodes[8].b=-1;
  patch.modRoutes={};
  const auto full=renderPatch(patch);
  assert(rms(full)>.001);
  auto noA=patch;noA.graph.nodes[2].a=-1;
  auto noB=patch;noB.graph.nodes[2].b=-1;
  const auto xa=renderPatch(noA),xb=renderPatch(noB);
  assert(compare(full,xa)>1.e-9);
  assert(compare(full,xb)>1.e-9);
  assert(rms(xa)>.0001&&rms(xb)>.0001);
  // A high-register patch stays finite as some partials cross the fade band.
  Synth high;high.prepare(44100);high.setPatch(patch);high.noteOn(119,.85f);
  for(int i=0;i<12000;i++){
   const auto s=high.processOne();
   assert(std::isfinite(s.left)&&std::isfinite(s.right));
   assert(std::abs(s.left)<=1.01f&&std::abs(s.right)<=1.01f);
  }
  // An explicit graph feedback loop must remain prohibited.
  auto bad=patch;bad.graph.nodes[0].a=2;
  std::array<int,kGraphNodes> order{};
  assert(!compileGraph(bad.graph,order));
 }
 // Re-authoring is intentional, and the newly chosen root sources are tested.
 assert(factoryPresets()[16].graph.nodes[0].type==NodeType::SpectralMatter);
 assert(factoryPresets()[24].graph.nodes[0].type==NodeType::SpectralMatter);
 assert(factoryPresets()[42].graph.nodes[0].type==NodeType::SpectralMatter);
 // v0.9 typed spectral-state bus: topology validates spectral-only links,
 // each patch produces bounded audio across the standard host sample rates.
 for(int idx=48;idx<52;idx++){
  const auto patch=factoryPresets()[(size_t)idx];
  std::array<int,kGraphNodes> order{};
  assert(compileGraph(patch.graph,order));
  assert(validateModRoutes(patch.graph,patch.modRoutes));
  for(int rate:{44100,48000,96000}){
   Synth synth;synth.prepare(rate);synth.setPatch(patch);
   assert(synth.graphValid());
   for(int n=43;n<75;n+=6)synth.noteOn(n,.85f);
   double power=0.;
   for(int k=0;k<4096;k++){
    auto sample=synth.processOne();
    assert(std::isfinite(sample.left)&&std::isfinite(sample.right));
    assert(std::abs(sample.left)<1.02f&&std::abs(sample.right)<1.02f);
    power+=double(sample.left)*sample.left;
   }
   assert(power/4096.>1.e-8);
  }
 }
 // Key invariant: two upstream spectral frames can differ while their
 // AUDIO outputs remain *exactly zero*. MatterWeave must still react to the
 // spectral frame B, proving this isn't audio-sidechain modulation.
 {
  auto patch=factoryPresets()[48];
  patch.modRoutes={};
  patch.graph.nodes[8]={NodeType::Output,2,-1,.90f,0.f};
  patch.graph.nodes[0].level=0.f;
  patch.graph.nodes[1].level=0.f;
  auto a=renderPatch(patch);
  assert(rms(a)>1.e-4);
  patch.graph.nodes[1].color=.03f;
  auto b=renderPatch(patch);
  assert(rms(b)>1.e-4);
  assert(compare(a,b)>1.e-7);
  // Invalid state wiring is rejected before rendering.
  auto bad=patch;bad.graph.nodes[2].a=-1;
  std::array<int,kGraphNodes> order{};
  assert(!compileGraph(bad.graph,order));
  bad=patch;bad.graph.nodes[1].type=NodeType::VA;
  assert(!compileGraph(bad.graph,order));
  // This is a one-way DAG, not a zero-delay feedback network.
  bad=patch;bad.graph.nodes[0].a=2;
  assert(!compileGraph(bad.graph,order));
 }
 std::cout<<"PANORAMA DSP ALL TESTS PASS\n";
 // v1.0 formant field receives *spectral state*, not audio. B is an
 // independent gesture. It can itself be a typed input to Matter Weave.
 for(int idx=52;idx<kPresetCount;idx++){
   auto patch=factoryPresets()[idx];
   assert(patch.engine==Engine::Modular);
   std::array<int,kGraphNodes> order{};
   assert(compileGraph(patch.graph,order));
   for(int rate:{44100,48000,96000}){
      Synth instrument;instrument.prepare(rate);instrument.setPatch(patch);
      instrument.noteOn(52,.8f);
      double power=0.;
      for(int i=0;i<4096;i++){
       const auto x=instrument.processOne();
       assert(std::isfinite(x.left)&&std::isfinite(x.right));
       assert(std::abs(x.left)<1.1f && std::abs(x.right)<1.1f);
       power+=x.left*x.left+x.right*x.right;
      }
      assert(power/8192.>.0000001);
   }
 }
 {
   auto patch=factoryPresets()[52];
   auto bad=patch;bad.graph.nodes[2].type=NodeType::VA;
   std::array<int,kGraphNodes> order{};
   assert(!compileGraph(bad.graph,order)); // Formant A must be typed
   // Nulling the *audio output* of spectral A keeps downstream active:
   patch.graph.nodes[0].level=0.f;
   patch.graph.nodes[1].level=0.f;
   assert(compileGraph(patch.graph,order));
   auto render=[&](Patch p){Synth sy;sy.prepare(48000);sy.setPatch(p);sy.noteOn(60,.9f);
     std::vector<float> out(8192);
     for(auto &x:out)x=sy.processOne().left;
     return out;
   };
   const auto tone=render(patch);
   assert(rms(tone)>.0001); // audio-muted states still yield sound
   auto modified=patch;
   modified.graph.nodes[3].color=.9f;
   const auto changed=render(modified);
   double difference=0.;for(size_t i=0;i<tone.size();i++){
     const double d=tone[i]-changed[i];difference+=d*d;
   }
   assert(difference/tone.size()>1.e-8);
 }

}
