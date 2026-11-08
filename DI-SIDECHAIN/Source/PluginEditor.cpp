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
 setLookAndFeel(&theme);
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
 sequenceCount->setRange(1,16,1);
 sequenceCount->setColour(juce::Slider::trackColourId,green);
 sequenceCount->setColour(juce::Slider::thumbColourId,green);
 sequenceCount->setColour(juce::Slider::textBoxTextColourId,juce::Colours::white);
 sequenceCount->setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
 addAndMakeVisible(*sequenceCount);
 sequenceCountAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,"sequenceSteps",*sequenceCount);
 shapeOffset=std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal,juce::Slider::TextBoxRight);
 shapeOffset->setRange(-1.0,1.0,0.01);
 shapeOffset->setColour(juce::Slider::trackColourId,orange);
 shapeOffset->setColour(juce::Slider::thumbColourId,orange);
 shapeOffset->setColour(juce::Slider::textBoxTextColourId,juce::Colours::white);
 shapeOffset->setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
 addAndMakeVisible(*shapeOffset);
 shapeOffsetAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,"shapeOffset",*shapeOffset);
 const char* laneNames[]={"VOLUME","FILTER","PAN"};
 for(int i=0;i<3;++i){
  laneTabs[i]=std::make_unique<juce::TextButton>(laneNames[i]);
  laneTabs[i]->onClick=[this,i]{if(i>0&&!sequenceToggle->getToggleState())sequenceToggle->setToggleState(true,juce::sendNotificationSync);selectedLane=i;for(int j=0;j<3;++j)laneTabs[j]->setToggleState(j==selectedLane,juce::dontSendNotification);repaint();};
  addAndMakeVisible(*laneTabs[i]);
 }
 const char* laneIds[]={"filterMod","panMod"};
 for(int i=0;i<2;++i){
  laneSliders[i]=std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal,juce::Slider::TextBoxRight);
  laneSliders[i]->setColour(juce::Slider::trackColourId,i==0?juce::Colour(0xff44bbff):juce::Colour(0xffc55bff));
  laneSliders[i]->setColour(juce::Slider::thumbColourId,i==0?juce::Colour(0xff44bbff):juce::Colour(0xffc55bff));
  laneSliders[i]->setColour(juce::Slider::textBoxTextColourId,juce::Colours::white);
  laneSliders[i]->setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
  addAndMakeVisible(*laneSliders[i]);
  laneAttachments[i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,laneIds[i],*laneSliders[i]);
 }
 fxTypeBox=std::make_unique<juce::ComboBox>();
 fxTypeBox->addItemList(juce::StringArray{"Saturator","Soft Clipper"},1);
 fxTypeBox->setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff212b36));
 fxTypeBox->setColour(juce::ComboBox::textColourId,juce::Colours::white);
 fxTypeBox->setColour(juce::ComboBox::outlineColourId,edge);
 addAndMakeVisible(*fxTypeBox);
 fxTypeAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,"fxType",*fxTypeBox);
 fxToggle=std::make_unique<juce::TextButton>("OFF");
 addAndMakeVisible(*fxToggle);
 fxToggleAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"fxEnabled",*fxToggle);
 saturatorToggle=std::make_unique<juce::TextButton>("OFF");addAndMakeVisible(*saturatorToggle);
 saturatorAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"saturatorEnabled",*saturatorToggle);
 // Two independent switches: their state is owned by the processor and saved by the host.
 for(auto* b:{fxToggle.get(),saturatorToggle.get()}){
  b->setClickingTogglesState(true);
  b->setColour(juce::TextButton::buttonColourId,juce::Colour(0xff25303d));
  b->setColour(juce::TextButton::buttonOnColourId,green);
  b->setColour(juce::TextButton::textColourOffId,juce::Colours::white);
  b->setColour(juce::TextButton::textColourOnId,bg);
 }
 for(auto* b:{sequenceToggle.get(),sync.get()}){
  b->setColour(juce::ToggleButton::tickColourId,green);
  b->setColour(juce::ToggleButton::textColourId,juce::Colours::white);
 }
 presetButton=std::make_unique<juce::TextButton>("PRESETS  -  Custom");
 shapePresetButton=std::make_unique<juce::TextButton>("SHAPE PRESETS");
 sequencePrev=std::make_unique<juce::TextButton>("<");
 sequenceNext=std::make_unique<juce::TextButton>(">");
 for(auto* b:{presetButton.get(),shapePresetButton.get(),sequencePrev.get(),sequenceNext.get()}){buttonStyle(*b);addAndMakeVisible(*b);}
 presetButton->onClick=[this]{showPresetMenu();};
 shapePresetButton->onClick=[this]{showShapeMenu();};
 sequencePrev->onClick=[this]{selectedStep=(selectedStep+15)%16;repaint();};
 sequenceNext->onClick=[this]{selectedStep=(selectedStep+1)%16;repaint();};
 const char* fxIds[]={"fxDrive","fxMix"};
 for(int i=0;i<2;++i){
  fxSliders[i]=std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal,juce::Slider::TextBoxRight);
  fxSliders[i]->setColour(juce::Slider::trackColourId,juce::Colour(0xffffb84d));
  fxSliders[i]->setColour(juce::Slider::thumbColourId,juce::Colour(0xffffb84d));
  fxSliders[i]->setColour(juce::Slider::textBoxTextColourId,juce::Colours::white);
  fxSliders[i]->setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
  addAndMakeVisible(*fxSliders[i]);
  fxAttachments[i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,fxIds[i],*fxSliders[i]);
 }
 copyStep=std::make_unique<juce::TextButton>("COPY STEP");
 pasteStep=std::make_unique<juce::TextButton>("PASTE STEP");
 resetStep=std::make_unique<juce::TextButton>("RESET STEP");
 for(auto* b:{copyStep.get(),pasteStep.get(),resetStep.get()}){buttonStyle(*b);addAndMakeVisible(*b);}
 undoButton=std::make_unique<juce::TextButton>("UNDO");
 redoButton=std::make_unique<juce::TextButton>("REDO");
 snapButton=std::make_unique<juce::TextButton>("SNAP 1/16");
 for(auto* b:{undoButton.get(),redoButton.get(),snapButton.get()}){buttonStyle(*b);addAndMakeVisible(*b);}
 snapButton->setClickingTogglesState(true);
 undoButton->onClick=[this]{undoEdit();};
 redoButton->onClick=[this]{redoEdit();};
 copyStep->onClick=[this]{clipboard=captureShape(selectedStep+selectedLane*32,true);hasClipboard=true;};
 pasteStep->onClick=[this]{if(hasClipboard){const int key=selectedStep+selectedLane*32;auto before=captureShape(key,true);applyShape(key,true,clipboard);pushEdit(key,true,before,clipboard);repaint();}};
 resetStep->onClick=[this]{const int key=selectedStep+selectedLane*32;auto before=captureShape(key,true);di::ShapeSequencer::Shape defaults=selectedLane==0?di::ShapeSequencer::Shape{1.f,.92f,.08f,.02f,.32f,.75f,.95f,1.f}:di::ShapeSequencer::Shape{selectedLane==2?0.5f:1.f,selectedLane==2?0.5f:1.f,selectedLane==2?0.5f:1.f,selectedLane==2?0.5f:1.f,selectedLane==2?0.5f:1.f,selectedLane==2?0.5f:1.f,selectedLane==2?0.5f:1.f,selectedLane==2?0.5f:1.f};applyShape(key,true,defaults);pushEdit(key,true,before,defaults);repaint();};
 const char* filterNames[]={"FULL","LOW","HIGH","BAND"};
 for(int i=0;i<4;++i){filterButtons[i]=std::make_unique<juce::TextButton>(filterNames[i]);filterButtons[i]->onClick=[this,i]{selectFilter(i);};addAndMakeVisible(*filterButtons[i]);}
 if(combos.size()>0)combos[0]->setVisible(false);
 refreshFilterButtons();refreshFxButtons();
 setSize(1536,950);
 startTimerHz(24);
}
di::ShapeSequencer::Shape DISidechainEditor::captureShape(int step,bool sequence) const {
 di::ShapeSequencer::Shape shape{};
 const int lane=step/32, actualStep=step%32;
 for(int i=0;i<8;++i)shape[i]=sequence?(lane==0?proc.getSequencePoint(actualStep,i):proc.getModulationPoint(lane-1,actualStep,i)):proc.points[i].load();
 return shape;
}
void DISidechainEditor::applyShape(int step,bool sequence,const di::ShapeSequencer::Shape& shape) {
 const int lane=step/32, actualStep=step%32;
 for(int i=0;i<8;++i){if(sequence){if(lane==0)proc.setSequencePoint(actualStep,i,shape[i]);else proc.setModulationPoint(lane-1,actualStep,i,shape[i]);}else if(std::isfinite(shape[i]))proc.points[i].store(juce::jlimit(0.f,1.f,shape[i]),std::memory_order_relaxed);}
}
void DISidechainEditor::pushEdit(int step,bool sequence,const di::ShapeSequencer::Shape& before,const di::ShapeSequencer::Shape& after) {
 if(before==after)return;
 if(undoStack.size()>=64)undoStack.erase(undoStack.begin());
 undoStack.push_back({step,sequence,before,after});
 redoStack.clear();
}
void DISidechainEditor::undoEdit(){
 if(undoStack.empty())return;
 const auto edit=undoStack.back();undoStack.pop_back();
 applyShape(edit.step,edit.sequence,edit.before);
 redoStack.push_back(edit);repaint();
}
void DISidechainEditor::redoEdit(){
 if(redoStack.empty())return;
 const auto edit=redoStack.back();redoStack.pop_back();
 applyShape(edit.step,edit.sequence,edit.after);
 undoStack.push_back(edit);repaint();
}
void DISidechainEditor::selectFilter(int index) {
 if(auto* p=proc.state.getParameter("mode")){
  const float normalized=p->convertTo0to1((float)juce::jlimit(0,3,index));
  p->beginChangeGesture();p->setValueNotifyingHost(normalized);p->endChangeGesture();
 }
 refreshFilterButtons();
}
void DISidechainEditor::refreshFilterButtons(){
 const auto* raw=proc.state.getRawParameterValue("mode");
 const int selected=raw?juce::jlimit(0,3,(int)raw->load()):0;
 for(int i=0;i<4;++i)if(filterButtons[i])filterButtons[i]->setToggleState(i==selected,juce::dontSendNotification);
}
void DISidechainEditor::refreshFxButtons() {
 if(fxToggle)fxToggle->setButtonText(fxToggle->getToggleState()?"ON":"OFF");
 if(saturatorToggle)saturatorToggle->setButtonText(saturatorToggle->getToggleState()?"ON":"OFF");
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
 const auto laneColour=selectedLane==0?orange:selectedLane==1?juce::Colour(0xff44bbff):juce::Colour(0xffc55bff);
 const float offset=seq&&selectedLane==0&&shapeOffset? (float)shapeOffset->getValue():0.f;
 auto value=[&](int i){return seq?(selectedLane==0?proc.getSequencePoint(step,i):proc.getModulationPoint(selectedLane-1,step,i)):proc.points[i].load();};
 juce::Path p;
 for(int j=0;j<=140;++j){
  const double t=j/140.0;const double x=t*7;const int k=juce::jmin(6,(int)x);
  float u=(float)(x-k);u=u*u*(3-2*u);
  const float v=juce::jlimit(0.f,1.f,value(k)+(value(k+1)-value(k))*u+offset);
  const float px=r.getX()+t*r.getWidth(),py=r.getBottom()-v*r.getHeight();
  if(j==0)p.startNewSubPath(px,py);else p.lineTo(px,py);
 }
 juce::Path fill=p;fill.lineTo((float)r.getRight(),(float)r.getBottom());fill.lineTo((float)r.getX(),(float)r.getBottom());fill.closeSubPath();
 g.setColour(laneColour.withAlpha(.20f));g.fillPath(fill);
 g.setColour(laneColour);g.strokePath(p,juce::PathStrokeType(drawNodes?3.f:2.f));
 if(drawNodes)for(int i=0;i<8;++i){
  const float x=r.getX()+r.getWidth()*i/7.f,y=r.getBottom()-juce::jlimit(0.f,1.f,value(i)+offset)*r.getHeight();
  g.setColour(juce::Colour(0xff121821));g.fillEllipse(x-6,y-6,12,12);
  g.setColour(laneColour);g.drawEllipse(x-5,y-5,10,10,2.f);
 }
}
void DISidechainEditor::paint(juce::Graphics& g) {
 g.fillAll(bg);
 text(g,"~", {18,12,55,45},34,juce::Colour(0xff21d6e9),true);
 text(g,"DI SIDECHAIN", {76,8,325,46},33,juce::Colours::white,true);
 text(g,"4.0", {398,11,70,43},29,orange,true);
 text(g,"DIGITAL ILLUSION", {78,49,340,19},12,muted);
 g.setColour(edge);g.drawHorizontalLine(77,0.f,(float)getWidth());
 auto nav=juce::Rectangle<int>(12,90,getWidth()-24,64);
 g.setColour(panel);g.fillRoundedRectangle(nav.toFloat(),12.f);
 text(g,"SIDECHAIN", {42,105,160,34},19,muted,true);
 g.setColour(sequenceToggle->getToggleState()?green:edge);
 g.fillRoundedRectangle(juce::Rectangle<float>(207,98,185,47),9.f);
 text(g,"SEQUENCER", {233,105,153,31},19,sequenceToggle->getToggleState()?bg:muted,true);
 text(g,"STEPS", {414,99,80,17},12,muted);
 text(g,"LENGTH / RATE", {578,99,135,17},12,muted);
 text(g,"FX", {1015,104,35,32},15,juce::Colour(0xffffb84d),true);
 text(g,"SATURATOR", {1103,104,110,32},15,juce::Colour(0xffffb84d),true);
 text(g,"DRIVE dB", {1090,96,110,18},11,muted);
 text(g,"WET", {1284,96,70,18},11,muted);
 text(g,"DI SIDECHAIN 4.1", {1280,52,240,22},13,muted);
 auto graphPanel=juce::Rectangle<int>(12,165,getWidth()-24,282);
 g.setColour(panel);g.fillRoundedRectangle(graphPanel.toFloat(),12.f);
 g.setColour(edge);g.drawRoundedRectangle(graphPanel.toFloat(),12.f,1.f);
 text(g,"100%",{18,186,57,20},12,muted);
 text(g,"50%",{18,292,57,20},12,muted);
 text(g,"0%",{18,390,57,20},12,muted);
 const auto selectedColour=selectedLane==0?orange:selectedLane==1?juce::Colour(0xff44bbff):juce::Colour(0xffc55bff);
 const bool sequenced=sequenceToggle->getToggleState();
 if(sequenced) {
  // Four successive shapes across the overview, with a live step cursor.
  const int total=juce::jlimit(1,16,(int)sequenceCount->getValue());
  const int first=(selectedStep/4)*4;
  for(int k=0;k<4;++k) {
   auto cell=juce::Rectangle<int>(graph.getX()+k*graph.getWidth()/4,graph.getY(),graph.getWidth()/4,graph.getHeight());
   paintShape(g,cell,(first+k)%total,false);
   if((first+k)%total==proc.activeSequenceStep.load(std::memory_order_relaxed)){
    g.setColour(green);g.fillRect(cell.getX(),cell.getY(),3,cell.getHeight());
   }
  }
 } else paintShape(g,graph,selectedStep,false);
 const int count=juce::jlimit(1,16,(int)sequenceCount->getValue());
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
    const float node=selectedLane==0?proc.getSequencePoint(i,j):proc.getModulationPoint(selectedLane-1,i,j);
    float y=r.getBottom()-4-node*(r.getHeight()-23);
    if(j==0)mini.startNewSubPath(x,y);else mini.lineTo(x,y);}
   g.setColour(selectedColour);g.strokePath(mini,juce::PathStrokeType(1.5f));
  }
 }
 paintPanel(g,sequencerPanel,"STEP NAVIGATION / WORKFLOW");
 paintPanel(g,juce::Rectangle<int>(442,460,752,243),"STEP EDIT  /  ENVELOPE & SHAPE");
 paintPanel(g,juce::Rectangle<int>(1206,460,318,243),"MODULATION LANES");
 paintPanel(g,filterPanel,"FILTER  /  ALWAYS VISIBLE");
 paintPanel(g,outputPanel,"OUTPUT  /  SIDECHAIN");
 text(g,"SELECT STEP", {35,520,160,23},14,muted);
 text(g,juce::String(selectedStep+1)+" / 16", {112,559,240,38},25,juce::Colours::white,true);
 text(g,"Choose a step above or use the arrows.", {35,607,382,30},13,muted);
 text(g,"Select VOLUME / FILTER / PAN, then drag nodes.", {35,642,382,30},13,muted);
 const juce::String selectedName=selectedLane==0?"VOLUME":selectedLane==1?"FILTER":"PAN";
 text(g,selectedName+"  /  STEP "+juce::String(selectedStep+1), {463,518,230,22},14,selectedColour,true);
 paintShape(g,detailGraph,selectedStep,true);
 text(g,"Drag vertically to edit nearest node. SNAP rounds to 1/16.", {464,664,480,23},13,muted);
 text(g,"OFFSET +/-  (VOLUME)", {970,665,175,23},12,orange,true);
 text(g,"LANE EDITOR", {1225,505,230,18},12,muted);
 text(g,"SEQUENCED", {1370,523,130,24},14,sequenceToggle->getToggleState()?green:muted,true);
 text(g,"FILTER - DUCK DEPTH", {1225,566,280,23},14,juce::Colour(0xff44bbff));
 text(g,"PAN - BIPOLAR", {1225,623,280,23},14,juce::Colour(0xffc55bff));
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
 if(sequenceToggle)sequenceToggle->setBounds(407,111,110,29);
 if(sequenceCount)sequenceCount->setBounds(522,111,82,29);
 if(sync)sync->setBounds(745,111,110,29);
 if(sequencePrev)sequencePrev->setBounds(40,560,45,34);
 if(sequenceNext)sequenceNext->setBounds(360,560,45,34);
 if(presetButton)presetButton->setBounds(548,24,390,40);
 if(shapePresetButton)shapePresetButton->setBounds(992,507,183,31);
 if(copyStep)copyStep->setBounds(969,544,208,33);
 if(pasteStep)pasteStep->setBounds(969,586,208,33);
 if(resetStep)resetStep->setBounds(969,628,208,33);
 if(shapeOffset){shapeOffset->setBounds(1100,665,77,28);shapeOffset->setEnabled(selectedLane==0);}
 if(fxTypeBox)fxTypeBox->setBounds(870,111,140,29);
 if(fxToggle)fxToggle->setBounds(1045,111,65,29);
 if(saturatorToggle)saturatorToggle->setBounds(1210,111,70,29);
 if(fxSliders[0])fxSliders[0]->setBounds(1340,105,85,42);
 if(fxSliders[1])fxSliders[1]->setBounds(1430,105,85,42);
 for(int i=0;i<3;++i)if(laneTabs[i]){laneTabs[i]->setBounds(1225+i*92,527,86,30);laneTabs[i]->setToggleState(selectedLane==i,juce::dontSendNotification);}
 if(laneSliders[0])laneSliders[0]->setBounds(1225,588,275,28);
 if(laneSliders[1])laneSliders[1]->setBounds(1225,646,275,28);
 if(undoButton)undoButton->setBounds(706,508,76,30);
 if(redoButton)redoButton->setBounds(788,508,76,30);
 if(snapButton)snapButton->setBounds(869,508,116,30);
 if(combos.size()==3){combos[0]->setBounds(0,0,0,0);combos[1]->setBounds(290,790,215,31);combos[2]->setBounds(617,111,125,29);}
 for(int i=0;i<4;++i)if(filterButtons[i])filterButtons[i]->setBounds(35+i*58,790,54,31);
 if(sliders.size()!=8||labels.size()!=8)return;
 const int xs[]={1230,1360,35,165,295,425,555,685};
 for(int i=0;i<8;++i){labels[i]->setBounds(xs[i],831,115,20);sliders[i]->setBounds(xs[i],851,110,78);}
}
void DISidechainEditor::updatePoint(const juce::MouseEvent& e) {
 if(!detailGraph.contains(e.getPosition()))return;
 const auto r=detailGraph;
 const int i=juce::jlimit(0,7,juce::roundToInt((e.position.x-r.getX())/juce::jmax(1,r.getWidth())*7.f));
 float v=juce::jlimit(0.f,1.f,(r.getBottom()-e.position.y)/(float)juce::jmax(1,r.getHeight()));
 if(sequenceToggle->getToggleState()&&selectedLane==0&&shapeOffset)
  v=juce::jlimit(0.f,1.f,v-(float)shapeOffset->getValue());
 if(snapButton&&snapButton->getToggleState())v=juce::jlimit(0.f,1.f,std::round(v*16.f)/16.f);
 if(sequenceToggle->getToggleState()){if(selectedLane==0)proc.setSequencePoint(selectedStep,i,v);else proc.setModulationPoint(selectedLane-1,selectedStep,i,v);}else proc.points[i]=v;
 repaint();
}
void DISidechainEditor::updateDetailPoint(const juce::MouseEvent& e){updatePoint(e);}
void DISidechainEditor::mouseDown(const juce::MouseEvent& e){
 if(juce::Rectangle<int>(25,98,173,47).contains(e.getPosition())){sequenceToggle->setToggleState(false,juce::sendNotificationSync);selectedLane=0;repaint();return;}
 if(juce::Rectangle<int>(207,98,185,47).contains(e.getPosition())){sequenceToggle->setToggleState(true,juce::sendNotificationSync);repaint();return;}
 if(stepStrip.contains(e.getPosition())){
  const int count=juce::jlimit(1,16,(int)sequenceCount->getValue());
  selectedStep=juce::jlimit(0,count-1,(e.getPosition().x-stepStrip.getX())*count/juce::jmax(1,stepStrip.getWidth()));
  repaint();return;
 }
 if(detailGraph.contains(e.getPosition())){
  draggingShape=true;dragStep=selectedStep+(sequenceToggle->getToggleState()?selectedLane*32:0);dragSequence=sequenceToggle->getToggleState();
  dragBefore=captureShape(dragStep,dragSequence);
 }
 updatePoint(e);
}
void DISidechainEditor::mouseDrag(const juce::MouseEvent& e){updatePoint(e);}
void DISidechainEditor::mouseUp(const juce::MouseEvent&) {
 if(!draggingShape)return;
 draggingShape=false;
 pushEdit(dragStep,dragSequence,dragBefore,captureShape(dragStep,dragSequence));
}

