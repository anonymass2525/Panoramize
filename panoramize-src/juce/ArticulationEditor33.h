#pragma once
#include <JuceHeader.h>
#include "ArticulatedPhysical33.h"
#include <array>
#include <functional>
class ArticulationEditor33 final : public juce::Component {
public:
 std::function<void(const panorama::articulated33::Model&)> onModelChanged;
 ArticulationEditor33(){
  addAndMakeVisible(heading);
  heading.setText("PHYSICAL ARTICULATION | STRING / FRETS / CONTACT",juce::dontSendNotification);
  heading.setFont(juce::Font(juce::FontOptions(19.f,juce::Font::bold)));
  heading.setColour(juce::Label::textColourId,juce::Colour(0xffe4fff4));
  constexpr const char* captions[]={"HAMMER CONTACT IMPULSE","PULL RELEASE IMPULSE","SLIDE FRICTION","PALM CONTACT LOSS / s","PALM POSITION / LENGTH","LEGATO GLIDE / ms","VELOCITY -> PICK","PRESSURE -> DAMP","BODY RESONANCE MIX"};
  constexpr double minv[]={0,0,0,0,.70,0,0,0,0};
  constexpr double maxv[]={2,2,2,600,.99,250,.5,180,1};
  for(int i=0;i<9;i++){
   auto&l=labels[size_t(i)];auto&sl=sliders[size_t(i)];
   addAndMakeVisible(l);addAndMakeVisible(sl);
   l.setText(captions[i],juce::dontSendNotification);
   l.setColour(juce::Label::textColourId,juce::Colour(0xffa8d7ce));
   sl.setRange(minv[i],maxv[i],.001);
   sl.setSliderStyle(juce::Slider::LinearHorizontal);
   sl.setTextBoxStyle(juce::Slider::TextBoxRight,false,65,22);
   sl.onDragEnd=[this](){changed();};
  }
  addAndMakeVisible(mono);
  mono.setButtonText("MONO / CONTINUOUS FRETTED STRING");
  mono.onClick=[this](){changed();};
  setModel({});
 }
 void setModel(const panorama::articulated33::Model&m){
  current_=m;
  const std::array<double,9> v{{m.hammerImpact,m.pullImpact,m.slideFriction,m.palmDamping,m.palmPosition,m.glideMs,m.velocityPickRange,m.pressureContactDamping,m.physical.bodyMix}};
  for(int i=0;i<9;i++)sliders[size_t(i)].setValue(v[size_t(i)],juce::dontSendNotification);
  mono.setToggleState(m.monoLegato,juce::dontSendNotification);
 }
 void resized() override{
  heading.setBounds(20,14,getWidth()-40,38);mono.setBounds(22,58,getWidth()-44,32);
  const int cw=std::max(175,(getWidth()-60)/3);
  for(int i=0;i<9;i++){
   const int x=20+(i%3)*cw,y=106+(i/3)*92;
   labels[size_t(i)].setBounds(x,y,cw-12,22);
   sliders[size_t(i)].setBounds(x,y+29,cw-14,31);
  }
 }
 void paint(juce::Graphics&g)override{
  g.fillAll(juce::Colour(0xff081c20));
  g.setColour(juce::Colour(0xff315a51));g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(3.f),10.f,1.f);
  g.setColour(juce::Colour(0xffb5d3c7));g.setFont(12.f);
  g.drawFittedText("CC20 transition: legato / hammer / pull / slide | CC21 impulse | CC22 palm depth | CC23 palm position",20,405,getWidth()-40,34,juce::Justification::centredLeft,2);
  g.drawFittedText("Physical modal-contact approximation; not a 3D collision solver.",20,443,getWidth()-40,25,juce::Justification::centredLeft,1);
 }
private:
 panorama::articulated33::Model current_{};
 juce::Label heading;juce::ToggleButton mono;
 std::array<juce::Label,9> labels{};std::array<juce::Slider,9> sliders{};
 void changed(){
  auto m=current_;
  m.hammerImpact=sliders[0].getValue();m.pullImpact=sliders[1].getValue();
  m.slideFriction=sliders[2].getValue();m.palmDamping=sliders[3].getValue();
  m.palmPosition=sliders[4].getValue();m.glideMs=sliders[5].getValue();
  m.velocityPickRange=sliders[6].getValue();m.pressureContactDamping=sliders[7].getValue();
  m.physical.bodyMix=sliders[8].getValue();m.monoLegato=mono.getToggleState();
  current_=m;if(onModelChanged)onModelChanged(m);
 }
};
