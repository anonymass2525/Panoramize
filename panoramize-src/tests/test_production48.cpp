#include "ProductionInverse48.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <set>
#include <vector>
using namespace panorama;
int main(){
 constexpr double pi=3.14159265358979323846;
 const int rate=8000;
 std::vector<float> longAudio(size_t(rate)*32);
 double phase=0;
 for(size_t i=0;i<longAudio.size();i++){
  double t=double(i)/rate,frequency=220+3*std::sin(2*pi*5*t);
  phase+=2*pi*frequency/rate;
  double harmonic=t<16?.13:.33;
  longAudio[i]=float(.12*std::sin(phase)+harmonic*.12*std::sin(2*phase)+.0008*std::sin(i*2.13));
 }
 auto quality=production48::assess(longAudio,rate);
 assert(!quality.questionable && quality.peak<1 && quality.duration==32);
 auto result=production48::analyzeLong(longAudio,rate,1024);
 assert(result.error.empty()&&result.sections.size()>=40);
 assert(result.sections.back().toSec>30&&result.validPitchFraction>=0);
 std::cout<<"long_sections="<<result.sections.size()<<" gestures="<<result.gestures.size()<<" dynamicRange_dB="<<quality.estimatedDynamicRangeDb<<"\n";
 auto contaminated=longAudio;for(size_t i=0;i<contaminated.size()/12;i++)contaminated[i]=1.f;
 assert(production48::assess(contaminated,rate).questionable);
 contaminated[10]=std::numeric_limits<float>::quiet_NaN();assert(!production48::analyzeLong(contaminated,rate).error.empty());
 production48::Freeform free;free.base.xCells=2;free.base.yCells=1;free.base.zCells=1;
 free.controls={{.1,.08,-.08,.1}};fem36::Mesh warped;
 std::string why;
 assert(production48::buildFreeform(free,warped,&why));
 fem36::Mesh baseline;auto flat=free;flat.controls={{0,0,0,0}};
 assert(production48::buildFreeform(flat,baseline));
 assert(warped.vertices.size()==baseline.vertices.size());
 double diff=0;for(size_t i=0;i<warped.vertices.size();i++)diff+=std::abs(warped.vertices[i][2]-baseline.vertices[i][2]);
 assert(diff>1e-5);
 auto invalid=free;invalid.controls[0]=2;assert(!production48::buildFreeform(invalid,warped));
 // Physically constrained inversion: multiple pitch takes, train separate from held out.
 fem36::Model seed;assert(production48::buildFreeform(flat,seed.mesh));
 seed.meshYoung=5.e8;seed.meshDensity=1.e5;
 std::vector<inverse36::Take>takes;
 for(int i=0;i<3;i++){
  inverse36::Take t;t.midi=40+i*5;t.velocity=.7f;t.training=i!=2;
  t.audio=inverse36::render(seed,22050,t.midi,t.velocity,384);
  assert(t.audio.size()==384);
  takes.push_back(std::move(t));
 }
 auto fit=production48::fitSurfaceMaterial(takes,22050,seed,9);
 if(!fit.error.empty()){std::cerr<<fit.error<<'\n';return 3;}
 assert(fit.evaluations<=9&&!fit.ranked.empty()&&!fit.uniqueIdentification);
 assert(std::isfinite(fit.ranked[0].trainError)&&std::isfinite(fit.ranked[0].holdoutError));
 std::cout<<"fem_train="<<fit.ranked[0].trainError<<" holdout="<<fit.ranked[0].holdoutError<<" candidates="<<fit.evaluations<<'\n';
 // Guided continuous-performance inverse: known note onsets from a long take.
 // The physical renderer is used without replacing its FEM algorithm.
 const int invRate=22050;
 std::vector<float> continuous(size_t(invRate)*2,0);
 std::vector<production48::AnnotatedNote> annotations;
 for(int i=0;i<3;i++){
  const int midi=40+i*5;auto audio=inverse36::render(seed,invRate,midi,.7f,1024);assert(audio.size()==1024);
  const size_t start=size_t(i)*size_t(invRate)/2;
  std::copy(audio.begin(),audio.end(),continuous.begin()+start);
  annotations.push_back({double(start)/invRate,midi,.7f,i!=2});
 }
 const auto guided=production48::fitAnnotatedPerformance(continuous,invRate,annotations,seed,7);
 assert(guided.error.empty()&&!guided.ranked.empty()&&guided.evaluations<=7);
 auto wrong=annotations;wrong[0].onsetSeconds=100;
 assert(!production48::fitAnnotatedPerformance(continuous,invRate,wrong,seed,7).error.empty());
 std::cout<<"guided_train="<<guided.ranked[0].trainError<<" holdout="<<guided.ranked[0].holdoutError<<'\n';
 auto graph=universal23::genuineNativeGraph();const auto registry=production48::parameterRegistry(graph);
 std::set<std::string>names;int live=0;
 for(const auto&v:registry){assert(names.insert(v.id).second);if(v.live)live++;}
 assert(live==9&&registry.size()>=25);
 // The ORIGINAL Native27/39 DSP is preserved: without automation its PCM is identical.
 auto recipe=genuine47::example();recipe.tone.wet=0;recipe.tone.output=1;recipe.routes.clear();
 auto a=genuine47::Instrument::prepare(recipe,48000);
 auto b=unified39::Instrument::prepare(recipe.original,48000);assert(a&&b);
 a->noteOn(57,.8f);b->noteOn(57,.8f);
 for(int i=0;i<1024;i++)assert(a->processOne()==b->processOne());
 // Host parameter rejection and actual rendered output difference on genuine DSP.
 auto c=genuine47::Instrument::prepare(genuine47::example(),48000);
 auto d=genuine47::Instrument::prepare(genuine47::example(),48000);
 assert(c&&d);c->noteOn(60,.8f);d->noteOn(60,.8f);
 assert(!c->setHostParameterNormalized(0,-1));assert(!c->setHostParameterNormalized(22,.3));
 assert(c->setHostParameterNormalized(0,.11));assert(c->setHostParameterNormalized(2,.95));assert(c->setHostParameterNormalized(5,.85));
 double error=0;for(int i=0;i<5000;i++){const auto x=c->processOne(),y=d->processOne();assert(std::isfinite(x)&&std::isfinite(y));error+=(x-y)*(x-y);}
 assert(error>1e-10);std::cout<<"host_tone_delta="<<error<<"\n";
 c->clearHostAutomation();
 for(int i=0;i<512;i++)assert(std::isfinite(c->processOne()));
 // Plugin-level mapping targets actual Native DSP with no rebuild.
 auto instrument=universal23::Instrument::prepare(graph,48000);assert(instrument);
 assert(instrument->setHostParameterNormalized(0,.5));
 instrument->noteOn(60,.9f);for(int i=0;i<1000;i++)assert(std::isfinite(instrument->processOne().left));
 std::cout<<"PRODUCTION48_PASS\n";
}
