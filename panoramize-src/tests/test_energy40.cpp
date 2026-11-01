#include "EnergyPorts40.h"
#include "UniversalInstrument23.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <vector>
using namespace panorama;
int main(){
 auto r=energy40::example();std::string err;
 assert(energy40::valid(r,&err));
 auto encoded=energy40::serialize(r);energy40::Recipe loaded;
 assert(energy40::deserialize(encoded,loaded,&err));assert(energy40::serialize(loaded)==encoded);
 // Block invalid parameter, type, topology, NaN, corrupted file.
 auto broken=loaded;broken.links[0].from=broken.links[0].to;assert(!energy40::valid(broken));
 broken=loaded;broken.nodes[0].mass=-1.;assert(!energy40::valid(broken));
 broken=loaded;broken.nodes[0].area=std::numeric_limits<double>::quiet_NaN();assert(!energy40::valid(broken));
 assert(!energy40::deserialize(encoded+"garbage\n",loaded,&err));
 // Purely passive network, prepare once and hold initial displacement via external forcing (active work).
 auto inst=energy40::Instrument::prepare(r,48000.,&err);assert(inst);
 inst->noteOn(57,0.9f);
 for(int i=0;i<48000;i++){float y=inst->processOne();assert(std::isfinite(y));}
 auto stats=inst->ledger();
 std::cout<<"energy budget: E="<<stats.finalEnergy<<" active="<<stats.active<<" dissipated="<<stats.dissipated<<" residual="<<stats.residual<<"\n";
 assert(std::abs(stats.residual)<1e-9+std::abs(stats.active)*1e-5);
 assert(stats.finalEnergy>=0&&stats.dissipated>=0);
 assert(stats.active>0);
 // Power-port disconnected control: zero energy transferred into unexcited node.
 auto disconnected=r;disconnected.links.clear();
 auto isolated=energy40::Instrument::prepare(disconnected,48000.);auto coupled=energy40::Instrument::prepare(r,48000.);
 isolated->noteOn(57,1.f);coupled->noteOn(57,1.f);
 for(int i=0;i<4800;i++){isolated->processOne();coupled->processOne();}
 assert(isolated->nodeEnergy(1)<1e-21);
 assert(coupled->nodeEnergy(1)>1e-18);
 // Numeric link update is bounded/smoothed; no state reset or heap allocation.
 auto mobile=energy40::Instrument::prepare(r,48000.);mobile->noteOn(57,1.f);
 for(int i=0;i<1000;i++)mobile->processOne();
 assert(!mobile->setLinkTarget(0,-1,0));assert(!mobile->setNodeTarget(0,0,1));
 assert(mobile->setLinkTarget(0,800,3.));assert(mobile->setNodeTarget(0,180,1.));
 for(int i=0;i<10000;i++){assert(std::isfinite(mobile->processOne()));}
 auto l=mobile->ledger();
 std::cout<<"smoothed: parameter work="<<l.parameter<<" residual="<<l.residual<<"\n";
 assert(std::abs(l.residual)<1e-8+std::abs(l.active)*1e-5+std::abs(l.parameter)*1e-5);
 // Pure lossless 2-domain oscillator should preserve energy, even for strong coupled edges.
 auto lossless=r;for(auto&n:lossless.nodes)n.damping=0;for(auto&e:lossless.links)e.damping=0;
 auto loss=energy40::Instrument::prepare(lossless,48000.);loss->noteOn(57,1.f);
 for(int i=0;i<12000;i++)loss->processOne();
 const auto z=loss->ledger();
 std::cout<<"lossless energy="<<z.finalEnergy<<" residual="<<z.residual<<"\n";
 assert(std::abs(z.residual)<1e-9+std::abs(z.active)*1e-5);
 // Embed new source in Universal Patchbay .pgraph and verify MIDI, render, reload.
 auto graph=universal23::energyGraph();const auto graphText=universal23::serialize(graph);
 universal23::Graph parsed;assert(universal23::deserialize(graphText,parsed,&err));
 assert(universal23::serialize(parsed)==graphText);
 auto player=universal23::Instrument::prepare(parsed,48000.,&err);assert(player);
 player->noteOn(57,.8f);double sum=0;
 for(int i=0;i<4000;i++){auto s=player->processOne();assert(std::isfinite(s.left)&&std::isfinite(s.right));sum+=std::abs(double(s.left));}
 // Compare live numeric target updates with topology edits (which must rebuild).
 auto dynamic=parsed;dynamic.energy40.nodes[0].frequency*=1.25;
 dynamic.energy40.links[0].damping*=2.;dynamic.energy40.nodes[0].x=.75;
 assert(player->tryUpdateEnergyGraph(dynamic));
 for(int i=0;i<1000;i++)assert(std::isfinite(player->processOne().left));
 dynamic.energy40.links[0].from=2;
 assert(!player->tryUpdateEnergyGraph(dynamic));
 std::cout<<"v4 source output absSum="<<sum<<"\n";
 assert(sum>1.e-6);
 std::cout<<"Panoramize 4.0 implicit energy ports PASS\n";
}
