#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
namespace {
using P = juce::AudioParameterFloat;
// Literal IDs avoid transient std::string allocations in processBlock().
constexpr const char* graphTypeIds[] = {"gType0", "gType1", "gType2", "gType3", "gType4", "gType5", "gType6", "gType7"};
constexpr const char* graphAIds[] = {"gA0", "gA1", "gA2", "gA3", "gA4", "gA5", "gA6", "gA7", "gA8"};
constexpr const char* graphBIds[] = {"gB0", "gB1", "gB2", "gB3", "gB4", "gB5", "gB6", "gB7", "gB8"};
constexpr const char* graphLevelIds[] = {"gLevel0", "gLevel1", "gLevel2", "gLevel3", "gLevel4", "gLevel5", "gLevel6", "gLevel7", "gLevel8"};
constexpr const char* graphColorIds[] = {"gColor0", "gColor1", "gColor2", "gColor3", "gColor4", "gColor5", "gColor6", "gColor7", "gColor8"};
constexpr const char* modSourceIds[] = {"modSource0", "modSource1"};
constexpr const char* modTargetIds[] = {"modTarget0", "modTarget1"};
constexpr const char* modDestIds[] = {"modDest0", "modDest1"};
constexpr const char* modDepthIds[] = {"modDepth0", "modDepth1"};
void addFloatLayoutParameter(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& v,const juce::String&id,const juce::String&name,float a,float b,float def){
 v.push_back(std::make_unique<P>(juce::ParameterID{id,1},name,juce::NormalisableRange<float>(a,b),def));
}
}
juce::AudioProcessorValueTreeState::ParameterLayout PanoramaProcessor::createLayout(){
 std::vector<std::unique_ptr<juce::RangedAudioParameter>> v;
 v.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"engine",1},"Synthesis Engine",juce::StringArray{"Crystal VA","Fractal Digital","Physical Resonance","Hybrid Cross-Synthesis","Modular Patchbay"},0));
 addFloatLayoutParameter(v,"cutoff","Filter Cutoff",0.01f,1.f,.84f);
 addFloatLayoutParameter(v,"resonance","Filter Resonance",0.f,1.f,.15f);
 addFloatLayoutParameter(v,"attack","Attack (s)",.001f,3.f,.015f);
 addFloatLayoutParameter(v,"decay","Decay (s)",.002f,3.f,.23f);
 addFloatLayoutParameter(v,"sustain","Sustain",0.f,1.f,.76f);
 addFloatLayoutParameter(v,"release","Release (s)",.01f,4.f,.48f);
 addFloatLayoutParameter(v,"detune","Unison Detune",0.f,1.f,.29f);
 addFloatLayoutParameter(v,"unison","Unison",0.f,1.f,.93f);
 addFloatLayoutParameter(v,"fm","FM Index",0.f,1.f,.08f);
 addFloatLayoutParameter(v,"fold","Fractal Fold",0.f,1.f,.12f);
 addFloatLayoutParameter(v,"body","Resonator Body",0.f,1.f,.2f);
 addFloatLayoutParameter(v,"damping","Damping",0.f,1.f,.42f);
 addFloatLayoutParameter(v,"motion","Motion",0.f,1.f,.28f);
 addFloatLayoutParameter(v,"blend","Cross-Synthesis Mix",0.f,1.f,.50f);
 addFloatLayoutParameter(v,"brightness","Exciter Brightness",0.f,1.f,.86f);
 addFloatLayoutParameter(v,"gain","Master",0.f,1.f,.51f);
 v.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"routing",1},"Cross Routing",juce::StringArray{"Classic","VA to Resonator","FM to Resonator","Ring to Resonator to PM","Resonator to VA AM"},0));
 addFloatLayoutParameter(v,"crossDepth","Routing Depth",0.f,1.f,0.f);
 addFloatLayoutParameter(v,"stereoResonance","Modal Stereo Width",0.f,1.f,0.f);
 // IDs appended; existing automation IDs and plugin identity remain unchanged.
 addFloatLayoutParameter(v,"metalMix","Bell Metal Body",0.f,1.f,0.f);
 addFloatLayoutParameter(v,"metalColor","Bell Partial Color",0.f,1.f,.5f);
 addFloatLayoutParameter(v,"metalDecay","Bell Modal Tail",0.f,1.f,.5f);
 addFloatLayoutParameter(v,"bellDynamics","Bell Spectral Evolution",0.f,1.f,0.f);
 addFloatLayoutParameter(v,"bellCoupling","Bell Mode Coupling",0.f,1.f,0.f);
 addFloatLayoutParameter(v,"bellClarity","Bell Tail Clarity",0.f,1.f,0.f);
 // v0.3: genuine audio-rate graph: nine nodes, each with editable type, inputs, and two controls.
 const juce::StringArray nodeTypes{"Off","VA","Digital FM","Noise","Ring","Mix","Wavefolder","Modal Resonator","Low-pass","Gain","Harmonic Additive","Phase Warp","Coupled Modes","Resonant Exchange","Phase Ecology","Spectral Matter","Matter Weave","Formant Field","Phase-Matter Bridge","Physical Body","Unified Coupler","Mesh Body","Feedback Delay","Passive Body"};
 juce::StringArray inputChoices{"None"};
 for(int i=0;i<panorama::kGraphNodes;i++)inputChoices.add("Node " + juce::String(i+1));
 for(int i=0;i<panorama::kGraphNodes;i++){
  const auto sid=juce::String(i),label="Node " + juce::String(i+1);
  if(i<panorama::kGraphNodes-1)
   v.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"gType"+sid,1},label+" Type",nodeTypes,0));
  v.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"gA"+sid,1},label+" Input A",inputChoices,0));
  v.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"gB"+sid,1},label+" Input B",inputChoices,0));
  addFloatLayoutParameter(v,"gLevel"+sid,label+" Level",0.f,1.f,.8f);
  addFloatLayoutParameter(v,"gColor"+sid,label+" Color",0.f,1.f,.5f);
 }
 // v0.4: two fully automatable, patch-serializable modulation lanes.
 juce::StringArray modSources{"Off","LFO A","LFO B","Envelope","Velocity","Mod Wheel"};
 for(int i=0;i<panorama::kGraphNodes;i++)modSources.add("Audio Node " + juce::String(i+1));
 juce::StringArray modTargets;
 for(int i=0;i<panorama::kGraphNodes;i++)modTargets.add("Node " + juce::String(i+1));
 juce::StringArray modDests{"Color","Level","Pitch","Phase"};
 for(int i=0;i<2;i++){
  const auto id=juce::String(i);
  v.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"modSource"+id,1},"Mod " + id + " Source",modSources,0));
  v.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"modTarget"+id,1},"Mod " + id + " Target",modTargets,0));
  v.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"modDest"+id,1},"Mod " + id + " Destination",modDests,0));
  addFloatLayoutParameter(v,"modDepth"+id,"Mod " + id + " Depth",-1.f,1.f,0.f);
 }
 addFloatLayoutParameter(v,"g48Enable","Native47 Host Automation Enable",0.f,1.f,0.f);
 const char*names[]={"Cutoff","Resonance","Drive","Output","Filter Mix","Macro 1","Macro 2","Macro 3","Macro 4"};
 const float defaults[9]={.847f,.21875f,.606f,.225f,.8f,.5f,.5f,.5f,.5f};
 for(int i=0;i<9;i++)addFloatLayoutParameter(v,"g48Param"+juce::String(i),"Native47 "+juce::String(names[i]),0.f,1.f,defaults[i]);
 addFloatLayoutParameter(v,"g49Enable","Common Graph Automation Enable",0.f,1.f,0.f);
 for(int i=0;i<4;i++)addFloatLayoutParameter(v,"g49Macro"+juce::String(i),"Common Macro "+juce::String(i+1),0.f,1.f,.5f);
 for(int i=0;i<8;i++)addFloatLayoutParameter(v,"g49Node"+juce::String(i),"Graph Node "+juce::String(i+1)+" Level",0.f,1.f,.8f);
 addFloatLayoutParameter(v,"g51Enable","Native27 Live Internal Controls",0.f,1.f,0.f);
 for(int i=0;i<8;i++){
  addFloatLayoutParameter(v,"g51Pickup"+juce::String(i),"Native Mode "+juce::String(i+1)+" Readout Scale",0.f,1.f,.5f);
  addFloatLayoutParameter(v,"g51Excite"+juce::String(i),"Native Mode "+juce::String(i+1)+" Excitation Scale",0.f,1.f,.5f);
 }
 return {v.begin(),v.end()};
}
PanoramaProcessor::PanoramaProcessor():AudioProcessor(BusesProperties().withOutput("Output",juce::AudioChannelSet::stereo(),true)),params(*this,nullptr,"PANORAMIZE",createLayout()) {
 loadPreset(0);
 std::string error;
 installUniversalGraph(universalRecipe,&error);
}
void PanoramaProcessor::prepareToPlay(double sr,int){
 currentSampleRate=sr;synth.prepare(sr);lastAppliedPatchValid=false;
 graphSwap.setSampleRate(sr);graphSwap.beginStopped(nullptr);
 std::string recipe;
 {const juce::ScopedLock lock(universalLock);recipe=universalRecipe;}
 std::string error;
 installUniversalGraph(recipe,&error);
}
void PanoramaProcessor::releaseResources(){
 synth.allNotesOff();
 graphSwap.allNotesOff();
}
bool PanoramaProcessor::isBusesLayoutSupported(const BusesLayout&b) const {return b.getMainOutputChannelSet()==juce::AudioChannelSet::stereo()||b.getMainOutputChannelSet()==juce::AudioChannelSet::mono();}
const juce::String PanoramaProcessor::getProgramName(int n){return n>=0&&n<panorama::kPresetCount?panorama::factoryPresets()[(size_t)n].name:juce::String();}
void PanoramaProcessor::loadPreset(int index){
 if(index<0||index>=panorama::kPresetCount)return;
 currentProgram.store(index);
 const auto&p=panorama::factoryPresets()[(size_t)index];
 auto write=[this](const char*id,float v){if(auto*pr=params.getParameter(id))pr->setValueNotifyingHost(pr->convertTo0to1(v));};
 write("engine",(float)(int)p.engine);
 write("cutoff",p.cutoff);write("resonance",p.resonance);write("attack",p.attack);write("decay",p.decay);
 write("sustain",p.sustain);write("release",p.release);write("detune",p.detune);write("unison",p.unison);
 write("fm",p.fm);write("fold",p.fold);write("body",p.resonanceBody);write("damping",p.damping);
 write("motion",p.motion);write("blend",p.blend);write("brightness",p.brightness);write("gain",p.gain);
 write("routing",(float)p.routing);write("crossDepth",p.crossDepth);write("stereoResonance",p.stereoResonance);
 write("metalMix",p.metalMix);write("metalColor",p.metalColor);write("metalDecay",p.metalDecay);
 write("bellDynamics",p.bellDynamics);write("bellCoupling",p.bellCoupling);write("bellClarity",p.bellClarity);
 for(int i=0;i<panorama::kGraphNodes;i++){
   const auto&n=p.graph.nodes[i];
   if(i<panorama::kGraphNodes-1)write(graphTypeIds[i],(float)((int)n.type>=11?(int)n.type-1:(int)n.type));
   write(graphAIds[i],(float)(n.a+1));write(graphBIds[i],(float)(n.b+1));
   write(graphLevelIds[i],n.level);write(graphColorIds[i],n.color);
 }
 for(int i=0;i<2;i++){
   const auto&m=p.modRoutes[(size_t)i];
   write(modSourceIds[i],(float)m.source);
   write(modTargetIds[i],(float)m.target);
   write(modDestIds[i],(float)m.destination);
   write(modDepthIds[i],m.depth);
 }
}
bool PanoramaProcessor::importGraphRecipeJson(const juce::String& recipe){
 // Parse and validate the complete recipe BEFORE notifying a single host
 // parameter. This function must be invoked from the editor/message thread.
 const auto doc=juce::JSON::parse(recipe);
 if(!doc.isObject()||doc.getProperty("format",{}).toString()!="PANORAMA-GraphInverse-v1")return false;
 const auto nodes=doc.getProperty("nodes",{});
 auto* arr=nodes.getArray();
 if(!arr||arr->size()!=panorama::kGraphNodes)return false;
 panorama::Patch proposed=getPatchFromParameters();
 proposed.engine=panorama::Engine::Modular;proposed.modRoutes={};
 proposed.graph={};
 for(int i=0;i<panorama::kGraphNodes;i++){
  const auto&v=arr->getReference(i);
  if(!v.isObject())return false;
  const int index=int(v.getProperty("index",juce::var(-1)));
  const int type=int(v.getProperty("type",juce::var(-1)));
  const int a=int(v.getProperty("a",juce::var(-2))),b=int(v.getProperty("b",juce::var(-2)));
  const double level=double(v.getProperty("level",juce::var(-1.)));
  const double color=double(v.getProperty("color",juce::var(-1.)));
  if(index!=i||type<0||type>24||a< -1||a>=panorama::kGraphNodes||
     b< -1||b>=panorama::kGraphNodes||!std::isfinite(level)||!std::isfinite(color)||
     level<0.||level>1.||color<0.||color>1.||
     (i==panorama::kGraphNodes-1 && type!=int(panorama::NodeType::Output))||
     (i<panorama::kGraphNodes-1 && type==int(panorama::NodeType::Output)))return false;
  proposed.graph.nodes[(size_t)i]={static_cast<panorama::NodeType>(type),a,b,float(level),float(color)};
 }
 const auto settings=doc.getProperty("parameters",{});
 if(!settings.isObject())return false;
 struct FloatEntry{const char*jsonKey;const char*parameterId;float panorama::Patch::*field;float lo,hi;};
 static constexpr FloatEntry entries[]={
  {"cutoff","cutoff",&panorama::Patch::cutoff,.01f,1.f},
  {"resonance","resonance",&panorama::Patch::resonance,0.f,1.f},
  {"attack","attack",&panorama::Patch::attack,.001f,3.f},
  {"decay","decay",&panorama::Patch::decay,.002f,3.f},
  {"sustain","sustain",&panorama::Patch::sustain,0.f,1.f},
  {"release","release",&panorama::Patch::release,.01f,4.f},
  {"detune","detune",&panorama::Patch::detune,0.f,1.f},
  {"unison","unison",&panorama::Patch::unison,0.f,1.f},
  {"fm","fm",&panorama::Patch::fm,0.f,1.f},
  {"fold","fold",&panorama::Patch::fold,0.f,1.f},
  {"resonanceBody","body",&panorama::Patch::resonanceBody,0.f,1.f},
  {"damping","damping",&panorama::Patch::damping,0.f,1.f},
  {"motion","motion",&panorama::Patch::motion,0.f,1.f},
  {"blend","blend",&panorama::Patch::blend,0.f,1.f},
  {"brightness","brightness",&panorama::Patch::brightness,0.f,1.f},
  {"gain","gain",&panorama::Patch::gain,0.f,1.f},
  {"crossDepth","crossDepth",&panorama::Patch::crossDepth,0.f,1.f},
  {"stereoResonance","stereoResonance",&panorama::Patch::stereoResonance,0.f,1.f}
 };
 for(const auto&e:entries){
  if(!settings.hasProperty(e.jsonKey))return false;
  const double value=double(settings.getProperty(e.jsonKey,{}));
  if(!std::isfinite(value)||value<e.lo||value>e.hi)return false;
  proposed.*(e.field)=float(value);
 }
 const int routing=int(settings.getProperty("routing",juce::var(-1)));
 if(routing<0||routing>4)return false;
 proposed.routing=routing;
 std::array<int,panorama::kGraphNodes> order{};
 if(!panorama::compileGraph(proposed.graph,order)||
    !panorama::validateModRoutes(proposed.graph,proposed.modRoutes))return false;
 const auto write=[this](const juce::String& id,float value){
  if(auto*pr=params.getParameter(id))pr->setValueNotifyingHost(pr->convertTo0to1(value));
 };
 // Clear any old audio-rate modulation links before installing the new graph.
 for(int j=0;j<2;j++){
  const juce::String suffix=juce::String(j);
  write("modDepth"+suffix,0.f);write("modSource"+suffix,0.f);
  write("modTarget"+suffix,0.f);write("modDest"+suffix,0.f);
 }
 for(const auto&e:entries)write(e.parameterId,proposed.*(e.field));
 write("routing",float(routing));
 for(int i=0;i<panorama::kGraphNodes;i++){
  const auto&n=proposed.graph.nodes[(size_t)i];
  const juce::String suffix=juce::String(i);
  if(i<panorama::kGraphNodes-1){
   const int actual=int(n.type);write("gType"+suffix,float(actual>=11?actual-1:actual));
  }
  write("gA"+suffix,float(n.a+1));write("gB"+suffix,float(n.b+1));
  write("gLevel"+suffix,n.level);write("gColor"+suffix,n.color);
 }
 write("engine",4.f);
 lastAppliedPatchValid=false;
 return true;
}
panorama::Patch PanoramaProcessor::getPatchFromParameters()const{
 const int index=juce::jlimit(0,panorama::kPresetCount-1,currentProgram.load());
 auto p=panorama::factoryPresets()[(size_t)index];
 auto get=[this](const char*id)->float{return params.getRawParameterValue(id)->load();};
 p.engine=static_cast<panorama::Engine>(juce::jlimit(0,4,(int)std::round(get("engine"))));
 p.cutoff=get("cutoff");p.resonance=get("resonance");p.attack=get("attack");p.decay=get("decay");
 p.sustain=get("sustain");p.release=get("release");p.detune=get("detune");p.unison=get("unison");
 p.fm=get("fm");p.fold=get("fold");p.resonanceBody=get("body");p.damping=get("damping");
 p.motion=get("motion");p.blend=get("blend");p.brightness=get("brightness");p.gain=get("gain");
 p.routing=juce::jlimit(0,4,(int)std::round(get("routing")));
 p.crossDepth=get("crossDepth");p.stereoResonance=get("stereoResonance");
 p.metalMix=get("metalMix");p.metalColor=get("metalColor");p.metalDecay=get("metalDecay");
 p.bellDynamics=get("bellDynamics");p.bellCoupling=get("bellCoupling");p.bellClarity=get("bellClarity");
 for(int i=0;i<panorama::kGraphNodes;i++){
   auto&n=p.graph.nodes[i];
   // Node 9 is a fixed Output and intentionally has no gType8 parameter.
   if(i<panorama::kGraphNodes-1){
     const int uiType=juce::jlimit(0,23,(int)std::round(get(graphTypeIds[i])));
     n.type=static_cast<panorama::NodeType>(uiType>=10?uiType+1:uiType);
   }else n.type=panorama::NodeType::Output;
   n.a=juce::jlimit(-1,panorama::kGraphNodes-1,(int)std::round(get(graphAIds[i]))-1);
   n.b=juce::jlimit(-1,panorama::kGraphNodes-1,(int)std::round(get(graphBIds[i]))-1);
   n.level=get(graphLevelIds[i]);n.color=get(graphColorIds[i]);
 }
 for(int i=0;i<2;i++){
   auto&m=p.modRoutes[(size_t)i];
   m.source=juce::jlimit(0,5+panorama::kGraphNodes,(int)std::round(get(modSourceIds[i])));
   m.target=juce::jlimit(0,panorama::kGraphNodes-1,(int)std::round(get(modTargetIds[i])));
   m.destination=juce::jlimit(0,3,(int)std::round(get(modDestIds[i])));
   m.depth=get(modDepthIds[i]);
 }
 // Also cover legacy saved host states without a GUI open. This only changes
 // the temporary DSP patch, not AudioProcessorValueTreeState from audio thread.
 if(p.engine==panorama::Engine::Modular && panorama::isUnconfiguredModularGraph(p.graph))
  p.graph=panorama::makeStarterModularGraph();
 return p;
}
void PanoramaProcessor::seedEmptyModularGraphOnUiThread(){
 const auto get=[this](const char*id)->float{return params.getRawParameterValue(id)->load();};
 if((int)std::round(get("engine"))!=(int)panorama::Engine::Modular)return;
 // Test raw parameters before fallback: getPatchFromParameters() already seeds
 // the temporary DSP patch for headless automation and restored old projects.
 for(int i=0;i<panorama::kGraphNodes;i++){
  if(i<panorama::kGraphNodes-1 && (int)std::round(get(graphTypeIds[i]))!=0)return;
  if((int)std::round(get(graphAIds[i]))!=0 || (int)std::round(get(graphBIds[i]))!=0)return;
 }
 const auto graph=panorama::makeStarterModularGraph();
 auto write=[this](const char*id,float v){if(auto*pr=params.getParameter(id))pr->setValueNotifyingHost(pr->convertTo0to1(v));};
 for(int i=0;i<panorama::kGraphNodes;i++){
  const auto &n=graph.nodes[i];
  if(i<panorama::kGraphNodes-1)write(graphTypeIds[i],(float)((int)n.type>=11?(int)n.type-1:(int)n.type));
  write(graphAIds[i],(float)(n.a+1));write(graphBIds[i],(float)(n.b+1));
  write(graphLevelIds[i],n.level);write(graphColorIds[i],n.color);
 }
}
bool PanoramaProcessor::installUniversalGraph(const std::string& recipe,std::string* error){
 panorama::universal23::Graph parsed;
 if(!panorama::universal23::deserialize(recipe,parsed,error))return false;
 // v5.2: always prepare a new structural graph off the audio callback; the
 // old physical DSP continues to render until a note-aware bounded fade ends.
 // In-place updates to an active graph from this UI thread were data-race prone.
 auto prepared=panorama::universal23::Instrument::prepare(parsed,currentSampleRate,error);
 if(!prepared)return false;
 {const juce::ScopedLock lock(universalLock);universalRecipe=panorama::universal23::serialize(parsed);}
 graphSwap.publish(prepared);
 std::atomic_store(&universalPrepared,std::move(prepared));
 return true;
}
std::string PanoramaProcessor::universalGraphText() const{
 const juce::ScopedLock lock(universalLock);return universalRecipe;
}
panorama::universal23::Graph PanoramaProcessor::universalGraph() const{
 panorama::universal23::Graph graph;
 std::string error;
 panorama::universal23::deserialize(universalGraphText(),graph,&error);
 return graph;
}
void PanoramaProcessor::handleMidi(const juce::MidiMessage& m,panorama::universal23::Instrument* graph){
 if(m.isNoteOn()){
  if(graph)graphSwap.noteOn(m.getNoteNumber(),m.getFloatVelocity(),m.getChannel());
  else synth.noteOn(m.getNoteNumber(),m.getFloatVelocity(),m.getChannel());
 }else if(m.isNoteOff()){
  if(graph)graphSwap.noteOff(m.getNoteNumber(),m.getChannel());
  else synth.noteOff(m.getNoteNumber(),m.getChannel());
 }else if(m.isAllNotesOff()||m.isAllSoundOff()){
  if(graph)graphSwap.allNotesOff();else synth.allNotesOff();
 }else if(m.isPitchWheel()){
  const float bend=2.f*(m.getPitchWheelValue()-8192)/8192.f;
  if(graph)graphSwap.pitchBend(bend);else synth.pitchBend(bend);
 }else if(m.isChannelPressure()){
  if(graph)graphSwap.channelPressure(m.getChannelPressureValue()/127.f);
 }else if(m.isAftertouch()){
  if(graph)graphSwap.polyPressure(m.getNoteNumber(),m.getAfterTouchValue()/127.f);
 }else if(m.isController()){
  const int controller=m.getControllerNumber();
  if(controller==64){if(graph)graphSwap.sustain(m.getControllerValue()>=64);else synth.sustain(m.getControllerValue()>=64);}
  if(controller>=20&&controller<=23&&graph)graphSwap.articulationControl(controller,m.getControllerValue()/127.f);
  if(controller==1){if(graph)graphSwap.modWheel(m.getControllerValue()/127.f);else synth.modWheel(m.getControllerValue()/127.f);}
 }
}
void PanoramaProcessor::processBlock(juce::AudioBuffer<float>& audio,juce::MidiBuffer& midi){
 juce::ScopedNoDenormals guard;
 const int count=audio.getNumSamples();
 if(audio.getNumChannels()<1)return;
 const bool universal=universalEnabled.load(std::memory_order_relaxed);
 auto graph=universal?std::atomic_load(&universalPrepared):nullptr;
 if(graph&&params.getRawParameterValue("g48Enable")->load(std::memory_order_relaxed)>.5f){
  static const char*keys[]={"g48Param0","g48Param1","g48Param2","g48Param3","g48Param4","g48Param5","g48Param6","g48Param7","g48Param8"};
  for(int i=0;i<9;i++)graph->setHostParameterNormalized(i,params.getRawParameterValue(keys[i])->load(std::memory_order_relaxed));
 }else if(graph)graph->clearNativeHostAutomation();
 if(graph&&params.getRawParameterValue("g49Enable")->load(std::memory_order_relaxed)>.5f){
  static const char* macKeys[]={"g49Macro0","g49Macro1","g49Macro2","g49Macro3"};
  static const char* nodeKeys[]={"g49Node0","g49Node1","g49Node2","g49Node3","g49Node4","g49Node5","g49Node6","g49Node7"};
  for(int i=0;i<4;i++)graph->setHostParameterNormalized(9+i,params.getRawParameterValue(macKeys[i])->load(std::memory_order_relaxed));
  for(int i=0;i<8;i++)graph->setHostParameterNormalized(16+i,params.getRawParameterValue(nodeKeys[i])->load(std::memory_order_relaxed));
 }else if(graph)graph->clearGlobalHostAutomation();
 if(graph&&params.getRawParameterValue("g51Enable")->load(std::memory_order_relaxed)>.5f){
  static const char*pickup[]={"g51Pickup0","g51Pickup1","g51Pickup2","g51Pickup3","g51Pickup4","g51Pickup5","g51Pickup6","g51Pickup7"};
  static const char*excite[]={"g51Excite0","g51Excite1","g51Excite2","g51Excite3","g51Excite4","g51Excite5","g51Excite6","g51Excite7"};
  for(int i=0;i<8;i++){
   graph->setHostParameterNormalized(80+i,params.getRawParameterValue(pickup[i])->load(std::memory_order_relaxed));
   graph->setHostParameterNormalized(88+i,params.getRawParameterValue(excite[i])->load(std::memory_order_relaxed));
  }
 }else if(graph){
  // Native51 controls only: preserve the existing 4.8 tone automation.
  graph->clearNativeModeAutomation();
 }
 if(!universal){
  const auto incoming=getPatchFromParameters();
  if(!lastAppliedPatchValid || !panorama::samePatchConfiguration(incoming,lastAppliedPatch)){
   synth.setPatch(incoming);lastAppliedPatch=incoming;lastAppliedPatchValid=true;
  }
 }
 float* l=audio.getWritePointer(0);float* r=audio.getNumChannels()>1?audio.getWritePointer(1):nullptr;
 juce::MidiBuffer::Iterator it(midi);
 juce::MidiMessage next;int nextAt=0;
 bool pending=it.getNextEvent(next,nextAt);
 for(int i=0;i<count;i++){
  while(pending&&nextAt<=i){handleMidi(next,graph.get());pending=it.getNextEvent(next,nextAt);}
  const auto s=graph?graphSwap.processOne():synth.processOne();
  l[i]=s.left;if(r)r[i]=s.right;
 }
 for(int c=2;c<audio.getNumChannels();c++)audio.clear(c,0,count);
}
void PanoramaProcessor::getStateInformation(juce::MemoryBlock& b){
 auto copy=params.copyState();copy.setProperty("factoryProgram",currentProgram.load(),nullptr);
 copy.setProperty("universalEnabledV27",universalEnabled.load(),nullptr);
 copy.setProperty("universalEnabledV25",universalEnabled.load(),nullptr);
 copy.setProperty("universalGraphV27",juce::String::fromUTF8(universalGraphText().c_str()),nullptr);
 copy.setProperty("universalGraphV25",juce::String::fromUTF8(universalGraphText().c_str()),nullptr);
 auto xml=copy.createXml();copyXmlToBinary(*xml,b);
}
void PanoramaProcessor::setStateInformation(const void* data,int size){
 auto xml=getXmlFromBinary(data,size);
 if(xml && xml->hasTagName(params.state.getType().toString())){
  const int program=juce::jlimit(0,panorama::kPresetCount-1,(int)xml->getIntAttribute("factoryProgram",0));
  const juce::String graph=xml->getStringAttribute("universalGraphV27",xml->getStringAttribute("universalGraphV25",xml->getStringAttribute("universalGraphV24",xml->getStringAttribute("universalGraphV23",{}))));
  const bool enabled=xml->getBoolAttribute("universalEnabledV27",xml->getBoolAttribute("universalEnabledV25",xml->getBoolAttribute("universalEnabledV24",xml->getBoolAttribute("universalEnabledV23",false))));
  params.replaceState(juce::ValueTree::fromXml(*xml));currentProgram.store(program);
  if(graph.isNotEmpty()){
   std::string err;
   if(installUniversalGraph(graph.toStdString(),&err))universalEnabled.store(enabled);
  }else universalEnabled.store(false); // old 0.4-2.2 host states load as legacy
 }
}
juce::AudioProcessorEditor* PanoramaProcessor::createEditor(){return new PanoramaEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new PanoramaProcessor();}
