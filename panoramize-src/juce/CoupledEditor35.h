#pragma once
#include <JuceHeader.h>
#include "CoupledGuitar35.h"
#include <array>
#include <functional>
// Human-scale editor for 3.5 physical soundboard. Recompile/reallocate only
// on the JUCE message thread when slider drag finishes, not in audio callback.
class CoupledEditor35 final : public juce::Component {
public:
 std::function<void(const panorama::coupled35::Model&)> onModelChanged;
 CoupledEditor35(){
  addAndMakeVisible(heading);
  heading.setText("PANORAMIZE | SIX-STRING COUPLED 3D SOUNDBOARD",juce::dontSendNotification);
  heading.setFont(juce::Font(juce::FontOptions(20.f,juce::Font::bold)));
  heading.setColour(juce::Label::textColourId,juce::Colour(0xffe0fff5));
  constexpr const char* names[]={"STRING LOSS / s","AXIAL / TRANSVERSE RATIO","TORSION / TRANSVERSE RATIO",
   "FRETBOARD GAP","FRET CONTACT STIFFNESS","FRET FRICTION","FINGER STIFFNESS",
   "PALM CONTACT DAMPING","SOUNDBOARD YOUNG / Pa","SOUNDBOARD DENSITY","SOUNDBOARD LOSS",
   "BRIDGE / MESH COUPLING","OUTPUT GAIN","FINGER GLIDE / ms","POISSON RATIO"};
  constexpr double lo[]={0,.01,.0001,.000001,.01,0,.1,0,1.e6,1.e3,0,0,.01,1,0};
  constexpr double hi[]={100,1,.1,.01,2,2,10,1000,2.e9,1.e6,300,.5,20,1000,.46};
  for(int i=0;i<15;i++){
   addAndMakeVisible(labels[size_t(i)]);addAndMakeVisible(sliders[size_t(i)]);
   labels[size_t(i)].setText(names[i],juce::dontSendNotification);
   labels[size_t(i)].setColour(juce::Label::textColourId,juce::Colour(0xffb4d8cd));
   auto& slider=sliders[size_t(i)];slider.setRange(lo[i],hi[i],0);slider.setSkewFactorFromMidPoint((lo[i]+hi[i])*.5);
   slider.setSliderStyle(juce::Slider::LinearHorizontal);
   slider.setTextBoxStyle(juce::Slider::TextBoxRight,false,90,22);
   slider.onDragEnd=[this](){commit();};
  }
  setModel({});
 }
 void setModel(const panorama::coupled35::Model&m){
  current_=m;const std::array<double,15>a={m.stringLoss,m.longitudinalRatio,m.torsionRatio,
   m.fretGap,m.fretStiffness,m.fretFriction,m.fingerStiffness,m.palmDamping,
   m.meshYoung,m.meshDensity,m.meshLoss,m.bridgeCoupling,m.outputGain,m.glideMs,m.meshPoisson};
  for(size_t i=0;i<a.size();i++)sliders[i].setValue(a[i],juce::dontSendNotification);
 }
 void resized()override{
  heading.setBounds(18,12,getWidth()-36,35);
  const int w=std::max(190,(getWidth()-40)/3);
  for(int i=0;i<15;i++){
   const int x=12+(i%3)*w,y=60+(i/3)*73;
   labels[size_t(i)].setBounds(x,y,w-14,20);
   sliders[size_t(i)].setBounds(x,y+24,w-18,29);
  }
 }
 void paint(juce::Graphics&g)override{
  g.fillAll(juce::Colour(0xff06191c));g.setColour(juce::Colour(0xff2fa894));
  g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(3),10,1);
 }
private:
 panorama::coupled35::Model current_{};
 juce::Label heading;std::array<juce::Label,15>labels{};std::array<juce::Slider,15>sliders{};
 void commit(){
  auto m=current_;m.stringLoss=sliders[0].getValue();m.longitudinalRatio=sliders[1].getValue();
  m.torsionRatio=sliders[2].getValue();m.fretGap=sliders[3].getValue();
  m.fretStiffness=sliders[4].getValue();m.fretFriction=sliders[5].getValue();
  m.fingerStiffness=sliders[6].getValue();m.palmDamping=sliders[7].getValue();
  m.meshYoung=sliders[8].getValue();m.meshDensity=sliders[9].getValue();
  m.meshLoss=sliders[10].getValue();m.bridgeCoupling=sliders[11].getValue();
  m.outputGain=sliders[12].getValue();m.glideMs=sliders[13].getValue();
  m.meshPoisson=sliders[14].getValue();current_=m;if(onModelChanged)onModelChanged(m);
 }
};
