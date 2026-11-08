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
// Click target stays fully transparent so the vector setup cog drawn by the
// editor remains visible, including during hover/press interactions.
class TransparentTextButton final : public juce::TextButton {
public:
 void paintButton(juce::Graphics&,bool,bool) override {}
};
}
DISidechainEditor::DISidechainEditor(DISidechainProcessor& p):AudioProcessorEditor(&p),proc(p) {
 setLookAndFeel(&theme);
 const char* ids[]={"depth","mix","threshold","length","smooth","low","high","retrigger"};
 const char* names[]={"DEPTH","MIX","SENSITIVITY","LENGTH MS","SMOOTH MS","LOW HZ","HIGH HZ","RETRIGGER MS"};
 for(int i=0;i<8;++i) {
  auto s=std::make_unique<juce::Slider>();
  s->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  s->setTextBoxStyle(juce::Slider::TextBoxBelow,false,100,20);
  s->setColour(juce::Slider::rotarySliderFillColourId,i<2?green:juce::Colour(0xff24def0));
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
 sequenceCount=std::make_unique<juce::Slider>(juce::Slider::IncDecButtons,juce::Slider::TextBoxLeft);
 sequenceCount->setIncDecButtonsMode(juce::Slider::incDecButtonsNotDraggable);
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
 setupButton=std::make_unique<TransparentTextButton>();
 setupButton->setTooltip("SETUP - Plugin settings");
 setupBounds={1449,31,61,45};
 addAndMakeVisible(*setupButton);
 setupButton->onClick=[this]{showSetupMenu();};
 shapePresetButton=std::make_unique<juce::TextButton>("SHAPE PRESETS");
 sequencePrev=std::make_unique<juce::TextButton>("<");
 sequenceNext=std::make_unique<juce::TextButton>(">");
 for(auto* b:{presetButton.get(),shapePresetButton.get(),sequencePrev.get(),sequenceNext.get()}){buttonStyle(*b);addAndMakeVisible(*b);}
 presetButton->onClick=[this]{showPresetMenu();};
 shapePresetButton->onClick=[this]{showShapeMenu();};
 sequencePrev->onClick=[this]{
  const int count=juce::jlimit(1,16,(int)sequenceCount->getValue());
  selectedStep=(selectedStep+count-1)%count;repaint();
 };
 sequenceNext->onClick=[this]{
  const int count=juce::jlimit(1,16,(int)sequenceCount->getValue());
  selectedStep=(selectedStep+1)%count;repaint();
 };
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
 copyStep->onClick=[this]{
  const bool seq=sequenceToggle->getToggleState();
  clipboard=captureShape(seq?selectedStep+selectedLane*32:0,seq);
  hasClipboard=true;
 };
 pasteStep->onClick=[this]{
  if(!hasClipboard)return;
  const bool seq=sequenceToggle->getToggleState();
  const int key=seq?selectedStep+selectedLane*32:0;
  const auto before=captureShape(key,seq);
  applyShape(key,seq,clipboard);
  pushEdit(key,seq,before,clipboard);repaint();
 };
 resetStep->onClick=[this]{
  const bool seq=sequenceToggle->getToggleState();
  const int key=seq?selectedStep+selectedLane*32:0;
  const auto before=captureShape(key,seq);
  constexpr di::ShapeSequencer::Shape volume{1.f,.92f,.08f,.02f,.32f,.75f,.95f,1.f};
  const di::ShapeSequencer::Shape defaults=seq&&selectedLane>0
   ? di::ShapeSequencer::Shape{selectedLane==2?.5f:1.f,selectedLane==2?.5f:1.f,
       selectedLane==2?.5f:1.f,selectedLane==2?.5f:1.f,selectedLane==2?.5f:1.f,
       selectedLane==2?.5f:1.f,selectedLane==2?.5f:1.f,selectedLane==2?.5f:1.f}
   : volume;
  applyShape(key,seq,defaults);
  pushEdit(key,seq,before,defaults);repaint();
 };
 const char* filterNames[]={"FULL","LOW","HIGH","BAND"};
 for(int i=0;i<4;++i){filterButtons[i]=std::make_unique<juce::TextButton>(filterNames[i]);filterButtons[i]->onClick=[this,i]{selectFilter(i);};addAndMakeVisible(*filterButtons[i]);}
 if(combos.size()>0)combos[0]->setVisible(false);
 refreshFilterButtons();refreshFxButtons();
 renderQuality=juce::jlimit(0,2,(int)p.state.getRawParameterValue("renderQuality")->load());
 const int scaleIndex=juce::jlimit(0,2,(int)p.state.getRawParameterValue("uiScale")->load());
 uiScale=scaleIndex==0?.5f:scaleIndex==1?.75f:1.f;
 setSize(juce::roundToInt(designWidth*uiScale),juce::roundToInt(designHeight*uiScale));
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
 const auto f=r.toFloat();
 g.setColour(juce::Colour(0xff030b12).withAlpha(.5f));
 g.fillRoundedRectangle(f.translated(1.f,3.f),12.f);
 juce::ColourGradient body(juce::Colour(0xff0b2533),f.getX(),f.getY(),
                           juce::Colour(0xff06131d),f.getRight(),f.getBottom(),false);
 g.setGradientFill(body);g.fillRoundedRectangle(f,11.f);
 g.setColour(juce::Colour(0xff22617a));g.drawRoundedRectangle(f.reduced(.5f),11.f,1.f);
 auto top=r.removeFromTop(39).toFloat().reduced(1.f);
 juce::ColourGradient header(juce::Colour(0xff163b51),top.getX(),top.getY(),
                             juce::Colour(0xff0a1b27),top.getX(),top.getBottom(),false);
 g.setGradientFill(header);g.fillRoundedRectangle(top,10.f);
 g.setColour(juce::Colour(0xff1a9ab0).withAlpha(.55f));g.drawHorizontalLine((int)top.getY()+1,top.getX()+14.f,top.getRight()-14.f);
 text(g,name,juce::Rectangle<int>((int)f.getX()+19,(int)f.getY()+7,(int)f.getWidth()-30,27),16,juce::Colours::white,true);
}
void DISidechainEditor::paintFrequencyResponse(juce::Graphics& g,juce::Rectangle<int> rect) {
 if(rect.isEmpty())return;
 g.setColour(juce::Colour(0xff091722));g.fillRoundedRectangle(rect.toFloat(),5.f);
 g.setColour(juce::Colour(0xff1c3c4b));
 for(int k=1;k<8;++k)g.drawVerticalLine(rect.getX()+rect.getWidth()*k/8,(float)rect.getY(),(float)rect.getBottom());
 for(int k=1;k<4;++k)g.drawHorizontalLine(rect.getY()+rect.getHeight()*k/4,(float)rect.getX(),(float)rect.getRight());
 const int mode=juce::jlimit(0,3,(int)proc.state.getRawParameterValue("mode")->load());
 const float cutoff=juce::jlimit(25.f,18000.f,proc.state.getRawParameterValue("low")->load());
 const float high=juce::jlimit(60.f,18000.f,proc.state.getRawParameterValue("high")->load());
 const auto position=[](float hz){return juce::jlimit(0.f,1.f,(std::log(hz)-std::log(20.f))/(std::log(20000.f)-std::log(20.f)));};
 juce::Path curve;
 for(int i=0;i<=170;++i){
  const float x=i/170.f;
  const float f=20.f*std::pow(1000.f,x);
  const auto sigmoid=[](float z){return 1.f/(1.f+std::exp(-z));};
  const float lowpass=1.f-sigmoid((std::log(f)-std::log(cutoff))*9.f);
  const float highpass=1.f-lowpass;
  const float band=highpass*(1.f-sigmoid((std::log(f)-std::log(juce::jmax(cutoff+1.f,high)))*9.f));
  const float value=mode==0?1.f:mode==1?lowpass:mode==2?highpass:band;
  const float px=rect.getX()+x*rect.getWidth();
  const float py=rect.getBottom()-5.f-value*(rect.getHeight()-10.f);
  if(i==0)curve.startNewSubPath(px,py);else curve.lineTo(px,py);
 }
 juce::Path fill(curve);fill.lineTo((float)rect.getRight(),(float)rect.getBottom());fill.lineTo((float)rect.getX(),(float)rect.getBottom());fill.closeSubPath();
 g.setColour(juce::Colour(0xffff8b19).withAlpha(.15f));g.fillPath(fill);
 g.setColour(juce::Colour(0xffffa126).withAlpha(.20f));g.strokePath(curve,juce::PathStrokeType(6.f));
 g.setColour(juce::Colour(0xffff981b));g.strokePath(curve,juce::PathStrokeType(2.5f));
 const float knobX=rect.getX()+position(cutoff)*rect.getWidth();
 g.setColour(juce::Colours::white);g.fillEllipse(knobX-4.f,rect.getY()+4.f,8.f,8.f);
 const juce::StringArray ticks{"20","50","100","200","500","1k","2k","5k","10k","20k"};
 for(int i=0;i<ticks.size();++i)text(g,ticks[i],{rect.getX()+i*rect.getWidth()/10,rect.getBottom()+2,35,14},10,juce::Colour(0xff8bafc1));
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
 const int segments=renderQuality==0?42:renderQuality==2?420:140;
 for(int j=0;j<=segments;++j){
  const double t=(double)j/segments;const double x=t*7;const int k=juce::jmin(6,(int)x);
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
 g.fillAll(juce::Colour(0xff06121b));
 g.addTransform(juce::AffineTransform::scale(uiScale));
 const auto white=juce::Colour(0xffecf8ff);
 const auto faint=juce::Colour(0xff9bb6c7);
 const auto cyan=DITheme::cyan();
 const auto lime=DITheme::lime();
 const auto orange=DITheme::orange();
 // Polished header with an original waveform logo, entirely vector-rendered.
 juce::ColourGradient top(juce::Colour(0xff123448),0.f,0.f,juce::Colour(0xff06111a),0.f,102.f,false);
 g.setGradientFill(top);g.fillRect(0,0,designWidth,101);
 g.setColour(cyan.withAlpha(.65f));g.drawHorizontalLine(100,12.f,1524.f);
 juce::Path logo;
 for(int i=0;i<=75;++i) {
  const float t=i/75.f;
  const float amp=std::pow(juce::jmax(0.f,1.f-std::abs(t-.5f)*2.f),1.6f)*16.f;
  const float x=27.f+t*70.f,y=58.f-std::sin(t*juce::MathConstants<float>::twoPi*4.f)*amp;
  if(i==0)logo.startNewSubPath(x,y);else logo.lineTo(x,y);
 }
 g.setColour(cyan.withAlpha(.23f));g.strokePath(logo,juce::PathStrokeType(6.f));
 g.setColour(cyan);g.strokePath(logo,juce::PathStrokeType(2.5f));
 text(g,"DI SIDECHAIN", {117,30,280,39},32,white,true);
 text(g,"4.1",{365,30,70,39},29,orange,true);
 text(g,"D I G I T A L   I L L U S I O N",{120,72,310,16},12,faint);
 g.setColour(juce::Colour(0xff2d5668));g.drawVerticalLine(435,30.f,80.f);
 text(g,"PRESET",{450,43,80,21},12,faint,true);
 text(g,"SETUP",{1351,44,70,23},13,cyan,true);
 // Crisp vector cog: the actual settings menu is a separate hidden popup.
 const auto gear=juce::Point<float>(1470.f,51.f);
 g.setColour(cyan.withAlpha(.18f));g.fillEllipse(gear.x-17.f,gear.y-17.f,34.f,34.f);
 g.setColour(cyan);g.drawEllipse(gear.x-13.f,gear.y-13.f,26.f,26.f,1.5f);
 g.drawEllipse(gear.x-4.5f,gear.y-4.5f,9.f,9.f,2.f);
 for(int tooth=0;tooth<8;++tooth){
  const float a=juce::MathConstants<float>::twoPi*tooth/8.f;
  g.drawLine(gear.x+std::cos(a)*13.f,gear.y+std::sin(a)*13.f,
             gear.x+std::cos(a)*17.f,gear.y+std::sin(a)*17.f,2.f);
 }
 // Toolbar / sequencer: dark glass and green active states.
 const auto nav=juce::Rectangle<float>(12,108,1512,62);
 g.setColour(juce::Colour(0xff102a37));g.fillRoundedRectangle(nav,11.f);
 g.setColour(juce::Colour(0xff24546a));g.drawRoundedRectangle(nav,11.f,1.f);
 const bool enabled=sequenceToggle&&sequenceToggle->getToggleState();
 g.setColour(enabled?juce::Colour(0xff0a2533):lime);
 g.fillRoundedRectangle(juce::Rectangle<float>(24,117,153,43),8.f);
 text(g,"SIDECHAIN",{40,122,136,30},16,enabled?white:DITheme::dark(),true);
 g.setColour(enabled?lime:juce::Colour(0xff102d3c));
 g.fillRoundedRectangle(juce::Rectangle<float>(183,117,155,43),8.f);
 if(enabled){g.setColour(lime.withAlpha(.25f));g.drawRoundedRectangle(juce::Rectangle<float>(184,118,153,41),8.f,3.f);}
 text(g,"SEQUENCER",{199,122,137,30},16,enabled?DITheme::dark():white,true);
 text(g,"STEPS",{503,110,70,14},11,faint);
 text(g,"LENGTH / RATE",{600,110,135,14},11,faint);
 text(g,"FX",{998,123,37,31},15,white,true);
 text(g,"SATURATOR",{1105,123,105,31},15,white,true);
 text(g,"DRIVE",{1283,110,72,14},11,faint);
 text(g,"WET",{1415,110,60,14},11,faint);
 // Shape overview.
 paintPanel(g,{12,177,1512,269},"");
 text(g,"100%",{19,198,58,18},12,white);
 text(g,"50%",{25,270,52,18},12,white);
 text(g,"0%",{31,352,46,18},12,white);
 const bool sequenced=sequenceToggle->getToggleState();
 const int count=juce::jlimit(1,16,(int)sequenceCount->getValue());
 if(sequenced) {
  // Show the whole active sequence, not merely the four steps around selection.
  // Graph and clickable thumbnails now use the same absolute step mapping.
  const int active=proc.activeSequenceStep.load(std::memory_order_relaxed);
  for(int k=0;k<count;++k) {
   const int x0=graph.getX()+k*graph.getWidth()/count;
   const int x1=graph.getX()+(k+1)*graph.getWidth()/count;
   auto cell=juce::Rectangle<int>(x0,graph.getY(),x1-x0,graph.getHeight());
   paintShape(g,cell,k,false);
   if(k==active) {
    g.setColour(lime.withAlpha(.85f));
    g.fillRect(cell.getX(),cell.getY(),2,cell.getHeight());
   }
   if(k==selectedStep){
    g.setColour(orange.withAlpha(.5f));
    g.drawRect(cell,1);
   }
  }
 } else paintShape(g,graph,selectedStep,false);
 const auto laneColour=selectedLane==0?orange:selectedLane==1?cyan:juce::Colour(0xffd36afb);
 const int playing=proc.activeSequenceStep.load(std::memory_order_relaxed);
 // One visible row, never more than sixteen steps.
 for(int i=0;i<count;++i){
  const float width=(float)stepStrip.getWidth()/count;
  const auto r=juce::Rectangle<float>(stepStrip.getX()+i*width+1,stepStrip.getY(),width-3,stepStrip.getHeight());
  const bool selected=i==selectedStep;
  juce::ColourGradient tile(selected?juce::Colour(0xffa24d0d):juce::Colour(0xff153447),r.getX(),r.getY(),
                            selected?juce::Colour(0xff3b230f):juce::Colour(0xff091b27),r.getX(),r.getBottom(),false);
  g.setGradientFill(tile);g.fillRoundedRectangle(r,6.f);
  g.setColour(selected?orange: i==playing&&sequenced?lime:juce::Colour(0xff28546a));
  g.drawRoundedRectangle(r,6.f,selected?2.f:1.f);
  text(g,juce::String(i+1),r.toNearestInt().reduced(5,3).withHeight(16),12,white,true);
  juce::Path miniature;
  for(int node=0;node<8;++node) {
   const float value=selectedLane==0?proc.getSequencePoint(i,node):proc.getModulationPoint(selectedLane-1,i,node);
   const float px=r.getX()+5.f+(r.getWidth()-10.f)*node/7.f;
   const float py=r.getBottom()-5.f-value*(r.getHeight()-25.f);
   if(node==0)miniature.startNewSubPath(px,py);else miniature.lineTo(px,py);
  }
  g.setColour(laneColour);g.strokePath(miniature,juce::PathStrokeType(1.8f));
 }
 // Step editor and modulation lanes: lanes are a single-click selection.
 paintPanel(g,sequencerPanel,"STEP EDIT");
 paintPanel(g,{897,456,305,240},"MODULATION LANES");
 paintPanel(g,{1212,456,312,240},"LANE SETTINGS");
 text(g,"STEP",{33,503,82,21},12,faint);
 text(g,juce::String(selectedStep+1)+" / "+juce::String(count),{90,532,105,35},25,white,true);
 text(g,"CURVE",{32,591,84,19},12,faint);
 text(g,"Drag a node to change the step shape.",{29,645,192,28},12,faint);
 const juce::String laneName=selectedLane==0?"VOLUME":selectedLane==1?"FILTER":"PAN";
 text(g,laneName+"   /   STEP "+juce::String(selectedStep+1),{231,531,265,16},12,laneColour,true);
 paintShape(g,detailGraph,selectedStep,true);
 text(g,"Drag nodes to edit. Toggle SNAP for 1/16 increments.",{232,667,496,19},11,faint);
 // Six interactive shape palette buttons (mouseDown hits corresponding rectangles).
 for(int i=0;i<6;++i){
  const int x=734+(i%3)*48,y=552+(i/3)*48;
  auto rr=juce::Rectangle<float>((float)x,(float)y,42.f,40.f);
  g.setColour(juce::Colour(0xff122c3b));g.fillRoundedRectangle(rr,6.f);
  g.setColour(juce::Colour(0xff35667b));g.drawRoundedRectangle(rr,6.f,1.f);
  juce::Path shape;
  for(int j=0;j<=24;++j){
   const float t=j/24.f;
   const float v=i==0?t*t*(3.f-2.f*t):i==1?t:i==2?std::sqrt(t):i==3?(t>.5f?1.f:0.f):i==4?1.f-t:std::abs(std::sin(t*juce::MathConstants<float>::pi));
   if(j==0)shape.startNewSubPath(x+7.f,y+32.f-24.f*v);
   else shape.lineTo(x+7.f+28.f*t,y+32.f-24.f*v);
  }
  g.setColour(i==0?orange:cyan);g.strokePath(shape,juce::PathStrokeType(1.9f));
 }
 text(g,"CURVE SHAPES",{737,654,142,17},11,faint);
 for(int i=0;i<3;++i){
  const int y=505+i*60;
  auto r=juce::Rectangle<float>(913.f,(float)y,272.f,50.f);
  g.setColour(juce::Colour(0xff0b2030));g.fillRoundedRectangle(r,7.f);
  g.setColour(juce::Colour(0xff295369));g.drawRoundedRectangle(r,7.f,1.f);
  const auto colour=i==0?lime:i==1?cyan:juce::Colour(0xffd66bfa);
  g.setColour(selectedLane==i?colour:colour.withAlpha(.5f));
  g.drawRoundedRectangle(juce::Rectangle<float>(917.f,(float)y+5.f,100.f,40.f),7.f,selectedLane==i?2.f:1.f);
  // Native JUCE button overlays the left section, preserving keyboard access.
  juce::Path wave;
  for(int j=0;j<=48;++j){
   const float t=j/48.f;
   const float px=1029.f+120.f*t;
   const float py=y+25.f-11.f*std::sin(t*juce::MathConstants<float>::twoPi*(i+1));
   if(j==0)wave.startNewSubPath(px,py);else wave.lineTo(px,py);
  }
  g.setColour(i==0?orange:i==1?cyan:juce::Colour(0xffd66bfa));g.strokePath(wave,juce::PathStrokeType(2.f));
  text(g,"EDIT",{1150,y+13,41,24},11,faint,true);
 }
 text(g,"LANE: "+laneName,{1230,504,254,20},13,white,true);
 text(g,"OFFSET",{1231,543,95,20},12,faint);
 text(g,"FILTER DEPTH",{1231,592,110,20},12,cyan);
 text(g,"PAN DEPTH",{1231,640,110,20},12,juce::Colour(0xffd66bfa));
 // Always visible frequency controls and output meters.
 paintPanel(g,filterPanel,"FILTER");
 paintPanel(g,outputPanel,"OUTPUT / SIDECHAIN");
 text(g,"MODE",{30,747,70,18},11,faint);
 text(g,"TRIGGER SOURCE",{375,747,170,18},11,faint);
 text(g,"BEAT DIVISION",{594,747,150,18},11,faint);
 text(g,"LOW / HIGH FREQUENCY RESPONSE",{756,745,330,18},11,faint);
 const char* knobs[]={"SENSITIVITY","LENGTH","SMOOTH","LOW HZ","HIGH HZ","RETRIGGER"};
 for(int i=0;i<6;++i)text(g,knobs[i],{29+i*116,866,112,19},11,faint);
 paintFrequencyResponse(g,{755,790,348,106});
 text(g,"DEPTH",{1170,766,110,20},13,white,true);
 text(g,"MIX",{1320,766,110,20},13,white,true);
 text(g,"OUT",{1459,752,57,21},13,white,true);
 const float level=juce::jlimit(0.f,1.f,proc.meter.load(std::memory_order_relaxed));
 for(int ch=0;ch<2;++ch){
  auto meter=juce::Rectangle<float>(1459.f+ch*17.f,786.f,11.f,109.f);
  g.setColour(juce::Colour(0xff061016));g.fillRect(meter);
  g.setColour(cyan);g.fillRect(meter.removeFromBottom(level*meter.getHeight()));
 }
 const juce::StringArray db{"0","-6","-12","-18","-24","-36"};
 for(int i=0;i<db.size();++i)text(g,db[i],{1496,781+i*20,39,16},10,faint);
}
void DISidechainEditor::resized() {
 // All child component bounds use design pixels. The parent editor, graph
 // renderer and pointer handling use the same scale to avoid mismatched hitboxes.
 graph={79,203,1428,157};
 stepStrip={78,380,1430,53};
 detailGraph={230,549,488,110};
 sequencerPanel={12,456,873,240};
 filterPanel={12,706,1110,232};
 outputPanel={1132,706,392,232};
 if(presetButton)presetButton->setBounds(538,30,395,45);
 if(setupButton)setupButton->setBounds(setupBounds);
 if(sequenceToggle)sequenceToggle->setBounds(360,124,137,32);
 if(sequenceCount)sequenceCount->setBounds(506,124,87,30);
 if(combos.size()==3){
  combos[0]->setBounds(0,0,0,0); // filter mode uses premium segment buttons
  combos[1]->setBounds(376,769,204,30);
  combos[2]->setBounds(603,124,120,29);
 }
 if(sync)sync->setBounds(739,123,119,30);
 if(fxTypeBox)fxTypeBox->setBounds(866,124,124,30);
 if(fxToggle)fxToggle->setBounds(1036,123,65,31);
 if(saturatorToggle)saturatorToggle->setBounds(1215,123,65,31);
 if(fxSliders[0])fxSliders[0]->setBounds(1324,123,89,30);
 if(fxSliders[1])fxSliders[1]->setBounds(1443,123,71,30);
 if(sequencePrev)sequencePrev->setBounds(33,533,47,34);
 if(sequenceNext)sequenceNext->setBounds(156,533,47,34);
 if(copyStep)copyStep->setBounds(230,499,75,32);
 if(pasteStep)pasteStep->setBounds(310,499,75,32);
 if(resetStep)resetStep->setBounds(390,499,75,32);
 if(undoButton)undoButton->setBounds(470,499,75,32);
 if(redoButton)redoButton->setBounds(550,499,75,32);
 if(snapButton)snapButton->setBounds(630,499,85,32);
 if(shapePresetButton)shapePresetButton->setBounds(727,499,151,32);
 if(shapeOffset)shapeOffset->setBounds(1320,543,184,31);
 if(laneSliders[0])laneSliders[0]->setBounds(1320,593,184,31);
 if(laneSliders[1])laneSliders[1]->setBounds(1320,640,184,31);
 for(int i=0;i<3;++i)if(laneTabs[i]){
  laneTabs[i]->setBounds(919,510+i*60,101,39);
  laneTabs[i]->setToggleState(i==selectedLane,juce::dontSendNotification);
 }
 for(int i=0;i<4;++i)if(filterButtons[i])filterButtons[i]->setBounds(31+i*78,768,73,30);
 if(sliders.size()==8 && labels.size()==8){
  const int xs[]={1163,1290,28,144,260,376,492,608};
  for(int i=0;i<8;++i){
   labels[i]->setVisible(false); // titles are painted in the aligned premium panel
   sliders[i]->setBounds(xs[i],i<2?791:809,111,110);
  }
 }
 // Scale actual child bounds (including origin), not component-local transforms.
 // A local transform scales each button about its own position and would leave
 // all its x/y coordinates unscaled at 50% or 75%.
 for(int i=0;i<getNumChildComponents();++i) {
  if(auto* child=getChildComponent(i)) {
   child->setTransform(juce::AffineTransform());
   const auto r=child->getBounds();
   child->setBounds(juce::roundToInt(r.getX()*uiScale),
                    juce::roundToInt(r.getY()*uiScale),
                    juce::jmax(1,juce::roundToInt(r.getWidth()*uiScale)),
                    juce::jmax(1,juce::roundToInt(r.getHeight()*uiScale)));
  }
 }
}
void DISidechainEditor::updatePoint(const juce::MouseEvent& e) {
 if(!detailGraph.contains((e.position/uiScale).toInt()))return;
 const auto r=detailGraph;
 const auto point=e.position/uiScale;
 const int i=juce::jlimit(0,7,juce::roundToInt((point.x-r.getX())/juce::jmax(1,r.getWidth())*7.f));
 float v=juce::jlimit(0.f,1.f,(r.getBottom()-point.y)/(float)juce::jmax(1,r.getHeight()));
 if(sequenceToggle->getToggleState()&&selectedLane==0&&shapeOffset)
  v=juce::jlimit(0.f,1.f,v-(float)shapeOffset->getValue());
 if(snapButton&&snapButton->getToggleState())v=juce::jlimit(0.f,1.f,std::round(v*16.f)/16.f);
 if(sequenceToggle->getToggleState()){if(selectedLane==0)proc.setSequencePoint(selectedStep,i,v);else proc.setModulationPoint(selectedLane-1,selectedStep,i,v);}else proc.points[i]=v;
 repaint();
}
void DISidechainEditor::updateDetailPoint(const juce::MouseEvent& e){updatePoint(e);}
void DISidechainEditor::mouseDown(const juce::MouseEvent& e){
 const auto pos=(e.position/uiScale).toInt();
 // Six shape palette shortcuts mirror the dedicated shape preset browser.
 for(int i=0;i<6;++i)if(juce::Rectangle<int>(734+(i%3)*48,552+(i/3)*48,42,40).contains(pos)){
  applyShapePreset(i);return;
 }
 if(juce::Rectangle<int>(24,117,153,43).contains(pos)){sequenceToggle->setToggleState(false,juce::sendNotificationSync);selectedLane=0;repaint();return;}
 if(juce::Rectangle<int>(183,117,155,43).contains(pos)){sequenceToggle->setToggleState(true,juce::sendNotificationSync);repaint();return;}
 if(stepStrip.contains(pos)){
  const int count=juce::jlimit(1,16,(int)sequenceCount->getValue());
  selectedStep=juce::jlimit(0,count-1,(pos.x-stepStrip.getX())*count/juce::jmax(1,stepStrip.getWidth()));
  repaint();return;
 }
 if(detailGraph.contains(pos)){
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
 const char* names[]={"Classic Duck","Deep Pump","Sidechain 4/4","Breakbeat Pump","Pump Rise"};
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


void DISidechainEditor::selectUiScale(int index) {
 index=juce::jlimit(0,2,index);
 if(auto* param=proc.state.getParameter("uiScale")){
  param->beginChangeGesture();
  param->setValueNotifyingHost(param->convertTo0to1((float)index));
  param->endChangeGesture();
 }
 uiScale=index==0?0.5f:index==1?0.75f:1.f;
 setSize(juce::roundToInt(designWidth*uiScale),juce::roundToInt(designHeight*uiScale));
 repaint();
}
void DISidechainEditor::selectRenderQuality(int index) {
 index=juce::jlimit(0,2,index);
 if(auto* param=proc.state.getParameter("renderQuality")){
  param->beginChangeGesture();
  param->setValueNotifyingHost(param->convertTo0to1((float)index));
  param->endChangeGesture();
 }
 renderQuality=index;
 repaint();
}
void DISidechainEditor::showSetupMenu() {
 // A native popup lives on top of the plugin only while open. Nothing is
 // permanently laid over the editor, including when its size is reduced.
 juce::PopupMenu menu;
 juce::PopupMenu scale;
 const int currentScale=uiScale<0.6f?0:uiScale<0.9f?1:2;
 scale.addItem(101,"50%",true,currentScale==0);
 scale.addItem(102,"75%",true,currentScale==1);
 scale.addItem(103,"100%",true,currentScale==2);
 menu.addSubMenu("UI size",scale);
 juce::PopupMenu quality;
 quality.addItem(201,"Economy",true,renderQuality==0);
 quality.addItem(202,"Standard",true,renderQuality==1);
 quality.addItem(203,"High",true,renderQuality==2);
 menu.addSubMenu("Curve rendering quality",quality);
 juce::PopupMenu oversampling;
 oversampling.addItem(301,"2x (current processing mode)",false,true);
 oversampling.addItem(302,"4x (not implemented yet)",false,false);
 menu.addSubMenu("DSP oversampling",oversampling);
 menu.addSeparator();
 menu.addItem(401,"Close settings");
 juce::Component::SafePointer<DISidechainEditor> safe(this);
 menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(setupButton.get()),
  [safe](int chosen){
   if(!safe)return;
   if(chosen>=101&&chosen<=103)safe->selectUiScale(chosen-101);
   else if(chosen>=201&&chosen<=203)safe->selectRenderQuality(chosen-201);
  });
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
    // A preset may contain GUI scale and curve rendering preferences.
    // Apply them to this editor immediately, not only after reopening it.
    if(auto* scale=safe->proc.state.getRawParameterValue("uiScale")) {
     const int index=juce::jlimit(0,2,(int)scale->load());
     safe->uiScale=index==0?.5f:index==1?.75f:1.f;
     safe->setSize(juce::roundToInt(designWidth*safe->uiScale),
                   juce::roundToInt(designHeight*safe->uiScale));
    }
    if(auto* quality=safe->proc.state.getRawParameterValue("renderQuality"))
     safe->renderQuality=juce::jlimit(0,2,(int)quality->load());
    safe->selectedStep=juce::jlimit(0,juce::jlimit(1,16,(int)safe->sequenceCount->getValue())-1,safe->selectedStep);
    safe->presetName=file.getFileNameWithoutExtension();
    safe->presetButton->setButtonText("PRESETS  -  "+safe->presetName);
    safe->repaint();
   }
  });
}