void DISidechainEditor::applyFactoryPreset(int index) {
 const float shapes[][8]={
  {1.f,.85f,.4f,.02f,.12f,.55f,.88f,1.f},
  {1.f,.2f,.02f,.02f,.08f,.32f,.75f,1.f},
  {1.f,.9f,.15f,.03f,.35f,.8f,.95f,1.f},
  {1.f,.75f,.05f,.35f,.08f,.7f,.9f,1.f},
  {0.f,0.f,.25f,.6f,.95f,1.f,1.f,1.f}
 };
 index=juce::jlimit(0,4,index);
 const auto* names[]={"Classic Duck","Deep Pump","Sidechain 4/4","Breakbeat Pump","Pump Rise"};
 presetName=names[index];presetButton->setButtonText("PRESETS  -  "+presetName);
 for(int step=0;step<16;++step) {
  auto before=captureShape(step,true),after=before;
  for(int j=0;j<8;++j)after[j]=shapes[index][j];
  applyShape(step,true,after);pushEdit(step,true,before,after);
 }
 if(auto* parameter=proc.state.getParameter("sequenceEnabled")){
  parameter->beginChangeGesture();parameter->setValueNotifyingHost(index==0?0.f:1.f);parameter->endChangeGesture();
 }
 repaint();
}
void DISidechainEditor::applyShapePreset(int index) {
 const float shapes[][8]={
  {1.f,.82f,.4f,.05f,.13f,.42f,.79f,1.f},
  {0.f,.04f,.14f,.32f,.68f,.92f,1.f,1.f},
  {1.f,1.f,.98f,.9f,.7f,.42f,.15f,0.f},
  {0.f,0.f,0.f,0.f,1.f,1.f,1.f,1.f},
  {1.f,.8f,.4f,.1f,.1f,.4f,.8f,1.f},
  {0.f,.14f,.28f,.43f,.57f,.72f,.86f,1.f}
 };
 index=juce::jlimit(0,5,index);
 const bool seq=sequenceToggle->getToggleState();
 const int key=selectedStep+(seq?selectedLane*32:0);
 auto before=captureShape(key,seq),after=before;
 for(int i=0;i<8;++i)after[i]=shapes[index][i];
 applyShape(key,seq,after);pushEdit(key,seq,before,after);
 repaint();
}
void DISidechainEditor::showPresetMenu() {
 juce::PopupMenu menu;const juce::StringArray names{"Classic Duck","Deep Pump","Sidechain 4/4","Breakbeat Pump","Pump Rise"};
 for(int i=0;i<names.size();++i)menu.addItem(i+1,names[i]);
 menu.addSeparator();menu.addItem(100,"Save user preset...");menu.addItem(101,"Load user preset...");
 juce::Component::SafePointer<DISidechainEditor> safe(this);
 menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(presetButton.get()),[safe](int choice){
  if(!safe)return;
  if(choice==100)safe->saveUserPreset();
  else if(choice==101)safe->loadUserPreset();
  else if(choice>0)safe->applyFactoryPreset(choice-1);
 });
}
void DISidechainEditor::showShapeMenu() {
 juce::PopupMenu menu;const juce::StringArray names{"Classic Duck","Pump Rise","Pump Fall","Gate","Wave","Linear Rise"};
 for(int i=0;i<names.size();++i)menu.addItem(i+1,names[i]);
 juce::Component::SafePointer<DISidechainEditor> safe(this);
 menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(shapePresetButton.get()),[safe](int choice){if(safe&&choice>0)safe->applyShapePreset(choice-1);});
}

