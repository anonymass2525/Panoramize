#pragma once
// Panoramize Panoramic GUI — JUCE vector dashboard.
// Visualizations are illustrative, NOT FFT measurements or a live node topology.
// All operative sound knobs bind to the existing factory-engine APVTS parameters.
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>
#include <cmath>
#include <memory>
#include <vector>

class PanoramicDashboard final : public juce::Component {
public:
 explicit PanoramicDashboard(PanoramaProcessor& p) : processor(p) {
  using A = juce::AudioProcessorValueTreeState;
  const char* ids[] = {"cutoff","resonance","fm","blend","detune","motion","brightness","gain",
                       "cutoff","resonance","attack","decay","sustain","release","body","damping",
                       "crossDepth","stereoResonance"};
  const char* captions[] = {"TONE","RESONANCE","FM INDEX","HYBRID MIX","DETUNE","MOTION","EXCITER","OUTPUT",
                            "CUTOFF","RESONANCE","ATTACK","DECAY","SUSTAIN","RELEASE","BODY","DAMPING",
                            "CROSS DEPTH","STEREO WIDTH"};
  // Coordinates are expressed in a 1500 x 795 logical surface.
  const int x[] = { 196,358,520,682,844,1006,1168,1330, 393,490,608,682,756,830,965,1044,1218,1371 };
  const int y[] = { 18,18,18,18,18,18,18,18, 478,478,478,478,478,478,478,478,487,487 };
  const int w[] = { 92,92,92,92,92,92,92,92, 84,84,72,72,72,72,72,72,106,106 };
  for (int i=0;i<18;++i) {
   auto c=std::make_unique<Control>();
   c->id=ids[i];c->caption=captions[i];c->rect={x[i],y[i],w[i],100};
   c->slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
   c->slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,72,18);
   c->slider.setColour(juce::Slider::textBoxTextColourId,juce::Colour(0xffb9e8e2));
   c->slider.setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(i==1||i==9?0xfff8bf74:0xff34edcf));
   c->slider.setColour(juce::Slider::rotarySliderOutlineColourId,juce::Colour(0xff25434b));
   c->slider.setColour(juce::Slider::thumbColourId,juce::Colour(0xffe9fdf8));
   c->attachment=std::make_unique<A::SliderAttachment>(processor.params,ids[i],c->slider);
   addAndMakeVisible(c->slider);
   controls.push_back(std::move(c));
  }
  for(const char* s:{"Crystal VA","Fractal Digital","Physical Resonance","Hybrid Cross-Synthesis","Modular Patchbay"})
   engine.addItem(s,engine.getNumItems()+1);
  engineAttachment=std::make_unique<A::ComboBoxAttachment>(processor.params,"engine",engine);
  addAndMakeVisible(engine);
  engine.setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff0b282b));
  engine.setColour(juce::ComboBox::outlineColourId,juce::Colour(0xff317b7a));
  engine.setColour(juce::ComboBox::textColourId,juce::Colour(0xffd8f8f2));
 }
 void refreshEngineMode() {
  const bool factory=!processor.isUniversalEnabled();
  if (factory!=lastFactory) {
   lastFactory=factory;
   engine.setEnabled(factory);
   for(auto& c:controls)c->slider.setEnabled(factory);
   repaint();
  }
 }
 void resized() override {
  const float scale=std::min(getWidth()/1500.f,getHeight()/795.f);
  const float offX=(getWidth()-1500.f*scale)*0.5f;
  const float offY=(getHeight()-795.f*scale)*0.5f;
  for(auto& c:controls) {
   auto r=c->rect;
   c->slider.setBounds(juce::roundToInt(offX+r.getX()*scale),
                       juce::roundToInt(offY+r.getY()*scale),
                       juce::roundToInt(r.getWidth()*scale),
                       juce::roundToInt(r.getHeight()*scale));
  }
  engine.setBounds(juce::roundToInt(offX+31*scale),juce::roundToInt(offY+153*scale),
                   juce::roundToInt(292*scale),juce::roundToInt(33*scale));
 }
 void paint(juce::Graphics& gr) override {
  gr.fillAll(juce::Colour(0xff061316));
  const auto scale=std::min(getWidth()/1500.f,getHeight()/795.f);
  const float ox=(getWidth()-1500.f*scale)*0.5f, oy=(getHeight()-795.f*scale)*0.5f;
  juce::Graphics::ScopedSaveState saved(gr);
  gr.addTransform(juce::AffineTransform::scale(scale).translated(ox,oy));
  auto& g=gr;
  g.setColour(juce::Colour(0xff061316));g.fillRect(0.f,0.f,1500.f,795.f);
  // Accent ribbons in background.
  for(int i=0;i<9;i++){
   juce::Path ribbon;ribbon.startNewSubPath(-80.f,700.f+i*8.f);
   ribbon.cubicTo(180.f,600.f+i*11.f,220.f,835.f-i*8.f,370.f,745.f+i*3.f);
   g.setColour(juce::Colour(0xff14dbc4).withAlpha(.07f));g.strokePath(ribbon,juce::PathStrokeType(1.f));
  }
  panel(g,12,7,1476,92);txt(g,"MACRO CONTROLS",24,22,151,22,13,juce::Colour(0xffdef8ef),true);
  line(g,174,22,174,83,juce::Colour(0xff28505a),1.f);
  const char* macroNames[]={"M1 • TONE","M2 • RESONANCE","M3 • FM","M4 • BLEND","M5 • DETUNE","M6 • MOTION","M7 • EXCITER","M8 • GAIN"};
  for(int i=0;i<8;i++)txt(g,macroNames[i],180.f+i*162.f,74.f,155,18,10,juce::Colour(0xff9bd5cf));
  // Left source panel: one factory DSP engine selection, not invented oscillator slots.
  panel(g,12,111,336,305);
  txt(g,"SOUND SOURCES",29,123,290,21,16,juce::Colour(0xffe2fcf5),true);
  txt(g,"FACTORY DSP ENGINE",30,199,270,20,11,juce::Colour(0xffa4cfc8));
  drawWave(g,39,226,276,67,0,juce::Colour(0xff18dcc8));
  labelChip(g,32,306,90,"VA / DIGITAL");labelChip(g,129,306,84,"MODAL");labelChip(g,220,306,92,"HYBRID");
  txt(g,"One selected engine drives this layer.",31,354,303,20,11,juce::Colour(0xff99b9b5));
  txt(g,"Advanced and Graph reveal deeper routing.",31,377,303,20,11,juce::Colour(0xff99b9b5));
  panel(g,12,426,336,252);
  txt(g,"PERFORMANCE / STATUS",30,442,288,22,14,juce::Colour(0xffdef8ef),true);
  txt(g,lastFactory?"FACTORY SOUND  •  EDITABLE":"UNIFIED SOUND  •  GRAPH MODE",30,480,284,28,14,
    lastFactory?juce::Colour(0xff4fffe0):juce::Colour(0xffffce80),true);
  txt(g,lastFactory?"These knobs control real APVTS parameters.":"Factory knobs paused while Unified is active.",30,522,285,48,12,juce::Colour(0xffb2d4cd));
  txt(g,"Use the ENGINE button above to switch DSP.",30,570,289,40,12,juce::Colour(0xff8cbbb3));
  labelChip(g,31,627,294,"LIVE GRAPH CONTROLS IN GRAPH TAB");
  // Signature visualization.
  panel(g,359,111,830,305);
  txt(g,"PANORAMIC SOUND ENGINE",375,122,480,21,17,juce::Colour(0xffe8fcf6),true);
  labelChip(g,965,120,106,"LANDSCAPE");labelChip(g,1078,120,94,"ENERGY");
  {
  juce::Graphics::ScopedSaveState vizSave(g);
  g.reduceClipRegion(juce::Rectangle<int>(367,151,812,231));
  juce::ColourGradient bg(juce::Colour(0xff091e2a),768.f,150.f,juce::Colour(0xff102e2b),768.f,390.f,false);
  g.setGradientFill(bg);g.fillRect(367.f,153.f,812.f,231.f);
  for(int j=0;j<42;j++){
   const auto xx=375.f+j*19.f;const auto h=12.f+64.f*(.5f+.5f*std::sin(.31f*j)*std::sin(.17f*j+2.f));
   g.setColour(juce::Colour(j%7==0?0xffe6bb78:0xff3bccb8).withAlpha(.10f));
   g.drawLine(xx,360.f-h,xx,362.f,.9f);
  }
  // Multi-layer ridges rendered procedurally; not a measured spectrum.
  for(int ridge=0;ridge<15;ridge++){
   juce::Path p;const float base=350.f-ridge*7.5f;
   for(int j=0;j<=190;j++){
    const float x=366.f+j*(812.f/190.f),u=j/190.f;
    float peaks=0.f;
    for(int n=0;n<6;n++){
     const float center=.08f+n*.177f;
     peaks+=std::exp(-std::pow((u-center)/(.037f+ridge*.0006f),2.f))*(.24f+.12f*std::sin(n*8.f));
    }
    const float v=base-53.f*peaks-15.f*std::sin(u*33.f+ridge*.26f)*std::cos(u*9.f);
    if(j==0)p.startNewSubPath(x,v);else p.lineTo(x,v);
   }
   const auto c=juce::Colour::fromHSV(std::fmod(.45f+ridge*.029f,1.f),.70f,.87f,.75f);
   g.setColour(c);g.strokePath(p,juce::PathStrokeType(ridge%4==0?1.7f:.8f));
  }
  // Horizontal spectral horizon.
  juce::ColourGradient glow(juce::Colour(0xff0be7d5),360,365,juce::Colour(0xfff4bf78),1190,365,false);
  g.setGradientFill(glow);g.fillRect(375.f,363.f,797.f,1.5f);
  for(int i=0;i<6;i++){
   float cx=442.f+i*130.f,cy=215.f+std::sin(i*1.4f)*32.f;
   if(i>0)line(g,cx-130.f,215.f+std::sin((i-1)*1.4f)*32.f,cx,cy,juce::Colour(0xff56f4db).withAlpha(.7f),1.2f);
   g.setColour(juce::Colour(0xff0d3739));g.fillEllipse(cx-17,cy-17,34.f,34.f);
   g.setColour(juce::Colour(i%2?0xffefc882:0xff5debd2));g.drawEllipse(cx-17,cy-17,34.f,34.f,1.6f);
   g.fillEllipse(cx-3,cy-3,6.f,6.f);
  }
  }
  txt(g,"VISUAL MOTIF  •  NOT A LIVE SPECTRUM ANALYSER",375,383,785,24,11,juce::Colour(0xff98bab6));
  // Graph diagram, schematic only.
  panel(g,1200,111,288,305);txt(g,"STATE GRAPH",1217,124,255,21,15,juce::Colour(0xffe6fff9),true);
  txt(g,"SCHEMATIC PREVIEW",1217,150,250,16,10,juce::Colour(0xff8eaea9));
  constexpr float nodes[][2]={{1344,200},{1274,258},{1419,257},{1320,302},{1437,331},{1257,350}};
  constexpr int edges[][2]={{0,1},{0,2},{1,3},{2,3},{3,4},{3,5},{5,1},{4,2}};
  for(const auto&e:edges)line(g,nodes[e[0]][0],nodes[e[0]][1],nodes[e[1]][0],nodes[e[1]][1],juce::Colour(0xff4de9d1).withAlpha(.55f),1.2f);
  const char* nodeLabels[]={"SRC","FLT","FM","MIX","OUT","RES"};
  for(int i=0;i<6;i++){
   const float x=nodes[i][0],y=nodes[i][1];
   g.setColour(juce::Colour(0xff0e3433));g.fillEllipse(x-23,y-23,46.f,46.f);
   g.setColour(juce::Colour(i==3?0xffedbd79:0xff59f2d5));g.drawEllipse(x-23,y-23,46.f,46.f,1.5f);
   txt(g,nodeLabels[i],x-25,y-10,50,20,10,juce::Colour(0xffe2fcf5),true);
  }
  txt(g,"Edit actual connections in GRAPH mode",1214,391,260,18,10,juce::Colour(0xffa3c7be));
  // Controls panels: all real factory APVTS attachments.
  panel(g,359,426,224,252);txt(g,"FILTER",374,439,197,21,14,juce::Colour(0xffe5fff7),true);
  drawWave(g,376,592,188,53,1,juce::Colour(0xfff3c179));
  panel(g,592,426,319,252);txt(g,"ENVELOPES  •  ADSR",607,439,288,21,14,juce::Colour(0xffe5fff7),true);
  juce::Path env;env.startNewSubPath(605,635);env.lineTo(655,590);env.lineTo(730,610);env.lineTo(817,610);env.lineTo(896,640);
  g.setColour(juce::Colour(0xff44eacd));g.strokePath(env,juce::PathStrokeType(2.f));
  panel(g,920,426,269,252);txt(g,"PHYSICAL BODY",934,440,239,21,14,juce::Colour(0xffe5fff7),true);
  drawWave(g,939,590,229,48,2,juce::Colour(0xff68e2d1));
  panel(g,1200,426,288,252);txt(g,"CROSS / SPACE",1218,439,253,21,14,juce::Colour(0xffe5fff7),true);
  txt(g,"Modulation / graph patching",1218,599,258,20,12,juce::Colour(0xffa9c5c1));
  txt(g,"available in ADVANCED + GRAPH",1218,620,258,20,12,juce::Colour(0xffa9c5c1));
  // Lower compact effects vision strip: label clearly as planned, not implemented effects.
  panel(g,12,687,1476,99);
  txt(g,"SIGNAL CHAIN  •  DESIGN TARGET",27,700,390,20,13,juce::Colour(0xffdafbf4),true);
  const char* tiles[]={"CHORUS","PHASER","DRIVE","EQ","DELAY","REVERB","SHIMMER"};
  for(int i=0;i<7;i++) {
   const int x=385+i*156;
   g.setColour(juce::Colour(0xff0d262b));g.fillRoundedRectangle(float(x),699.f,145.f,69.f,7.f);
   g.setColour(juce::Colour(0xff275756));g.drawRoundedRectangle(float(x),699.f,145.f,69.f,7.f,1.f);
   txt(g,tiles[i],x+8,706,129,20,11,juce::Colour(0xffa4d7cf),true);
   drawWave(g,x+12,732,115,25,i%3,juce::Colour(i==2?0xffe5bd78:0xff4be0c9));
  }
  txt(g,"Effect tiles are visual placeholders; existing DSP routing is unchanged.",27,741,347,34,10,juce::Colour(0xff81aca6));
 }
