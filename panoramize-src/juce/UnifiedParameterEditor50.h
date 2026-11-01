#pragma once
// 5.0: one generic inspector for public numeric internal parameters of all
// registered historical engines. Changes that cannot update live are prepared
// on the message thread. Does not claim full DSP reflection.
#include <JuceHeader.h>
#include "UnifiedControls50.h"
#include <functional>
#include <vector>
class UnifiedParameterEditor50 final:public juce::Component{
public:
 using Graph=panorama::universal23::Graph;
 std::function<bool(const Graph&)> onChanged;
 UnifiedParameterEditor50(){
  addAndMakeVisible(selector_);addAndMakeVisible(value_);addAndMakeVisible(info_);
  addAndMakeVisible(filter_);addAndMakeVisible(liveOnly_);
  filter_.setTextToShowWhenEmpty("Filter parameter IDs / groups",juce::Colours::grey);
  filter_.onTextChange=[this]{rebuildOptions();};
  liveOnly_.setButtonText("Only host-safe / live controls");
  liveOnly_.onClick=[this]{rebuildOptions();};
  value_.setSliderStyle(juce::Slider::LinearHorizontal);
  value_.setTextBoxStyle(juce::Slider::TextBoxRight,false,120,24);
  selector_.onChange=[this]{syncSelection();};
  value_.onDragEnd=[this]{commit();};
  value_.onValueChange=[this]{if(!value_.isMouseButtonDown())commit();};
  info_.setColour(juce::Label::textColourId,juce::Colours::lightblue);
 }
 void setGraph(const Graph&g){
  graph_=g;parameters_=panorama::controls50::catalog(graph_);
  rebuildOptions();
 }
 void paint(juce::Graphics&g)override{
  g.fillAll(juce::Colour(0xff111f32));
  g.setColour(juce::Colours::lightblue);g.setFont(16.f);
  g.drawText("UNIFIED DSP CONTROL INSPECTOR / 5.2",15,10,getWidth()-30,30,juce::Justification::centredLeft);
  g.setFont(12.f);g.drawText("Host-safe controls update live; complex DSP and FEM parameters require graph recompilation.",15,45,getWidth()-25,23,juce::Justification::centredLeft);
 }
 void resized()override{
  filter_.setBounds(15,82,getWidth()-30,28);
  liveOnly_.setBounds(15,112,getWidth()-30,25);
  selector_.setBounds(15,142,getWidth()-30,35);
  value_.setBounds(15,185,getWidth()-30,45);
  info_.setBounds(15,235,getWidth()-30,85);
 }
private:
 Graph graph_;
 std::vector<panorama::controls50::Parameter> parameters_;
 juce::ComboBox selector_;
 juce::TextEditor filter_;
 juce::ToggleButton liveOnly_;
 juce::Slider value_;
 juce::Label info_;
 bool changing_=false;
 void rebuildOptions(){
  changing_=true;
  selector_.clear(juce::dontSendNotification);
  const auto term=filter_.getText().trim().toLowerCase();
  int first=0;
  for(int i=0;i<int(parameters_.size());i++){
   const auto&p=parameters_[size_t(i)];
   if(liveOnly_.getToggleState()&&!p.realtime)continue;
   const auto label=juce::String::fromUTF8((p.group+" / "+p.id).c_str());
   if(term.isNotEmpty()&&!label.toLowerCase().contains(term))continue;
   if(first==0)first=i+1;
   selector_.addItem(label,i+1);
  }
  if(first)selector_.setSelectedId(first,juce::dontSendNotification);
  else info_.setText("No matching controls. Structural parameters need off-thread preparation.",juce::dontSendNotification);
  changing_=false;if(first)syncSelection();
 }
 void syncSelection(){
  if(changing_)return;
  const int index=selector_.getSelectedId()-1;
  if(index<0||index>=int(parameters_.size()))return;
  const auto&p=parameters_[size_t(index)];
  changing_=true;
  value_.setRange(p.lo,p.hi,0.);
  value_.setValue(p.read(graph_),juce::dontSendNotification);
  const bool logarithmic=p.lo>0&&p.hi/p.lo>100.;
  value_.setSkewFactor(logarithmic?std::log(.5)/std::log((std::sqrt(p.lo*p.hi)-p.lo)/(p.hi-p.lo)) :1.);
  info_.setText("ID: "+juce::String::fromUTF8(p.id.c_str())+" | Unit: "+juce::String::fromUTF8(p.unit.c_str())+
    (p.realtime?" | Live host parameter":" | Recompile graph on change"),juce::dontSendNotification);
  changing_=false;
 }
 void commit(){
  if(changing_)return;
  const int index=selector_.getSelectedId()-1;
  if(index<0||index>=int(parameters_.size()))return;
  const auto&p=parameters_[size_t(index)];
  const double x=value_.getValue();
  Graph next=graph_;std::string error;
  if(!panorama::controls50::change(next,p.id,x,&error)){
   info_.setText("INVALID: "+juce::String::fromUTF8(error.c_str()),juce::dontSendNotification);return;
  }
  if(onChanged&&!onChanged(next)){
   info_.setText("Graph validation / compilation rejected the change",juce::dontSendNotification);
   return;
  }
  graph_=std::move(next);
 }
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UnifiedParameterEditor50)
};
