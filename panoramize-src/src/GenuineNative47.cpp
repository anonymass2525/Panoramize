#include "GenuineNative47.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
namespace panorama::genuine47 {
namespace {
constexpr double pi=3.14159265358979323846;
bool limit(double x,double low,double high){return std::isfinite(x)&&x>=low&&x<=high;}
}
Recipe example(){
 Recipe r;
 // The exact 3.9 engine and its original native27 graph remain unchanged.
 r.routes={{Source::Wheel,Target::Cutoff,.45},{Source::Pressure,Target::Resonance,.25},
           {Source::Macro1,Target::Drive,.6},{Source::Lfo,Target::Cutoff,.22}};
 return r;
}
bool valid(const Recipe&r,std::string*reason){
 auto fail=[&](const char*m){if(reason)*reason=m;return false;};
 if(!limit(r.tone.cutoff,20,20000)||!limit(r.tone.resonance,.08,2)||
    !limit(r.tone.drive,.01,20)||!limit(r.tone.output,0,4)||!limit(r.tone.wet,0,1)||
    !limit(r.tone.lfoHz,0,30)||r.routes.size()>16)return fail("invalid tone control");
 for(double x:r.macros)if(!limit(x,0,1))return fail("invalid macro");
 for(auto&m:r.original.acoustic.modes){
  // Native27 implements powered VA/FM, and passive normalized modes for other
  // types. Positive excitation on others would be silently ignored by 2.7.
  if(m.engine!=native27::Engine::VA&&m.engine!=native27::Engine::FM&&m.excitation>0)
   return fail("unsupported active excitation on non-oscillator native mode");
 }
 for(auto&a:r.routes)if(int(a.source)<0||int(a.source)>8||int(a.target)<0||int(a.target)>5||!limit(a.depth,-4,4))return fail("invalid modulation route");
 // Verify nested graph with original proven preparers in Instrument::prepare.
 if(reason)reason->clear();return true;
}
std::string serialize(const Recipe&r){
 std::ostringstream o;o<<std::setprecision(17)<<"PANORAMIZE-GENUINE47|1\n";
 o<<"TONE "<<r.tone.cutoff<<' '<<r.tone.resonance<<' '<<r.tone.drive<<' '<<r.tone.output<<' '<<r.tone.wet<<' '<<r.tone.lfoHz<<'\n';
 o<<"MACROS";for(double x:r.macros)o<<' '<<x;o<<'\n';
 o<<"ROUTES "<<r.routes.size()<<'\n';
 for(auto&m:r.routes)o<<"R "<<int(m.source)<<' '<<int(m.target)<<' '<<m.depth<<'\n';
 o<<"ORIGINAL39_BEGIN\n"<<unified39::serialize(r.original)<<"ORIGINAL39_END\n";
 return o.str();
}
bool deserialize(const std::string&data,Recipe&out,std::string*reason){
 auto fail=[&](const char*m){if(reason)*reason=m;return false;};
 if(data.size()>72000)return fail("patch too large");
 std::istringstream in(data);std::string line,tag;Recipe candidate;int n=-1;
 if(!std::getline(in,line)||line!="PANORAMIZE-GENUINE47|1")return fail("invalid patch version");
 if(!std::getline(in,line))return fail("missing tone");
 {std::istringstream s(line);std::string extra;
  if(!(s>>tag>>candidate.tone.cutoff>>candidate.tone.resonance>>candidate.tone.drive>>candidate.tone.output>>candidate.tone.wet>>candidate.tone.lfoHz)||tag!="TONE"||(s>>extra))return fail("bad tone");}
 if(!std::getline(in,line))return fail("missing macros");
 {std::istringstream s(line);std::string extra;
  if(!(s>>tag>>candidate.macros[0]>>candidate.macros[1]>>candidate.macros[2]>>candidate.macros[3])||tag!="MACROS"||(s>>extra))return fail("bad macros");}
 if(!std::getline(in,line))return fail("missing routes");
 {std::istringstream s(line);std::string extra;if(!(s>>tag>>n)||tag!="ROUTES"||n<0||n>16||(s>>extra))return fail("bad routes header");}
 for(int i=0;i<n;i++){
  if(!std::getline(in,line))return fail("missing route");std::istringstream s(line);std::string extra;int a=-1,b=-1;double depth;
  if(!(s>>tag>>a>>b>>depth)||tag!="R"||(s>>extra))return fail("bad route");
  candidate.routes.push_back({Source(a),Target(b),depth});
 }
 if(!std::getline(in,line)||line!="ORIGINAL39_BEGIN")return fail("missing native39 begin");
 std::string sub;bool close=false;
 while(std::getline(in,line)){
  if(line=="ORIGINAL39_END"){close=true;break;}
  sub+=line+'\n';if(sub.size()>65000)return fail("oversized original graph");
 }
 if(!close||std::getline(in,line))return fail("unterminated native39 graph");
 if(!unified39::deserialize(sub,candidate.original,reason))return false;
 if(!valid(candidate,reason))return false;
 out=std::move(candidate);return true;
}
std::unique_ptr<Instrument> Instrument::prepare(const Recipe&r,int rate,std::string*why){
 if(!valid(r,why)||rate<22050||rate>192000){if(why&&why->empty())*why="invalid sample rate";return {};}
 auto audio=unified39::Instrument::prepare(r.original,rate,why);if(!audio)return {};
 auto result=std::unique_ptr<Instrument>(new Instrument());result->original_=std::move(audio);
 result->recipe_=r;result->sr_=rate;
 for(size_t i=0;i<4;i++)result->macros_[i].store(r.macros[i],std::memory_order_relaxed);
 const double toneDefaults[5]={r.tone.cutoff,r.tone.resonance,r.tone.drive,r.tone.output,r.tone.wet};
 for(int i=0;i<5;i++)result->toneTargets_[size_t(i)].store(toneDefaults[i],std::memory_order_relaxed);
 result->smoothedWet_=r.tone.wet;
 result->filteredCutoff_=r.tone.cutoff;result->filteredQ_=r.tone.resonance;
 result->filteredDrive_=r.tone.drive;result->filteredOutput_=r.tone.output;
 result->smoothedPhysical_=r.original.physicalMix;result->smoothedAcoustic_=r.original.acousticMix;
 if(why)why->clear();return result;
}
void Instrument::noteOn(int midi,float velocity) noexcept{
 if(midi<0||midi>127||!std::isfinite(velocity)||velocity<=0)return;
 velocity_=std::clamp(double(velocity),0.,1.);key_=double(midi)/127.;original_->noteOn(midi,velocity);
}
void Instrument::noteOff(int midi)noexcept{original_->noteOff(midi);}
void Instrument::allNotesOff()noexcept{original_->allNotesOff();velocity_=0;}
void Instrument::pitchBend(float semitones)noexcept{original_->pitchBend(semitones);}
void Instrument::modWheel(float value)noexcept{
 if(!std::isfinite(value))return;wheel_=std::clamp(double(value),0.,1.);original_->modWheel(value);
}
void Instrument::channelPressure(float value)noexcept{
 if(std::isfinite(value))pressure_=std::clamp(double(value),0.,1.);
}
void Instrument::setMacro(int index,float value) noexcept{
 if(index>=0&&index<4&&std::isfinite(value))macros_[size_t(index)].store(std::clamp(double(value),0.,1.),std::memory_order_relaxed);
}
bool Instrument::setHostParameterNormalized(int index,double value) noexcept{
 if(index<0||index>=9||!std::isfinite(value)||value<0||value>1)return false;
 if(index>=5){setMacro(index-5,float(value));return true;}
 double out=value;
 if(index==0)out=20.*std::pow(1000.,value);
 else if(index==1)out=.08+value*1.92;
 else if(index==2)out=.01*std::pow(2000.,value);
 else if(index==3)out=4.*value;
 toneTargets_[size_t(index)].store(out,std::memory_order_relaxed);
 useHostTone_.store(true,std::memory_order_relaxed);return true;
}
void Instrument::clearHostAutomation() noexcept{
 if(useHostTone_.exchange(false,std::memory_order_relaxed)){
  for(size_t i=0;i<macros_.size();i++)macros_[i].store(recipe_.macros[i],std::memory_order_relaxed);
 }
}
bool Instrument::setNativeModeNormalized(int mode,bool excitation,double normalized) noexcept{
 return original_->setNativeModeNormalized(mode,excitation,normalized);
}
void Instrument::clearNativeModeAutomation() noexcept{original_->clearNativeModeAutomation();}
float Instrument::processOne()noexcept{
 // Entire genuine Native27 and mechanical system are processed unchanged;
 // PCM filtering is a readout, never fed back as if it were passive effort.
 double targets[6]={recipe_.tone.cutoff,recipe_.tone.resonance,recipe_.tone.drive,recipe_.tone.output,
                    recipe_.original.physicalMix,recipe_.original.acousticMix};
 double wetTarget=recipe_.tone.wet;
 if(useHostTone_.load(std::memory_order_relaxed)){
  for(int i=0;i<4;i++)targets[i]=toneTargets_[size_t(i)].load(std::memory_order_relaxed);
  wetTarget=toneTargets_[4].load(std::memory_order_relaxed);
 }
 lfoPhase_+=2*pi*recipe_.tone.lfoHz/sr_;if(lfoPhase_>=2*pi)lfoPhase_-=2*pi;
 const double sources[9]={macros_[0].load(std::memory_order_relaxed)-.5,
                           macros_[1].load(std::memory_order_relaxed)-.5,
                           macros_[2].load(std::memory_order_relaxed)-.5,
                           macros_[3].load(std::memory_order_relaxed)-.5,
                           wheel_-.5,pressure_-.5,velocity_-.5,key_-.5,std::sin(lfoPhase_)};
 for(const auto&r:recipe_.routes){
  const double x=sources[int(r.source)]*r.depth;
  if(r.target==Target::Cutoff)targets[0]*=std::pow(2.,3.*x);
  else if(r.target==Target::Resonance)targets[1]+=x*.7;
  else if(r.target==Target::Drive)targets[2]*=std::pow(2.,2.*x);
  else if(r.target==Target::Output)targets[3]*=std::pow(2.,x);
  else if(r.target==Target::PhysicalMix)targets[4]+=x*.25;
  else if(r.target==Target::AcousticMix)targets[5]+=x*.25;
 }
 targets[0]=std::clamp(targets[0],20.,.44*sr_);
 targets[1]=std::clamp(targets[1],.08,2.);
 targets[2]=std::clamp(targets[2],.01,20.);
 targets[3]=std::clamp(targets[3],0.,4.);
 targets[4]=std::clamp(targets[4],0.,1.);
 targets[5]=std::clamp(targets[5],0.,1.);
 const double smooth=1.-std::exp(-1./(sr_*.015));
 filteredCutoff_+=smooth*(targets[0]-filteredCutoff_);
 filteredQ_+=smooth*(targets[1]-filteredQ_);
 filteredDrive_+=smooth*(targets[2]-filteredDrive_);
 filteredOutput_+=smooth*(targets[3]-filteredOutput_);
 smoothedPhysical_+=smooth*(targets[4]-smoothedPhysical_);
 smoothedAcoustic_+=smooth*(targets[5]-smoothedAcoustic_);
 smoothedWet_+=smooth*(wetTarget-smoothedWet_);
 // Stereo-independent TPT state-variable filter, 2-pole LP response.
 const double g=std::tan(pi*std::min(filteredCutoff_,.44*sr_)/sr_);
 const double k=1./filteredQ_;
 original_->setOutputMix(smoothedPhysical_,smoothedAcoustic_);
 const double dry=original_->processOne();
 if(smoothedWet_==0.&&filteredOutput_==1.&&recipe_.routes.empty()) {frames_++;peak_=std::max(peak_,std::abs(dry));return float(dry);}
 const double input=dry*filteredDrive_;
 const double hp=(input-(k+g)*band_-low_)/(1.+g*(k+g));
 const double bp=g*hp+band_;
 const double lp=g*bp+low_;
 band_=g*hp+bp;low_=g*bp+lp;
 // Soft-saturation is a powered/PCM post-processor; not a passive port.
 const double signal=(1.-smoothedWet_)*dry+smoothedWet_*lp;
 const double out=filteredOutput_*std::tanh(signal);
 const float result=std::isfinite(out)?float(std::clamp(out,-1.,1.)):0.f;
 peak_=std::max(peak_,double(std::abs(result)));frames_++;
 return result;
}
Meter Instrument::meter()const noexcept{
 return {original_->acousticActiveWork(),original_->physicalExternalWork(),original_->energyResidual(),peak_,frames_};
}
}
