#pragma once
#include <JuceHeader.h>
#include "UniversalInstrument23.h"
#include "PluginProcessor.h"
#include "NativeGraphEditor27.h"
#include "ExpressiveEditor32.h"
#include "ArticulationEditor33.h"
#include "ContactEditor34.h"
#include "CoupledEditor35.h"
#include "EnergyGraphEditor40.h"
#include "GenuineNativeEditor47.h"
#include "AudioDesign49.h"
#include "PhysicalStringEditor49.h"
#include "UnifiedParameterEditor50.h"
#include <memory>
#include <vector>
// Functional, scrollable editor for the new variable-length synth graph.
// All graph compilations occur on message thread. Host session stores text.
class UniversalGraphPanel23 final : public juce::Component {
public:
 explicit UniversalGraphPanel23(PanoramaProcessor&);
 void resized() override;
 void paint(juce::Graphics&) override;
 void refreshFromProcessor();
private:
 class ListContent final:public juce::Component {
 public: void paint(juce::Graphics&)override;
 } content;
 struct Row {
  std::unique_ptr<juce::Label> label;
  std::unique_ptr<juce::ComboBox> kind,a,b,preset;
  std::unique_ptr<juce::Slider> level;
 };
 PanoramaProcessor& processor;
 panorama::universal23::Graph editing;
 std::vector<std::unique_ptr<Row>> rows;
 juce::Viewport viewport;
 juce::Label status;
 juce::TextButton addButton{"+ NODE"}, removeButton{"- NODE"};
 juce::TextButton saveButton{"SAVE GRAPH"},loadButton{"LOAD GRAPH"};
 juce::TextButton unifiedParamsButton{"INTERNAL PARAMS 5.2"};
 juce::TextButton audioInferButton{"INFER WAV / PATCH"},stringEditButton{"EDIT STRING"};
 juce::TextButton defaultButton{"RESET GRAPH"},expressiveNewButton{"NEW 6 STRING 3.5"};
 juce::TextButton energyNewButton{"NEW ENERGY 4.0"},energyButton{"EDIT ENERGY GRAPH"};
 juce::TextButton genuineNewButton{"NEW NATIVE 4.7"},genuineEditButton{"EDIT TONE & MOD"};
 juce::TextButton stateButton{"EDIT ACOUSTIC STATE"};
 juce::TextButton expressiveButton{"EDIT EXPRESSION"};
 std::unique_ptr<ExpressiveEditor32> expressiveEditor;
 std::unique_ptr<ArticulationEditor33> articulationEditor;
 std::unique_ptr<ContactEditor34> contactEditor;
 std::unique_ptr<CoupledEditor35> coupledEditor;
 std::unique_ptr<EnergyGraphEditor40> energyEditor;
 std::unique_ptr<GenuineNativeEditor47> genuineEditor;
 std::unique_ptr<PhysicalStringEditor49> stringEditor;
 std::unique_ptr<UnifiedParameterEditor50> parameterEditor;
 std::unique_ptr<NativeGraphEditor27> stateEditor;
 std::unique_ptr<juce::FileChooser> chooser;
 bool refreshing=false;
 void rebuildRows();
 bool apply(const panorama::universal23::Graph&);
 void changed(int index);
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UniversalGraphPanel23)
};
