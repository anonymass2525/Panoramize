// Host-independent JUCE integration smoke test for Windows.
// Runs with the same Processor/Editor implementation as the VST3 target.
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "GraphInverse.h"
#include <JuceHeader.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>

namespace {
int failures=0;
void require(bool cond,const char*message){
 if(!cond){std::cerr << "FAIL: " << message << "\n"; ++failures;}
}
double render(PanoramaProcessor& proc, int frames){
 juce::AudioBuffer<float> audio(2,frames);
 audio.clear();
 juce::MidiBuffer midi;
 midi.addEvent(juce::MidiMessage::noteOn(1,60,static_cast<juce::uint8>(100)),0);
 midi.addEvent(juce::MidiMessage::noteOff(1,60),std::max(1,frames-1));
 proc.processBlock(audio,midi);
 double energy=0;
 for(int ch=0;ch<2;ch++)
  for(int i=0;i<frames;i++){
   const float x=audio.getSample(ch,i);
   require(std::isfinite(x),"audio contains NaN / Inf");
   energy+=double(x)*x;
  }
 return energy;
}
}
int main(){
 juce::ScopedJuceInitialiser_GUI gui;
 // Construction must work even when an editor has not been created.
 PanoramaProcessor synth;
 require(synth.getNumPrograms()==panorama::kPresetCount,"expected 72 factory programs");
 require(synth.acceptsMidi() && synth.hasEditor(),"MIDI / editor capability");
 require(synth.isUniversalEnabled(),"v2.3 new sessions default to unified graph");
 // Keep all previous tests on the legacy path; unified mode is tested separately.
 synth.setUniversalEnabled(false);
 // v1.7: a generated graph recipe is installed via APVTS and survives DAW
 // save/restore. Parser rejects bad recipes without mutating the patch.
 {
  panorama::GraphInverseResult inferred;
  inferred.success=true;inferred.patch=panorama::factoryPresets()[68];
  inferred.distance=0.;inferred.evaluated=54;
  const auto json=panorama::graphInverseJson(inferred);
  require(synth.importGraphRecipeJson(juce::String(json.c_str())),
          "inverse JSON recipe imports into host parameters");
  require(synth.getPatchFromParameters().graph.nodes[1].type==panorama::NodeType::PassiveBody,
          "imported passive mechanical graph node has correct enum mapping");
  require(render(synth,4096)>1.e-12,"imported inverse graph is audible");
  juce::MemoryBlock importState;synth.getStateInformation(importState);
  PanoramaProcessor imported;
  imported.prepareToPlay(48000,512);
  imported.setStateInformation(importState.getData(),int(importState.getSize()));
  require(imported.getPatchFromParameters().graph.nodes[1].type==panorama::NodeType::PassiveBody,
          "imported inverse graph survives host state roundtrip");
  const auto before=synth.getPatchFromParameters();
  require(!synth.importGraphRecipeJson("{ invalid json"),"reject invalid JSON");
  require(panorama::samePatchConfiguration(before,synth.getPatchFromParameters()),
          "invalid inverse graph import must leave patch unchanged");
 }

 for(const int rate:{44100,48000,96000}){
  synth.prepareToPlay(rate,512);
  for(const int index:{0,16,24,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59}){
   synth.loadPreset(index);
   require(synth.getCurrentProgram()==index,"factory program selection");
   const auto patch=synth.getPatchFromParameters();
   require(patch.engine==panorama::factoryPresets()[index].engine,"synthesis engine mapping");
   const double energy=render(synth,512);
   require(energy>1.e-12,"non-silent rendered factory preset");
   synth.releaseResources();
   synth.prepareToPlay(rate,512);
  }
 }
 // Hotfix 2: reproduce the real Studio One operation. Select a legacy
 // factory preset, then choose the Modular Patchbay engine in the host.
 // There must be audio even before a GUI is opened, and the GUI must be able
 // to persist the seeded nodes in APVTS for editing and state restoration.
 synth.prepareToPlay(48000,512);
 synth.loadPreset(0);
 auto* engineChoice=synth.params.getParameter("engine");
 require(engineChoice!=nullptr,"engine parameter exists");
 if(engineChoice)engineChoice->setValueNotifyingHost(engineChoice->convertTo0to1(4.f));
 const auto legacySwitch=synth.getPatchFromParameters();
 require(legacySwitch.engine==panorama::Engine::Modular,"manual engine select works");
 require(legacySwitch.graph.nodes[0].type==panorama::NodeType::Harmonic,
         "unconfigured legacy preset uses starter harmonic oscillator");
 require(render(synth,8192)>1.e-12,"legacy-to-Modular engine switch is audible headless");
 synth.seedEmptyModularGraphOnUiThread();
 const auto savedSwitch=synth.getPatchFromParameters();
 require(savedSwitch.graph.nodes[8].a==7,"patchbay output is connected");
 require((int)std::round(synth.params.getRawParameterValue("gType0")->load())==10,
         "GUI graph parameters seeded with harmonic node");
 require(render(synth,8192)>1.e-12,"persisted starter graph produces audio");
 juce::MemoryBlock modularState;
 synth.getStateInformation(modularState);
 PanoramaProcessor loadedModular;
 loadedModular.prepareToPlay(48000,512);
 loadedModular.setStateInformation(modularState.getData(),(int)modularState.getSize());
 require(loadedModular.getPatchFromParameters().graph.nodes[8].a==7,
         "modular starter survives saved host project");
 require(render(loadedModular,8192)>1.e-12,"restored modular starter sounds");
 // v5.2: a structural graph install while a note is sounding must keep
 // the old source alive across the fade. This is a processor-level host smoke
 // test, not a Studio One integration claim.
 {
  PanoramaProcessor live;
  live.prepareToPlay(48000,128);
  std::string error;
  require(live.installUniversalGraph(panorama::universal23::serialize(panorama::universal23::directGraph()),&error),"v5.2 start native source");
  live.setUniversalEnabled(true);
  juce::AudioBuffer<float> audio(2,128); juce::MidiBuffer events;
  events.addEvent(juce::MidiMessage::noteOn(1,57,(juce::uint8)100),0);
  audio.clear();live.processBlock(audio,events);
  require(live.installUniversalGraph(panorama::universal23::serialize(panorama::universal23::genuineNativeGraph()),&error),"v5.2 structural replacement");
  double continuityEnergy=0;
  for(int block=0;block<12;block++){
   juce::MidiBuffer empty;audio.clear();live.processBlock(audio,empty);
   for(int ch=0;ch<2;ch++)for(int j=0;j<128;j++){
    const auto x=audio.getSample(ch,j);require(std::isfinite(x),"v5.2 finite patch-transition samples");
    continuityEnergy+=double(x)*x;
   }
  }
  require(continuityEnergy>1.e-9,"v5.2 held note audible after graph replacement");
  juce::MemoryBlock saved;live.getStateInformation(saved);
  PanoramaProcessor restore;restore.prepareToPlay(48000,128);
  restore.setStateInformation(saved.getData(),(int)saved.getSize());
  require(restore.isUniversalEnabled(),"v5.2 patch persists after DAW restore");
 }
 // v5.3 host-level contract (runs on Windows CI once dispatched): exercise
 // actual APVTS automation, state persistence, processor lifecycle, and MIDI
 // while the universal Native27 graph is installed. This is NOT Studio One.
 {
  PanoramaProcessor hosted;
  hosted.prepareToPlay(48000,128);
  const auto native=panorama::universal23::genuineNativeGraph();
  std::string err;
  require(hosted.installUniversalGraph(panorama::universal23::serialize(native),&err),
          "v5.3 host native instrument installs");
  hosted.setUniversalEnabled(true);
  auto automate=[&](const char* id,float value){
   auto* p=hosted.params.getParameter(id);
   require(p!=nullptr,"v5.3 expected host automation ID exists");
   if(p)p->setValueNotifyingHost(p->convertTo0to1(value));
  };
  automate("g48Enable",1.f);
  automate("g48Param0",.37f);
  automate("g49Enable",1.f);
  automate("g49Macro0",.82f);
  automate("g51Enable",1.f);
  automate("g51Pickup0",.73f);
  automate("g51Excite0",.26f);
  double sum=0.;
  for(int block=0;block<12;block++){
   juce::AudioBuffer<float> buffer(2,128); buffer.clear();
   juce::MidiBuffer events;
   if(block==0)events.addEvent(juce::MidiMessage::noteOn(1,57,(juce::uint8)110),0);
   if(block==8)events.addEvent(juce::MidiMessage::noteOff(1,57),48);
   hosted.processBlock(buffer,events);
   for(int i=0;i<128;i++)for(int c=0;c<2;c++){
    const float x=buffer.getSample(c,i);
    require(std::isfinite(x),"v5.3 automated audio finite");sum+=x*x;
   }
  }
  require(sum>1.e-12,"v5.3 automation-enabled MIDI remains audible");
  juce::MemoryBlock saved;hosted.getStateInformation(saved);
  require(saved.getSize()>100,"v5.3 host generated serialized state");
  PanoramaProcessor resumed;resumed.prepareToPlay(48000,128);
  resumed.setStateInformation(saved.getData(),int(saved.getSize()));
  for(const auto* id:{"g48Enable","g48Param0","g49Macro0","g51Pickup0","g51Excite0"}){
   const auto* a=hosted.params.getRawParameterValue(id);
   const auto* b=resumed.params.getRawParameterValue(id);
   require(a&&b&&std::abs(a->load()-b->load())<1.e-4f,
           "v5.3 all host automation values restored");
  }
  require(resumed.universalGraphText()==hosted.universalGraphText(),
          "v5.3 full native graph state restored");
  require(render(resumed,2048)>1.e-12,"v5.3 restored automation instrument audible");
 }
 // v0.8: Reforged is deliberately redesigned with the Spectral Matter source.
 synth.loadPreset(24);
 const auto reforged=synth.getPatchFromParameters();
 require(reforged.engine==panorama::Engine::Modular,"Midnight Geometry Reforged is modular");
 require(reforged.graph.nodes[0].type==panorama::NodeType::SpectralMatter,
         "Reforged uses spectral matter");
 // Persist an edited host parameter, selected program, and modular structure.
 if(auto*gain=synth.params.getParameter("gain"))
  gain->setValueNotifyingHost(gain->convertTo0to1(0.31f));
 else require(false,"gain parameter is registered");
 juce::MemoryBlock state;
 synth.getStateInformation(state);
 require(state.getSize()>0,"serialization generates a state blob");
 PanoramaProcessor restored;
 restored.prepareToPlay(48000,512);
 restored.setStateInformation(state.getData(),static_cast<int>(state.getSize()));
 const auto restoredPatch=restored.getPatchFromParameters();
 require(restored.getCurrentProgram()==24,"selected preset restored");
 require(std::abs(restoredPatch.gain-0.31f)<0.002f,"edited gain restored");
 require(restoredPatch.graph.nodes[0].type==panorama::NodeType::SpectralMatter,
         "modular routing restored");
 require(restoredPatch.modRoutes[0].source==reforged.modRoutes[0].source,
         "modulation lane source restored");
 require(render(restored,512)>1.e-12,"restored state produces audio");
 // v0.5: the appended JUCE node choice must retain the enum mapping and
 // survive both factory selection and host state serialization.
 synth.loadPreset(32);
 auto coupled=synth.getPatchFromParameters();
 require(coupled.graph.nodes[2].type==panorama::NodeType::CoupledModes,
         "coupled modes maps correctly to the JUCE node selection");
 require(render(synth,2048)>1.e-12,"coupled preset produces VST processor audio");
 juce::MemoryBlock coupledState;
 synth.getStateInformation(coupledState);
 PanoramaProcessor loadedCoupled;
 loadedCoupled.prepareToPlay(48000,512);
 loadedCoupled.setStateInformation(coupledState.getData(),(int)coupledState.getSize());
 require(loadedCoupled.getCurrentProgram()==32,"coupled factory program restored");
 require(loadedCoupled.getPatchFromParameters().graph.nodes[2].type==panorama::NodeType::CoupledModes,
         "coupled node survives saved host session");
 require(render(loadedCoupled,2048)>1.e-12,"restored coupled graph produces audio");
 // v0.6: new node must survive JUCE parameter IDs and saved state.
 synth.loadPreset(36);
 require(synth.getNumPrograms()>=40,"v0.6 factory program count");
 auto exchange=synth.getPatchFromParameters();
 require(exchange.graph.nodes[2].type==panorama::NodeType::ResonantExchange,
         "v0.6 reciprocal exchange mapped in JUCE choice");
 require(render(synth,2048)>1.e-12,"exchange patch produces audio in processor");
 juce::MemoryBlock exchangeState;
 synth.getStateInformation(exchangeState);
 PanoramaProcessor loadedExchange;
 loadedExchange.prepareToPlay(48000,512);
 loadedExchange.setStateInformation(exchangeState.getData(),(int)exchangeState.getSize());
 require(loadedExchange.getCurrentProgram()==36,"exchange factory program restored");
 require(loadedExchange.getPatchFromParameters().graph.nodes[2].type==panorama::NodeType::ResonantExchange,
         "exchange node survives saved session");
 require(render(loadedExchange,2048)>1.e-12,"restored exchange patch produces audio");
 // v0.7: phase network is serializable and correctly mapped to the
 // JUCE node choice without changing the old gType0..7 parameter IDs.
 require(synth.getNumPrograms()==72,"v1.7 factory program count");
 synth.loadPreset(40);
 const auto phasePatch=synth.getPatchFromParameters();
 require(phasePatch.graph.nodes[2].type==panorama::NodeType::PhaseEcology,
         "Phase Ecology resolves from the JUCE type choice");
 require(render(synth,2048)>1.e-12,"Phase Ecology sounds in JUCE processor");
 juce::MemoryBlock phaseState;
 synth.getStateInformation(phaseState);
 PanoramaProcessor restoredPhase;
 restoredPhase.prepareToPlay(48000,512);
 restoredPhase.setStateInformation(phaseState.getData(),(int)phaseState.getSize());
 require(restoredPhase.getCurrentProgram()==40,"v0.7 factory program restored");
 require(restoredPhase.getPatchFromParameters().graph.nodes[2].type==panorama::NodeType::PhaseEcology,
         "Phase Ecology survives saved session");
 require(render(restoredPhase,2048)>1.e-12,"restored Phase Ecology produces audio");
 // v0.8: the appended Spectral Matter choice must be host-visible and stable
 // after saving and restoring a session. Parameter IDs stay unchanged.
 synth.loadPreset(44);
 const auto spectralPatch=synth.getPatchFromParameters();
 require(spectralPatch.graph.nodes[0].type==panorama::NodeType::SpectralMatter,
         "Spectral Matter node choice mapping");
 require(render(synth,2048)>1.e-12,"Spectral Matter renders from JUCE processor");
 juce::MemoryBlock spectralState;
 synth.getStateInformation(spectralState);
 PanoramaProcessor restoredSpectral;
 restoredSpectral.prepareToPlay(48000,512);
 restoredSpectral.setStateInformation(spectralState.getData(),(int)spectralState.getSize());
 require(restoredSpectral.getCurrentProgram()==44,"Spectral Matter program restored");
 require(restoredSpectral.getPatchFromParameters().graph.nodes[0].type==panorama::NodeType::SpectralMatter,
         "Spectral Matter node restores from saved host state");
 require(render(restoredSpectral,2048)>1.e-12,"Restored Spectral Matter produces audio");
 // v0.9: typed spectral bus exists in the DSP and is selectable/storable
 // through the unchanged APVTS gType parameter identifiers.
 synth.loadPreset(48);
 const auto weaved=synth.getPatchFromParameters();
 require(weaved.graph.nodes[0].type==panorama::NodeType::SpectralMatter,
         "v0.9 spectral input A mapping, not clamped to Phase Ecology");
 require(weaved.graph.nodes[2].type==panorama::NodeType::MatterWeave,
         "v0.9 MatterWeave host choice resolves as NodeType 17");
 require(render(synth,2048)>1.e-12,"v0.9 typed spectral synthesis produces audio");
 juce::MemoryBlock weaveState;
 synth.getStateInformation(weaveState);
 PanoramaProcessor restoredWeave;
 restoredWeave.prepareToPlay(48000,512);
 restoredWeave.setStateInformation(weaveState.getData(),(int)weaveState.getSize());
 require(restoredWeave.getCurrentProgram()==48,"v0.9 program number restores");
 require(restoredWeave.getPatchFromParameters().graph.nodes[2].type==panorama::NodeType::MatterWeave,
         "v0.9 MatterWeave preserves saved host state");
 require(render(restoredWeave,2048)>1.e-12,"v0.9 restored MatterWeave audible");
 // v1.0: the appended Formant Field must remain editable and survive
 // host state serialization. This runs only on Windows JUCE CI.
 synth.loadPreset(52);
 const auto formant=synth.getPatchFromParameters();
 require(formant.graph.nodes[3].type==panorama::NodeType::FormantField,
         "FormantField maps from the new JUCE choice without index clamp");
 require(render(synth,2048)>1.e-12,"v1.0 formant synthesis emits sound");
 juce::MemoryBlock formantState;
 synth.getStateInformation(formantState);
 PanoramaProcessor restoredFormant;
 restoredFormant.prepareToPlay(48000,512);
 restoredFormant.setStateInformation(formantState.getData(),(int)formantState.getSize());
 require(restoredFormant.getCurrentProgram()==52,"formant preset restores");
 require(restoredFormant.getPatchFromParameters().graph.nodes[3].type==panorama::NodeType::FormantField,
         "FormantField survives saved host state");
 require(render(restoredFormant,2048)>1.e-12,"restored formant patch audible");
 // v1.4: a new GUI choice must round-trip as type 19, across saved state.
 synth.loadPreset(56);
 const auto bridge=synth.getPatchFromParameters();
 require(bridge.graph.nodes[2].type==panorama::NodeType::PhaseMatterBridge,
         "PhaseMatterBridge maps from JUCE choice");
 require(render(synth,2048)>1.e-12,"bridge produces sound");
 juce::MemoryBlock phaseBridgeState;synth.getStateInformation(phaseBridgeState);
 PanoramaProcessor restoredBridge;restoredBridge.prepareToPlay(48000,512);
 restoredBridge.setStateInformation(phaseBridgeState.getData(),(int)phaseBridgeState.getSize());
 require(restoredBridge.getCurrentProgram()==56,"bridge preset restores");
 require(restoredBridge.getPatchFromParameters().graph.nodes[2].type==panorama::NodeType::PhaseMatterBridge,
         "bridge type survives saved host state");
 require(render(restoredBridge,2048)>1.e-12,"restored bridge audible");
 // v1.5: all new typed bus nodes must round-trip via APVTS.
 synth.loadPreset(60);
 const auto unified=synth.getPatchFromParameters();
 require(unified.graph.nodes[2].type==panorama::NodeType::PhysicalBody,
         "Physical Body GUI choice correctly maps to type 20");
 require(unified.graph.nodes[3].type==panorama::NodeType::UnifiedCoupler,
         "Unified Coupler GUI choice correctly maps to type 21");
 require(render(synth,2048)>1.e-12,"unified state patch sounds in JUCE");
 juce::MemoryBlock unifiedState;synth.getStateInformation(unifiedState);
 PanoramaProcessor restoredUnified;restoredUnified.prepareToPlay(48000,512);
 restoredUnified.setStateInformation(unifiedState.getData(),(int)unifiedState.getSize());
 require(restoredUnified.getCurrentProgram()==60,"unified factory preset restored");
 require(restoredUnified.getPatchFromParameters().graph.nodes[2].type==panorama::NodeType::PhysicalBody,
         "Physical Body type survives saved host state");
 require(restoredUnified.getPatchFromParameters().graph.nodes[3].type==panorama::NodeType::UnifiedCoupler,
         "Unified Coupler type survives saved host state");
 require(render(restoredUnified,2048)>1.e-12,"restored unified patch sounds");
 // v1.6: physical mesh node, explicit feedback and presets roundtrip.
 synth.loadPreset(65);
 const auto meshPatch=synth.getPatchFromParameters();
 require(meshPatch.graph.nodes[3].type==panorama::NodeType::MeshBody,
         "MeshBody GUI choice maps to type 22");
 require(meshPatch.graph.nodes[4].type==panorama::NodeType::UnifiedCoupler,
         "v1.6 accepts typed physical mesh in unified coupler");
 require(render(synth,2048)>1.e-12,"material-network patch sounds in JUCE");
 juce::MemoryBlock meshState;synth.getStateInformation(meshState);
 PanoramaProcessor restoredMesh;restoredMesh.prepareToPlay(48000,512);
 restoredMesh.setStateInformation(meshState.getData(),(int)meshState.getSize());
 require(restoredMesh.getCurrentProgram()==65,"material preset restored");
 require(restoredMesh.getPatchFromParameters().graph.nodes[3].type==panorama::NodeType::MeshBody,
         "MeshBody survives APVTS saved state");
 synth.loadPreset(66);
 require(synth.getPatchFromParameters().graph.nodes[1].type==panorama::NodeType::FeedbackDelay,
         "FeedbackDelay GUI choice maps to type 23");
 require(render(synth,2048)>1.e-12,"delayed-cycle patch sounds in JUCE");
 juce::MemoryBlock feedbackState;synth.getStateInformation(feedbackState);
 PanoramaProcessor restoredFeedback;restoredFeedback.prepareToPlay(48000,512);
 restoredFeedback.setStateInformation(feedbackState.getData(),(int)feedbackState.getSize());
 require(restoredFeedback.getCurrentProgram()==66,"feedback program restored");
 require(restoredFeedback.getPatchFromParameters().graph.nodes[1].type==panorama::NodeType::FeedbackDelay,
         "FeedbackDelay survives saved state");
 // v2.3: the unified graph must sound from the VST3 processBlock path,
 // reject invalid graph updates without losing state, and survive DAW reload.
 {
  PanoramaProcessor unified23;
  unified23.prepareToPlay(48000,512);
  require(unified23.isUniversalEnabled(),"v2.3 unified mode defaults on");
  require(render(unified23,2048)>1.e-12,"unified 72-sound/state-core graph renders MIDI");
  auto before=unified23.universalGraphText();
  std::string error;
  require(!unified23.installUniversalGraph("not a graph",&error),"invalid unified graph rejected");
  require(unified23.universalGraphText()==before,"failed graph install preserves live graph");
  auto alternative=panorama::universal23::defaultGraph();
  alternative.name="Host State Saved Graph";
  alternative.nodes[0].preset=0;
  require(unified23.installUniversalGraph(panorama::universal23::serialize(alternative),&error),
          "new graph installed with a changed sound source");
  juce::MemoryBlock saved;unified23.getStateInformation(saved);
  PanoramaProcessor reloaded;
  reloaded.prepareToPlay(48000,512);
  reloaded.setStateInformation(saved.getData(),int(saved.getSize()));
  require(reloaded.isUniversalEnabled(),"unified mode survives DAW host restore");
  require(reloaded.universalGraph().name==alternative.name,"graph name survives host state");
  require(reloaded.universalGraph().nodes[0].preset==0,"graph routing/source survives host state");
  require(render(reloaded,2048)>1.e-12,"restored unified MIDI is audible");
 }
 // v2.7: editable single-solver graph, host state persistence, and
 // actual processor MIDI path. Runtime/GUI checks execute on Windows CI.
 {
  PanoramaProcessor direct;
  direct.prepareToPlay(48000,512);
  auto before=direct.universalGraph();
  require(before.nodes.size()>=2 && before.nodes[0].kind==panorama::universal23::Kind::DirectState27,
          "new sessions start with direct-state playable instrument");
  auto changed=before;changed.instrument.modes[0].ratio=1.78;
  changed.instrument.modes[1].excitation=380.;
  std::string error;
  require(direct.installUniversalGraph(panorama::universal23::serialize(changed),&error),
          "edited mode graph compiles in JUCE processor");
  require(render(direct,4096)>1.e-9,"edited direct-state instrument makes sound");
  juce::MemoryBlock state27;direct.getStateInformation(state27);
  PanoramaProcessor again;again.prepareToPlay(48000,512);
  again.setStateInformation(state27.getData(),int(state27.getSize()));
  require(again.isUniversalEnabled(),"v27 enabled state restored");
  require(again.universalGraphText()==direct.universalGraphText(),"v27 topology+mode settings survive host state");
  require(render(again,4096)>1.e-9,"restored direct-state processor renders MIDI");
  std::unique_ptr<juce::AudioProcessorEditor> editor(again.createEditor());
  require(editor!=nullptr,"v27 redesigned editor construction");
  if(editor)editor->setSize(1100,800);
 }
 // Opening, resizing and destroying the actual editor (with attached sliders
 // and comboboxes) catches many construction/lifetime issues before pluginval.
 {
  std::unique_ptr<juce::AudioProcessorEditor> editor(restored.createEditor());
  require(editor!=nullptr,"editor creation");
  if(editor){
   editor->setSize(1130,900);
   editor->setSize(800,620);
   require(editor->getWidth()==800,"editor resizing");
   require(render(restored,512)>1.e-12,"audio while editor exists");
  }
 }
 std::cout<<"PANORAMA JUCE integration: "<<(failures==0?"PASS":"FAIL")
          <<" (failures="<<failures<<")\n";
 return failures==0?0:1;
}
