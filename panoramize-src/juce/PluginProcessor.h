#pragma once
#include <JuceHeader.h>
#include "PanoramaDSP.h"
#include "UniversalInstrument23.h"
#include "SmoothGraphSwap52.h"
#include <atomic>
#include <memory>
class PanoramaProcessor final: public juce::AudioProcessor {
public:
 PanoramaProcessor();
 ~PanoramaProcessor() override=default;
 void prepareToPlay(double sampleRate,int samplesPerBlock) override;
 void releaseResources() override;
 void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
 bool isBusesLayoutSupported(const BusesLayout&) const override;
 juce::AudioProcessorEditor* createEditor() override;
 bool hasEditor() const override{return true;}
 const juce::String getName() const override{return "Panoramize v5.3.0";}
 bool acceptsMidi() const override{return true;}
 bool producesMidi() const override{return false;}
 bool isMidiEffect() const override{return false;}
 double getTailLengthSeconds() const override{return 8.;}
 int getNumPrograms() override{return panorama::kPresetCount;}
 int getCurrentProgram() override{return currentProgram.load();}
 void setCurrentProgram(int n) override{loadPreset(n);}
 const juce::String getProgramName(int n) override;
 void changeProgramName(int,const juce::String&) override{}
 void getStateInformation(juce::MemoryBlock&) override;
 void setStateInformation(const void*,int) override;
 void loadPreset(int index);
 // Import an offline GraphInverse JSON recipe on the UI thread. Never call from audio.
 bool importGraphRecipeJson(const juce::String& recipe);
 panorama::Patch getPatchFromParameters() const;
 // Called from the GUI message thread; persist the default patchbay graph into
 // host parameters so the visible node controls match the sound being rendered.
 void seedEmptyModularGraphOnUiThread();
 // Unified v2.3: off-thread graph compile, realtime MIDI/audio path, DAW state.
 bool installUniversalGraph(const std::string& serialized,std::string* error=nullptr);
 panorama::universal23::Graph universalGraph() const;
 std::string universalGraphText() const;
 bool isUniversalEnabled() const noexcept {return universalEnabled.load();}
 void setUniversalEnabled(bool enabled) noexcept {universalEnabled.store(enabled);}

 juce::AudioProcessorValueTreeState params;
 static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
private:
 panorama::Synth synth;
 std::shared_ptr<panorama::universal23::Instrument> universalPrepared;
 panorama::swap52::SmoothGraphSwap graphSwap;
 mutable juce::CriticalSection universalLock;
 std::string universalRecipe=panorama::universal23::serialize(panorama::universal23::directGraph());
 std::atomic<bool> universalEnabled{true};
 double currentSampleRate=48000.;
 panorama::Patch lastAppliedPatch{};
 bool lastAppliedPatchValid=false;
 std::atomic<int> currentProgram{0};
 void handleMidi(const juce::MidiMessage& m,panorama::universal23::Instrument* graph);
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PanoramaProcessor)
};