void DISidechainEditor::saveUserPreset() {
 const auto destination=juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("DI-Sidechain.dipreset");
 presetChooser=std::make_unique<juce::FileChooser>("Save DI SIDECHAIN preset",destination,"*.dipreset");
 juce::Component::SafePointer<DISidechainEditor> safe(this);
 presetChooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles,
  [safe](const juce::FileChooser& chooser){
   if(!safe)return;
   const auto file=chooser.getResult();
   if(file==juce::File())return;
   juce::MemoryBlock data;safe->proc.getStateInformation(data);
   if(file.replaceWithData(data.getData(),data.getSize())) {
    safe->presetName=file.getFileNameWithoutExtension();
    safe->presetButton->setButtonText("PRESETS  -  "+safe->presetName);
   }
  });
}
void DISidechainEditor::loadUserPreset() {
 const auto folder=juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
 presetChooser=std::make_unique<juce::FileChooser>("Load DI SIDECHAIN preset",folder,"*.dipreset");
 juce::Component::SafePointer<DISidechainEditor> safe(this);
 presetChooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,
  [safe](const juce::FileChooser& chooser){
   if(!safe)return;
   const auto file=chooser.getResult();
   if(file==juce::File())return;
   juce::MemoryBlock data;
   if(file.loadFileAsData(data) && data.getSize()>0){
    safe->proc.setStateInformation(data.getData(),(int)data.getSize());
    safe->presetName=file.getFileNameWithoutExtension();
    safe->presetButton->setButtonText("PRESETS  -  "+safe->presetName);
    safe->repaint();
   }
  });
}
