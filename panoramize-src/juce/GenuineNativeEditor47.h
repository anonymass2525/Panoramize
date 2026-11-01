#pragma once
// Native27 original synthesis + unified physical39 tone/performance parameters.
// Rebuilds on message thread; atomic prepared-graph swap in PluginProcessor.
#include <JuceHeader.h>
#include "GenuineNative47.h"
#include <array>
#include <functional>
class GenuineNativeEditor47 final: public juce::Component {
public:
 using Recipe=panorama::genuine47::Recipe;
 std::function<bool(const Recipe&)> onChanged;
 GenuineNativeEditor47(){
  for(auto*s:{&cutoff_,&resonance_,&drive_,&output_,&wet_,&physical_,&acoustic_,&vaExcitation_,&fmExcitation_,&vocalDamping_,&nonlinear_,&macro_[0],&macro_[1],&macro_[2],&macro_[3]}){
   addAndMakeVisible(*s);s->setSliderStyle(juce::Slider::LinearHorizontal);
   s->setTextBoxStyle(juce::Slider::TextBoxRight,false,70,22);
   s->onDragEnd=[this]{commit();};
  }
  cutoff_.setRange(20,20000,.1);cutoff_.setSkewFactorFromMidPoint(1500);
  resonance_.setRange(.08,2,.01);drive_.setRange(.01,20,.01);
  output_.setRange(0,4,.01);wet_.setRange(0,1,.01);
  physical_.setRange(0,1,.01);acoustic_.setRange(0,1,.01);
  vaExcitation_.setRange(0,2000,1);fmExcitation_.setRange(0,2000,1);
  vocalDamping_.setRange(0,2000,.1);nonlinear_.setRange(0,1e12,1e8);
  for(auto&v:macro_)v.setRange(0,1,.01);
  const char*labels[]={"Cutoff", "Resonance / Q", "Drive", "Output", "Wet/Dry", "Physical monitor", "Native monitor", "VA powered effort", "FM powered effort", "Vocal modal damping", "Nonlinear quartic", "Macro 1", "Macro 2", "Macro 3", "Macro 4"};
  for(int i=0;i<15;i++){
   auto l=std::make_unique<juce::Label>();l->setText(labels[i],juce::dontSendNotification);
   l->setColour(juce::Label::textColourId,juce::Colours::lightblue);addAndMakeVisible(*l);labels_.push_back(std::move(l));
  }
  for(int i=0;i<4;i++){
   auto&route=route_[size_t(i)];addAndMakeVisible(route.source);addAndMakeVisible(route.target);
   addAndMakeVisible(route.depth);route.depth.setRange(-4,4,.01);
   route.depth.setSliderStyle(juce::Slider::LinearHorizontal);
   route.depth.setTextBoxStyle(juce::Slider::TextBoxRight,false,70,22);
   for(int k=0;k<9;k++)route.source.addItem(sourceNames_[size_t(k)],k+1);
   for(int k=0;k<6;k++)route.target.addItem(targetNames_[size_t(k)],k+1);
   route.source.onChange=[this]{commit();};route.target.onChange=[this]{commit();};route.depth.onDragEnd=[this]{commit();};
  }
  addAndMakeVisible(description_);description_.setColour(juce::Label::textColourId,juce::Colours::lightgrey);
  description_.setText("Native27 original oscillator + native modes. VA/FM are powered; modal coupling is conservative/dissipative. PCM filter is NOT a power port.",juce::dontSendNotification);
  setRecipe(panorama::genuine47::example());
 }
 void setRecipe(const Recipe&r){
  current_=r;refreshing_=true;
  const double values[]={r.tone.cutoff,r.tone.resonance,r.tone.drive,r.tone.output,r.tone.wet,
   r.original.physicalMix,r.original.acousticMix,r.original.acoustic.modes.size()>0?r.original.acoustic.modes[0].excitation:0,
   r.original.acoustic.modes.size()>1?r.original.acoustic.modes[1].excitation:0,
   r.original.acoustic.modes.size()>6?r.original.acoustic.modes[6].damping:0,
   r.original.acoustic.modes.size()>7?r.original.acoustic.modes[7].nonlinear:0,
   r.macros[0],r.macros[1],r.macros[2],r.macros[3]};
  auto controls=sliders();for(size_t i=0;i<controls.size();i++)controls[i]->setValue(values[i],juce::dontSendNotification);
  for(size_t i=0;i<4;i++){
   const auto&v=r.routes.size()>i?r.routes[i]:panorama::genuine47::Route{};
   route_[i].source.setSelectedId(int(v.source)+1,juce::dontSendNotification);
   route_[i].target.setSelectedId(int(v.target)+1,juce::dontSendNotification);
   route_[i].depth.setValue(v.depth,juce::dontSendNotification);
  }
  refreshing_=false;
 }
 void paint(juce::Graphics&g)override{g.fillAll(juce::Colour(0xff0c1422));
  g.setColour(juce::Colours::lightblue);g.setFont(18.f);
  g.drawText("GENUINE NATIVE27  |  GLOBAL MODULATION & TONE",15,8,getWidth()-30,30,juce::Justification::centredLeft);
  g.setFont(12.f);g.drawText("4 mod routes: source / destination / depth",20, getHeight()-185,getWidth()-40,22,juce::Justification::centredLeft);
 }
 void resized()override{
  auto controls=sliders();const int left=20,columns=3,cell=std::max(200,(getWidth()-40)/columns),start=76;
  for(size_t i=0;i<controls.size();i++){
   int x=left+int(i%3)*cell,y=start+int(i/3)*75;
   labels_[i]->setBounds(x,y,cell-12,20);controls[i]->setBounds(x,y+22,cell-18,32);
  }
  int y=std::max(start+5*75,getHeight()-155);
  for(int i=0;i<4;i++){
   auto&rt=route_[size_t(i)];const int x=left+(i%2)*(getWidth()/2),yy=y+(i/2)*46;
   rt.source.setBounds(x,yy,std::max(85,getWidth()/6),30);
   rt.target.setBounds(x+std::max(85,getWidth()/6)+5,yy,std::max(85,getWidth()/6),30);
   rt.depth.setBounds(x+2*std::max(85,getWidth()/6)+10,yy,std::max(65,getWidth()/6-20),30);
  }
  description_.setBounds(20,getHeight()-30,getWidth()-40,24);
 }
private:
 static constexpr std::array<const char*,9> sourceNames_={{"Macro 1","Macro 2","Macro 3","Macro 4","Wheel","Pressure","Velocity","Key Track","LFO"}};
 static constexpr std::array<const char*,6> targetNames_={{"Cutoff","Resonance","Drive","Output","Physical Mix","Native Mix"}};
 struct Row {juce::ComboBox source,target;juce::Slider depth;};
 std::array<Row,4> route_{};
 std::array<juce::Slider,4> macro_{};
 juce::Slider cutoff_,resonance_,drive_,output_,wet_,physical_,acoustic_,vaExcitation_,fmExcitation_,vocalDamping_,nonlinear_;
 juce::Label description_;
 std::vector<std::unique_ptr<juce::Label>> labels_;
 Recipe current_;
 bool refreshing_=false;
 std::array<juce::Slider*,15> sliders(){return {{&cutoff_,&resonance_,&drive_,&output_,&wet_,&physical_,&acoustic_,&vaExcitation_,&fmExcitation_,&vocalDamping_,&nonlinear_,&macro_[0],&macro_[1],&macro_[2],&macro_[3]}};}
 void commit(){
  if(refreshing_)return;auto next=current_;
  auto v=sliders();
  next.tone.cutoff=v[0]->getValue();next.tone.resonance=v[1]->getValue();next.tone.drive=v[2]->getValue();next.tone.output=v[3]->getValue();next.tone.wet=v[4]->getValue();
  next.original.physicalMix=v[5]->getValue();next.original.acousticMix=v[6]->getValue();
  if(next.original.acoustic.modes.size()>7){
   next.original.acoustic.modes[0].excitation=v[7]->getValue();next.original.acoustic.modes[1].excitation=v[8]->getValue();
   next.original.acoustic.modes[6].damping=v[9]->getValue();next.original.acoustic.modes[7].nonlinear=v[10]->getValue();
  }
  for(int i=0;i<4;i++)next.macros[size_t(i)]=v[size_t(11+i)]->getValue();
  next.routes.clear();for(auto&rt:route_){
   next.routes.push_back({panorama::genuine47::Source(rt.source.getSelectedId()-1),
                           panorama::genuine47::Target(rt.target.getSelectedId()-1),rt.depth.getValue()});
  }
  if(onChanged&&onChanged(next))current_=std::move(next);
 }
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GenuineNativeEditor47)
};
