#pragma once
#include <JuceHeader.h>
#include "ContactPhysical34.h"
#include <array>
#include <functional>
// Edit the actual spatial string solver's physics, not the legacy modal proxy.
// Rebuild is triggered on slider drag end (message thread only).
class ContactEditor34 final : public juce::Component {
public:
 std::function<void(const panorama::contact34::Model&)> onModelChanged;
 ContactEditor34(){
  addAndMakeVisible(heading);
  heading.setText("PANORAMIZE | SPATIAL STRING / CONTACT DYNAMICS",juce::dontSendNotification);
  heading.setFont(juce::Font(juce::FontOptions(20.f,juce::Font::bold)));
  heading.setColour(juce::Label::textColourId,juce::Colour(0xffc6edff));
  constexpr const char* names[]={"OPEN STRING / Hz","STRING LOSS / s","FINGER CLAMP STIFFNESS","FRETBOARD GAP","BRIDGE RESONANCE / Hz","BODY X RESONANCE / Hz","BODY Y RESONANCE / Hz","BODY Z RESONANCE / Hz","BRIDGE-BODY COUPLING","PALM CONTACT LOSS / s","SLIDE FRICTION","FRETTING GLIDE / ms"};
  constexpr double minv[]={35,0,.1,.000001,30,20,20,20,0,0,0,2};
  constexpr double maxv[]={400,200,12,.01,1100,800,800,800,1,1000,1,1000};
  for(int i=0;i<12;i++){
   auto&label=labels[size_t(i)];auto&slider=sliders[size_t(i)];
   addAndMakeVisible(label);addAndMakeVisible(slider);
   label.setText(names[i],juce::dontSendNotification);
   label.setColour(juce::Label::textColourId,juce::Colour(0xff9fcbe3));
   slider.setRange(minv[i],maxv[i],i==3?.000001:.001);
   slider.setSliderStyle(juce::Slider::LinearHorizontal);
   slider.setTextBoxStyle(juce::Slider::TextBoxRight,false,66,22);
   slider.onDragEnd=[this](){commit();};
  }
  setModel({});
 }
 void setModel(const panorama::contact34::Model&m){
  current_=m;const std::array<double,12> a{{m.openHz,m.stringLoss,m.fingerStiffness,m.fretGap,m.bridgeHz,m.bodyHzX,m.bodyHzY,m.bodyHzZ,m.bodyCoupling,m.palmLoss,m.slideFriction,m.glideMs}};
  for(int i=0;i<12;i++)sliders[size_t(i)].setValue(a[size_t(i)],juce::dontSendNotification);
 }
 void resized() override{
  heading.setBounds(20,18,getWidth()-40,38);const int w=std::max(180,(getWidth()-45)/3);
  for(int i=0;i<12;i++){
   const int x=15+(i%3)*w,y=82+(i/3)*87;
   labels[size_t(i)].setBounds(x,y,w-16,22);sliders[size_t(i)].setBounds(x,y+30,w-20,31);
  }
 }
 void paint(juce::Graphics&g)override{
  g.fillAll(juce::Colour(0xff081424));
  g.setColour(juce::Colour(0xff4ba2ce));g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(3.f),11.f,1.f);
  g.setColour(juce::Colour(0xff99b8cb));g.setFont(12.f);
  g.drawFittedText("SPATIAL STRING (40 DOF)  /  MOVING FINGER CONTACT  /  UNILATERAL FRET  /  RECIPROCAL BRIDGE + 3-AXIS BODY",16,446,getWidth()-32,25,juce::Justification::centredLeft,1);
 }
private:
 panorama::contact34::Model current_{};
 juce::Label heading;std::array<juce::Label,12> labels{};std::array<juce::Slider,12> sliders{};
 void commit(){auto m=current_;
  m.openHz=sliders[0].getValue();m.stringLoss=sliders[1].getValue();
  m.fingerStiffness=sliders[2].getValue();m.fretGap=sliders[3].getValue();
  m.bridgeHz=sliders[4].getValue();m.bodyHzX=sliders[5].getValue();
  m.bodyHzY=sliders[6].getValue();m.bodyHzZ=sliders[7].getValue();
  m.bodyCoupling=sliders[8].getValue();m.palmLoss=sliders[9].getValue();
  m.slideFriction=sliders[10].getValue();m.glideMs=sliders[11].getValue();
  current_=m;if(onModelChanged)onModelChanged(m);
 }
};