private:
 struct Control {
  juce::String id,caption;
  juce::Rectangle<int> rect;
  juce::Slider slider;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
 };
 PanoramaProcessor& processor;
 std::vector<std::unique_ptr<Control>> controls;
 juce::ComboBox engine;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> engineAttachment;
 bool lastFactory=true;
 static void txt(juce::Graphics&g,const juce::String&s,float x,float y,float w,float h,float size,juce::Colour c,bool bold=false) {
  g.setColour(c);g.setFont(juce::Font(juce::FontOptions(size,bold?juce::Font::bold:juce::Font::plain)));
  g.drawFittedText(s,juce::Rectangle<int>(juce::roundToInt(x),juce::roundToInt(y),
    juce::roundToInt(w),juce::roundToInt(h)),juce::Justification::centredLeft,2);
 }
 static void line(juce::Graphics&g,float x1,float y1,float x2,float y2,juce::Colour c,float weight=1.f) {
  g.setColour(c);g.drawLine(x1,y1,x2,y2,weight);
 }
 static void panel(juce::Graphics&g,float x,float y,float w,float h) {
  auto rect=juce::Rectangle<float>(x,y,w,h);
  juce::ColourGradient gradient(juce::Colour(0xff132f31),x,y,
    juce::Colour(0xff081b20),x+w,y+h,false);
  g.setGradientFill(gradient);g.fillRoundedRectangle(rect,11.f);
  g.setColour(juce::Colour(0xff25685e).withAlpha(.68f));
  g.drawRoundedRectangle(rect.reduced(.5f),11.f,1.f);
  g.setColour(juce::Colour(0xff5cebd0).withAlpha(.10f));g.drawHorizontalLine(int(y+1),x+12,x+w-12);
 }
 static void labelChip(juce::Graphics&g,float x,float y,float w,const juce::String&s) {
  g.setColour(juce::Colour(0xff143a3b));g.fillRoundedRectangle(x,y,w,24.f,6.f);
  g.setColour(juce::Colour(0xff337a75));g.drawRoundedRectangle(x,y,w,24.f,6.f,1.f);
  txt(g,s,x+7,y+2,w-10,19,10,juce::Colour(0xffa5e8db),true);
 }
 static void drawWave(juce::Graphics&g,float x,float y,float w,float h,int mode,juce::Colour color) {
  for(int layer=0;layer<4;layer++){
   juce::Path p;
   for(int j=0;j<=95;j++){
    float xx=x+w*j/95.f,u=j/95.f;
    float env=.15f+.85f*std::sin(juce::MathConstants<float>::pi*u);
    float wave= mode==0?std::sin(u*17.f+layer*.24f)+.18f*std::sin(u*40.f):
                mode==1?std::sin(u*10.f)+.4f*std::sin(u*26.f):
                std::sin(u*31.f)*(0.25f+.6f*std::sin(u*5.f));
    float yy=y+h*.5f-h*.27f*env*wave+layer*1.6f;
    if(j==0)p.startNewSubPath(xx,yy);else p.lineTo(xx,yy);
   }
   g.setColour(color.withAlpha(.78f-layer*.16f));
   g.strokePath(p,juce::PathStrokeType(layer==0?1.6f:1.f));
  }
 }
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PanoramicDashboard)
};
