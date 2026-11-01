#include "JointInverse50.h"
#include "RecordedValidation51.h"
#include "InverseDecision54.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <map>
#include <sstream>
namespace fs=std::filesystem;
using namespace panorama;
namespace {
uint32_t u32(const unsigned char*b){return uint32_t(b[0])|uint32_t(b[1])<<8|uint32_t(b[2])<<16|uint32_t(b[3])<<24;}
uint16_t u16(const unsigned char*b){return uint16_t(b[0]|(b[1]<<8));}
void write32(std::ostream&f,uint32_t x){for(int j=0;j<4;j++)f.put(char((x>>(j*8))&255));}
void write16(std::ostream&f,uint16_t x){for(int j=0;j<2;j++)f.put(char((x>>(j*8))&255));}
bool wavWrite(const fs::path&f,const std::vector<float>&a,int sr){
 std::ofstream o(f,std::ios::binary);if(!o||a.size()>16000000)return false;
 o.write("RIFF",4);write32(o,uint32_t(36+4*a.size()));o.write("WAVEfmt ",8);
 write32(o,16);write16(o,3);write16(o,1);write32(o,sr);write32(o,sr*4);write16(o,4);write16(o,32);
 o.write("data",4);write32(o,uint32_t(4*a.size()));for(float x:a)o.write(reinterpret_cast<const char*>(&x),4);return bool(o);
}
bool wavRead(const fs::path&file,std::vector<float>&a,int&sr){
 std::ifstream f(file,std::ios::binary);char h[12];if(!f.read(h,12)||std::string(h,4)!="RIFF"||std::string(h+8,4)!="WAVE")return false;
 int codec=0,bits=0,ch=0;sr=0;std::vector<unsigned char>data;
 while(f){unsigned char hdr[8];if(!f.read((char*)hdr,8))break;
  unsigned n=u32(hdr+4);if(n>32000000)return false;std::string tag((char*)hdr,4);
  if(tag=="fmt "){if(n<16||n>512)return false;std::vector<unsigned char>v(n);if(!f.read((char*)v.data(),n))return false;
   codec=u16(v.data());ch=u16(v.data()+2);sr=int(u32(v.data()+4));bits=u16(v.data()+14);
  }else if(tag=="data"){data.resize(n);if(!f.read((char*)data.data(),n))return false;}
  else f.seekg(n,std::ios::cur);if(n%2)f.seekg(1,std::ios::cur);
 }
 if(sr<22050||sr>96000||ch<1||ch>2||!((codec==1&&(bits==16||bits==24))||(codec==3&&bits==32)))return false;
 size_t bytes=size_t(bits)/8,frame=bytes*size_t(ch);if(!frame||data.size()%frame)return false;
 a.resize(data.size()/frame);if(a.size()>size_t(sr)*10)return false;
 for(size_t i=0;i<a.size();i++){
  double sum=0;for(int c=0;c<ch;c++){
   const auto*p=data.data()+i*frame+size_t(c)*bytes;double x=0;
   if(codec==3){float t;std::memcpy(&t,p,4);x=t;}
   else if(bits==16)x=int16_t(u16(p))/32768.;else {int n=int(p[0]|p[1]<<8|p[2]<<16);if(n&0x800000)n|=~0xffffff;x=n/8388608.;}
   sum+=x;
  }a[i]=float(sum/ch);if(!std::isfinite(a[i]))return false;
 }return true;
}
int evaluate(const std::vector<joint50::Take>&takes,int sr,const fs::path&out){
 fs::create_directories(out);
 auto result=joint50::identify(takes,sr,18);
 std::ofstream(out/"recording_quality_audit.csv")<<recorded51::auditCSV(takes,sr);
 if(!result.error.empty()){std::cerr<<result.error<<"\n";return 3;}
 const auto decision=decision54::review(result);
 std::ofstream(out/"candidate_uncertainty54.csv")<<decision54::candidatesCSV(decision);
 std::ofstream(out/"next_measurements54.csv")<<decision54::measurementsCSV(decision54::suggestedMeasurements());
 std::ofstream(out/"decision54.txt")<<decision54::reportText(decision);
 std::ofstream rank(out/"ranked_structural_hypotheses.csv");rank<<"rank,family,train_descriptor_error,holdout_descriptor_error,evaluations,patch_file\n";
 for(size_t i=0;i<result.ranked.size();i++){
  const auto&c=result.ranked[i];const auto name="hypothesis_"+std::to_string(i+1);
  std::ofstream(out/(name+".pgraph"))<<universal23::serialize(c.editable);
  std::ofstream(out/(name+"_controls.csv"))<<controls50::catalogCSV(c.editable);
  auto audio=design49::renderGraph(c.editable,takes[0].midi,sr,.26);
  if(!audio.empty())wavWrite(out/(name+"_preview.wav"),audio,sr);
  rank<<i+1<<','<<c.family<<','<<c.trainError<<','<<c.holdoutError<<','<<c.evaluations<<','<<name<<".pgraph\n";
 }
 std::ofstream note(out/"IDENTIFIABILITY_AND_LIMITS.txt");note<<result.warning<<"\n";
 note<<"Train and heldout scores use annotated MIDI notes, and 10 normalized harmonic amplitudes plus 5 envelope values.\n";
 note<<"No claim of full unknown FEM/unknown material/vocal tract recovery or globally unified audio-thread automation.\n";
 note<<"Source stems are independent WAV examples, controls are editable in 5.0 via a shared property editor.\n";
 note<<"Recording provenance is NOT verified by this executable. Real acoustic test results require documented microphone, instrument, pitch and held-out sessions.\n";
 note<<"A full rank on an identifiable parameter subset would not imply unique 3D geometry, material or articulatory tract identification.\n";
 std::cout<<"candidates="<<result.ranked.size()<<" evaluations="<<result.totalEvaluations<<" top="<<result.ranked[0].family<<" uncertain="<<!result.identifiable<<"\n";
 return 0;
}
}
int main(int argc,char**argv){
 if(argc==3&&std::string(argv[1])=="--demo"){
  const fs::path out=argv[2];fs::create_directories(out);
  auto graph=design49::editableGraph(design49::Family::PhysicalString,220.,.28);
  const int sr=24000;std::vector<joint50::Take>takes;
  for(int i=0;i<3;i++){
   int midi=57+i*3;auto a=design49::renderGraph(graph,midi,sr,.28);if(a.empty())return 4;
   // deterministic recording nuisances: 2-frame slip, gain and noise
   for(size_t j=a.size();j-->2;)a[j]=a[j-2];a[0]=a[1]=0;
   for(size_t j=0;j<a.size();j++)a[j]=float(a[j]*(.65+.13*i)+.00003*std::sin(.137*j));
   wavWrite(out/("annotated_take_"+std::to_string(i)+".wav"),a,sr);
   takes.push_back({std::move(a),midi,i<2});
  }
  return evaluate(takes,sr,out);
 }
 if(argc==4&&std::string(argv[1])=="--infer-provenance"){
  // TSV: absolute WAV path, MIDI, train(0/1), instrument ID, session ID,
  // URL or DOI, license. Enforces instrument-level consistency and disjoint
  // session IDs across train/test to prevent same-session data leakage.
  std::ifstream in(argv[2]);if(!in)return 2;
  std::vector<joint50::Take> takes;std::vector<std::string> rows;
  std::set<std::string> names,trainSessions,holdSessions;
  std::string instrument,source,license,line;int sr=0;
  while(std::getline(in,line)){
   if(line.empty()||line[0]=='#')continue;
   std::istringstream ss(line);std::string path,inst,session,url,lic,midiS,trainS;
   if(!std::getline(ss,path,'\t')||!std::getline(ss,midiS,'\t')||
      !std::getline(ss,trainS,'\t')||!std::getline(ss,inst,'\t')||
      !std::getline(ss,session,'\t')||!std::getline(ss,url,'\t')||
      !std::getline(ss,lic,'\t')||path.empty()||inst.empty()||session.empty()||
      url.rfind("https://",0)!=0||lic.empty()||!names.insert(path).second)return 2;
   int midi=-1,training=-1;
   try{midi=std::stoi(midiS);training=std::stoi(trainS);}catch(...){return 2;}
   if(midi<36||midi>96||(training!=0&&training!=1))return 2;
   if(!instrument.empty()&&instrument!=inst){std::cerr<<"Mixed instruments not supported in this single-instrument benchmark\n";return 2;}
   instrument=inst;
   if(training)trainSessions.insert(session);else holdSessions.insert(session);
   int rate=0;std::vector<float> audio;
   if(!wavRead(path,audio,rate))return 2;
   if(sr&&sr!=rate)return 2;sr=rate;
   takes.push_back({std::move(audio),midi,bool(training)});
   rows.push_back(line);
  }
  for(const auto&s:trainSessions)if(holdSessions.count(s)){
   std::cerr<<"DATA LEAKAGE: session appears in both train and held-out set: "<<s<<"\n";return 2;
  }
  if(trainSessions.empty()||holdSessions.empty()||takes.size()<2||takes.size()>12)return 2;
  const fs::path out=argv[3];fs::create_directories(out);
  std::ofstream ledger(out/"external_provenance.tsv");
  ledger<<"wav\tmidi\ttraining\tinstrument\tsession\tsource_url\tlicense\n";
  for(const auto&r:rows)ledger<<r<<"\n";
  ledger.close();
  return evaluate(takes,sr,out);
 }
 if(argc==4&&std::string(argv[1])=="--infer-manifest"){
  std::ifstream in(argv[2]);if(!in){std::cerr<<"Cannot open manifest\n";return 2;}
  std::vector<joint50::Take>takes;int sr=0;std::string line;
  while(std::getline(in,line)){
   if(line.empty()||line[0]=='#')continue;
   std::istringstream ss(line);std::string path;int midi,training;
   if(!(ss>>path>>midi>>training)||training<0||training>1)return 2;
   std::vector<float>a;int rate=0;if(!wavRead(path,a,rate))return 2;
   if(sr&&sr!=rate){std::cerr<<"Sample rates must match\n";return 2;}sr=rate;
   takes.push_back({std::move(a),midi,bool(training)});
  }
  return evaluate(takes,sr,argv[3]);
 }
 std::cerr<<"Usage: panoramize_joint50 --demo OUTDIR | --infer-manifest TAKES.txt OUTDIR\n"
          <<"TAKES.txt: WAV_PATH MIDI_NOTE TRAINING(1=yes,0=heldout). Use path without spaces.\n"
          <<"or --infer-provenance TAKES.tsv OUTDIR: WAV\tMIDI\tTRAIN\tINSTRUMENT\tSESSION\tHTTPS_SOURCE\tLICENSE\n";
 return 2;
}
