#include "PluginEditor.h"
namespace {
class PanoramaLook:public juce::LookAndFeel_V4 {
public:
 PanoramaLook(){setColour(juce::Slider::thumbColourId,juce::Colour(0xff60c0eb));
  setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(0xff45b0db));
  setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff19243c));
  setColour(juce::ComboBox::textColourId,juce::Colours::white);
  setColour(juce::PopupMenu::backgroundColourId,juce::Colour(0xff1a2840));}
};
PanoramaLook gLook;
}
void PanoramaEditor::Content::paint(juce::Graphics&g){
 g.fillAll(juce::Colour(0xff0e1628));
 g.setColour(juce::Colour(0xff1b2c48));
 g.setColour(juce::Colour(0xffe3f5ff));
 g.setFont(juce::Font(juce::FontOptions(17.f,juce::Font::bold)));
 g.drawText("MODULAR PATCHBAY  |  9 AUDIO NODES / 2 MOD LANES",20,5,getWidth()-40,25,juce::Justification::centredLeft);
 g.setFont(12.f);g.setColour(juce::Colour(0xff9eb5c9));
 g.drawText("Connect sources A/B; choose node DSP and change LEVEL / COLOR. No cycles allowed.",20,29,getWidth()-40,18,juce::Justification::centredLeft);
 for(int row=0;row<panorama::kGraphNodes;row++){
   auto bounds=juce::Rectangle<float>(12.f,55.f+row*65.f,(float)getWidth()-24.f,61.f);
   g.setColour(juce::Colour(row==panorama::kGraphNodes-1?0xff274155:0xff1b2c48));
   g.fillRoundedRectangle(bounds,8.f);
   g.setColour(juce::Colour(0xff365171));g.drawRoundedRectangle(bounds,8.f,1.f);
 }
 g.setColour(juce::Colour(0xffe3f5ff));g.setFont(juce::Font(juce::FontOptions(17.f,juce::Font::bold)));
 g.drawText("MODULATION MATRIX  |  Source > Target > Property",20,640,getWidth()-40,26,juce::Justification::centredLeft);
 g.setColour(juce::Colour(0xff1b2c48));
 g.fillRoundedRectangle(juce::Rectangle<float>(12.f,672.f,(float)getWidth()-24.f,120.f),9.f);
 const int knobTop=860;
 g.setColour(juce::Colour(0xffe3f5ff));g.setFont(juce::Font(juce::FontOptions(17.f,juce::Font::bold)));
 g.drawText("GLOBAL SOUND CONTROLS",20,820,getWidth()-40,30,juce::Justification::centredLeft);
 g.setColour(juce::Colour(0xff1b2c48));
 for(int row=0;row<5;row++){
  auto bounds=juce::Rectangle<float>(12.f,(float)knobTop+row*180.f,(float)getWidth()-24.f,172.f);
  g.fillRoundedRectangle(bounds,12.f);
  g.setColour(juce::Colour(0xff365171));g.drawRoundedRectangle(bounds,12.f,1.f);
  g.setColour(juce::Colour(0xff1b2c48));
 }
}
PanoramaEditor::PanoramaEditor(PanoramaProcessor&p):AudioProcessorEditor(p),processor(p),unifiedPanel(p){
 setLookAndFeel(&gLook);
 addAndMakeVisible(graphToggle);
 addAndMakeVisible(unifiedPanel);
 graphToggle.onClick=[this](){
   processor.setUniversalEnabled(!processor.isUniversalEnabled());
   unifiedPanel.setVisible(processor.isUniversalEnabled());
   viewport.setVisible(!processor.isUniversalEnabled());
   graphToggle.setButtonText(processor.isUniversalEnabled()?"FACTORY SOUND MODE":"UNIFIED STATE MODE");
 };
 unifiedPanel.setVisible(processor.isUniversalEnabled());
 graphToggle.setButtonText(processor.isUniversalEnabled()?"FACTORY SOUND MODE":"UNIFIED STATE MODE");
 subtitle.setText("VA / DIGITAL / MODAL / COUPLED MODES / MODULAR PATCHBAY",juce::dontSendNotification);
 subtitle.setJustificationType(juce::Justification::centredLeft);
 subtitle.setColour(juce::Label::textColourId,juce::Colour(0xff91a7c4));addAndMakeVisible(subtitle);
 for(int i=0;i<panorama::kPresetCount;i++)preset.addItem(panorama::factoryPresets()[(size_t)i].name,i+1);
 preset.setSelectedId(processor.getCurrentProgram()+1,juce::dontSendNotification);
 preset.onChange=[this](){processor.loadPreset(preset.getSelectedId()-1);};addAndMakeVisible(preset);
 for(auto name:{"Crystal VA","Fractal Digital","Physical Resonance","Hybrid Cross-Synthesis","Modular Patchbay"})engine.addItem(name,engine.getNumItems()+1);
 addAndMakeVisible(engine);
 for(auto name:{"Classic / v0.1","VA to Resonator","FM to Resonator","Ring to Modes to PM","Modes to VA AM"})routing.addItem(name,routing.getNumItems()+1);
 addAndMakeVisible(routing);
 addAndMakeVisible(importGraphButton);
 importGraphButton.onClick=[this](){
  inverseChooser=std::make_unique<juce::FileChooser>("Open an inverse graph JSON recipe",juce::File{},"*.json");
  const juce::Component::SafePointer<PanoramaEditor> safe(this);
  inverseChooser->launchAsync(juce::FileBrowserComponent::openMode |
     juce::FileBrowserComponent::canSelectFiles,[safe](const juce::FileChooser&chooser){
   if(!safe)return;
   const auto file=chooser.getResult();
   if(file==juce::File{})return;
   if(!safe->processor.importGraphRecipeJson(file.loadFileAsString()))
    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
     "Panoramize graph import","The JSON recipe is unsupported or the graph is invalid.");
  });
 };
 routingAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.params,"routing",routing);
 engineAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.params,"engine",engine);
 addKnob("cutoff","FILTER CUTOFF");addKnob("resonance","RESONANCE");
 addKnob("attack","ATTACK");addKnob("decay","DECAY");addKnob("sustain","SUSTAIN");addKnob("release","RELEASE");
 addKnob("detune","DETUNE");addKnob("unison","UNISON");addKnob("fm","FM INDEX");addKnob("fold","FRACTAL FOLD");
 addKnob("body","RESONATOR BODY");addKnob("damping","DAMPING");addKnob("motion","MOTION");addKnob("blend","HYBRID MIX");
 addKnob("brightness","EXCITER BRIGHTNESS");addKnob("gain","MASTER GAIN");
 addKnob("crossDepth","ROUTING DEPTH");addKnob("stereoResonance","RESONATOR WIDTH");
 initGraph();
 initModulation();
 viewport.setViewedComponent(&content,false);viewport.setScrollBarsShown(true,false);addAndMakeVisible(viewport);
 viewport.setVisible(!processor.isUniversalEnabled());
 setResizable(true,true);setResizeLimits(760,560,1920,1400);setSize(1130,900);
 startTimerHz(5);
}
void PanoramaEditor::addKnob(const char*id,const char*name){
 Knob k;k.label=std::make_unique<juce::Label>();k.label->setText(name,juce::dontSendNotification);
 k.label->setJustificationType(juce::Justification::centred);k.label->setColour(juce::Label::textColourId,juce::Colour(0xffb8cadf));
 k.slider=std::make_unique<juce::Slider>();k.slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
 k.slider->setTextBoxStyle(juce::Slider::TextBoxBelow,false,84,20);
 k.slider->setColour(juce::Slider::textBoxTextColourId,juce::Colours::white);
 k.attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.params,id,*k.slider);
 content.addAndMakeVisible(*k.label);content.addAndMakeVisible(*k.slider);
 knobs.push_back(std::move(k));
}
void PanoramaEditor::paint(juce::Graphics&g){
 g.fillAll(juce::Colour(0xff0b1020));
 g.setColour(juce::Colour(0xffe2f4ff));
 g.setFont(juce::Font(juce::FontOptions(30.f,juce::Font::bold)));
 g.drawText("P A N O R A M A I Z E",24,12,getWidth()-48,46,juce::Justification::centredLeft);
 // PANORAMA identity: cyan/gold horizon line, premium dark-graphite surface.
 juce::ColourGradient horizon(juce::Colour(0xff2d8fce),20.f,63.f,
  juce::Colour(0xffe0ab65),float(getWidth()-24),63.f,false);
 g.setGradientFill(horizon);g.fillRect(24,64,getWidth()-48,2);
 g.setColour(juce::Colour(0xff98a9ba));g.setFont(12.f);
 g.drawText("typeGPT  |  Unified Acoustic State Graph  |  Prototype 2.5.0",24,getHeight()-29,getWidth()-48,20,juce::Justification::centredLeft);
}
void PanoramaEditor::resized(){
 const int w=getWidth();
 subtitle.setBounds(26,78,w-52,22);
 preset.setBounds(26,109,(w-70)/2,32);
 engine.setBounds(44+(w-70)/2,109,(w-70)/2,32);
 routing.setBounds(26,151,w-450,32);
 importGraphButton.setBounds(w-427,151,195,32);
 graphToggle.setBounds(w-220,151,194,32);
 viewport.setBounds(24,220,w-48,getHeight()-260);
 unifiedPanel.setBounds(24,201,w-48,getHeight()-243);
 const int cw=std::max(700,viewport.getWidth()-14);
 content.setSize(cw,860+5*180+24);
 for(int i=0;i<panorama::kGraphNodes;i++){
  auto& r=graphRows[i];const int y=57+i*65;
  r.label->setBounds(19,y+12,61,35);
  r.type->setBounds(85,y+12,154,34);
  r.a->setBounds(245,y+12,112,34);
  r.b->setBounds(362,y+12,112,34);
  const int available=cw-490;
  r.level->setBounds(482,y+10,available/2,40);
  r.color->setBounds(482+available/2,y+10,available/2,40);
 }
 for(int i=0;i<2;i++){
  auto &m=modRows[i];int y=674+i*55;
  m.label->setBounds(20,y,88,36);
  m.source->setBounds(115,y,155,36);
  m.target->setBounds(275,y,110,36);
  m.destination->setBounds(390,y,115,36);
  m.depth->setBounds(510,y,cw-530,36);
 }
 graphStatus.setBounds(27,190,w-54,20);
 for(size_t i=0;i<knobs.size();i++){
  const int row=int(i/4),col=int(i%4),cellWidth=(cw-40)/4;
  const int cx=20+col*cellWidth;
  knobs[i].slider->setBounds(cx+6,860+15+row*180,cellWidth-12,128);
  knobs[i].label->setBounds(cx+2,860+145+row*180,cellWidth-4,20);
 }
}

