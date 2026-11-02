#pragma once
// Panoramize 4.0 message-thread graph editor. Draggable physical/acoustic nodes,
// two-click link creation, editable topology and per-node/link numeric parameters.
// The callback installs a *prepared* immutable DSP graph via PluginProcessor;
// no heap allocations are performed by EnergyPorts40::processOne().
#include <JuceHeader.h>
#include "EnergyPorts40.h"
#include <algorithm>
#include <cmath>
#include <functional>

class EnergyGraphEditor40 final : public juce::Component {
public:
 using Recipe=panorama::energy40::Recipe;
 using Domain=panorama::energy40::Domain;
 using Link=panorama::energy40::Link;
 using Node=panorama::energy40::Node;
 std::function<bool(const Recipe&)> onGraphChanged;
 EnergyGraphEditor40(){
  setOpaque(true);
  for(auto* b:{&addNode_,&removeNode_,&connect_,&removeLink_})addAndMakeVisible(*b);
  addAndMakeVisible(domain_);addAndMakeVisible(linkKind_);addAndMakeVisible(linkPicker_);
  for(auto* s:{&frequency_,&mass_,&nodeDamping_,&drive_,&area_,&output_,&linkStiffness_,&linkDamping_}){
   addAndMakeVisible(*s);s->setSliderStyle(juce::Slider::LinearHorizontal);
   s->setTextBoxStyle(juce::Slider::TextBoxRight,false,92,23);
  }
  frequency_.setRange(10,2500,.1);mass_.setRange(.00001,1e7,0);nodeDamping_.setRange(0,1e6,0);
  drive_.setRange(0,500,0);area_.setRange(.0001,10.,0);output_.setRange(-1000.,1000.,0);
  mass_.setSkewFactorFromMidPoint(.2);nodeDamping_.setSkewFactorFromMidPoint(1.);linkDamping_.setSkewFactorFromMidPoint(.1);
  linkStiffness_.setRange(0,1e6,0);linkDamping_.setRange(0,1e5,0);
  addAndMakeVisible(status_);status_.setColour(juce::Label::textColourId,juce::Colour(0xff65e4cf));
  domain_.addItem("MECHANICAL / force-velocity",1);domain_.addItem("ACOUSTIC / pressure-flow",2);
  linkKind_.addItem("SPRING / CONSERVATIVE",1);linkKind_.addItem("DAMPER / DISSIPATIVE",2);linkKind_.addItem("SPRING + DAMPER",3);
  addNode_.onClick=[this]{
   if(recipe_.nodes.size()>=12){show("Maximum 12 domains");return;}
   Node n;n.x=.15+(.115*double(recipe_.nodes.size()) );n.y=(recipe_.nodes.size()%2)? .66:.35;
   recipe_.nodes.push_back(n);selectedNode_=int(recipe_.nodes.size())-1;refreshControls();commit();
  };
  removeNode_.onClick=[this]{
   if(selectedNode_<0||recipe_.nodes.size()<=1)return;
   const int i=selectedNode_;
   auto&links=recipe_.links;
   links.erase(std::remove_if(links.begin(),links.end(),[i](const Link&l){return l.from==i||l.to==i;}),links.end());
   for(auto&l:links){if(l.from>i)--l.from;if(l.to>i)--l.to;}
   recipe_.nodes.erase(recipe_.nodes.begin()+i);
   if(recipe_.exciteNode==i)recipe_.exciteNode=0;else if(recipe_.exciteNode>i)--recipe_.exciteNode;
   selectedNode_=std::min(i,int(recipe_.nodes.size())-1);selectedLink_=-1;refreshControls();commit();
  };
  connect_.onClick=[this]{
   if(recipe_.links.size()>=24){show("Maximum 24 links");return;}
   startLink_=selectedNode_>=0?selectedNode_:0;
   show("Connect: click another domain in the canvas");repaint();
  };
  removeLink_.onClick=[this]{
   if(selectedLink_<0||selectedLink_>=int(recipe_.links.size()))return;
   recipe_.links.erase(recipe_.links.begin()+selectedLink_);selectedLink_=-1;refreshControls();commit();
  };
  domain_.onChange=[this]{
   if(refresh_||selectedNode_<0)return;
   auto&n=recipe_.nodes[size_t(selectedNode_)];
   const auto old=n.domain;n.domain=domain_.getSelectedId()==2?Domain::Acoustic:Domain::Mechanical;
   if(n.domain!=old){
    if(n.domain==Domain::Acoustic){n.mass=10000.;n.damping=2000.;n.area=.04;n.output=20.;}
    else {n.mass=.025;n.damping=.05;n.area=.02;n.output=20.;}
   }
   refreshControls();commit();
  };
  linkPicker_.onChange=[this]{if(refresh_)return;selectedLink_=linkPicker_.getSelectedId()-2;refreshControls();repaint();};
  linkKind_.onChange=[this]{if(refresh_||selectedLink_<0)return;recipe_.links[size_t(selectedLink_)].kind=panorama::energy40::LinkKind(linkKind_.getSelectedId()-1);commit();};
  auto attach=[this](juce::Slider&s){
   s.onDragEnd=[this]{applyNumbers();};
   s.onValueChange=[this,&s]{if(!s.isMouseButtonDown())applyNumbers();};
  };
  for(auto* s:{&frequency_,&mass_,&nodeDamping_,&drive_,&area_,&output_,&linkStiffness_,&linkDamping_})attach(*s);
  setRecipe(panorama::energy40::example());
 }
 void setRecipe(const Recipe&r){recipe_=r;selectedNode_=recipe_.nodes.empty()?-1:0;selectedLink_=recipe_.links.empty()?-1:0;startLink_=-1;refreshControls();repaint();}
 void paint(juce::Graphics&g) override{
  g.fillAll(juce::Colour(0xff07191e));
  const auto canvas=canvasBounds();
  g.setColour(juce::Colour(0xff102e32));g.fillRoundedRectangle(canvas.toFloat(),10.f);
  g.setColour(juce::Colour(0xff387f73));g.drawRoundedRectangle(canvas.toFloat(),10.f,1.f);
   g.setColour(juce::Colour(0xff35d7bd).withAlpha(.35f));
   g.drawLine(float(canvas.getX()+9),float(canvas.getY()+2),float(canvas.getRight()-9),float(canvas.getY()+2),1.3f);
  for(size_t i=0;i<recipe_.links.size();i++){
   const auto&l=recipe_.links[i];if(l.from<0||l.to<0||l.from>=int(recipe_.nodes.size())||l.to>=int(recipe_.nodes.size()))continue;
   const auto a=location(l.from),b=location(l.to);
   g.setColour(int(i)==selectedLink_?juce::Colour(0xfff1c079):juce::Colour(0xff39cbb8));
   g.drawLine(a.x,a.y,b.x,b.y,int(i)==selectedLink_?4.f:2.f);
   auto mid=(a+b)*.5f;g.setFont(11.f);g.drawText(l.kind==panorama::energy40::LinkKind::Conservative?"K":l.kind==panorama::energy40::LinkKind::Dissipative?"D":"K+D",int(mid.x)-23,int(mid.y)-10,46,20,juce::Justification::centred);
  }
  for(size_t i=0;i<recipe_.nodes.size();i++){
   const auto c=location(int(i));
   const auto col=recipe_.nodes[i].domain==Domain::Mechanical?juce::Colour(0xff18665d):juce::Colour(0xff645441);
   g.setColour(col);g.fillEllipse(c.x-28,c.y-28,56.f,56.f);
   g.setColour(int(i)==selectedNode_?juce::Colour(0xfff1c079):juce::Colour(0xff65e4cf));
   g.drawEllipse(c.x-28,c.y-28,56.f,56.f,selectedNode_==int(i)?3.f:1.f);
   g.setFont(12.f);g.drawFittedText(juce::String(int(i)+1),int(c.x)-17,int(c.y)-13,34,26,juce::Justification::centred,1);
   if(int(i)==recipe_.exciteNode){g.setColour(juce::Colour(0xffffdc94));g.drawText("E",int(c.x)+17,int(c.y)-34,20,20,juce::Justification::centred);}
   g.setFont(10.f);g.drawText(recipe_.nodes[i].domain==Domain::Mechanical?"N · m/s":"Pa · m3/s",int(c.x)-46,int(c.y)+32,92,16,juce::Justification::centred);
  }
  g.setColour(juce::Colour(0xff65e4cf));g.setFont(12.f);
  g.drawText("DRAG NODES / SELECT LINKS / CONNECT: TWO CLICKS / DOUBLE-CLICK: EXCITER",canvas.getX()+12,canvas.getY()+8,canvas.getWidth()-24,20,juce::Justification::centredLeft);
  g.drawText("NODE: frequency | mass/inertance | damping | active drive | area | pickup gain",14,getHeight()-300,getWidth()-18,22,juce::Justification::centredLeft);
  g.drawText("LINK: spring stiffness (N/m) | damper (N*s/m)",14,getHeight()-167,getWidth()-18,22,juce::Justification::centredLeft);
 }
 void mouseDown(const juce::MouseEvent&e)override{
  if(!canvasBounds().contains(e.getPosition()))return;
  const auto pos=e.position;int candidate=-1;float closest=35.f*35.f;
  for(int i=0;i<int(recipe_.nodes.size());i++){
   float d=location(i).getDistanceSquaredFrom(pos);
   if(d<closest){closest=d;candidate=i;}
  }
  if(startLink_>=0&&candidate>=0){
   if(candidate!=startLink_&&recipe_.links.size()<24){Link l;l.from=startLink_;l.to=candidate;recipe_.links.push_back(l);selectedLink_=int(recipe_.links.size())-1;refreshControls();commit();}
   startLink_=-1;return;
  }
  if(candidate>=0){selectedNode_=candidate;selectedLink_=-1;dragging_=true;refreshControls();repaint();return;}
  // Pick nearest edge by point-to-segment distance.
  float best=150;int hit=-1;
  for(int i=0;i<int(recipe_.links.size());i++){
   const auto&l=recipe_.links[size_t(i)];auto a=location(l.from),b=location(l.to);
   const auto ab=b-a;const float denom=ab.getDistanceSquaredFromOrigin();
   const float t=denom>0?juce::jlimit(0.f,1.f,(pos-a).getDotProduct(ab)/denom):0.f;
   const float d=(pos-(a+ab*t)).getDistanceSquaredFromOrigin();
   if(d<best){best=d;hit=i;}
  }
  if(hit>=0){selectedLink_=hit;refreshControls();repaint();}
 }
 void mouseDoubleClick(const juce::MouseEvent&e)override{
  if(!canvasBounds().contains(e.getPosition()))return;
  for(int i=0;i<int(recipe_.nodes.size());i++)if(location(i).getDistanceSquaredFrom(e.position)<35.f*35.f){
   recipe_.exciteNode=i;selectedNode_=i;refreshControls();commit();return;
  }
 }
 void mouseDrag(const juce::MouseEvent&e)override{
  if(!dragging_||selectedNode_<0)return;
  auto c=canvasBounds().toFloat();
  recipe_.nodes[size_t(selectedNode_)].x=juce::jlimit(.06,.94,double((e.position.x-c.getX())/c.getWidth()));
  recipe_.nodes[size_t(selectedNode_)].y=juce::jlimit(.10,.88,double((e.position.y-c.getY())/c.getHeight()));repaint();
 }
 void mouseUp(const juce::MouseEvent&)override{if(dragging_){dragging_=false;commit();}}
 void resized()override{
  const int y=getHeight()-84;
  int x=12;
  for(auto*b:{&addNode_,&removeNode_,&connect_,&removeLink_}){b->setBounds(x,y,115,30);x+=120;}
  domain_.setBounds(x+8,y,180,30);linkPicker_.setBounds(x+194,y,130,30);linkKind_.setBounds(x+330,y,190,30);
  const int mid=getHeight()-274;
  frequency_.setBounds(12,mid,getWidth()/2-20,30);mass_.setBounds(getWidth()/2+5,mid,getWidth()/2-18,30);
  nodeDamping_.setBounds(12,mid+34,getWidth()/2-20,30);drive_.setBounds(getWidth()/2+5,mid+34,getWidth()/2-18,30);
  area_.setBounds(12,mid+68,getWidth()/2-20,30);output_.setBounds(getWidth()/2+5,mid+68,getWidth()/2-18,30);
  linkStiffness_.setBounds(12,mid+140,getWidth()/2-20,30);linkDamping_.setBounds(getWidth()/2+5,mid+140,getWidth()/2-18,30);
  status_.setBounds(12,getHeight()-48,getWidth()-22,18);
 }
private:
 Recipe recipe_;
 juce::TextButton addNode_{"+ DOMAIN"},removeNode_{"- DOMAIN"},connect_{"CONNECT"},removeLink_{"- LINK"};
 juce::ComboBox domain_,linkPicker_,linkKind_;
 juce::Slider frequency_,mass_,nodeDamping_,drive_,area_,output_,linkStiffness_,linkDamping_;
 juce::Label status_;
 int selectedNode_=-1,selectedLink_=-1,startLink_=-1;bool refresh_=false,dragging_=false;
 juce::Rectangle<int> canvasBounds()const{return {12,8,std::max(100,getWidth()-24),std::max(115,getHeight()-322)};}
 juce::Point<float> location(int i)const{
  auto c=canvasBounds();if(i<0||i>=int(recipe_.nodes.size()))return {};
  const auto&n=recipe_.nodes[size_t(i)];return {float(c.getX()+n.x*c.getWidth()),float(c.getY()+n.y*c.getHeight())};
 }
 void show(const juce::String& msg){status_.setText(msg,juce::dontSendNotification);}
 void refreshControls(){
  refresh_=true;
  linkPicker_.clear(juce::dontSendNotification);linkPicker_.addItem("LINK: NONE",1);
  for(int i=0;i<int(recipe_.links.size());i++){
   const auto&l=recipe_.links[size_t(i)];linkPicker_.addItem(juce::String(l.from+1)+" -> "+juce::String(l.to+1),i+2);
  }
  linkPicker_.setSelectedId(selectedLink_+2,juce::dontSendNotification);
  if(selectedNode_>=0&&selectedNode_<int(recipe_.nodes.size())){
   const auto&n=recipe_.nodes[size_t(selectedNode_)];domain_.setSelectedId(n.domain==Domain::Mechanical?1:2,juce::dontSendNotification);
   frequency_.setValue(n.frequency,juce::dontSendNotification);mass_.setValue(n.mass,juce::dontSendNotification);
   nodeDamping_.setValue(n.damping,juce::dontSendNotification);drive_.setValue(n.drive,juce::dontSendNotification);area_.setValue(n.area,juce::dontSendNotification);output_.setValue(n.output,juce::dontSendNotification);
  }
  if(selectedLink_>=0&&selectedLink_<int(recipe_.links.size())){
   const auto&l=recipe_.links[size_t(selectedLink_)];linkKind_.setSelectedId(int(l.kind)+1,juce::dontSendNotification);
   linkStiffness_.setValue(l.stiffness,juce::dontSendNotification);linkDamping_.setValue(l.damping,juce::dontSendNotification);
  }
  refresh_=false;repaint();
 }
 void applyNumbers(){
  if(refresh_)return;
  if(selectedNode_>=0&&selectedNode_<int(recipe_.nodes.size())){
   auto&n=recipe_.nodes[size_t(selectedNode_)];n.frequency=frequency_.getValue();n.mass=mass_.getValue();n.damping=nodeDamping_.getValue();n.drive=drive_.getValue();n.area=area_.getValue();n.output=output_.getValue();
  }
  if(selectedLink_>=0&&selectedLink_<int(recipe_.links.size())){
   auto&l=recipe_.links[size_t(selectedLink_)];l.stiffness=linkStiffness_.getValue();l.damping=linkDamping_.getValue();
  }
  commit();
 }
 void commit(){
  std::string err;if(!panorama::energy40::valid(recipe_,&err)){show("INVALID: "+juce::String(err));return;}
  if(onGraphChanged&&!onGraphChanged(recipe_)){show("PROCESSOR REJECTED / original active graph unchanged");return;}repaint();
  show("ACTIVE / IMPLICIT MIDPOINT / ENERGY LEDGER / " +juce::String(int(recipe_.nodes.size()))+" domains");
 }
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnergyGraphEditor40)
};
