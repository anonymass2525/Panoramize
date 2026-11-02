#include "NonlinearPassive45.h"
#include "UniversalInstrument23.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
using namespace panorama;
int main(){
 auto r=passive45::example();std::string why;
 assert(passive45::validate(r,&why));
 auto b=passive45::serialize(r);passive45::Recipe back;
 assert(passive45::deserialize(b,back,&why));assert(passive45::serialize(back)==b);
 auto inst=passive45::Instrument::prepare(back,48000,&why);
 if(!inst){std::cerr<<"PREPARE: "<<why<<'\n';return 1;}
 inst->noteOn(57,.7f);double sum=0;
 for(int i=0;i<2000;i++){float s=inst->processOne();assert(std::isfinite(s));sum+=std::abs(s);}
 auto led=inst->ledger();auto st=inst->stats();
 std::cout<<"n="<<st.dofs<<" frames="<<st.frames<<" failed="<<st.failures<<" abs="<<sum
          <<" energy="<<led.finalEnergy<<" active="<<led.active<<" dissipated="<<led.dissipation<<" residual="<<led.residual<<" lastError="<<st.lastError<<" contacts="<<st.contactsEngaged<<'\n';
 assert(st.frames==2000 && st.failures==0 && sum>1e-8);
 assert(st.contactsEngaged>0);
 const auto moving=inst->deformedVertex(2,1);
 const auto&reference=r.spatial.regions[2].vertices[1];
 assert(std::isfinite(moving[0])&&std::isfinite(moving[1])&&std::isfinite(moving[2]));
 const double motion=std::abs(moving[0]-reference.x)+std::abs(moving[1]-reference.y)+std::abs(moving[2]-reference.z);
 assert(motion>1e-15);
 assert(inst->contactPotential()>=0&&inst->stretchPotential()>=0&&inst->modalPotential()>=0);
 // Contact and geometric nonlinear effects must be audible, not just metadata.
 auto withoutContact=r;withoutContact.contacts.clear();withoutContact.stretchEdges.clear();
 auto dry=passive45::Instrument::prepare(withoutContact,48000,&why);assert(dry);
 auto wet=passive45::Instrument::prepare(r,48000,&why);assert(wet);
 dry->noteOn(57,.7f);wet->noteOn(57,.7f);double difference=0;
 for(int i=0;i<1300;i++){double a=dry->processOne(),b=wet->processOne();difference+=(a-b)*(a-b);}
 assert(difference>1e-12);assert(dry->stats().failures==0&&wet->stats().failures==0);
 std::cout<<"contact+geometric A/B difference="<<difference<<" potential="<<wet->contactPotential()<<"/"<<wet->stretchPotential()<<'\n';
 assert(std::abs(led.residual)<1e-9+std::abs(led.active)*1e-4);
 auto invalid=r;invalid.contacts[0].friction=-1.;assert(!passive45::validate(invalid));
 invalid=r;invalid.stretchEdges[0].vertexB=999;assert(!passive45::validate(invalid));
 invalid=r;invalid.native.modes[0].excitation=std::numeric_limits<double>::quiet_NaN();assert(!passive45::validate(invalid));
 invalid=r;invalid.strictPassive=true;assert(!passive45::validate(invalid));
 for(auto&m:invalid.native.modes)m.excitation=0.;assert(passive45::validate(invalid));
 auto p=passive45::Instrument::prepare(invalid,48000,&why);if(!p){std::cerr<<why<<'\n';return 2;}
 p->noteOn(57,.7f);for(int i=0;i<2000;i++)p->processOne();
 assert(p->stats().activeModes==0&&p->stats().failures==0);
 auto l=p->ledger();assert(std::abs(l.residual)<1e-9+std::abs(l.active)*1e-4);
 // New instrument source round-trip through existing Universal Patchbay.
 auto source=universal23::nonlinearPassiveGraph();source.passive45=r;
 auto graphText=universal23::serialize(source);
 universal23::Graph loaded;
 assert(universal23::deserialize(graphText,loaded,&why));
 assert(universal23::serialize(loaded)==graphText);
 auto host=universal23::Instrument::prepare(loaded,48000,&why);
 if(!host){std::cerr<<"HOST: "<<why<<'\n';return 4;}
 host->noteOn(57,.7f);double hostAbs=0;
 for(int i=0;i<320;i++){auto x=host->processOne();assert(std::isfinite(x.left));hostAbs+=std::abs(double(x.left));}
 assert(hostAbs>1e-6);
 // A v4.4 preset remains loadable and renders.
 auto legacy=universal23::reducedMultirateGraph();
 auto old=universal23::serialize(legacy);
 assert(universal23::deserialize(old,loaded,&why));
 assert(universal23::serialize(loaded)==old);
 std::cout<<"PASSIVE45_OK host_abs="<<hostAbs<<"\n";
}
