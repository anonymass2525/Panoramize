#include "UniversalGraphEditor23.h"
#include "NativeState26.h"
#include <algorithm>
namespace {
using namespace panorama::universal23;
const juce::StringArray names{"FACTORY SOUND", "UNIFIED STATE", "MIX A/B", "GAIN", "FOLD", "1-SAMPLE DELAY", "OUTPUT", "STATE -> FORCE", "STATE -> FACTORY (z^-1)", "NATIVE 2.6 / PRESET", "DIRECT STATE 2.7 / EDITABLE", "PHYSICAL GUITAR / INVERSE 2.9", "COUPLED PHYSICAL / INVERSE 3.0", "EXPRESSIVE PHYSICAL / 3.2", "ARTICULATED PHYSICAL / 3.3", "SPATIAL STRING CONTACT / 3.4", "SIX STRING FEM / 3.5", "UNIFIED PHYSICAL / ACOUSTIC 3.9", "ENERGY PORTS / IMPLICIT 4.0", "SI BOUNDARY FEM / 4.1", "SPARSE PORT HAMILTONIAN / 4.2", "SPARSE SPECTRAL + IC0 / 4.3", "BOUNDARY-REDUCED MULTIRATE / 4.4", "NONLINEAR CONTACT + NATIVE PROXY / 4.5", "GENUINE NATIVE27 / QUALITY 4.7"};
}
void UniversalGraphPanel23::ListContent::paint(juce::Graphics& g){
 g.fillAll(juce::Colour(0xff061316));
 for(int i=0;i<getHeight()/64;i++){
  const auto y=14.f+float(i*64);
  g.setColour(juce::Colour(i%2==0?0xff123033:0xff0c2229));
  g.fillRoundedRectangle(8.f,y,float(getWidth()-16),57.f,8.f);
  g.setColour(juce::Colour(0xff2a615b));g.drawRoundedRectangle(8.f,y,float(getWidth()-16),57.f,8.f,1.f);
  g.setColour(juce::Colour(0xff32d3be));g.fillRoundedRectangle(9.f,y+4.f,3.f,49.f,1.5f);
  g.setColour(juce::Colour(0xff6cead5).withAlpha(.12f));
  g.drawLine(18.f,y+1.f,float(getWidth()-19),y+1.f,1.f);
 }
}
UniversalGraphPanel23::UniversalGraphPanel23(PanoramaProcessor& p):processor(p){
 for(auto* button:{&addButton,&removeButton,&saveButton,&loadButton,&defaultButton,&expressiveNewButton})addAndMakeVisible(*button);
 addAndMakeVisible(unifiedParamsButton);addAndMakeVisible(audioInferButton);addAndMakeVisible(stringEditButton);addAndMakeVisible(status);addAndMakeVisible(stateButton);addAndMakeVisible(expressiveButton);addAndMakeVisible(energyNewButton);addAndMakeVisible(energyButton);addAndMakeVisible(genuineNewButton);addAndMakeVisible(genuineEditButton);
 unifiedParamsButton.onClick=[this](){
  if(!parameterEditor){
   parameterEditor=std::make_unique<UnifiedParameterEditor50>();
   parameterEditor->onChanged=[this](const Graph&graph){return apply(graph);};
   addAndMakeVisible(*parameterEditor);parameterEditor->setVisible(false);
  }
  const bool show=!parameterEditor->isVisible();
  if(show)parameterEditor->setGraph(editing);
  parameterEditor->setVisible(show);viewport.setVisible(!show);
  if(stringEditor)stringEditor->setVisible(false);
  if(energyEditor)energyEditor->setVisible(false);
  if(genuineEditor)genuineEditor->setVisible(false);
  if(stateEditor)stateEditor->setVisible(false);
  if(expressiveEditor)expressiveEditor->setVisible(false);
  if(articulationEditor)articulationEditor->setVisible(false);
  if(contactEditor)contactEditor->setVisible(false);
  if(coupledEditor)coupledEditor->setVisible(false);
  unifiedParamsButton.setButtonText(show?"BACK TO PATCHBAY":"INTERNAL PARAMS 5.2");resized();
 };
 stringEditButton.onClick=[this](){
  const bool exists=std::any_of(editing.nodes.begin(),editing.nodes.end(),[](const auto&n){return n.kind==Kind::GuitarString29;});
  if(!exists){status.setText("Select an inferred physical string or hybrid first",juce::dontSendNotification);return;}
  if(!stringEditor){
   stringEditor=std::make_unique<PhysicalStringEditor49>();
   stringEditor->onChanged=[this](const panorama::guitar29::Model&m){Graph g=editing;g.guitar=m;return apply(g);};
   addAndMakeVisible(*stringEditor);stringEditor->setVisible(false);
  }
  const bool show=!stringEditor->isVisible();if(show)stringEditor->setModel(editing.guitar);
  stringEditor->setVisible(show);viewport.setVisible(!show);
  if(energyEditor)energyEditor->setVisible(false);
  if(genuineEditor)genuineEditor->setVisible(false);
  if(stateEditor)stateEditor->setVisible(false);
  if(expressiveEditor)expressiveEditor->setVisible(false);
  if(articulationEditor)articulationEditor->setVisible(false);
  if(contactEditor)contactEditor->setVisible(false);
  if(coupledEditor)coupledEditor->setVisible(false);
  stringEditButton.setButtonText(show?"BACK TO PATCHBAY":"EDIT STRING");resized();
 };
 genuineNewButton.onClick=[this](){if(apply(genuineNativeGraph())){rebuildRows();genuineEditButton.triggerClick();}};
 genuineEditButton.onClick=[this](){
  const bool found=std::any_of(editing.nodes.begin(),editing.nodes.end(),[](const auto&n){return n.kind==Kind::GenuineNative47;});
  if(!found){status.setText("Choose GENUINE NATIVE 4.7 first",juce::dontSendNotification);return;}
  if(!genuineEditor){
   genuineEditor=std::make_unique<GenuineNativeEditor47>();
   genuineEditor->onChanged=[this](const panorama::genuine47::Recipe&r)->bool{Graph g=editing;g.genuine47=r;return apply(g);};
   addAndMakeVisible(*genuineEditor);genuineEditor->setVisible(false);
  }
  const bool show=!genuineEditor->isVisible();
  if(show)genuineEditor->setRecipe(editing.genuine47);
  genuineEditor->setVisible(show);viewport.setVisible(!show);
  if(stringEditor)stringEditor->setVisible(false);
  if(energyEditor)energyEditor->setVisible(false);
  if(stateEditor)stateEditor->setVisible(false);
  if(expressiveEditor)expressiveEditor->setVisible(false);
  if(articulationEditor)articulationEditor->setVisible(false);
  if(contactEditor)contactEditor->setVisible(false);
  if(coupledEditor)coupledEditor->setVisible(false);
  genuineEditButton.setButtonText(show?"BACK TO PATCHBAY":"EDIT TONE & MOD");resized();
 };
 energyNewButton.onClick=[this](){if(apply(energyGraph())){rebuildRows();energyButton.triggerClick();}};
 energyButton.onClick=[this](){
  const bool found=std::any_of(editing.nodes.begin(),editing.nodes.end(),[](const auto&n){return n.kind==Kind::EnergyGraph40;});
  if(!found){status.setText("Create an ENERGY PORTS source first",juce::dontSendNotification);return;}
  if(!energyEditor){
   energyEditor=std::make_unique<EnergyGraphEditor40>();
   energyEditor->onGraphChanged=[this](const panorama::energy40::Recipe&r)->bool{Graph next=editing;next.energy40=r;return apply(next);};
   addAndMakeVisible(*energyEditor);energyEditor->setVisible(false);
  }
  const bool show=!energyEditor->isVisible();
  if(genuineEditor)genuineEditor->setVisible(false);
  if(stringEditor)stringEditor->setVisible(false);
  if(show)energyEditor->setRecipe(editing.energy40);
  energyEditor->setVisible(show);viewport.setVisible(!show);
  if(stateEditor)stateEditor->setVisible(false);
  if(expressiveEditor)expressiveEditor->setVisible(false);
  if(articulationEditor)articulationEditor->setVisible(false);
  if(contactEditor)contactEditor->setVisible(false);
  if(coupledEditor)coupledEditor->setVisible(false);
  energyButton.setButtonText(show?"BACK TO PATCHBAY":"EDIT ENERGY GRAPH");resized();
 };
 expressiveButton.onClick=[this](){
  if(energyEditor)energyEditor->setVisible(false);
  if(genuineEditor)genuineEditor->setVisible(false);
  if(stringEditor)stringEditor->setVisible(false);
  const bool is35=std::any_of(editing.nodes.begin(),editing.nodes.end(),[](const auto&n){return n.kind==Kind::PhysicalCoupled35;});
  const bool is34=std::any_of(editing.nodes.begin(),editing.nodes.end(),[](const auto&n){return n.kind==Kind::PhysicalContact34;});
  const bool is33=std::any_of(editing.nodes.begin(),editing.nodes.end(),[](const auto&n){return n.kind==Kind::PhysicalArticulated33;});
  const bool is32=std::any_of(editing.nodes.begin(),editing.nodes.end(),[](const auto&n){return n.kind==Kind::PhysicalExpressive32;});
  if(!is35&&!is34&&!is33&&!is32){status.setText("Select an expressive physical source first",juce::dontSendNotification);return;}
  if(is35){
   if(!coupledEditor){
    coupledEditor=std::make_unique<CoupledEditor35>();
    coupledEditor->onModelChanged=[this](const panorama::coupled35::Model&m){Graph next=editing;next.coupled35=m;apply(next);};
    addAndMakeVisible(*coupledEditor);coupledEditor->setVisible(false);
   }
   const bool show=!coupledEditor->isVisible();
   if(show)coupledEditor->setModel(editing.coupled35);
   coupledEditor->setVisible(show);
   if(contactEditor)contactEditor->setVisible(false);
   if(articulationEditor)articulationEditor->setVisible(false);
   if(coupledEditor)coupledEditor->setVisible(false);
   if(expressiveEditor)expressiveEditor->setVisible(false);
   if(coupledEditor)coupledEditor->setVisible(false);
   if(stateEditor)stateEditor->setVisible(false);
   viewport.setVisible(!show);stateButton.setButtonText("EDIT ACOUSTIC STATE");
   expressiveButton.setButtonText(show?"BACK TO PATCHBAY":"EDIT SIX-STRING FEM");
  }else if(is34){
   if(!contactEditor){
    contactEditor=std::make_unique<ContactEditor34>();
    contactEditor->onModelChanged=[this](const panorama::contact34::Model&m){Graph next=editing;next.contact34=m;apply(next);};
    addAndMakeVisible(*contactEditor);contactEditor->setVisible(false);
   }
   const bool show=!contactEditor->isVisible();
   if(show)contactEditor->setModel(editing.contact34);
   contactEditor->setVisible(show);
   if(articulationEditor)articulationEditor->setVisible(false);
   if(coupledEditor)coupledEditor->setVisible(false);
   if(expressiveEditor)expressiveEditor->setVisible(false);
   if(coupledEditor)coupledEditor->setVisible(false);
   if(stateEditor)stateEditor->setVisible(false);
   viewport.setVisible(!show);stateButton.setButtonText("EDIT ACOUSTIC STATE");
   expressiveButton.setButtonText(show?"BACK TO PATCHBAY":"EDIT PHYSICAL CONTACT");
  }else if(is33){
   if(!articulationEditor){
    articulationEditor=std::make_unique<ArticulationEditor33>();
    articulationEditor->onModelChanged=[this](const panorama::articulated33::Model&m){Graph next=editing;next.articulated33=m;apply(next);};
    addAndMakeVisible(*articulationEditor);articulationEditor->setVisible(false);
   }
   const bool show=!articulationEditor->isVisible();
   if(show)articulationEditor->setModel(editing.articulated33);
   articulationEditor->setVisible(show);
   if(expressiveEditor)expressiveEditor->setVisible(false);
   if(coupledEditor)coupledEditor->setVisible(false);
   if(stateEditor)stateEditor->setVisible(false);
   viewport.setVisible(!show);stateButton.setButtonText("EDIT ACOUSTIC STATE");
   expressiveButton.setButtonText(show?"BACK TO PATCHBAY":"EDIT ARTICULATION");
  }else{
   if(!expressiveEditor){
    expressiveEditor=std::make_unique<ExpressiveEditor32>();
    expressiveEditor->onModelChanged=[this](const panorama::expressive32::Model&m){Graph next=editing;next.expressive32=m;apply(next);};
    addAndMakeVisible(*expressiveEditor);expressiveEditor->setVisible(false);
   }
   const bool show=!expressiveEditor->isVisible();
   if(show)expressiveEditor->setModel(editing.expressive32);
   expressiveEditor->setVisible(show);
   if(articulationEditor)articulationEditor->setVisible(false);
   if(stateEditor)stateEditor->setVisible(false);
   viewport.setVisible(!show);stateButton.setButtonText("EDIT ACOUSTIC STATE");
   expressiveButton.setButtonText(show?"BACK TO PATCHBAY":"EDIT EXPRESSION");
  }
  resized();
 };
 stateButton.onClick=[this](){
  if(!stateEditor){stateEditor=std::make_unique<NativeGraphEditor27>(processor);addAndMakeVisible(*stateEditor);stateEditor->setVisible(false);}
  else if(!stateEditor->isVisible())stateEditor->refresh();
  const bool show=!stateEditor->isVisible();
  if(energyEditor)energyEditor->setVisible(false);
  if(genuineEditor)genuineEditor->setVisible(false);
  if(stringEditor)stringEditor->setVisible(false);
  if(expressiveEditor)expressiveEditor->setVisible(false);
  if(articulationEditor)articulationEditor->setVisible(false);
  if(contactEditor)contactEditor->setVisible(false);
  if(coupledEditor)coupledEditor->setVisible(false);
  expressiveButton.setButtonText("EDIT ARTICULATION");
  stateEditor->setVisible(show);viewport.setVisible(!show);
  stateButton.setButtonText(show?"BACK TO PATCHBAY":"EDIT ACOUSTIC STATE");
  if(!show)refreshFromProcessor();resized();
 };
 status.setColour(juce::Label::textColourId,juce::Colour(0xffa5e8dc));
 status.setFont(juce::Font(juce::FontOptions(13.f)));
 viewport.setViewedComponent(&content,false);viewport.setScrollBarsShown(true,false);addAndMakeVisible(viewport);
 addButton.onClick=[this](){
  Graph next=editing;
  if(next.nodes.size()>=64){status.setText("64 node limit",juce::dontSendNotification);return;}
  const auto output=next.nodes.end()-1;
  if(output->kind!=Kind::Output)return;
  const int newId=int(next.nodes.size()-1);
  const int from=output->a;
  next.nodes.insert(output,Node{Kind::Gain,from,-1,0,0.8f});
  next.nodes.back().a=newId;
  if(apply(next))rebuildRows();
 };
 removeButton.onClick=[this](){
  Graph next=editing;
  if(next.nodes.size()<=2)return;
  const int deleted=int(next.nodes.size()-2);
  const int replacement=next.nodes[size_t(deleted)].a;
  if(replacement<0){status.setText("Remove node: no upstream source",juce::dontSendNotification);return;}
  next.nodes.erase(next.nodes.begin()+deleted);
  for(auto& n:next.nodes){
   if(n.a==deleted)n.a=replacement;
   else if(n.a>deleted)--n.a;
   if(n.b==deleted)n.b=replacement;
   else if(n.b>deleted)--n.b;
  }
  if(apply(next))rebuildRows();
 };
 defaultButton.onClick=[this](){if(apply(directGraph()))rebuildRows();};
 expressiveNewButton.onClick=[this](){if(apply(coupledGraph()))rebuildRows();};
 saveButton.onClick=[this](){
  chooser=std::make_unique<juce::FileChooser>("Save a Panoramize unified graph",juce::File{},"*.pgraph");
  const juce::Component::SafePointer<UniversalGraphPanel23> safe(this);
  chooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles,
   [safe](const juce::FileChooser& c){if(!safe)return;const auto file=c.getResult();
    if(file!=juce::File{}&&!file.replaceWithText(juce::String::fromUTF8(serialize(safe->editing).c_str())))
      safe->status.setText("Save failed",juce::dontSendNotification);
   });
 };
 audioInferButton.onClick=[this](){
  chooser=std::make_unique<juce::FileChooser>("Suggest an editable instrument from a WAV",juce::File{},"*.wav");
  const juce::Component::SafePointer<UniversalGraphPanel23> safe(this);
  chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,
   [safe](const juce::FileChooser&c){
    if(!safe)return;
    const auto file=c.getResult();if(file==juce::File{})return;
    juce::AudioFormatManager formats;formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
    if(!reader){safe->status.setText("Cannot read WAV",juce::dontSendNotification);return;}
    const int sr=int(std::lround(reader->sampleRate));
    if(sr<22050||sr>96000||reader->lengthInSamples<sr/4){
     safe->status.setText("WAV needs 22.05–96 kHz and >=0.25 seconds",juce::dontSendNotification);return;
    }
    // Bounded short import: render candidates off the audio callback, on message thread.
    // For long recordings, CLI produces all candidates and preserves multi-second detail.
    const int frames=int(std::min<int64_t>(reader->lengthInSamples,int64_t(sr)*2));
    juce::AudioBuffer<float> b(std::min(2,int(reader->numChannels)),frames);
    if(!reader->read(&b,0,frames,0,true,b.getNumChannels()>1)){
     safe->status.setText("WAV decode failed",juce::dontSendNotification);return;
    }
    std::vector<float> mono;
     mono.resize(static_cast<std::size_t>(frames));
    for(int i=0;i<frames;i++){
     double v=0;for(int k=0;k<b.getNumChannels();k++)v+=b.getSample(k,i);
     mono[size_t(i)]=float(v/b.getNumChannels());
    }
    const auto result=panorama::design49::infer(mono,sr,5);
    if(!result.error.empty()){safe->status.setText("INFER: "+juce::String(result.error),juce::dontSendNotification);return;}
    const auto&choice=result.ranked.front();
    if(safe->apply(choice.graph)){
     safe->rebuildRows();
     safe->status.setText("EDITABLE suggestion: "+juce::String(panorama::design49::name(choice.family))+
      (result.ambiguous?" / AMBIGUOUS — try alternate CLI candidates":" / compare alternatives via CLI"),juce::dontSendNotification);
    }
   });
 };
 loadButton.onClick=[this](){
  chooser=std::make_unique<juce::FileChooser>("Open a Panoramize unified graph",juce::File{},"*.pgraph");
  const juce::Component::SafePointer<UniversalGraphPanel23> safe(this);
  chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,
   [safe](const juce::FileChooser& c){if(!safe)return;const auto file=c.getResult();
    if(file!=juce::File{}&&safe->processor.installUniversalGraph(file.loadFileAsString().toStdString()))
      safe->refreshFromProcessor();
    else safe->status.setText("Invalid or unsupported graph",juce::dontSendNotification);
   });
 };
 refreshFromProcessor();
}
void UniversalGraphPanel23::refreshFromProcessor(){editing=processor.universalGraph();if(parameterEditor)parameterEditor->setGraph(editing);if(expressiveEditor)expressiveEditor->setModel(editing.expressive32);if(articulationEditor)articulationEditor->setModel(editing.articulated33);if(contactEditor)contactEditor->setModel(editing.contact34);if(coupledEditor)coupledEditor->setModel(editing.coupled35);if(energyEditor)energyEditor->setRecipe(editing.energy40);if(stringEditor)stringEditor->setModel(editing.guitar);rebuildRows();}
bool UniversalGraphPanel23::apply(const panorama::universal23::Graph& next){
 std::string error;
 if(!processor.installUniversalGraph(serialize(next),&error)){
  status.setText("INVALID GRAPH: "+juce::String(error),juce::dontSendNotification);return false;
 }
 editing=next;
 status.setText("ACTIVE / MIDI PLAYABLE / HOST STATE ENABLED  |  "+juce::String(int(editing.nodes.size()))+" nodes",juce::dontSendNotification);
 return true;
}
void UniversalGraphPanel23::rebuildRows(){
 refreshing=true;rows.clear();
 for(int i=0;i<int(editing.nodes.size());i++){
  auto row=std::make_unique<Row>();
  row->label=std::make_unique<juce::Label>();
  row->label->setText(juce::String(i+1).paddedLeft('0',2),juce::dontSendNotification);
  row->label->setColour(juce::Label::textColourId,juce::Colour(0xff43e6c9));
  content.addAndMakeVisible(*row->label);
  row->kind=std::make_unique<juce::ComboBox>();
  for(int t=0;t<names.size();t++)row->kind->addItem(names[t],t+1);
  content.addAndMakeVisible(*row->kind);
  row->a=std::make_unique<juce::ComboBox>();row->a->addItem("NONE",1);
  row->b=std::make_unique<juce::ComboBox>();row->b->addItem("NONE",1);
  for(int id=0;id<int(editing.nodes.size());id++){
   row->a->addItem("NODE "+juce::String(id+1),id+2);
   row->b->addItem("NODE "+juce::String(id+1),id+2);
  }
  content.addAndMakeVisible(*row->a);content.addAndMakeVisible(*row->b);
  const auto& node=editing.nodes[size_t(i)];
  row->preset=std::make_unique<juce::ComboBox>();
  if(node.kind==Kind::NativeState){
   for(int idx=0;idx<6;idx++)row->preset->addItem(juce::String::fromUTF8(panorama::native26::example(idx).name.c_str()),idx+1);
  }else if((node.kind==Kind::StateDrive||node.kind==Kind::StateReturn)){
   for(auto name:{"PHASE / turns", "SPECTRAL / energy", "PHYSICAL / displacement", "PHYSICAL / velocity"})
    row->preset->addItem(name,row->preset->getNumItems()+1);
  }else{
   for(int k=0;k<panorama::kPresetCount;k++)row->preset->addItem(panorama::factoryPresets()[size_t(k)].name,k+1);
  }
  content.addAndMakeVisible(*row->preset);
  row->level=std::make_unique<juce::Slider>();row->level->setRange(0,1,0.01);
  row->level->setSliderStyle(juce::Slider::LinearHorizontal);
  row->level->setTextBoxStyle(juce::Slider::TextBoxRight,false,45,18);
  content.addAndMakeVisible(*row->level);
  row->kind->setSelectedId(int(node.kind)+1,juce::dontSendNotification);
  row->a->setSelectedId(node.a+2,juce::dontSendNotification);
  row->b->setSelectedId(node.b+2,juce::dontSendNotification);
  row->preset->setSelectedId(node.preset+1,juce::dontSendNotification);
  row->level->setValue(node.level,juce::dontSendNotification);
  row->preset->setEnabled(node.kind==Kind::Legacy||node.kind==Kind::NativeState||(node.kind==Kind::StateDrive||node.kind==Kind::StateReturn));
  row->b->setEnabled(node.kind==Kind::Mix||(node.kind==Kind::StateDrive||node.kind==Kind::StateReturn));
  const auto callback=[this,i](){changed(i);};
  row->kind->onChange=callback;row->a->onChange=callback;row->b->onChange=callback;
  row->preset->onChange=callback;row->level->onValueChange=callback;
  rows.push_back(std::move(row));
 }
 refreshing=false;
 content.setSize(std::max(930,viewport.getWidth()-14),std::max(64,20+int(rows.size())*64));
 resized();
 status.setText("PANORAMIZE PATCH: "+juce::String(editing.name)+"  |  "+juce::String(int(rows.size()))+" nodes",juce::dontSendNotification);
}
void UniversalGraphPanel23::changed(int i){
 if(refreshing||i<0||i>=int(rows.size()))return;
 const auto& row=*rows[size_t(i)];
 Graph next=editing;auto& node=next.nodes[size_t(i)];
 node.kind=Kind(row.kind->getSelectedId()-1);
 node.a=row.a->getSelectedId()-2;node.b=row.b->getSelectedId()-2;
 node.preset=row.preset->getSelectedId()-1;
 node.level=float(row.level->getValue());
 if(node.kind==Kind::Legacy||node.kind==Kind::UnifiedState||node.kind==Kind::NativeState||node.kind==Kind::DirectState27||node.kind==Kind::GuitarString29||node.kind==Kind::PhysicalCoupled30||node.kind==Kind::PhysicalExpressive32||node.kind==Kind::PhysicalArticulated33||node.kind==Kind::PhysicalContact34||node.kind==Kind::PhysicalCoupled35||node.kind==Kind::UnifiedPhysical39||node.kind==Kind::EnergyGraph40||node.kind==Kind::BoundaryGraph41||node.kind==Kind::SparseHamiltonian42||node.kind==Kind::SparseHamiltonian43||node.kind==Kind::ReducedHamiltonian44||node.kind==Kind::NonlinearPassive45||node.kind==Kind::GenuineNative47){node.a=node.b=-1;}
 if((node.kind==Kind::StateDrive||node.kind==Kind::StateReturn) && editing.nodes[size_t(i)].kind!=Kind::StateDrive && editing.nodes[size_t(i)].kind!=Kind::StateReturn){
  node.preset=0;
  row.preset->clear(juce::dontSendNotification);
  for(auto name:{"PHASE / turns", "SPECTRAL / energy", "PHYSICAL / displacement", "PHYSICAL / velocity"})
   row.preset->addItem(name,row.preset->getNumItems()+1);
  row.preset->setSelectedId(1,juce::dontSendNotification);
 }else if(node.kind==Kind::NativeState && editing.nodes[size_t(i)].kind!=Kind::NativeState){
  node.preset=0;row.preset->clear(juce::dontSendNotification);
  for(int k=0;k<6;k++)row.preset->addItem(juce::String::fromUTF8(panorama::native26::example(k).name.c_str()),k+1);
  row.preset->setSelectedId(1,juce::dontSendNotification);
 }else if(node.kind==Kind::Legacy && editing.nodes[size_t(i)].kind!=Kind::Legacy){
  node.preset=0;
  row.preset->clear(juce::dontSendNotification);
  for(int k=0;k<panorama::kPresetCount;k++)row.preset->addItem(panorama::factoryPresets()[size_t(k)].name,k+1);
  row.preset->setSelectedId(1,juce::dontSendNotification);
 }
 if(node.kind!=Kind::Mix&&node.kind!=Kind::StateDrive&&node.kind!=Kind::StateReturn)node.b=-1;
 if(i==int(rows.size())-1&&node.kind!=Kind::Output){status.setText("Final node must be OUTPUT",juce::dontSendNotification);return;}
 // Avoid rebuilding all controls while responding to a change event.
 if(apply(next)){
  row.preset->setEnabled(node.kind==Kind::Legacy||node.kind==Kind::NativeState||(node.kind==Kind::StateDrive||node.kind==Kind::StateReturn));
  row.b->setEnabled(node.kind==Kind::Mix||(node.kind==Kind::StateDrive||node.kind==Kind::StateReturn));
 }
}
void UniversalGraphPanel23::paint(juce::Graphics&g){
 g.fillAll(juce::Colour(0xff061316));
 juce::ColourGradient gradient(juce::Colour(0xff153739),0.f,0.f,
      juce::Colour(0xff07191e),0.f,float(getHeight()),false);
 g.setGradientFill(gradient);g.fillRect(getLocalBounds());
 g.setColour(juce::Colour(0xffe4fff7));g.setFont(juce::Font(juce::FontOptions(20.f,juce::Font::bold)));
 g.drawText("PANORAMIZE | UNIFIED ACOUSTIC PATCHBAY",18,9,getWidth()-30,35,juce::Justification::centredLeft);
 g.setColour(juce::Colour(0xffa4cdc4));g.setFont(12.f);
 g.drawText("DIRECT STATE DESIGN / 72 FACTORY SOUNDS / ENERGY LINKS / SESSION SAVE",18,43,getWidth()-30,24,juce::Justification::centredLeft);
 juce::ColourGradient accent(juce::Colour(0xff22dfc3),18.f,72.f,
                             juce::Colour(0xfff2c47b),float(getWidth()-20),72.f,false);
 g.setGradientFill(accent);g.fillRoundedRectangle(18.f,72.f,float(getWidth()-36),1.7f,.8f);
}
void UniversalGraphPanel23::resized(){
 const int width=getWidth();const int y=78;
 const int w=std::max(60,(width-60)/6);
 for(int i=0;i<6;i++){
  auto* button=i==0?&addButton:i==1?&removeButton:i==2?&saveButton:i==3?&loadButton:i==4?&defaultButton:&expressiveNewButton;
  button->setBounds(12+i*w,y,w-7,32);
 }
 unifiedParamsButton.setBounds(std::max(25,width-500),43,170,29);
 audioInferButton.setBounds(std::max(25,width-320),43,185,29);
 stringEditButton.setBounds(std::max(25,width-130),43,110,29);
 status.setBounds(16,116,std::max(110,width-660),27);
 genuineNewButton.setBounds(std::max(20,width-910),115,115,29);
 genuineEditButton.setBounds(std::max(20,width-790),115,110,29);
 energyNewButton.setBounds(std::max(20,width-660),115,115,29);
 energyButton.setBounds(std::max(20,width-540),115,108,29);
 stateButton.setBounds(std::max(20,width-430),115,192,29);
 expressiveButton.setBounds(std::max(20,width-230),115,208,29);
 viewport.setBounds(12,148,width-24,std::max(100,getHeight()-162));
 if(stateEditor)stateEditor->setBounds(12,148,width-24,std::max(100,getHeight()-162));
 if(parameterEditor)parameterEditor->setBounds(12,148,width-24,std::max(100,getHeight()-162));
 if(energyEditor)energyEditor->setBounds(12,148,width-24,std::max(100,getHeight()-162));
 if(genuineEditor)genuineEditor->setBounds(12,148,width-24,std::max(100,getHeight()-162));
 if(stringEditor)stringEditor->setBounds(12,148,width-24,std::max(100,getHeight()-162));
 if(expressiveEditor)expressiveEditor->setBounds(12,148,width-24,std::max(100,getHeight()-162));
 if(articulationEditor)articulationEditor->setBounds(12,148,width-24,std::max(100,getHeight()-162));
 if(contactEditor)contactEditor->setBounds(12,148,width-24,std::max(100,getHeight()-162));
 if(coupledEditor)coupledEditor->setBounds(12,148,width-24,std::max(100,getHeight()-162));
 const int cw=std::max(930,viewport.getWidth()-13);
 content.setSize(cw,std::max(64,20+int(rows.size())*64));
 for(int i=0;i<int(rows.size());i++){
  const auto& r=*rows[size_t(i)];const int top=22+i*64;
  r.label->setBounds(20,top,42,40);
  r.kind->setBounds(62,top+3,155,35);
  r.a->setBounds(223,top+3,111,35);
  r.b->setBounds(341,top+3,111,35);
  r.preset->setBounds(461,top+3,290,35);
  r.level->setBounds(760,top+3,cw-780,35);
 }
}