void PanoramaEditor::initGraph(){
 const juce::StringArray typeNames{"Off","VA Osc","FM Digital","Noise","Ring","Mix","Wavefolder","Modal","Low-pass","Gain","Harmonic Additive","Phase Warp","Coupled Modes","Resonant Exchange","Phase Ecology","Spectral Matter","Matter Weave","Formant Field","Phase-Matter Bridge","Physical Body","Unified Coupler","Mesh Body","Feedback Delay","Passive Body"};
 for(int i=0;i<panorama::kGraphNodes;i++){
  auto& r=graphRows[i];const auto sid=juce::String(i);
  r.label=std::make_unique<juce::Label>();
  r.label->setText(i==panorama::kGraphNodes-1?"OUT":"N"+juce::String(i+1),juce::dontSendNotification);
  r.label->setColour(juce::Label::textColourId,juce::Colour(0xffe1f1fe));
  content.addAndMakeVisible(*r.label);
  if(i<panorama::kGraphNodes-1){
    r.type=std::make_unique<juce::ComboBox>();
    for(int j=0;j<typeNames.size();j++)r.type->addItem(typeNames[j],j+1);
    content.addAndMakeVisible(*r.type);
    r.tAttach=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.params,"gType"+sid,*r.type);
  }else{
    r.type=std::make_unique<juce::ComboBox>();r.type->addItem("OUTPUT",1);r.type->setSelectedId(1,juce::dontSendNotification);
    r.type->setEnabled(false);content.addAndMakeVisible(*r.type);
  }
  auto mkSource=[&](std::unique_ptr<juce::ComboBox>& box,const juce::String& id,
                    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>& att){
    box=std::make_unique<juce::ComboBox>();box->addItem("None",1);
    for(int j=0;j<panorama::kGraphNodes;j++)box->addItem("Node " + juce::String(j+1),j+2);
    content.addAndMakeVisible(*box);
    att=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.params,id,*box);
  };
  mkSource(r.a,"gA"+sid,r.aAttach);mkSource(r.b,"gB"+sid,r.bAttach);
  auto mkSlider=[&](std::unique_ptr<juce::Slider>& slider,const juce::String&id,
                    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& att){
    slider=std::make_unique<juce::Slider>();slider->setSliderStyle(juce::Slider::LinearHorizontal);
    slider->setTextBoxStyle(juce::Slider::TextBoxRight,false,47,20);
    content.addAndMakeVisible(*slider);
    att=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.params,id,*slider);
  };
  mkSlider(r.level,"gLevel"+sid,r.levelAttach);
  mkSlider(r.color,"gColor"+sid,r.colorAttach);
 }
 graphStatus.setColour(juce::Label::textColourId,juce::Colour(0xff91cadd));
 graphStatus.setJustificationType(juce::Justification::centredRight);
 addAndMakeVisible(graphStatus);
}
void PanoramaEditor::timerCallback(){
 // Materialize the audio-thread-safe starter fallback into editable APVTS
 // controls for pre-v0.4.3 sessions and for manual engine switching.
 processor.seedEmptyModularGraphOnUiThread();
 // Reflect host program changes and restored session state in the UI. Avoid
 // recursively calling loadPreset() from a passive UI sync operation.
 const int programId=processor.getCurrentProgram()+1;
 if(preset.getSelectedId()!=programId)
  preset.setSelectedId(programId,juce::dontSendNotification);
 std::array<int,panorama::kGraphNodes> order{};
 auto current=processor.getPatchFromParameters();
 const bool valid=panorama::compileGraph(current.graph,order)&&panorama::validateModRoutes(current.graph,current.modRoutes);
 const bool modular=current.engine==panorama::Engine::Modular;
 if(unifiedPanel.isVisible()!=processor.isUniversalEnabled()){
   unifiedPanel.setVisible(processor.isUniversalEnabled());
   viewport.setVisible(!processor.isUniversalEnabled());
   graphToggle.setButtonText(processor.isUniversalEnabled()?"FACTORY SOUND MODE":"UNIFIED STATE MODE");
   if(processor.isUniversalEnabled())unifiedPanel.refreshFromProcessor();
 }
 graphStatus.setText(modular?(valid?"MODULAR GRAPH: VALID":"MODULAR GRAPH: CYCLE / INVALID AUDIO OR MOD ROUTING - MUTED"):
    "Select Modular Patchbay engine to hear this graph",juce::dontSendNotification);
 graphStatus.setColour(juce::Label::textColourId,valid?juce::Colour(0xff86dcca):juce::Colour(0xffff9292));
}

