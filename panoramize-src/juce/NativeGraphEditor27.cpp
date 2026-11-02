#include "NativeGraphEditor27.h"
#include "UniversalInstrument23.h"
#include <algorithm>
#include <cmath>
namespace {
constexpr const char* kEngineNames[]={"VA / ACTIVE", "FM / ACTIVE", "SPECTRAL", "PHASE", "RESONANCE", "PHYSICAL", "VOCAL / FORMANT", "NONLINEAR"};
constexpr int rowHeight=62;
}
NativeGraphEditor27::NativeGraphEditor27(PanoramaProcessor& p):processor(p){
 addAndMakeVisible(addMode);addAndMakeVisible(removeMode);addAndMakeVisible(addEdge);addAndMakeVisible(removeEdge);
 addAndMakeVisible(status);addAndMakeVisible(strike);
 status.setColour(juce::Label::textColourId,juce::Colour(0xffb6dfd4));
 strike.onChange=[this](){if(updating)return;state.strikeMode=strike.getSelectedId()-1;apply();};
 viewport.setViewedComponent(&content,false);viewport.setScrollBarsShown(true,true);addAndMakeVisible(viewport);
 addMode.onClick=[this](){
  if(state.modes.size()>=32){status.setText("Maximum 32 editable modes for GUI",juce::dontSendNotification);return;}
  const int newIndex=int(state.modes.size());
  state.modes.push_back({panorama::native27::Engine::Resonance,1.25,6.,0.,.2,0.});
  if(newIndex>0)state.edges.push_back({newIndex-1,newIndex,panorama::native26::Coupling::Conservative,6000.,1.});
  apply();rebuild();
 };
 removeMode.onClick=[this](){
  if(state.modes.size()<=1)return;
  const int removed=int(state.modes.size()-1);state.modes.pop_back();
  state.edges.erase(std::remove_if(state.edges.begin(),state.edges.end(),[removed](const auto& e){return e.a==removed||e.b==removed;}),state.edges.end());
  if(state.strikeMode>=removed)state.strikeMode=0;
  apply();rebuild();
 };
 addEdge.onClick=[this](){
  if(state.modes.size()<2||state.edges.size()>=64)return;
  state.edges.push_back({0,1,panorama::native26::Coupling::Conservative,5000.,1.});apply();rebuild();
 };
 removeEdge.onClick=[this](){if(state.edges.empty())return;state.edges.pop_back();apply();rebuild();};
 refresh();
}
void NativeGraphEditor27::Content::paint(juce::Graphics& g){
 g.fillAll(juce::Colour(0xff07191e));
 const int edgeStart=38+modeCount*rowHeight+50;
 g.setFont(juce::Font(juce::FontOptions(11.f,juce::Font::bold)));
 g.setColour(juce::Colour(0xffa0d3c7));
 g.drawText("ENGINE",50,9,165,22,juce::Justification::centredLeft);
 g.drawText("PITCH RATIO",219,9,175,22,juce::Justification::centredLeft);
 g.drawText("DAMPING",398,9,175,22,juce::Justification::centredLeft);
 g.drawText("NONLINEAR",577,9,175,22,juce::Justification::centredLeft);
 g.drawText("PICKUP",756,9,165,22,juce::Justification::centredLeft);
 g.drawText("ACTIVE EFFORT",922,9,180,22,juce::Justification::centredLeft);
 g.setColour(juce::Colour(0xfff4c783));
 g.drawText("COUPLING: SOURCE",51,edgeStart-30,149,22,juce::Justification::centredLeft);
 g.drawText("DESTINATION",200,edgeStart-30,140,22,juce::Justification::centredLeft);
 g.drawText("ENERGY LAW",352,edgeStart-30,154,22,juce::Justification::centredLeft);
 g.drawText("STRENGTH",520,edgeStart-30,284,22,juce::Justification::centredLeft);
 g.drawText("RATIO",821,edgeStart-30,170,22,juce::Justification::centredLeft);
 for(int i=0;i<modeCount+edgeCount;i++){
  const float y=float(i<modeCount?38+i*rowHeight:edgeStart+(i-modeCount)*rowHeight);
  g.setColour(juce::Colour((i&1)?0xff0f2b2c:0xff163537));
  g.fillRoundedRectangle(8.f,y-6.f,float(getWidth()-16),50.f,7.f);
 }
}
void NativeGraphEditor27::configureSlider(juce::Slider& s,double min,double max,double interval){
 s.setRange(min,max,interval);s.setSliderStyle(juce::Slider::LinearHorizontal);
 s.setTextBoxStyle(juce::Slider::TextBoxRight,false,72,20);
 s.setColour(juce::Slider::trackColourId,juce::Colour(0xff2ca895));
}
void NativeGraphEditor27::refresh(){state=processor.universalGraph().instrument;rebuild();}
void NativeGraphEditor27::apply(){
 if(updating)return;
 auto graph=processor.universalGraph();
 if(std::none_of(graph.nodes.begin(),graph.nodes.end(),[](const auto& n){return n.kind==panorama::universal23::Kind::DirectState27;})){
  status.setText("Add DIRECT STATE 2.7 source to the Patchbay first",juce::dontSendNotification);return;
 }
 graph.instrument=state;
 std::string err;
 if(!processor.installUniversalGraph(panorama::universal23::serialize(graph),&err)){
  status.setText("INVALID: "+juce::String(err),juce::dontSendNotification);return;
 }
 status.setText("ACTIVE: "+juce::String(int(state.modes.size()))+" modes / "+juce::String(int(state.edges.size()))+" couplings | saved with Studio One project",juce::dontSendNotification);
}
void NativeGraphEditor27::updateMode(size_t id){
 if(updating||id>=modes.size())return;
 const auto& row=*modes[id];auto& m=state.modes[id];
 m.engine=panorama::native27::Engine(row.engine->getSelectedId()-1);
 m.ratio=row.ratio->getValue();m.damping=row.damping->getValue();
 const double n=row.nonlinear->getValue();m.nonlinear=n*n*2.e11;
 m.pickup=row.pickup->getValue();m.excitation=row.excitation->getValue();
 apply();
}
void NativeGraphEditor27::updateEdge(size_t id){
 if(updating||id>=edges.size())return;
 const auto& row=*edges[id];auto& e=state.edges[id];
 e.a=row.a->getSelectedId()-1;e.b=row.b->getSelectedId()-1;
 e.kind=panorama::native26::Coupling(row.kind->getSelectedId()-1);
 e.strength=row.strength->getValue();e.ratio=row.ratio->getValue();
 apply();
}
void NativeGraphEditor27::rebuild(){
 updating=true;modes.clear();edges.clear();
 strike.clear(juce::dontSendNotification);
 for(int i=0;i<int(state.modes.size());i++)strike.addItem("STRIKE  M"+juce::String(i+1),i+1);
 strike.setSelectedId(state.strikeMode+1,juce::dontSendNotification);
 for(size_t i=0;i<state.modes.size();i++){
  auto row=std::make_unique<ModeRow>();const auto& mode=state.modes[i];
  row->title=std::make_unique<juce::Label>();row->title->setText("M"+juce::String(int(i)+1),juce::dontSendNotification);
  row->title->setColour(juce::Label::textColourId,juce::Colour(0xff4ce3c6));content.addAndMakeVisible(*row->title);
  row->engine=std::make_unique<juce::ComboBox>();
  for(int k=0;k<8;k++)row->engine->addItem(kEngineNames[k],k+1);
  row->engine->setSelectedId(int(mode.engine)+1,juce::dontSendNotification);content.addAndMakeVisible(*row->engine);
  auto setup=[&](std::unique_ptr<juce::Slider>& s,double lo,double hi,double step,double value){
   s=std::make_unique<juce::Slider>();configureSlider(*s,lo,hi,step);s->setValue(value,juce::dontSendNotification);content.addAndMakeVisible(*s);
  };
  setup(row->ratio,.05,16.,.01,mode.ratio);setup(row->damping,0.,200.,.1,mode.damping);
  setup(row->nonlinear,0.,1.,.001,std::sqrt(mode.nonlinear/2.e11));
  setup(row->pickup,0.,1.,.01,mode.pickup);setup(row->excitation,0.,1000.,1.,mode.excitation);
  row->engine->onChange=[this,i](){updateMode(i);};
  for(auto* s:{row->ratio.get(),row->damping.get(),row->nonlinear.get(),row->pickup.get(),row->excitation.get()}){
   s->onValueChange=[this,i,s](){if(!s->isMouseButtonDown())updateMode(i);};
   s->onDragEnd=[this,i](){updateMode(i);};
  }
  modes.push_back(std::move(row));
 }
 for(size_t i=0;i<state.edges.size();i++){
  auto row=std::make_unique<EdgeRow>();const auto& e=state.edges[i];
  row->title=std::make_unique<juce::Label>();row->title->setText("E"+juce::String(int(i)+1),juce::dontSendNotification);
  row->title->setColour(juce::Label::textColourId,juce::Colour(0xffeac282));content.addAndMakeVisible(*row->title);
  row->a=std::make_unique<juce::ComboBox>();row->b=std::make_unique<juce::ComboBox>();
  for(int k=0;k<int(state.modes.size());k++){
   const auto label="MODE "+juce::String(k+1);row->a->addItem(label,k+1);row->b->addItem(label,k+1);
  }
  row->a->setSelectedId(e.a+1,juce::dontSendNotification);row->b->setSelectedId(e.b+1,juce::dontSendNotification);
  content.addAndMakeVisible(*row->a);content.addAndMakeVisible(*row->b);
  row->kind=std::make_unique<juce::ComboBox>();row->kind->addItem("CONSERVE",1);row->kind->addItem("DISSIPATE",2);
  row->kind->setSelectedId(int(e.kind)+1,juce::dontSendNotification);content.addAndMakeVisible(*row->kind);
  row->strength=std::make_unique<juce::Slider>();configureSlider(*row->strength,0.,200000.,100.);
  row->strength->setValue(e.strength,juce::dontSendNotification);content.addAndMakeVisible(*row->strength);
  row->ratio=std::make_unique<juce::Slider>();configureSlider(*row->ratio,0.01,2.,.01);
  row->ratio->setValue(e.ratio,juce::dontSendNotification);content.addAndMakeVisible(*row->ratio);
  row->a->onChange=[this,i](){updateEdge(i);};row->b->onChange=[this,i](){updateEdge(i);};row->kind->onChange=[this,i](){updateEdge(i);};
  for(auto* s:{row->strength.get(),row->ratio.get()}){
   s->onValueChange=[this,i,s](){if(!s->isMouseButtonDown())updateEdge(i);};
   s->onDragEnd=[this,i](){updateEdge(i);};
  }
  edges.push_back(std::move(row));
 }
 content.modeCount=int(modes.size());content.edgeCount=int(edges.size());
 updating=false;resized();content.repaint();
}
void NativeGraphEditor27::paint(juce::Graphics& g){
 g.fillAll(juce::Colour(0xff061316));
 auto header=juce::ColourGradient(juce::Colour(0xff1e4943),0,0,juce::Colour(0xff0b1f23),0,120,false);
 g.setGradientFill(header);g.fillRect(0,0,getWidth(),128);
 g.setColour(juce::Colour(0xffe4fff7));g.setFont(juce::Font(juce::FontOptions(19.f,juce::Font::bold)));
 g.drawText("DIRECT ACOUSTIC STATE  /  INSTRUMENT DESIGN",18,7,getWidth()-25,28,juce::Justification::centredLeft);
 g.setColour(juce::Colour(0xff9fcec3));g.setFont(juce::Font(juce::FontOptions(12.f)));
 g.drawText("VA / FM active input  ·  Spectral / Phase / Resonance / Physical / Vocal / Nonlinear  ·  JOINT SOLVER",18,36,getWidth()-25,22,juce::Justification::centredLeft);
 juce::ColourGradient accent(juce::Colour(0xff30e6ce),16.f,62.f,
                             juce::Colour(0xffedbd78),float(getWidth()-16),62.f,false);
 g.setGradientFill(accent);g.fillRoundedRectangle(16.f,62.f,float(getWidth()-32),1.5f,.75f);

}
void NativeGraphEditor27::resized(){
 const int width=getWidth(),buttonWidth=std::max(80,(width-34)/4);
 for(int i=0;i<4;i++)(i==0?addMode:i==1?removeMode:i==2?addEdge:removeEdge).setBounds(10+i*buttonWidth,68,buttonWidth-6,31);
 status.setBounds(13,101,std::max(60,width-245),24);
 strike.setBounds(width-218,102,203,26);
 viewport.setBounds(8,135,width-16,std::max(100,getHeight()-144));
 const int cw=std::max(1100,viewport.getWidth()-16);
 const int modeStart=38,edgeStart=modeStart+int(modes.size())*rowHeight+50;
 content.setSize(cw,edgeStart+int(edges.size())*rowHeight+36);

 for(int i=0;i<int(modes.size());i++){
  auto& r=*modes[size_t(i)];const int y=modeStart+i*rowHeight;
  r.title->setBounds(16,y,36,35);r.engine->setBounds(50,y,165,34);
  r.ratio->setBounds(219,y,175,35);r.damping->setBounds(398,y,175,35);
  r.nonlinear->setBounds(577,y,175,35);r.pickup->setBounds(756,y,165,35);
  r.excitation->setBounds(922,y,cw-932,35);
 }
 for(int i=0;i<int(edges.size());i++){
  auto& r=*edges[size_t(i)];const int y=edgeStart+i*rowHeight;
  r.title->setBounds(16,y,36,35);r.a->setBounds(51,y,140,35);r.b->setBounds(200,y,140,35);
  r.kind->setBounds(352,y,154,35);r.strength->setBounds(520,y,284,35);
  r.ratio->setBounds(821,y,cw-835,35);
 }
}
