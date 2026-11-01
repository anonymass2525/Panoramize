#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UniversalGraphEditor23.h"
#include <memory>
#include <vector>
class PanoramaEditor final:public juce::AudioProcessorEditor,private juce::Timer {
public:
 explicit PanoramaEditor(PanoramaProcessor&);
 ~PanoramaEditor() override=default;
 void resized() override;
 void paint(juce::Graphics&) override;
private:
 class Content final:public juce::Component{
 public:
  void paint(juce::Graphics&)override;
 } content;
 struct GraphRow {
  std::unique_ptr<juce::Label> label;
  std::unique_ptr<juce::ComboBox> type,a,b;
  std::unique_ptr<juce::Slider> level,color;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> tAttach,aAttach,bAttach;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> levelAttach,colorAttach;
 };
 struct ModRow {
  std::unique_ptr<juce::Label> label;
  std::unique_ptr<juce::ComboBox> source,target,destination;
  std::unique_ptr<juce::Slider> depth;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> sourceAttach,targetAttach,destAttach;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> depthAttach;
 };
 std::array<ModRow,2> modRows;
 std::array<GraphRow,panorama::kGraphNodes> graphRows;
 juce::Label graphStatus;
 void initGraph();
 void initModulation();
 void timerCallback() override;
 struct Knob{
  std::unique_ptr<juce::Slider> slider;
  std::unique_ptr<juce::Label> label;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
 };
 PanoramaProcessor& processor;
 UniversalGraphPanel23 unifiedPanel;
 juce::TextButton graphToggle{"UNIFIED PATCHBAY"};
 juce::Viewport viewport;
 juce::ComboBox preset,engine,routing;
 juce::TextButton importGraphButton{"LOAD INVERSE GRAPH"};
 std::unique_ptr<juce::FileChooser> inverseChooser;
 juce::Label subtitle;
 std::vector<Knob> knobs;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> engineAttachment,routingAttachment;
 void addKnob(const char*parameter,const char*label);
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PanoramaEditor)
};
