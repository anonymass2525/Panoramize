#include "TemporalInverse28.h"
#include "UniversalInstrument23.h"
#undef NDEBUG
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
using namespace panorama;
using namespace panorama::temporal28;
struct Take {std::vector<Event> events;std::vector<float> pcm;Example clip;};
static Take make(int type,const native27::Graph& target){
 const int sr=12000,frames=15600; Take take;
 if(type==0)take.events={{0,EventType::NoteOn,60,.8f},{5500,EventType::NoteOff,60,0.f}};
 if(type==1){take.events={{0,EventType::NoteOn,55,.67f}};for(int i=1500;i<14000;i+=300){const auto x=float(1.4*std::sin(2*3.14159265358979323846*5*i/sr));take.events.push_back({i,EventType::PitchBend,0,x});}take.events.push_back({14500,EventType::NoteOff,55,0.f});}
 if(type==2)take.events={{0,EventType::NoteOn,48,.6f},{5400,EventType::NoteOn,55,.85f},{6600,EventType::NoteOff,48,0.f},{11200,EventType::NoteOff,55,0.f}};
 if(type==3)take.events={{0,EventType::NoteOn,67,.9f},{6500,EventType::NoteOff,67,0.f}};
 take.clip={nullptr,frames,sr,take.events.data(),int(take.events.size())};
 assert(validate(take.clip));assert(render(target,take.clip,take.pcm));
 take.clip.audio=take.pcm.data();
 return take;
}
int main(){
 native27::Graph target;target.name="Known Acoustic Target";
 target.modes={{native27::Engine::FM,1.07,6.7,0.,.34,155.},{native27::Engine::Vocal,2.3,8.1,0.,.26,0.}};
 target.edges={{0,1,native26::Coupling::Conservative,5.1e4,.77}};target.strikeMode=0;
 auto a=make(0,target),b=make(1,target),c=make(2,target),d=make(3,target);
 const auto fa=describe(a.clip.audio,a.clip.frames,a.clip.sampleRate);assert(fa.size()>8*25);
 std::vector<float> changed=a.pcm;
 for(size_t i=changed.size()/2;i<changed.size();i++)changed[i]*=.1f;
 assert(distance(fa,describe(changed.data(),int(changed.size()),a.clip.sampleRate))>.01);
 const auto zero=distance(fa,fa);assert(zero==0.);
 Example tr[3]={a.clip,b.clip,c.clip}; Example hold[1]={d.clip};
 Settings settings;settings.evaluations=10;settings.proposals=2;settings.seed=0x91234;
 auto result=infer(tr,3,hold,1,settings);
 assert(result.success);assert(result.evaluated>0);assert(result.trainCount==3&&result.holdoutCount==1);
 assert(result.proposals[0].trainingLoss>=0&&std::isfinite(result.proposals[0].unseenLoss));
 auto instrument=universal23::directGraph();instrument.instrument=result.proposals[0].graph;
 std::string err;const auto file=universal23::serialize(instrument);
 universal23::Graph parsed;assert(universal23::deserialize(file,parsed,&err));
 assert(file==universal23::serialize(parsed));
 assert(universal23::Instrument::prepare(parsed,12000,&err));
 auto again=infer(tr,3,hold,1,settings);
 assert(again.success);assert(result.proposals[0].topology==again.proposals[0].topology);
 assert(result.proposals[0].trainingLoss==again.proposals[0].trainingLoss);
 // Holdout cannot change the search or topology; it changes only the report.
 auto altered=d.clip;std::vector<float> different=d.pcm;
 for(auto& x:different)x=0.;altered.audio=different.data();
 const auto blind=infer(tr,3,&altered,1,settings);
 assert(blind.success&&blind.proposals[0].trainingLoss==result.proposals[0].trainingLoss);
 assert(blind.proposals[0].topology==result.proposals[0].topology);
 assert(std::abs(blind.proposals[0].unseenLoss-result.proposals[0].unseenLoss)>1.e-6);
 // Reject unsorted, unbounded and nonfinite performance annotations.
 auto bad=a.clip;std::vector<Event> broken=a.events;broken[0].value=3.f;bad.events=broken.data();assert(!validate(bad));
 bad=a.clip;bad.frames=13*12000;assert(!validate(bad));
 bad=a.clip;bad.audio=nullptr;assert(!infer(&bad,1,nullptr,0,settings).success);
 std::cout<<"PANORAMIZE_TEMPORAL_28_OK train="<<result.proposals[0].trainingLoss
          <<" unseen="<<result.proposals[0].unseenLoss<<" evaluations="<<result.evaluated
          <<" structures="<<result.distinctTopologies<<"\n";
}
