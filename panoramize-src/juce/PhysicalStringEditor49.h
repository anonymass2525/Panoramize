#pragma once
// 4.9 small physical-string (Guitar29) editor shared by imported string + hybrid patches.
#include <JuceHeader.h>
#include "GuitarInverse29.h"
#include <functional>
class PhysicalStringEditor49 final : public juce::Component {
public:
 using Model=panorama::guitar29::Model;
 std::function<bool(const Model&)> onChanged;
 PhysicalStringEditor49(){
  for(auto* s:{&baseHz_,&pluck_,&pickup_,&damping_,&harmonics_}){
   addAndMakeVisible(*s);s->setSliderStyle(juce::Slider::LinearHorizontal);
   s->setTextBoxStyle(juce::Slider::TextBoxRight,false,100,28);
   s->onDragEnd=[this]{commit();};
  }
  baseHz_.setRange(40,1100,.1);pluck_.setRange(.05,.48,.001);
  pickup_.setRange(.05,.95,.001);damping_.setRange(.1,15.,.01);harmonics_.setRange(1,48,1);
  addAndMakeVisible(title_);title_.setText("PHYSICAL STRING / INVERSE EDIT",juce::dontSendNotification);
  addAndMakeVisible(help_);help_.setText("Pluck and pickup positions are normalized fractions of string length.",juce::dontSendNotification);
  setModel({});
 }
 void setModel(const Model&m){model_=m;
  baseHz_.setValue(m.frequencyHz,juce::dontSendNotification);
  pluck_.setValue(m.pickPosition,juce::dontSendNotification);
  pickup_.setValue(m.pickupPosition,juce::dontSendNotification);
  damping_.setValue(m.decayBase,juce::dontSendNotification);
  harmonics_.setValue(m.harmonics,juce::dontSendNotification);
 }
 void paint(juce::Graphics&g)override{g.fillAll(juce::Colour(0xff091f24));
  g.setColour(juce::Colour(0xff65e4cf));g.setFont(14.f);
  const char*names[]={"Fundamental Hz","Pluck Position","Pickup Position","Decay / second","Number of harmonics"};
  for(int i=0;i<5;i++)g.drawText(names[i],24,75+i*55,185,34,juce::Justification::centredLeft);
 }
 void resized()override{
  title_.setBounds(24,14,getWidth()-48,30);help_.setBounds(24,45,getWidth()-48,24);
  int i=0;for(auto*s:{&baseHz_,&pluck_,&pickup_,&damping_,&harmonics_})
   s->setBounds(210,75+55*i++,std::max(160,getWidth()-245),34);
 }
private:
 Model model_;
 juce::Label title_,help_;
 juce::Slider baseHz_,pluck_,pickup_,damping_,harmonics_;
 void commit(){Model next=model_;next.frequencyHz=baseHz_.getValue();next.pickPosition=pluck_.getValue();
  next.pickupPosition=pickup_.getValue();next.decayBase=damping_.getValue();next.harmonics=int(harmonics_.getValue());
  if(onChanged&&onChanged(next))model_=next;
  else setModel(model_);
 }
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhysicalStringEditor49)
};