void PanoramaEditor::initModulation(){
 juce::StringArray sources{"Off","LFO A","LFO B","Envelope","Velocity","Mod Wheel"};
 for(int j=0;j<panorama::kGraphNodes;j++)sources.add("Audio Node "+juce::String(j+1));
 juce::StringArray targets;
 for(int j=0;j<panorama::kGraphNodes;j++)targets.add("Node "+juce::String(j+1));
 const juce::StringArray dests{"Color","Level","Pitch","Phase"};
 for(int i=0;i<2;i++){
  auto& m=modRows[i];const juce::String id=juce::String(i);
  m.label=std::make_unique<juce::Label>();m.label->setText("MOD "+juce::String(i+1),juce::dontSendNotification);
  m.label->setColour(juce::Label::textColourId,juce::Colours::white);content.addAndMakeVisible(*m.label);
  auto makeCombo=[&](std::unique_ptr<juce::ComboBox>& box,const juce::StringArray&choices,const juce::String&name,
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>& att){
   box=std::make_unique<juce::ComboBox>();for(int j=0;j<choices.size();j++)box->addItem(choices[j],j+1);
   content.addAndMakeVisible(*box);
   att=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.params,name,*box);
  };
  makeCombo(m.source,sources,"modSource"+id,m.sourceAttach);
  makeCombo(m.target,targets,"modTarget"+id,m.targetAttach);
  makeCombo(m.destination,dests,"modDest"+id,m.destAttach);
  m.depth=std::make_unique<juce::Slider>();m.depth->setSliderStyle(juce::Slider::LinearHorizontal);
  m.depth->setTextBoxStyle(juce::Slider::TextBoxRight,false,55,20);content.addAndMakeVisible(*m.depth);
  m.depthAttach=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.params,"modDepth"+id,*m.depth);
 }
}
