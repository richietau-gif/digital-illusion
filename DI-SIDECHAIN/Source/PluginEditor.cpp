#include "PluginEditor.h"
#include <cmath>

namespace {
const auto bg=juce::Colour(0xff0d1117), panel=juce::Colour(0xff161d26);
const auto edge=juce::Colour(0xff303a46), muted=juce::Colour(0xff9da8b7);
const auto orange=juce::Colour(0xffff8b18), green=juce::Colour(0xffa5ff35);
void text(juce::Graphics& g,const juce::String& s,juce::Rectangle<int> r,float size,juce::Colour colour,bool bold=false) {
 g.setColour(colour);g.setFont(juce::FontOptions(size,bold?juce::Font::bold:juce::Font::plain));
 g.drawFittedText(s,r,juce::Justification::centredLeft,1);
}
void buttonStyle(juce::TextButton& b) {
 b.setColour(juce::TextButton::buttonColourId,juce::Colour(0xff25303d));
 b.setColour(juce::TextButton::textColourOffId,juce::Colours::white);
}
}
DISidechainEditor::DISidechainEditor(DISidechainProcessor& p):AudioProcessorEditor(&p),proc(p) {
 const char* ids[]={"depth","mix","threshold","length","smooth","low","high","retrigger"};
 const char* names[]={"DEPTH","MIX","SENSITIVITY","LENGTH MS","SMOOTH MS","LOW HZ","HIGH HZ","RETRIGGER MS"};
 for(int i=0;i<8;++i) {
  auto s=std::make_unique<juce::Slider>();
  s->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  s->setTextBoxStyle(juce::Slider::TextBoxBelow,false,100,20);
  s->setColour(juce::Slider::rotarySliderFillColourId,green);
  s->setColour(juce::Slider::rotarySliderOutlineColourId,edge);
  s->setColour(juce::Slider::thumbColourId,juce::Colours::white);
  s->setColour(juce::Slider::textBoxTextColourId,juce::Colours::white);
  s->setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
  auto l=std::make_unique<juce::Label>();
  l->setText(names[i],juce::dontSendNotification);
  l->setJustificationType(juce::Justification::centred);
  l->setColour(juce::Label::textColourId,muted);
  addAndMakeVisible(*s);addAndMakeVisible(*l);
  sa.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,ids[i],*s));
  sliders.push_back(std::move(s));labels.push_back(std::move(l));
 }
 const char* cids[]={"mode","trigger","division"};
 const juce::StringArray choices[]={{"Full","Low","High","Band"},{"Audio + MIDI","Audio only","MIDI only"},{"1/32","1/16","1/8","1/4","1/2","1 bar","2 bars"}};
 for(int i=0;i<3;++i) {
  auto c=std::make_unique<juce::ComboBox>();c->addItemList(choices[i],1);
  c->setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff212b36));
  c->setColour(juce::ComboBox::textColourId,juce::Colours::white);
  c->setColour(juce::ComboBox::outlineColourId,edge);
  addAndMakeVisible(*c);
  ca.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,cids[i],*c));
  combos.push_back(std::move(c));
 }
 sync=std::make_unique<juce::ToggleButton>("TEMPO SYNC");addAndMakeVisible(*sync);
 ba=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"sync",*sync);
 sequenceToggle=std::make_unique<juce::ToggleButton>("SEQUENCER ON");
 addAndMakeVisible(*sequenceToggle);
 sequenceAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"sequenceEnabled",*sequenceToggle);
 sequenceCount=std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal,juce::Slider::TextBoxRight);
 sequenceCount->setRange(1,32,1);
 sequenceCount->setColour(juce::Slider::trackColourId,green);
 sequenceCount->setColour(juce::Slider::thumbColourId,green);
 sequenceCount->setColour(juce::Slider::textBoxTextColourId,juce::Colours::white);
 sequenceCount->setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
 addAndMakeVisible(*sequenceCount);
 sequenceCountAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,"sequenceSteps",*sequenceCount);
 copyStep=std::make_unique<juce::TextButton>("COPY STEP");
 pasteStep=std::make_unique<juce::TextButton>("PASTE STEP");
 resetStep=std::make_unique<juce::TextButton>("RESET STEP");
 for(auto* b:{copyStep.get(),pasteStep.get(),resetStep.get()}){buttonStyle(*b);addAndMakeVisible(*b);}
 copyStep->onClick=[this]{for(int i=0;i<8;++i)clipboard[i]=proc.getSequencePoint(selectedStep,i);hasClipboard=true;};
 pasteStep->onClick=[this]{if(hasClipboard){for(int i=0;i<8;++i)proc.setSequencePoint(selectedStep,i,clipboard[i]);repaint();}};
 resetStep->onClick=[this]{const float defaults[]={1,.92f,.08f,.02f,.32f,.75f,.95f,1};for(int i=0;i<8;++i)proc.setSequencePoint(selectedStep,i,defaults[i]);repaint();};
 setSize(1536,950);
 startTimerHz(24);
}
void DISidechainEditor::paintPanel(juce::Graphics& g,juce::Rectangle<int> r,const juce::String& name) {
 g.setColour(panel);g.fillRoundedRectangle(r.toFloat(),12.f);
 g.setColour(edge);g.drawRoundedRectangle(r.toFloat().reduced(.5f),12.f,1.f);
 auto header=r.removeFromTop(42);
 g.setColour(juce::Colour(0xff202a35));g.fillRoundedRectangle(header.toFloat(),10.f);
 text(g,name,header.reduced(17,4),16,juce::Colours::white,true);
}
void DISidechainEditor::paintShape(juce::Graphics& g,juce::Rectangle<int> r,int step,bool drawNodes) {
 if(r.isEmpty())return;
 g.setColour(juce::Colour(0xff111820));g.fillRoundedRectangle(r.toFloat(),5.f);
 g.setColour(juce::Colour(0xff2c3844));
 for(int i=1;i<8;++i){const int x=r.getX()+r.getWidth()*i/8;g.drawVerticalLine(x,(float)r.getY(),(float)r.getBottom());}
 for(int i=1;i<4;++i){const int y=r.getY()+r.getHeight()*i/4;g.drawHorizontalLine(y,(float)r.getX(),(float)r.getRight());}
 const bool seq=sequenceToggle->getToggleState();
 auto value=[&](int i){return seq?proc.getSequencePoint(step,i):proc.points[i].load();};
 juce::Path p;
 for(int j=0;j<=140;++j){
  const double t=j/140.0;const double x=t*7;const int k=juce::jmin(6,(int)x);
  float u=(float)(x-k);u=u*u*(3-2*u);
  const float v=value(k)+(value(k+1)-value(k))*u;
  const float px=r.getX()+t*r.getWidth(),py=r.getBottom()-v*r.getHeight();
  if(j==0)p.startNewSubPath(px,py);else p.lineTo(px,py);
 }
 juce::Path fill=p;fill.lineTo((float)r.getRight(),(float)r.getBottom());fill.lineTo((float)r.getX(),(float)r.getBottom());fill.closeSubPath();
 g.setColour(orange.withAlpha(.20f));g.fillPath(fill);
 g.setColour(orange);g.strokePath(p,juce::PathStrokeType(drawNodes?3.f:2.f));
 if(drawNodes)for(int i=0;i<8;++i){
  const float x=r.getX()+r.getWidth()*i/7.f,y=r.getBottom()-value(i)*r.getHeight();
  g.setColour(juce::Colour(0xff121821));g.fillEllipse(x-6,y-6,12,12);
  g.setColour(orange);g.drawEllipse(x-5,y-5,10,10,2.f);
 }
}
void DISidechainEditor::paint(juce::Graphics& g) {
 g.fillAll(bg);
 text(g,"◉", {18,12,55,45},34,orange,true);
 text(g,"DI SIDECHAIN", {76,8,325,46},33,juce::Colours::white,true);
 text(g,"4.0", {398,11,70,43},29,orange,true);
 text(g,"D I G I T A L   I L L U S I O N", {78,49,340,19},12,muted);
 g.setColour(edge);g.drawHorizontalLine(77,0.f,(float)getWidth());
 auto nav=juce::Rectangle<int>(12,90,getWidth()-24,64);
 g.setColour(panel);g.fillRoundedRectangle(nav.toFloat(),12.f);
 text(g,"SIDECHAIN", {42,105,160,34},19,muted,true);
 g.setColour(sequenceToggle->getToggleState()?green:edge);
 g.fillRoundedRectangle(juce::Rectangle<float>(207,98,185,47),9.f);
 text(g,"SEQUENCER", {233,105,153,31},19,sequenceToggle->getToggleState()?bg:muted,true);
 text(g,"MULTI-LANE  ·  COMING SOON", {424,106,305,32},16,muted);
 text(g,"FX  ·  COMING SOON", {755,106,245,32},16,muted);
 text(g,"TRIGGER", {1055,106,100,32},15,muted,true);
 text(g,"DI SIDECHAIN 4.0  •  EXPERIMENTAL", {1160,52,340,22},13,muted);
 auto graphPanel=juce::Rectangle<int>(12,165,getWidth()-24,282);
 g.setColour(panel);g.fillRoundedRectangle(graphPanel.toFloat(),12.f);
 g.setColour(edge);g.drawRoundedRectangle(graphPanel.toFloat(),12.f,1.f);
 text(g,"100%",{18,186,57,20},12,muted);
 text(g,"50%",{18,292,57,20},12,muted);
 text(g,"0%",{18,390,57,20},12,muted);
 const bool sequenced=sequenceToggle->getToggleState();
 if(sequenced) {
  // Four successive shapes across the overview, with a live step cursor.
  const int total=juce::jlimit(1,32,(int)sequenceCount->getValue());
  const int first=(selectedStep/4)*4;
  for(int k=0;k<4;++k) {
   auto cell=juce::Rectangle<int>(graph.getX()+k*graph.getWidth()/4,graph.getY(),graph.getWidth()/4,graph.getHeight());
   paintShape(g,cell,(first+k)%total,false);
   if((first+k)%total==proc.activeSequenceStep.load(std::memory_order_relaxed)){
    g.setColour(green);g.fillRect(cell.getX(),cell.getY(),3,cell.getHeight());
   }
  }
 } else paintShape(g,graph,selectedStep,false);
 const int count=juce::jlimit(1,32,(int)sequenceCount->getValue());
 const int playing=proc.activeSequenceStep.load(std::memory_order_relaxed);
 for(int i=0;i<count;++i){
  const float w=(float)stepStrip.getWidth()/count;
  auto r=juce::Rectangle<float>(stepStrip.getX()+i*w,stepStrip.getY(),juce::jmax(2.f,w-3),stepStrip.getHeight());
  g.setColour(i==selectedStep?orange.withAlpha(.65f):i==playing&&sequenceToggle->getToggleState()?green.withAlpha(.7f):juce::Colour(0xff25303b));
  g.fillRoundedRectangle(r,5.f);
  if(w>=25){text(g,juce::String(i+1),r.toNearestInt().reduced(4,2).withHeight(17),12,juce::Colours::white);}
  if(w>=23){
   juce::Path mini;
   for(int j=0;j<8;++j){float x=r.getX()+3+(r.getWidth()-6)*j/7.f;
    float y=r.getBottom()-4-proc.getSequencePoint(i,j)*(r.getHeight()-23);
    if(j==0)mini.startNewSubPath(x,y);else mini.lineTo(x,y);}
   g.setColour(orange);g.strokePath(mini,juce::PathStrokeType(1.5f));
  }
 }
 paintPanel(g,sequencerPanel,"SEQUENCER");
 paintPanel(g,juce::Rectangle<int>(442,460,752,243),"STEP EDIT  /  ENVELOPE & SHAPE");
 paintPanel(g,juce::Rectangle<int>(1206,460,318,243),"MODULATION LANES");
 paintPanel(g,filterPanel,"FILTER  /  ALWAYS VISIBLE");
 paintPanel(g,outputPanel,"OUTPUT  /  SIDECHAIN");
 text(g,"STEPS", {35,520,100,23},14,muted);
 text(g,"LENGTH / RATE", {215,520,180,23},14,muted);
 text(g,"EDIT STEP "+juce::String(selectedStep+1), {463,518,220,22},14,orange,true);
 paintShape(g,detailGraph,selectedStep,true);
 text(g,"Drag nodes to edit  •  Copy / Paste / Reset", {464,664,480,23},13,muted);
 text(g,"VOLUME", {1225,523,120,24},16,orange,true);
 text(g,"SEQUENCED", {1370,523,130,24},14,sequenceToggle->getToggleState()?green:muted,true);
 text(g,"FILTER  •  modulation not active", {1225,573,280,23},15,juce::Colour(0xff44bbff));
 text(g,"PAN  •  modulation not active", {1225,625,280,23},15,juce::Colour(0xffc55bff));
 text(g,"FILTER MODE", {35,763,170,20},13,muted);
 text(g,"TRIGGER SOURCE", {290,763,180,20},13,muted);
 text(g,"BEAT DIVISION", {540,763,180,20},13,muted);
 text(g,"LIVE DSP CONTROLS  /  ACTIVE", {35,907,400,20},12,green);
 text(g,"OUTPUT CONTROLS", {1225,764,260,23},14,muted);
 const float peak=juce::jlimit(0.f,1.f,proc.meter.load(std::memory_order_relaxed));
 g.setColour(edge);g.fillRoundedRectangle(juce::Rectangle<float>(1491,765,12,126),4.f);
 g.setColour(green);g.fillRoundedRectangle(juce::Rectangle<float>(1491,891-peak*126,12,peak*126),4.f);
}
void DISidechainEditor::resized() {
 graph={77,185,getWidth()-100,184};
 stepStrip={77,386,getWidth()-100,46};
 sequencerPanel={12,460,418,243};
 detailGraph={465,548,490,102};
 filterPanel={12,713,1177,225};
 outputPanel={1201,713,323,225};
 if(sequenceToggle)sequenceToggle->setBounds(38,562,184,30);
 if(sequenceCount)sequenceCount->setBounds(36,602,348,38);
 if(sync)sync->setBounds(235,652,165,28);
 if(copyStep)copyStep->setBounds(969,544,208,33);
 if(pasteStep)pasteStep->setBounds(969,586,208,33);
 if(resetStep)resetStep->setBounds(969,628,208,33);
 for(int i=0;i<3&&i<(int)combos.size();++i)combos[i]->setBounds(35+i*250,790,215,31);
 if(sliders.size()!=8||labels.size()!=8)return;
 const int xs[]={1230,1360,35,165,295,425,555,685};
 for(int i=0;i<8;++i){labels[i]->setBounds(xs[i],831,115,20);sliders[i]->setBounds(xs[i],851,110,78);}
}
void DISidechainEditor::updatePoint(const juce::MouseEvent& e) {
 if(!detailGraph.contains(e.getPosition()))return;
 const auto r=detailGraph;
 const int i=juce::jlimit(0,7,juce::roundToInt((e.position.x-r.getX())/juce::jmax(1,r.getWidth())*7.f));
 const float v=juce::jlimit(0.f,1.f,(r.getBottom()-e.position.y)/(float)juce::jmax(1,r.getHeight()));
 if(sequenceToggle->getToggleState())proc.setSequencePoint(selectedStep,i,v);else proc.points[i]=v;
 repaint();
}
void DISidechainEditor::updateDetailPoint(const juce::MouseEvent& e){updatePoint(e);}
void DISidechainEditor::mouseDown(const juce::MouseEvent& e){
 if(stepStrip.contains(e.getPosition())){
  const int count=juce::jlimit(1,32,(int)sequenceCount->getValue());
  selectedStep=juce::jlimit(0,count-1,(e.getPosition().x-stepStrip.getX())*count/juce::jmax(1,stepStrip.getWidth()));
  repaint();return;
 }
 updatePoint(e);
}
void DISidechainEditor::mouseDrag(const juce::MouseEvent& e){updatePoint(e);}
