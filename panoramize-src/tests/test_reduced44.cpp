#include "ReducedHamiltonian44.h"
#include "UniversalInstrument23.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
using namespace panorama;
int main(){
 std::string error;auto r=reduced44::example();assert(reduced44::valid(r,&error));
 auto blob=reduced44::serialize(r);reduced44::Recipe decoded;
 assert(reduced44::deserialize(blob,decoded,&error));assert(reduced44::serialize(decoded)==blob);
 auto m=reduced44::Instrument::prepare(r,48000,&error);
 if(!m){std::cerr<<"default prepare: "<<error<<'\n';return 1;}
 auto stats=m->stats();std::cout<<"default full="<<stats.fullDofs<<" reduced="<<stats.reducedDofs<<" boundary="<<stats.boundaryDofs<<" nnz="<<stats.nnz<<'\n';
 assert(stats.reducedDofs<stats.fullDofs&&stats.boundaryDofs>=int(r.source.ports.size())-2);
 m->noteOn(57,.85f);
 double peak=0,transfer=0;
 for(int i=0;i<6000;i++){float v=m->processOne();assert(std::isfinite(v));peak=std::max(peak,std::abs(double(v)));
  for(int region=1;region<stats.regions;region++)transfer=std::max(transfer,m->regionEnergy(region));}
 auto ledger=m->ledger();stats=m->stats();
 std::cout<<"default peak="<<peak<<" split_defect="<<ledger.splitError<<" input="<<ledger.active<<" dissipated="<<ledger.dissipated<<" failed="<<stats.fails<<"\n";
 assert(peak>1e-10&&stats.fails==0&&transfer>1e-28);
 // Independent dissipated-energy accounting must close the subcycled boundary
 // power balance without deriving damping from a global energy difference.
 assert(std::abs(ledger.splitError)<1e-10+std::abs(ledger.active)*1e-4);
 auto reference=sparse43::Instrument::prepare(r.source,48000,&error);
 auto approximated=reduced44::Instrument::prepare(r,48000,&error);
 assert(reference&&approximated);reference->noteOn(57,.8f);approximated->noteOn(57,.8f);
 double refnorm=0,difference=0;
 for(int i=0;i<3500;i++){
  const double original=reference->processOne(),reduced=approximated->processOne();
  refnorm+=original*original;difference+=(original-reduced)*(original-reduced);
 }
 std::cout<<"base relative waveform L2="<<std::sqrt(difference/refnorm)<<'\n';
 assert(std::sqrt(difference/refnorm)<.10);
 assert(std::isfinite(ledger.splitError)&&std::isfinite(ledger.finalEnergy));
 // Single-rate implicit on the reduced space: exactly balanced discrete energy.
 // Dissipation-free multirate link is passive: no active work after note onset.
 auto lossless=r;
 for(auto&region:lossless.source.regions){region.damping=0;for(auto&mode:region.modes)mode.damping=0;}
 for(auto&link:lossless.source.links)link.damping=0;
 auto conservation=reduced44::Instrument::prepare(lossless,48000,&error);assert(conservation);
 conservation->noteOn(57,.8f);
 for(int i=0;i<10000;i++)assert(std::isfinite(conservation->processOne()));
 auto clean=conservation->ledger();
 assert(clean.dissipated<1e-12);
 assert(std::abs(clean.splitError)<1e-10+std::abs(clean.active)*1e-5);
 auto single=r;single.multirate=false;auto full=reduced44::Instrument::prepare(single,48000,&error);
 if(!full){std::cerr<<error<<'\n';return 2;}
 full->noteOn(57,.85f);for(int i=0;i<6000;i++){float s=full->processOne();assert(std::isfinite(s));}
 auto e=full->ledger();auto st=full->stats();
 std::cout<<"single residual="<<e.splitError<<" failed="<<st.fails<<'\n';
 assert(st.fails==0&&std::abs(e.splitError)<1e-9+std::abs(e.active)*1e-4);
 // No coupling: unexcited remote region has no physical energy.
 auto isolated=r;isolated.source.links.clear();isolated.multirate=false;
 auto dis=reduced44::Instrument::prepare(isolated,48000,&error);assert(dis);
 dis->noteOn(57,1.f);for(int i=0;i<2000;i++)dis->processOne();
 for(int j=1;j<dis->stats().regions;j++)assert(dis->regionEnergy(j)<1e-19);
 // Validate strict format and algorithm constraints.
 auto bad=r;bad.rateFactor[0]=3;assert(!reduced44::valid(bad));
 bad=r;bad.modesPerRegion=-1;assert(!reduced44::valid(bad));
 bad=r;bad.rateFactor.pop_back();assert(!reduced44::valid(bad));
 auto unchanged=decoded;assert(!reduced44::deserialize(blob+"oops",unchanged));assert(reduced44::serialize(unchanged)==blob);
 // Stress: add long string + distributed membrane from 4.3 benchmark.
 auto big=r;
 sparse43::Region string;string.family=sparse43::Family::String1D;string.density=.003;string.tension=65;string.damping=.003;
 for(int i=0;i<210;i++)string.vertices.push_back({.55*i/209.,.5,0});
 for(int i=1;i<210;i++)string.elements.push_back({i-1,i});
 int stringRegion=int(big.source.regions.size());big.source.regions.push_back(string);
 sparse43::Region membrane;membrane.family=sparse43::Family::Membrane2D;membrane.tension=120;membrane.density=.16;membrane.damping=.003;
 constexpr int side=13;
 for(int y=0;y<side;y++)for(int x=0;x<side;x++)membrane.vertices.push_back({.16*x/12.,.16*y/12.,.02});
 for(int y=0;y<12;y++)for(int x=0;x<12;x++){int a=x+side*y;membrane.elements.push_back({a,a+1,a+side+1});membrane.elements.push_back({a,a+side+1,a+side});}
 int membraneRegion=int(big.source.regions.size());big.source.regions.push_back(membrane);
 big.source.ports.push_back({stringRegion,sparse43::PortUnit::Metres,{{100,1.}}});
 big.source.ports.push_back({membraneRegion,sparse43::PortUnit::Metres,{{84,1.}}});
 big.source.links.push_back({0,int(big.source.ports.size())-2,sparse43::PortUnit::Metres,1,1,100,.002});
 big.source.links.push_back({int(big.source.ports.size())-2,int(big.source.ports.size())-1,sparse43::PortUnit::Metres,1,1,140,.003});
 big.rateFactor.resize(big.source.regions.size(),1);big.rateFactor[size_t(stringRegion)]=2;big.rateFactor[size_t(membraneRegion)]=4;
 big.source.relativeTolerance=1e-8;big.source.maxIterations=220;
 auto heavy=reduced44::Instrument::prepare(big,48000,&error);
 if(!heavy){std::cerr<<"large prepare "<<error<<'\n';return 3;}
 auto s=heavy->stats();std::cout<<"large full="<<s.fullDofs<<" reduced="<<s.reducedDofs<<" boundary="<<s.boundaryDofs<<" nnz="<<s.nnz<<'\n';
 assert(s.fullDofs>400&&s.reducedDofs<s.fullDofs/2);
 heavy->noteOn(57,.8f);for(int i=0;i<500;i++)assert(std::isfinite(heavy->processOne()));assert(heavy->stats().fails==0);
 // Universal patchbank: new kind, MIDI, full graph save/load and older 4.3 format.
 auto universal=universal23::reducedMultirateGraph();universal.reduced44=r;
 const auto graphText=universal23::serialize(universal);universal23::Graph recovered;
 assert(universal23::deserialize(graphText,recovered,&error));
 assert(universal23::serialize(recovered)==graphText);
 auto player=universal23::Instrument::prepare(recovered,48000,&error);
 if(!player){std::cerr<<"universal: "<<error<<'\n';return 4;}
 player->noteOn(57,.8f);double output=0;
 for(int i=0;i<1024;i++){auto v=player->processOne();assert(std::isfinite(v.left));output+=std::abs(v.left);}
 assert(output>1e-9);
 auto legacy=universal23::sparseEnhancedGraph();const auto legacyText=universal23::serialize(legacy);
 assert(universal23::deserialize(legacyText,recovered,&error));
 assert(universal23::serialize(recovered)==legacyText);
 std::cout<<"REDUCED44_PASS\n";
}
