#pragma once
// GUI for the MIDI expressive physical source. All patch preparation is on the
// JUCE message thread; audio processing never reads slider objects.
#include <JuceHeader.h>
#include "ExpressivePhysical32.h"
#include <array>
#include <functional>
class ExpressiveEditor32 final:public juce::Component {
public:
 std::function<void(const panorama::expressive32::Model&)> onModelChanged;
 ExpressiveEditor32(){
  addAndMakeVisible(heading);
  heading.setText("EXPRESSIVE PHYSICAL | MIDI TO STRING DYNAMICS",juce::dontSendNotification);
  heading.setColour(juce::Label::textColourId,juce::Colour(0xffc3eaff));
  heading.setFont(juce::Font(juce::FontOptions(18.f,juce::Font::bold)));
  const char* names[]={"GLIDE (ms)","BEND SMOOTH (ms)","VELOCITY > PICK", "PRESSURE > DAMP", "RELEASE > DAMP", "VELOCITY FORCE", "BASE PICK POSITION", "BRIDGE MIX", "BODY MIX"};
  const double minima[]={0.,0.,0.,0.,0.,.2,.03,0.,0.};
  const double maxima[]={500.,100.,.8,180.,120.,4.,.97,1.,1.};
  for(int i=0;i<int(sliders.size());i++){
   auto&sl=sliders[size_t(i)];auto&lb=labels[size_t(i)];
   addAndMakeVisible(sl);addAndMakeVisible(lb);
   lb.setText(names[i],juce::dontSendNotification);
   lb.setColour(juce::Label::textColourId,juce::Colour(0xff8db7cb));
   sl.setRange(minima[i],maxima[i],.001);
   sl.setSliderStyle(juce::Slider::LinearHorizontal);
   sl.setTextBoxStyle(juce::Slider::TextBoxRight,false,73,22);
   sl.onValueChange=[this,i](){if(!sliders[size_t(i)].isMouseButtonDown())changed();};
   sl.onDragEnd=[this](){changed();};
  }
  addAndMakeVisible(mono);
  mono.setButtonText("MONO LEGATO / NO RE-ATTACK");
  mono.setColour(juce::ToggleButton::textColourId,juce::Colours::white);
  mono.onClick=[this](){changed();};
  setModel(panorama::expressive32::Model{});
 }
 void setModel(const panorama::expressive32::Model&m){
  const std::array<double,9> values{{m.glideMs,m.bendSmoothingMs,m.velocityPickRange,m.pressureContactDamping,
   m.releaseContactDamping,m.velocityForceCurve,m.physical.string.pickPosition,m.physical.bridgeMix,m.physical.bodyMix}};
  for(int i=0;i<int(sliders.size());i++)sliders[size_t(i)].setValue(values[size_t(i)],juce::dontSendNotification);
  mono.setToggleState(m.monoLegato,juce::dontSendNotification);
  current_=m;
 }
 void resized() override{
  heading.setBounds(16,8,getWidth()-32,35);
  mono.setBounds(20,48,getWidth()-40,32);
  const int left=22,columns=3,colw=std::max(190,(getWidth()-40)/columns);
  for(int i=0;i<int(sliders.size());i++){
   const int x=left+(i%columns)*colw,y=100+(i/columns)*93;
   labels[size_t(i)].setBounds(x,y,colw-18,24);
   sliders[size_t(i)].setBounds(x,y+28,colw-25,29);
  }
 }
 void paint(juce::Graphics&g)override{
  g.fillAll(juce::Colour(0xff101e30));
  g.setColour(juce::Colour(0xff294e65));g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(3.f),9.f,1.f);
  g.setColour(juce::Colour(0xff88afc6));g.setFont(12.f);
  g.drawText("Pressure adds contact damping; glide preserves modal phase. No full 3D string/contact simulation.",20,400,getWidth()-40,28,juce::Justification::left);
 }
private:
 panorama::expressive32::Model current_{};
 juce::Label heading;
 juce::ToggleButton mono;
 std::array<juce::Slider,9> sliders;
 std::array<juce::Label,9> labels;
 void changed(){
  auto m=current_;
  m.glideMs=sliders[0].getValue();m.bendSmoothingMs=sliders[1].getValue();
  m.velocityPickRange=sliders[2].getValue();m.pressureContactDamping=sliders[3].getValue();
  m.releaseContactDamping=sliders[4].getValue();m.velocityForceCurve=sliders[5].getValue();
  m.physical.string.pickPosition=sliders[6].getValue();
  m.physical.bridgeMix=sliders[7].getValue();m.physical.bodyMix=sliders[8].getValue();
  m.monoLegato=mono.getToggleState();
  current_=m;
  if(onModelChanged)onModelChanged(m);
 }
};
