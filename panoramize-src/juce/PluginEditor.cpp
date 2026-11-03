#include "PluginEditor.h"
namespace {
class PanoramaLook:public juce::LookAndFeel_V4 {
public:
 PanoramaLook(){setColour(juce::Slider::thumbColourId,juce::Colour(0xff5af0d8));
  setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(0xff35e4c9));
  setColour(juce::Slider::trackColourId,juce::Colour(0xff279987));
  setColour(juce::Slider::backgroundColourId,juce::Colour(0xff17383a));
  setColour(juce::Slider::textBoxTextColourId,juce::Colour(0xffd6f8f1));
  setColour(juce::Slider::textBoxBackgroundColourId,juce::Colour(0xff0a2126));
  setColour(juce::Slider::textBoxOutlineColourId,juce::Colour(0xff28534e));
  setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff102a2c));
  setColour(juce::ComboBox::textColourId,juce::Colour(0xffd9f7ef));
  setColour(juce::ComboBox::arrowColourId,juce::Colour(0xff52dcc3));
  setColour(juce::TextButton::buttonColourId,juce::Colour(0xff143336));
  setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff287c70));
  setColour(juce::TextButton::textColourOnId,juce::Colour(0xfff5fffb));
  setColour(juce::PopupMenu::highlightedBackgroundColourId,juce::Colour(0xff23776c));
  setColour(juce::PopupMenu::highlightedTextColourId,juce::Colour(0xffeffff7));
  setColour(juce::ScrollBar::thumbColourId,juce::Colour(0xff31766a));
  setColour(juce::ToggleButton::textColourId,juce::Colour(0xffc4eee3));
  setColour(juce::PopupMenu::backgroundColourId,juce::Colour(0xff0b2228));
  setColour(juce::TextButton::textColourOffId,juce::Colour(0xffd3f9ef));
  setColour(juce::ComboBox::outlineColourId,juce::Colour(0xff32665f));
  setColour(juce::Label::textColourId,juce::Colour(0xffc4e7e0));
 }
 void drawRotarySlider(juce::Graphics&g,int x,int y,int w,int h,
    float pos,float startAngle,float endAngle,juce::Slider&slider) override {
  const float diameter=juce::jmin(float(w),float(h))*0.75f;
  if(diameter<8.f)return;
  const float cx=x+w*.5f,cy=y+h*.48f,r=diameter*.43f;
  const float angle=startAngle+pos*(endAngle-startAngle);
  juce::ColourGradient face(juce::Colour(0xff19413f),cx-r,cy-r,
                            juce::Colour(0xff09181e),cx+r,cy+r,false);
  g.setGradientFill(face);g.fillEllipse(cx-r,cy-r,2*r,2*r);
  g.setColour(juce::Colour(0xff56887d));g.drawEllipse(cx-r,cy-r,2*r,2*r,1.f);
  const auto accent=slider.findColour(juce::Slider::rotarySliderFillColourId);
  juce::Path ring;ring.addCentredArc(cx,cy,r+3,r+3,0.f,startAngle,endAngle,true);
  g.setColour(juce::Colour(0xff315b59));
  g.strokePath(ring,juce::PathStrokeType(3.3f,juce::PathStrokeType::curved,
                                       juce::PathStrokeType::rounded));
  juce::Path active;active.addCentredArc(cx,cy,r+3,r+3,0.f,startAngle,angle,true);
  g.setColour(accent.withAlpha(.17f));
  g.strokePath(active,juce::PathStrokeType(7.f,juce::PathStrokeType::curved,
                                         juce::PathStrokeType::rounded));
  g.setColour(accent);
  g.strokePath(active,juce::PathStrokeType(3.4f,juce::PathStrokeType::curved,
                                         juce::PathStrokeType::rounded));
  const float in=r*.40f,out=r*.71f;
  g.setColour(juce::Colour(0xffe6fff8));
  g.drawLine(cx+std::sin(angle)*in,cy-std::cos(angle)*in,
             cx+std::sin(angle)*out,cy-std::cos(angle)*out,2.1f);
  g.setColour(juce::Colour(0xff6bbaa9).withAlpha(.45f));
  g.fillEllipse(cx-2,cy-2,4.f,4.f);
 }
};
PanoramaLook gLook;
}
void PanoramaEditor::Content::paint(juce::Graphics&g){
 g.fillAll(juce::Colour(0xff061316));
 g.setColour(juce::Colour(0xff102b30));
 g.setColour(juce::Colour(0xffdffcf4));
 g.setFont(juce::Font(juce::FontOptions(17.f,juce::Font::bold)));
 g.drawText("MODULAR PATCHBAY  |  9 AUDIO NODES / 2 MOD LANES",20,5,getWidth()-40,25,juce::Justification::centredLeft);
 juce::ColourGradient headerLine(juce::Colour(0xff2ce6c9),18.f,49.f,
                                 juce::Colour(0xfff3be79),float(getWidth()-20),49.f,false);
 g.setGradientFill(headerLine);g.fillRoundedRectangle(18.f,48.f,float(getWidth()-36),1.5f,.75f);
 g.setFont(12.f);g.setColour(juce::Colour(0xff9ccac2));
 g.drawText("Connect sources A/B; choose node DSP and change LEVEL / COLOR. No cycles allowed.",20,29,getWidth()-40,18,juce::Justification::centredLeft);
 for(int row=0;row<panorama::kGraphNodes;row++){
   auto bounds=juce::Rectangle<float>(12.f,55.f+row*65.f,(float)getWidth()-24.f,61.f);
   g.setColour(juce::Colour(row==panorama::kGraphNodes-1?0xff25443d:0xff102b30));
   g.fillRoundedRectangle(bounds,8.f);
   g.setColour(juce::Colour(0xff2f655e));g.drawRoundedRectangle(bounds,8.f,1.f);
   g.setColour(juce::Colour(row==panorama::kGraphNodes-1?0xffe7bc78:0xff27cbb6));
   g.fillRoundedRectangle(bounds.getX()+2.f,bounds.getY()+8.f,3.f,bounds.getHeight()-16.f,1.5f);
 }
 g.setColour(juce::Colour(0xffdffcf4));g.setFont(juce::Font(juce::FontOptions(17.f,juce::Font::bold)));
 g.drawText("MODULATION MATRIX  |  Source > Target > Property",20,640,getWidth()-40,26,juce::Justification::centredLeft);
 g.setColour(juce::Colour(0xff102b30));
 g.fillRoundedRectangle(juce::Rectangle<float>(12.f,672.f,(float)getWidth()-24.f,120.f),9.f);
  g.setColour(juce::Colour(0xff2b6259));
  g.drawRoundedRectangle(juce::Rectangle<float>(12.f,672.f,(float)getWidth()-24.f,120.f),9.f,1.f);
 const int knobTop=860;
 g.setColour(juce::Colour(0xffdffcf4));g.setFont(juce::Font(juce::FontOptions(17.f,juce::Font::bold)));
 g.drawText("GLOBAL SOUND CONTROLS",20,820,getWidth()-40,30,juce::Justification::centredLeft);
 g.setColour(juce::Colour(0xff102b30));
 for(int row=0;row<6;row++){
  auto bounds=juce::Rectangle<float>(12.f,(float)knobTop+row*180.f,(float)getWidth()-24.f,172.f);
  g.fillRoundedRectangle(bounds,12.f);
  g.setColour(juce::Colour(0xff2f655e));g.drawRoundedRectangle(bounds,12.f,1.f);
   g.setColour(juce::Colour(row==0?0xffe7bd79:0xff2bc3b1).withAlpha(.65f));
   g.fillRoundedRectangle(bounds.getX()+3.f,bounds.getY()+8.f,3.f,bounds.getHeight()-16.f,1.5f);
  g.setColour(juce::Colour(0xff102b30));
 }
}
PanoramaEditor::PanoramaEditor(PanoramaProcessor&p):AudioProcessorEditor(p),processor(p),unifiedPanel(p),dashboard(p){
 setLookAndFeel(&gLook);
 addAndMakeVisible(graphToggle);
 addAndMakeVisible(standardButton);
 addAndMakeVisible(advancedButton);
 addAndMakeVisible(graphButton);
 addAndMakeVisible(dashboard);
 addAndMakeVisible(unifiedPanel);
 standardButton.onClick=[this](){selectView(0);};
 advancedButton.onClick=[this](){selectView(1);};
 graphButton.onClick=[this](){selectView(2);};
 // Explicit DSP engine switch: navigating the three GUI views never changes sound.
 graphToggle.onClick=[this](){
   processor.setUniversalEnabled(!processor.isUniversalEnabled());
   if(processor.isUniversalEnabled() && viewMode==2)unifiedPanel.refreshFromProcessor();
   dashboard.refreshEngineMode();
   graphToggle.setButtonText(processor.isUniversalEnabled()?"ENGINE: UNIFIED  /  USE FACTORY":"ENGINE: FACTORY  /  USE UNIFIED");
 };
 graphToggle.setButtonText(processor.isUniversalEnabled()?"ENGINE: UNIFIED  /  USE FACTORY":"ENGINE: FACTORY  /  USE UNIFIED");
 subtitle.setText("VA / DIGITAL / MODAL / COUPLED MODES / MODULAR PATCHBAY",juce::dontSendNotification);
 subtitle.setJustificationType(juce::Justification::centredLeft);
 subtitle.setColour(juce::Label::textColourId,juce::Colour(0xff9ec4bc));addAndMakeVisible(subtitle);
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
 addKnob("metalMix","BELL METAL BODY");addKnob("metalColor","METALLIC COLOR");
 addKnob("metalDecay","BELL TAIL / NEXT NOTE");
 addKnob("bellDynamics","BELL EVOLUTION");
 addKnob("bellCoupling","MODE COUPLING / NEXT NOTE");
 addKnob("bellClarity","TAIL CLARITY / NEXT NOTE");
 initGraph();
 initModulation();
 viewport.setViewedComponent(&content,false);viewport.setScrollBarsShown(true,false);addAndMakeVisible(viewport);
 setResizable(true,true);setResizeLimits(1020,700,2300,1500);setSize(1500,965);
 selectView(0);
 startTimerHz(5);
}
void PanoramaEditor::addKnob(const char*id,const char*name){
 Knob k;k.label=std::make_unique<juce::Label>();k.label->setText(name,juce::dontSendNotification);
 k.label->setJustificationType(juce::Justification::centred);k.label->setColour(juce::Label::textColourId,juce::Colour(0xffb9e1d7));
 k.slider=std::make_unique<juce::Slider>();k.slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
 k.slider->setTextBoxStyle(juce::Slider::TextBoxBelow,false,84,20);
 if(juce::String(id)=="metalDecay")k.slider->setTooltip("Decay is applied when a NEW note is struck. Existing resonances finish naturally.");
 if(juce::String(id)=="metalMix")k.slider->setTooltip("0: original Fractal Digital sound. 1: stronger parallel inharmonic metallic body.");
 if(juce::String(id)=="bellDynamics")k.slider->setTooltip("0: reproduce Evolved 5.5. Add the coupled metallic companion body above 0.");
 if(juce::String(id)=="bellCoupling")k.slider->setTooltip("Passive, energy-preserving mode exchange; coupling coefficient is sampled when a new note starts.");
 if(juce::String(id)=="bellClarity")k.slider->setTooltip("Extra high-mode damping for a cleaner tail. Sampled at note-on.");
 k.slider->setColour(juce::Slider::textBoxTextColourId,juce::Colours::white);
 k.attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.params,id,*k.slider);
 content.addAndMakeVisible(*k.label);content.addAndMakeVisible(*k.slider);
 knobs.push_back(std::move(k));
}
void PanoramaEditor::paint(juce::Graphics&g){
 g.fillAll(juce::Colour(0xff061316));
 const int w=getWidth();
 juce::ColourGradient h(juce::Colour(0xff143b3d),0.f,0.f,juce::Colour(0xff061317),float(w),118.f,false);
 g.setGradientFill(h);g.fillRect(0,0,w,127);
 g.setColour(juce::Colour(0xffdffff8));
 g.setFont(juce::Font(juce::FontOptions(28.f,juce::Font::bold)));
 g.drawText("P A N O R A M I Z E",24,11,490,43,juce::Justification::centredLeft);
 g.setColour(juce::Colour(0xff8ad9ca));g.setFont(juce::Font(juce::FontOptions(10.f)));
 g.drawText("UNIVERSAL SOUND SYNTHESIS ENVIRONMENT",26,52,505,20,juce::Justification::centredLeft);
 juce::ColourGradient border(juce::Colour(0xff16efd4),20.f,120.f,juce::Colour(0xfff2bf73),float(w-20),120.f,false);
 g.setGradientFill(border);g.fillRect(18,119,w-36,2);
 g.setColour(juce::Colour(0xff91b5ae));g.setFont(juce::Font(juce::FontOptions(11.f)));
 g.drawText("PANORAMIC DESIGN  /  3 UI MODES  /  FACTORY + UNIFIED DSP",20,getHeight()-24,w-40,19,juce::Justification::centredLeft);
}
void PanoramaEditor::resized(){
 const int w=getWidth(),h=getHeight();
 subtitle.setBounds(26,81,std::max(235,w-850),25);
 preset.setBounds(std::max(360,w-873),20,290,34);
 standardButton.setBounds(w-571,20,124,34);
 advancedButton.setBounds(w-439,20,124,34);
 graphButton.setBounds(w-307,20,124,34);
 graphToggle.setBounds(w-322,72,297,35);
 engine.setBounds(24,143,259,34);
 routing.setBounds(292,143,270,34);
 importGraphButton.setBounds(570,143,218,34);
 dashboard.setBounds(9,130,w-18,h-159);
 viewport.setBounds(15,216,w-30,h-247);
 unifiedPanel.setBounds(15,136,w-30,h-167);
 const int cw0=std::max(700,viewport.getWidth()-14);
 content.setSize(cw0,860+6*180+24);
 graphStatus.setBounds(23,184,w-46,22);
 const int cw=cw0;
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

void PanoramaEditor::selectView(int mode){
 viewMode=juce::jlimit(0,2,mode);
 dashboard.setVisible(viewMode==0);
 viewport.setVisible(viewMode==1);
 unifiedPanel.setVisible(viewMode==2);
 engine.setVisible(viewMode==1);
 routing.setVisible(viewMode==1);
 importGraphButton.setVisible(viewMode==1);
 graphStatus.setVisible(viewMode==1);
 const juce::Colour active(0xff1c877d),inactive(0xff152e34);
 standardButton.setColour(juce::TextButton::buttonColourId,viewMode==0?active:inactive);
 advancedButton.setColour(juce::TextButton::buttonColourId,viewMode==1?active:inactive);
 graphButton.setColour(juce::TextButton::buttonColourId,viewMode==2?active:inactive);
 if(viewMode==2)unifiedPanel.refreshFromProcessor();
 dashboard.refreshEngineMode();
 resized();
}
void PanoramaEditor::initGraph(){
 const juce::StringArray typeNames{"Off","VA Osc","FM Digital","Noise","Ring","Mix","Wavefolder","Modal","Low-pass","Gain","Harmonic Additive","Phase Warp","Coupled Modes","Resonant Exchange","Phase Ecology","Spectral Matter","Matter Weave","Formant Field","Phase-Matter Bridge","Physical Body","Unified Coupler","Mesh Body","Feedback Delay","Passive Body"};
 for(int i=0;i<panorama::kGraphNodes;i++){
  auto& r=graphRows[i];const auto sid=juce::String(i);
  r.label=std::make_unique<juce::Label>();
  r.label->setText(i==panorama::kGraphNodes-1?"OUT":"N"+juce::String(i+1),juce::dontSendNotification);
  r.label->setColour(juce::Label::textColourId,juce::Colour(0xffd6f8ec));
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
 graphStatus.setColour(juce::Label::textColourId,juce::Colour(0xff92d9cb));
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
 dashboard.refreshEngineMode();
 graphToggle.setButtonText(processor.isUniversalEnabled()?"ENGINE: UNIFIED  /  USE FACTORY":"ENGINE: FACTORY  /  USE UNIFIED");
 preset.setEnabled(!processor.isUniversalEnabled());
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
