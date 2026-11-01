#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "NativeState27.h"
#include <memory>
#include <vector>
// Actual editable modal/edge matrix for the v2.7 joint acoustic-state voice.
// A user can construct a new instrument without editing or recompiling C++.
class NativeGraphEditor27 final:public juce::Component {
public:
 explicit NativeGraphEditor27(PanoramaProcessor&);
 void paint(juce::Graphics&) override;
 void resized() override;
 void refresh();
private:
 struct ModeRow{
  std::unique_ptr<juce::Label> title;
  std::unique_ptr<juce::ComboBox> engine;
  std::unique_ptr<juce::Slider> ratio,damping,nonlinear,pickup,excitation;
 };
 struct EdgeRow{
  std::unique_ptr<juce::Label> title;
  std::unique_ptr<juce::ComboBox> a,b,kind;
  std::unique_ptr<juce::Slider> strength,ratio;
 };
 PanoramaProcessor& processor;
 panorama::native27::Graph state;
 class Content final:public juce::Component{
 public:
  int modeCount=0,edgeCount=0;
  void paint(juce::Graphics&)override;
 };
 juce::Viewport viewport;
 Content content;
 std::vector<std::unique_ptr<ModeRow>> modes;
 std::vector<std::unique_ptr<EdgeRow>> edges;
 juce::TextButton addMode{"+ MODE"},removeMode{"- MODE"},addEdge{"+ COUPLING"},removeEdge{"- COUPLING"};
 juce::Label status;
 juce::ComboBox strike;
 bool updating=false;
 void rebuild();
 void apply();
 void updateMode(size_t);
 void updateEdge(size_t);
 static void configureSlider(juce::Slider&,double,double,double);
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NativeGraphEditor27)
};
