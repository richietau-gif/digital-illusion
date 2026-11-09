#include "PluginEditor.h"
#include "CurveSelection.h"
#include "CurvePresets.h"
#include <cmath>
#include <limits>

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
  s->setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
  s->setTooltip(juce::String(names[i])+" — drag up/down; double-click to reset.");
  s->textFromValueFunction=[i](double value){
   if(i==0||i==1)return juce::String(value,2);
   if(i==4||i==6)return juce::String(value,0);
   return juce::String(value,1);
  };
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
 shapeOffset->setTextBoxStyle(juce::Slider::TextBoxRight,false,68,20);
 shapeOffset->textFromValueFunction=[](double v){return juce::String(v,2);};
 shapeOffset->setColour(juce::Slider::trackColourId,orange);
 shapeOffset->setColour(juce::Slider::thumbColourId,orange);
 shapeOffset->setColour(juce::Slider::textBoxTextColourId,juce::Colours::white);
 shapeOffset->setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
 addAndMakeVisible(*shapeOffset);
 shapeOffsetAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,"shapeOffset",*shapeOffset);
 const char* laneNames[]={"VOLUME","FILTER","PAN","REVERB","DISTORT"};
 for(int i=0;i<5;++i){
  laneTabs[i]=std::make_unique<juce::TextButton>(laneNames[i]);
  buttonStyle(*laneTabs[i]);
  laneTabs[i]->onClick=[this,i]{
   if(i>0&&!sequenceToggle->getToggleState())sequenceToggle->setToggleState(true,juce::sendNotificationSync);
   selectedLane=i;
   if(shapeOffset)shapeOffset->setEnabled(selectedLane==0);
   for(int j=0;j<5;++j)laneTabs[j]->setToggleState(j==selectedLane,juce::dontSendNotification);
   refreshCreativePage();repaint();
  };
  addAndMakeVisible(*laneTabs[i]);
 }
 // Per-cell routing: duck volume, reverb send, distortion send, pre-reverb
 // reverse, and an absolute bypass. Unlike global rack switches, these flags
 // belong to one selected track/step in one scene.
 const char* cellLabels[]={"DUCK","RVB","DST","REV <","BYP"};
 for(int i=0;i<5;++i){
  cellControls[i]=std::make_unique<juce::TextButton>(cellLabels[i]);
  buttonStyle(*cellControls[i]);
  cellControls[i]->setClickingTogglesState(true);
  addAndMakeVisible(*cellControls[i]);
  cellControls[i]->onClick=[this,i]{
   if(i==0)proc.setGridCell(selectedTrack,selectedStep,!proc.getGridCell(selectedTrack,selectedStep));
   else proc.setCellFxFlag(selectedTrack,selectedStep,i==4?3:i-1,
                           !proc.getCellFxFlag(selectedTrack,selectedStep,i==4?3:i-1));
   // FX selection turns the step on automatically. Reverse also selects
   // the reverb send so a whoosh isn't silently configured but inaudible.
   if(i==3&&proc.getCellFxFlag(selectedTrack,selectedStep,2))
    proc.setCellFxFlag(selectedTrack,selectedStep,0,true);
   if(i>=1&&i<=3&&proc.getCellFxFlag(selectedTrack,selectedStep,i==3?2:i-1))
    proc.setGridCell(selectedTrack,selectedStep,true);
   if(i==1&&proc.getCellFxFlag(selectedTrack,selectedStep,0)&&reverbOnButton&&!reverbOnButton->getToggleState())
    reverbOnButton->setToggleState(true,juce::sendNotificationSync);
   if(i==2&&proc.getCellFxFlag(selectedTrack,selectedStep,1)&&distortOnButton&&!distortOnButton->getToggleState())
    distortOnButton->setToggleState(true,juce::sendNotificationSync);
   if(i==3&&proc.getCellFxFlag(selectedTrack,selectedStep,2)&&reverbOnButton&&!reverbOnButton->getToggleState())
    reverbOnButton->setToggleState(true,juce::sendNotificationSync);
   refreshCellControls();repaint();
  };
 }

 const char* laneIds[]={"filterMod","panMod"};
 for(int i=0;i<2;++i){
  laneSliders[i]=std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal,juce::Slider::TextBoxRight);
  laneSliders[i]->setTextBoxStyle(juce::Slider::TextBoxRight,false,63,20);
  laneSliders[i]->textFromValueFunction=[](double v){return juce::String(v,2);};
  laneSliders[i]->setColour(juce::Slider::trackColourId,i==0?juce::Colour(0xff44bbff):juce::Colour(0xffc55bff));
  laneSliders[i]->setColour(juce::Slider::thumbColourId,i==0?juce::Colour(0xff44bbff):juce::Colour(0xffc55bff));
  laneSliders[i]->setColour(juce::Slider::textBoxTextColourId,juce::Colours::white);
  laneSliders[i]->setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
  addAndMakeVisible(*laneSliders[i]);
  laneAttachments[i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,laneIds[i],*laneSliders[i]);
 }
 fxTypeBox=std::make_unique<juce::ComboBox>();
 fxTypeBox->addItemList(juce::StringArray{"Saturator","Soft Clipper"},1);
 fxTypeBox->setTooltip("Legacy FX type selector; independent SATURATOR is controlled by its own ON/OFF button.");
 fxTypeBox->setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff212b36));
 fxTypeBox->setColour(juce::ComboBox::textColourId,juce::Colours::white);
 fxTypeBox->setColour(juce::ComboBox::outlineColourId,edge);
 addAndMakeVisible(*fxTypeBox);
 fxTypeAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,"fxType",*fxTypeBox);
 fxTypeBox->setVisible(false); // retained only for legacy state/automation compatibility
 clipperButton=std::make_unique<juce::TextButton>("CLIP OFF");
 clipperButton->setTooltip("Clipper: Off, Soft, Hard. Click to cycle.");
 buttonStyle(*clipperButton);addAndMakeVisible(*clipperButton);
 clipperButton->onClick=[this]{
  if(auto* param=proc.state.getParameter("clipperMode")){
   const int current=juce::jlimit(0,2,(int)proc.state.getRawParameterValue("clipperMode")->load());
   param->beginChangeGesture();param->setValueNotifyingHost(param->convertTo0to1((float)((current+1)%3)));param->endChangeGesture();
  }
 };
 saturatorDriveSlider=std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal,juce::Slider::TextBoxRight);
 saturatorDriveSlider->setRange(0.0,24.0,0.1);
 saturatorDriveSlider->setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
 saturatorDriveSlider->setTooltip("Saturator drive 0-24 dB");
 saturatorDriveSlider->setTextBoxStyle(juce::Slider::TextBoxRight,false,59,20);
 saturatorDriveSlider->textFromValueFunction=[](double v){return juce::String(v,1);};saturatorDriveSlider->setTextValueSuffix(" dB");
 saturatorDriveSlider->setColour(juce::Slider::trackColourId,green);saturatorDriveSlider->setColour(juce::Slider::thumbColourId,green);
 addAndMakeVisible(*saturatorDriveSlider);
 saturatorDriveAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,"saturatorDrive",*saturatorDriveSlider);
 fxToggle=std::make_unique<juce::TextButton>("OFF");
 fxToggle->setTooltip("Toggle FX processing. The separate Saturator can run at the same time.");
 addAndMakeVisible(*fxToggle);
 fxToggleAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"fxEnabled",*fxToggle);
 saturatorToggle=std::make_unique<juce::TextButton>("OFF");
 saturatorToggle->setTooltip("Independent 2x oversampled saturator; can be combined with FX.");
 addAndMakeVisible(*saturatorToggle);
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
 presetName=p.state.state.getProperty("presetName","Custom").toString();
 presetButton=std::make_unique<juce::TextButton>("PRESETS  -  "+presetName);
 for(int i=0;i<2;++i){
  abButtons[i]=std::make_unique<juce::TextButton>(i==0?"A":"B");
  buttonStyle(*abButtons[i]);
  addAndMakeVisible(*abButtons[i]);
  abButtons[i]->onClick=[this,i]{ if(abStored[i]) recallAB(i); else captureAB(i); };
 }
 const char* copyLabels[] = {"A→B","B→A"};
 for(int i=0;i<2;++i){
  abCopyButtons[i]=std::make_unique<juce::TextButton>(copyLabels[i]);
  buttonStyle(*abCopyButtons[i]); addAndMakeVisible(*abCopyButtons[i]);
  abCopyButtons[i]->setTooltip(i==0?"Copy snapshot A to B":"Copy snapshot B to A");
  abCopyButtons[i]->onClick=[this,i]{copyAB(i==0?0:1,i==0?1:0);};
 }
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
  selectedStep=(selectedStep+count-1)%count;refreshCellControls();repaint();
 };
 sequenceNext->onClick=[this]{
  const int count=juce::jlimit(1,16,(int)sequenceCount->getValue());
  selectedStep=(selectedStep+1)%count;refreshCellControls();repaint();
 };
 const char* fxIds[]={"fxDrive","fxMix"};
 for(int i=0;i<2;++i){
  fxSliders[i]=std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal,juce::Slider::NoTextBox);
  fxSliders[i]->setTooltip(i==0?"Legacy FX drive dB": "FX wet mix, 0% dry to 100% wet");
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
  clipboard=captureShape(currentShapeKey(),seq);
  hasClipboard=true;
 };
 pasteStep->onClick=[this]{
  if(!hasClipboard)return;
  const bool seq=sequenceToggle->getToggleState();
  const int key=currentShapeKey();
  const auto before=captureShape(key,seq);
  applyShape(key,seq,clipboard);
  pushEdit(key,seq,before,clipboard);repaint();
 };
 resetStep->onClick=[this]{
  const bool seq=sequenceToggle->getToggleState();
  const int key=seq?selectedStep+selectedLane*32:0;
  const auto before=captureShape(key,seq);
  constexpr di::ShapeSequencer::Shape volume{1.f,.92f,.08f,.02f,.32f,.75f,.95f,1.f};
  const bool grid=seq&&proc.state.getRawParameterValue("creativeGridEnabled")->load()>0.5f;
  const di::ShapeSequencer::Shape defaults=grid&&selectedLane==0
    ? di::ShapeSequencer::Shape{0.f,.02f,.2f,.65f,.9f,1.f,1.f,1.f}
    : seq&&selectedLane>0
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
 // Reverb and distortion settings remain independent of legacy FX controls.
 const char* toggleIds[]={"creativeGridEnabled","reverbEnabled","distortEnabled","reverbReverse"};
 std::unique_ptr<juce::TextButton>* toggleButtons[]={&gridModeButton,&reverbOnButton,&distortOnButton,&reverseButton};
 const char* toggleLabels[]={"GRID ON","REVERB ON","DISTORT ON","REVERSE  <"};
 for(int i=0;i<4;++i){
  *toggleButtons[i]=std::make_unique<juce::TextButton>(toggleLabels[i]);
  (*toggleButtons[i])->setClickingTogglesState(true);
  (*toggleButtons[i])->setColour(juce::TextButton::buttonColourId,juce::Colour(0xff172d3b));
  (*toggleButtons[i])->setColour(juce::TextButton::buttonOnColourId,green);
  (*toggleButtons[i])->setColour(juce::TextButton::textColourOffId,juce::Colours::white);
  (*toggleButtons[i])->setColour(juce::TextButton::textColourOnId,bg);
  addAndMakeVisible(**toggleButtons[i]);
  creativeButtonAttachments[i]=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,toggleIds[i],**toggleButtons[i]);
 }
 const char* modelIds[]={"reverbModel","distortModel"};
 const juce::StringArray modelNames[]={{"Room","Plate","Hall","Ambient","Space"},
                                        {"Warm","Acid","Clip","Fold","Modern"}};
 for(int i=0;i<2;++i){
  creativeModelBoxes[i]=std::make_unique<juce::ComboBox>();
  creativeModelBoxes[i]->addItemList(modelNames[i],1);
  addAndMakeVisible(*creativeModelBoxes[i]);
  creativeModelAttachments[i]=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,modelIds[i],*creativeModelBoxes[i]);
 }
 const char* sliderIds[]={"reverbWet","reverbLength","reverbDecay","distortWet","distortDrive","reverbPreDelay","distortTone"};
 for(int i=0;i<7;++i){
  creativeSliders[i]=std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal,juce::Slider::TextBoxRight);
  creativeSliders[i]->setTextBoxStyle(juce::Slider::TextBoxRight,false,68,22);
  creativeSliders[i]->textFromValueFunction=[i](double v){return juce::String(v,(i==4||i==5)?1:2);};
  creativeSliders[i]->setColour(juce::Slider::trackColourId,(i<3||i==5)?juce::Colour(0xff24def0):orange);
  creativeSliders[i]->setColour(juce::Slider::thumbColourId,(i<3||i==5)?juce::Colour(0xff24def0):orange);
  creativeSliders[i]->setColour(juce::Slider::textBoxTextColourId,juce::Colours::white);
  creativeSliders[i]->setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
  addAndMakeVisible(*creativeSliders[i]);
  creativeSliderAttachments[i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,sliderIds[i],*creativeSliders[i]);
 }
 for(int i=0;i<4;++i){
  sceneButtons[i]=std::make_unique<juce::TextButton>(juce::String("ABCD").substring(i,i+1));
  buttonStyle(*sceneButtons[i]);
  sceneButtons[i]->setClickingTogglesState(true);
  addAndMakeVisible(*sceneButtons[i]);
  sceneButtons[i]->onClick=[this,i]{
   proc.setGridScene(i);
   refreshCellControls();
   for(int j=0;j<4;++j)if(sceneButtons[j])sceneButtons[j]->setToggleState(j==i,juce::dontSendNotification);
   repaint();
  };
 }
 sceneButtons[0]->setToggleState(true,juce::dontSendNotification);
 copySceneButton=std::make_unique<juce::TextButton>("COPY TO...");
 buttonStyle(*copySceneButton);
 addAndMakeVisible(*copySceneButton);
 copySceneButton->onClick=[this]{
  juce::PopupMenu menu;
  const int source=proc.getGridScene();
  for(int i=0;i<4;++i)if(i!=source)menu.addItem(i+1,"Copy to Scene "+juce::String("ABCD").substring(i,i+1));
  juce::Component::SafePointer<DISidechainEditor> safe(this);
  menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(copySceneButton.get()),[safe,source](int selected){
   if(safe&&selected>=1&&selected<=4){safe->proc.copyGridScene(source,selected-1);safe->repaint();}
  });
 };
 duckDepthSlider=std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal,juce::Slider::TextBoxRight);
 duckDepthSlider->setRange(0.,1.,.01);
 duckDepthSlider->setTextBoxStyle(juce::Slider::TextBoxRight,false,60,20);
 duckDepthSlider->textFromValueFunction=[](double v){return juce::String(v,2);};
 duckDepthSlider->setValue(proc.getDuckTrackDepth(0),juce::dontSendNotification);
 duckDepthSlider->setColour(juce::Slider::trackColourId,green);
 duckDepthSlider->setColour(juce::Slider::thumbColourId,green);
 duckDepthSlider->setColour(juce::Slider::textBoxTextColourId,juce::Colours::white);
 duckDepthSlider->onValueChange=[this]{
  if(selectedTrack<6)proc.setDuckTrackDepth(selectedTrack,(float)duckDepthSlider->getValue());
 };
 addAndMakeVisible(*duckDepthSlider);
 duckLengthSlider=std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal,juce::Slider::TextBoxRight);
 duckLengthSlider->setRange(.05,4.,.05);
 duckLengthSlider->setTextBoxStyle(juce::Slider::TextBoxRight,false,60,20);
 duckLengthSlider->textFromValueFunction=[](double v){return juce::String(v,2);};
 duckLengthSlider->setValue(proc.getDuckTrackLength(0),juce::dontSendNotification);
 duckLengthSlider->setColour(juce::Slider::trackColourId,orange);
 duckLengthSlider->setColour(juce::Slider::thumbColourId,orange);
 duckLengthSlider->setColour(juce::Slider::textBoxTextColourId,juce::Colours::white);
 duckLengthSlider->onValueChange=[this]{
  if(selectedTrack<6)proc.setDuckTrackLength(selectedTrack,(float)duckLengthSlider->getValue());
 };
 addAndMakeVisible(*duckLengthSlider);
 renderQuality=juce::jlimit(0,2,(int)p.state.getRawParameterValue("renderQuality")->load());
 const int scaleIndex=juce::jlimit(0,2,(int)p.state.getRawParameterValue("uiScale")->load());
 uiScale=scaleIndex==0?.5f:scaleIndex==1?.75f:1.f;
 setSize(juce::roundToInt(designWidth*uiScale),juce::roundToInt(designHeight*uiScale));
 startTimerHz(24);
}
int DISidechainEditor::currentShapeKey() const {
 const bool seq=sequenceToggle&&sequenceToggle->getToggleState();
 const bool grid=seq&&proc.state.getRawParameterValue("creativeGridEnabled")->load()>0.5f;
 return di::resolveCurveKey(seq,grid,selectedLane,selectedTrack,selectedStep);
}
di::ShapeSequencer::Shape DISidechainEditor::captureShape(int step,bool sequence) const {
 di::ShapeSequencer::Shape shape{};
 if(sequence&&step>=96&&step<102){
  for(int i=0;i<8;++i)shape[i]=proc.getDuckCurvePoint(step-96,i);
  return shape;
 }
 const int lane=step/32, actualStep=step%32;
 for(int i=0;i<8;++i)shape[i]=sequence?(lane==0?proc.getSequencePoint(actualStep,i):proc.getModulationPoint(lane-1,actualStep,i)):proc.points[i].load();
 return shape;
}
void DISidechainEditor::applyShape(int step,bool sequence,const di::ShapeSequencer::Shape& shape) {
 if(sequence&&step>=96){
  for(int i=0;i<8;++i)proc.setDuckCurvePoint(step-96,i,shape[i]);
  return;
 }
 const int lane=step/32, actualStep=step%32;
 for(int i=0;i<8;++i){if(sequence){if(lane==0)proc.setSequencePoint(actualStep,i,shape[i]);else proc.setModulationPoint(lane-1,actualStep,i,shape[i]);}else if(std::isfinite(shape[i]))proc.points[i].store(juce::jlimit(0.f,1.f,shape[i]),std::memory_order_relaxed);}
}
void DISidechainEditor::pushDynamicEdit(int track,const di::DynamicCurve& before,const di::DynamicCurve& after) {
 if(track<0||track>=6)return;
 if(undoStack.size()>=64)undoStack.erase(undoStack.begin());
 ShapeEdit e{};e.step=96+track;e.sequence=true;e.dynamic=true;
 e.beforeDynamic=before;e.afterDynamic=after;
 undoStack.push_back(e);redoStack.clear();
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
 if(edit.dynamic)proc.setDynamicDuckCurve(edit.step-96,edit.beforeDynamic);
 else applyShape(edit.step,edit.sequence,edit.before);
 redoStack.push_back(edit);repaint();
}
void DISidechainEditor::redoEdit(){
 if(redoStack.empty())return;
 const auto edit=redoStack.back();redoStack.pop_back();
 if(edit.dynamic)proc.setDynamicDuckCurve(edit.step-96,edit.afterDynamic);
 else applyShape(edit.step,edit.sequence,edit.after);
 undoStack.push_back(edit);repaint();
}
void DISidechainEditor::selectFilter(int index) {
 if(auto* p=proc.state.getParameter("mode")){
  const float normalized=p->convertTo0to1((float)juce::jlimit(0,3,index));
  p->beginChangeGesture();p->setValueNotifyingHost(normalized);p->endChangeGesture();
 }
 refreshFilterButtons();
}
void DISidechainEditor::updateContextualControls(){
 const auto raw=[this](const char* key)->float {
  const auto* p=proc.state.getRawParameterValue(key);
  return p?p->load(std::memory_order_relaxed):0.f;
 };
 const int mode=juce::jlimit(0,3,(int)raw("mode"));
 const int trigger=juce::jlimit(0,2,(int)raw("trigger"));
 const bool synced=raw("sync")>.5f;
 // FULL mode has no band cutoff; LOW uses high crossover,
 // HIGH uses low crossover; BAND uses both crossovers.
 const bool lowEnabled=mode!=0;
 const bool highEnabled=mode==3;
 const bool audioTrigger=trigger!=2;
 if(sliders.size()>=8){
  sliders[3]->setEnabled(!synced); // free-running envelope length
  sliders[5]->setEnabled(lowEnabled);
  sliders[6]->setEnabled(highEnabled);
  sliders[2]->setEnabled(audioTrigger);
  sliders[7]->setEnabled(true); // retrigger holdoff also applies to MIDI notes
 }
 if(combos.size()>=3)combos[2]->setEnabled(synced);
 if(sync)sync->setEnabled(true);
}
void DISidechainEditor::refreshFilterButtons(){
 const auto* raw=proc.state.getRawParameterValue("mode");
 const int selected=raw?juce::jlimit(0,3,(int)raw->load()):0;
 for(int i=0;i<4;++i)if(filterButtons[i])filterButtons[i]->setToggleState(i==selected,juce::dontSendNotification);
 updateContextualControls();
}
void DISidechainEditor::refreshFxButtons() {
 if(fxToggle)fxToggle->setButtonText(fxToggle->getToggleState()?"ON":"OFF");
 if(saturatorToggle)saturatorToggle->setButtonText(saturatorToggle->getToggleState()?"SAT ON":"SAT OFF");
 if(clipperButton){
  const int mode=juce::jlimit(0,2,(int)proc.state.getRawParameterValue("clipperMode")->load());
  clipperButton->setButtonText(mode==0?"CLIP OFF":mode==1?"CLIP SOFT":"CLIP HARD");
  clipperButton->setColour(juce::TextButton::buttonColourId,mode==0?juce::Colour(0xff25303d):green);
  clipperButton->setColour(juce::TextButton::textColourOffId,mode==0?juce::Colours::white:DITheme::dark());
 }
}
void DISidechainEditor::syncUiPreferences() {
 // Host state restore may occur while an existing editor stays open.
 const auto restoredName=proc.state.state.getProperty("presetName","Custom").toString();
 if(restoredName!=presetName){
  presetName=restoredName;
  if(presetButton)presetButton->setButtonText("PRESETS  -  "+presetName);
 }
 if(auto* scale=proc.state.getRawParameterValue("uiScale")) {
  const int index=juce::jlimit(0,2,(int)scale->load(std::memory_order_relaxed));
  const float target=index==0?.5f:index==1?.75f:1.f;
  if(std::abs(uiScale-target)>0.001f){
   uiScale=target;
   setSize(juce::roundToInt(designWidth*uiScale),
           juce::roundToInt(designHeight*uiScale));
  }
 }
 if(auto* quality=proc.state.getRawParameterValue("renderQuality"))
  renderQuality=juce::jlimit(0,2,(int)quality->load(std::memory_order_relaxed));
}
void DISidechainEditor::paintPanel(juce::Graphics& g,juce::Rectangle<int> r,const juce::String& name) {
 const auto f=r.toFloat();
 g.setColour(juce::Colour(0xff030b12).withAlpha(.5f));
 g.fillRoundedRectangle(f.translated(1.f,3.f),12.f);
 juce::ColourGradient body(juce::Colour(0xff141a24),f.getX(),f.getY(),
                           juce::Colour(0xff0a0e15),f.getRight(),f.getBottom(),false);
 g.setGradientFill(body);g.fillRoundedRectangle(f,11.f);
 g.setColour(juce::Colour(0xff2b4651));g.drawRoundedRectangle(f.reduced(.5f),11.f,1.f);
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
 if(seq&&selectedLane==0&&selectedTrack>=0&&selectedTrack<6&&
    proc.state.getRawParameterValue("creativeGridEnabled")->load()>.5f){
  const auto dc=proc.getDynamicDuckCurve(selectedTrack);
  const auto colour=DITheme::lime();
  juce::Path curve;
  const int n=renderQuality==2?360:renderQuality==0?65:170;
  for(int j=0;j<=n;++j){
   const float t=(float)j/n;
   const float px=r.getX()+r.getWidth()*t;
   const float py=r.getBottom()-dc.evaluate(t)*r.getHeight();
   if(j==0)curve.startNewSubPath(px,py);else curve.lineTo(px,py);
  }
  juce::Path fill=curve;fill.lineTo((float)r.getRight(),(float)r.getBottom());
  fill.lineTo((float)r.getX(),(float)r.getBottom());fill.closeSubPath();
  g.setColour(colour.withAlpha(.15f));g.fillPath(fill);
  g.setColour(colour);g.strokePath(curve,juce::PathStrokeType(drawNodes?2.5f:1.5f));
  if(drawNodes)for(int i=0;i<dc.count;++i){
   const float x=r.getX()+r.getWidth()*dc.x[(size_t)i];
   const float y=r.getBottom()-r.getHeight()*dc.y[(size_t)i];
   g.setColour(juce::Colour(0xff0b151a));g.fillEllipse(x-7.f,y-7.f,14.f,14.f);
   g.setColour(colour);g.drawEllipse(x-6.f,y-6.f,12.f,12.f,2.f);
  }
  return;
 }
 const auto laneColour=selectedLane==0?DITheme::lime():selectedLane==1?juce::Colour(0xff44bbff):juce::Colour(0xffc55bff);
 const float offset=seq&&selectedLane==0&&shapeOffset? (float)shapeOffset->getValue():0.f;
 auto value=[&](int i){
  if(seq&&selectedLane==0&&selectedTrack<6&&proc.state.getRawParameterValue("creativeGridEnabled")->load()>0.5f)
   return proc.getDuckCurvePoint(selectedTrack,i);
  return seq?(selectedLane==0?proc.getSequencePoint(step,i):proc.getModulationPoint(selectedLane-1,step,i)):proc.points[i].load();
 };
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
 g.fillAll(juce::Colour(0xff06080c));
 g.addTransform(juce::AffineTransform::scale(uiScale));
 const auto white=juce::Colour(0xffecf8ff);
 const auto faint=juce::Colour(0xff9bb6c7);
 const auto cyan=DITheme::cyan();
 const auto lime=DITheme::lime();
 const auto orange=DITheme::orange();
 // Polished header with an original waveform logo, entirely vector-rendered.
 juce::ColourGradient top(juce::Colour(0xff151e29),0.f,0.f,juce::Colour(0xff080b11),0.f,102.f,false);
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
 text(g,"DI SIDEFX", {117,30,280,39},32,white,true);
 text(g,"5.3",{365,30,100,39},27,orange,true);
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
 text(g,"LENGTH / RATE",{630,110,135,14},11,faint);
 text(g,"DRIVE",{1283,110,72,14},11,faint);
 text(g,"WET",{1415,110,60,14},11,faint);
 text(g,"SCENES",{784,180,83,20},12,faint,true);
 // Shape overview.
 paintPanel(g,{12,177,1512,269},"");
 text(g,"EVENTS",{19,203,59,18},10,faint,true);
 const bool sequenced=sequenceToggle->getToggleState();
 const int count=juce::jlimit(1,16,(int)sequenceCount->getValue());
 if(sequenced) {
  // Only six musical trigger tracks. All effects live in the modulation rack,
  // with routing flags belonging to every individual track/step cell.
  const int active=proc.activeSequenceStep.load(std::memory_order_relaxed);
  constexpr int labelWidth=116;
  const int left=graph.getX()+labelWidth;
  const int available=graph.getRight()-left;
  const int top=graph.getY()+23;
  const float rowHeight=(graph.getBottom()-top)/6.f;
  for(int step=0;step<count;++step){
   const int x0=left+step*available/count,x1=left+(step+1)*available/count;
   text(g,juce::String(step+1),{x0+5,graph.getY()+1,x1-x0-10,18},11,
        step==active?lime:faint,step==selectedStep);
   if(step==active){g.setColour(lime.withAlpha(.12f));g.fillRect(x0,top,x1-x0,graph.getBottom()-top);}
  }
  for(int track=0;track<6;++track){
   const int y=top+juce::roundToInt(track*rowHeight);
   const int nextY=top+juce::roundToInt((track+1)*rowHeight);
   const int h=juce::jmax(1,nextY-y-1);
   g.setColour(track==selectedTrack?juce::Colour(0xff244555):juce::Colour(0xff141d29));
   g.fillRoundedRectangle(juce::Rectangle<float>((float)graph.getX(),(float)y,(float)labelWidth-4,(float)h),3.f);
   text(g,"TRACK "+juce::String(track+1),{graph.getX()+7,y,labelWidth-12,h},12,
        track==selectedTrack?lime:white,track==selectedTrack);
   for(int step=0;step<count;++step){
    const int x0=left+step*available/count,x1=left+(step+1)*available/count;
    const auto cell=juce::Rectangle<float>((float)x0+2,(float)y+1,
        (float)juce::jmax(1,x1-x0-4),(float)juce::jmax(1,h-2));
    const bool on=proc.getGridCell(track,step);
    const bool bypass=proc.getCellFxFlag(track,step,3);
    const bool rvb=proc.getCellFxFlag(track,step,0);
    const bool dist=proc.getCellFxFlag(track,step,1);
    const bool reverse=proc.getCellFxFlag(track,step,2);
    g.setColour(!on?juce::Colour(0xff26303b):
        bypass?juce::Colour(0xff324350):lime);
    g.fillRoundedRectangle(cell,3.f);
    if(on&&bypass){
     g.setColour(orange.withAlpha(.85f));
     g.drawLine(cell.getX()+3,cell.getBottom()-3,cell.getRight()-3,cell.getY()+3,1.5f);
    }
    if(on&&!bypass){
     juce::Path miniature;
     for(int node=0;node<8;++node){
      const float v=proc.getDuckCurvePoint(track,node);
      const float px=cell.getX()+5.f+(cell.getWidth()-10.f)*node/7.f;
      const float py=cell.getBottom()-2.f-v*(cell.getHeight()*.32f);
      if(node==0)miniature.startNewSubPath(px,py);
      else miniature.lineTo(px,py);
     }
     g.setColour(juce::Colour(0xff06121b).withAlpha(.8f));
     g.strokePath(miniature,juce::PathStrokeType(1.f));
    }
    if(track==selectedTrack&&step==selectedStep){
     g.setColour(orange);g.drawRoundedRectangle(cell,3.f,1.8f);
    }
    // Small status chips make the active modulation audible at a glance.
    int flagX=(int)cell.getRight()-5;
    const auto flag=[&](bool enabled,const juce::String& caption,juce::Colour colour){
     if(!enabled)return;
     flagX-=16;
     g.setColour(colour);
     g.fillRoundedRectangle(juce::Rectangle<float>((float)flagX,(float)cell.getY()+3,14.f,14.f),2.f);
     text(g,caption,{flagX+2,(int)cell.getY()+3,12,14},9,DITheme::dark(),true);
    };
    flag(reverse&&rvb,"<",orange);
    flag(dist,"D",juce::Colour(0xffff9554));
    flag(rvb,"R",cyan);
    if(bypass)flag(true,"X",orange);
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
  if(sequenced) {
   const bool event=proc.getGridCell(selectedTrack,i);
   const auto colour=lime;
   g.setColour(event?colour:juce::Colour(0xff223744));
   g.fillRoundedRectangle(juce::Rectangle<float>(r.getX()+5.f,r.getY()+22.f,
                                                  r.getWidth()-10.f,19.f),3.f);
   if(event){
    juce::String tags;
    if(proc.getCellFxFlag(selectedTrack,i,0))tags+="R ";
    if(proc.getCellFxFlag(selectedTrack,i,1))tags+="D ";
    if(proc.getCellFxFlag(selectedTrack,i,2))tags+="< ";
    if(proc.getCellFxFlag(selectedTrack,i,3))tags+="OFF";
    if(tags.isNotEmpty())text(g,tags,r.toNearestInt().reduced(6,3).withTrimmedTop(20),10,DITheme::dark(),true);
   }
  } else {
   juce::Path miniature;
   for(int node=0;node<8;++node) {
    const float value=selectedLane==0?proc.getSequencePoint(i,node):proc.getModulationPoint(selectedLane-1,i,node);
    const float px=r.getX()+5.f+(r.getWidth()-10.f)*node/7.f;
    const float py=r.getBottom()-5.f-value*(r.getHeight()-25.f);
    if(node==0)miniature.startNewSubPath(px,py);else miniature.lineTo(px,py);
   }
   g.setColour(laneColour);g.strokePath(miniature,juce::PathStrokeType(1.8f));
  }
 }
 // STEP EDIT (left), five modulation lanes (middle), CELL / TRACK (right).
 paintPanel(g,sequencerPanel,selectedLane==3?"REVERB":selectedLane==4?"DISTORTION":"STEP ENVELOPE");
 paintPanel(g,{897,456,305,240},"MODULATION LANES");
 paintPanel(g,{1212,456,312,240},"CELL / TRACK");
 const juce::Colour lanes[]={lime,cyan,juce::Colour(0xffd16afb),cyan,orange};
 for(int i=0;i<5;++i){
  const int y=497+i*36;
  auto r=juce::Rectangle<float>(910.f,(float)y,278.f,31.f);
  g.setColour(selectedLane==i?juce::Colour(0xff1b4251):juce::Colour(0xff101a25));
  g.fillRoundedRectangle(r,5.f);
  g.setColour(lanes[i].withAlpha(selectedLane==i?1.f:.48f));
  g.drawRoundedRectangle(r,5.f,selectedLane==i?1.6f:.7f);
  juce::Path wave;
  for(int j=0;j<=24;++j){
   const float t=j/24.f;
   const float modulation=i==3?std::exp(-3.f*t):
                          i==4?std::sin(12.f*t)*std::exp(-2.f*t):
                          std::sin(t*juce::MathConstants<float>::twoPi*(i+1));
   const float py=y+16.f-8.f*modulation;
   if(j==0)wave.startNewSubPath(1029.f,py);
   else wave.lineTo(1029.f+125.f*t,py);
  }
  g.setColour(lanes[i]);g.strokePath(wave,juce::PathStrokeType(1.65f));
 }
 const bool fxPage=sequenced&&selectedLane>=3;
 if(!fxPage){
  const juce::String laneName=selectedLane==0?"DUCK ENVELOPE":selectedLane==1?"FILTER":"PAN";
  text(g,juce::String("TRACK ")+juce::String(selectedTrack+1)+"  /  "+laneName,
       {28,477,460,18},13,white,true);
  text(g,juce::String(selectedStep+1)+" / "+juce::String(count),{32,523,150,29},21,orange,true);
  text(g,"CURVE",{34,590,76,18},11,faint);
  paintShape(g,detailGraph,selectedStep,true);
  text(g,"Drag points to edit. Shape presets apply to the visible curve.",
       {232,665,460,18},11,faint);
  for(int i=0;i<9;++i){
   const int x=734+(i%3)*48,y=552+(i/3)*42;
   const auto r=juce::Rectangle<float>((float)x,(float)y,42.f,37.f);
   g.setColour(juce::Colour(0xff122c3b));g.fillRoundedRectangle(r,5.f);
   g.setColour(juce::Colour(0xff35667b));g.drawRoundedRectangle(r,5.f,1.f);
   juce::Path shape;
   for(int j=0;j<=24;++j){
    const float t=j/24.f;
    const float v=di::sampleCurvePreset(i,t);
    if(j==0)shape.startNewSubPath(x+7.f,y+30.f-23.f*v);
    else shape.lineTo(x+7.f+28.f*t,y+30.f-23.f*v);
   }
   g.setColour(i==0?lime:cyan);
   g.strokePath(shape,juce::PathStrokeType(1.8f));
  }
 }else{
  const bool verb=selectedLane==3;
  text(g,verb?"REVERB · SEND / TAIL / REVERSE":"DISTORT · 2x OVERSAMPLING",
       {30,479,480,22},14,verb?cyan:orange,true);
  text(g,verb?"MODEL":"WAVESHAPER MODEL",{326,499,215,18},11,faint);
  if(verb){
   text(g,"DRY / WET",{39,549,210,18},12,cyan,true);
   text(g,"LENGTH",{324,549,210,18},12,cyan,true);
   text(g,"DECAY",{610,549,190,18},12,cyan,true);
   text(g,"PREDELAY ms",{39,610,225,17},11,cyan,true);
   text(g,"The selected cell can reverse its captured input before reverb.",
        {39,648,800,18},11,faint);
  }else{
   text(g,"DRY / WET",{39,549,210,18},12,orange,true);
   text(g,"DRIVE dB",{327,549,210,18},12,orange,true);
   text(g,"TONE",{39,610,210,17},11,orange,true);
   text(g,"5 drive types: Warm / Acid / Clip / Fold / Modern",
        {39,648,780,18},11,faint);
  }
 }
 text(g,"TRACK "+juce::String(selectedTrack+1)+"  ·  STEP "+juce::String(selectedStep+1),
      {1225,480,278,21},12,white,true);
 text(g,"CELL ROUTING  ·  DUCK / FX / BYPASS",{1226,499,279,17},10,faint);
 text(g,"DUCK DEPTH",{1226,567,225,16},11,lime,true);
 text(g,"ENVELOPE LENGTH (STEPS)",{1226,619,264,16},11,orange,true);
 text(g,"BYP skips ducking; R/D remain active.",
      {1226,671,284,15},10,faint);
 // Always visible frequency controls and output meters.
 paintPanel(g,filterPanel,"FILTER");
 paintPanel(g,outputPanel,"OUTPUT / SIDECHAIN");
 text(g,"MODE",{30,747,70,18},11,faint);
 text(g,"TRIGGER SOURCE",{375,747,170,18},11,faint);
 text(g,"BEAT DIVISION",{594,747,150,18},11,faint);
 text(g,"LOW / HIGH FREQUENCY RESPONSE",{756,745,330,18},11,faint);
 const char* knobs[]={"SENSITIVITY","LENGTH","SMOOTH","LOW HZ","HIGH HZ","RETRIGGER"};
 for(int i=0;i<6;++i){
  const bool enabled=sliders.size()>static_cast<size_t>(i+2)&&sliders[(size_t)(i+2)]->isEnabled();
  text(g,knobs[i],{29+i*116,805,112,19},11,enabled?faint:faint.withAlpha(.32f));
 }
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
void DISidechainEditor::refreshCellControls(){
 const bool seq=sequenceToggle&&sequenceToggle->getToggleState();
 for(int i=0;i<5;++i)if(cellControls[i]){
  cellControls[i]->setVisible(seq&&selectedTrack<6);
  const bool active=i==0?proc.getGridCell(selectedTrack,selectedStep):
    proc.getCellFxFlag(selectedTrack,selectedStep,i==4?3:i-1);
  cellControls[i]->setToggleState(active,juce::dontSendNotification);
 }
}
void DISidechainEditor::refreshDuckControls(){
 if(selectedTrack>=6)return;
 if(duckDepthSlider)duckDepthSlider->setValue(proc.getDuckTrackDepth(selectedTrack),juce::dontSendNotification);
 if(duckLengthSlider)duckLengthSlider->setValue(proc.getDuckTrackLength(selectedTrack),juce::dontSendNotification);
}
void DISidechainEditor::refreshCreativePage(){
 refreshCellControls();
 const bool sceneActive=sequenceToggle&&sequenceToggle->getToggleState();
 if(copySceneButton)copySceneButton->setVisible(sceneActive);
 for(int j=0;j<4;++j)if(sceneButtons[j]){
  sceneButtons[j]->setVisible(sceneActive);
  sceneButtons[j]->setToggleState(proc.getGridScene()==j,juce::dontSendNotification);
 }
 const bool active=sequenceToggle&&sequenceToggle->getToggleState();
 const bool fxPage=active&&selectedLane>=3;
 const bool duckPage=active&&selectedTrack<6;
 if(duckDepthSlider)duckDepthSlider->setVisible(duckPage);
 if(duckLengthSlider)duckLengthSlider->setVisible(duckPage);
 const bool verb=selectedLane==3;
 if(reverbOnButton)reverbOnButton->setVisible(fxPage&&verb);
 if(distortOnButton)distortOnButton->setVisible(fxPage&&selectedLane==4);
 if(reverseButton)reverseButton->setVisible(fxPage&&verb);
 for(int i=0;i<2;++i)if(creativeModelBoxes[i])creativeModelBoxes[i]->setVisible(fxPage&&i==(verb?0:1));
 for(int i=0;i<7;++i)if(creativeSliders[i])creativeSliders[i]->setVisible(fxPage&&(verb?(i<3||i==5):(i==3||i==4||i==6)));
 // Legacy step shape controls are hidden only while editing an FX track.
 for(auto* b:{copyStep.get(),pasteStep.get(),resetStep.get(),undoButton.get(),redoButton.get(),
              snapButton.get(),shapePresetButton.get(),sequencePrev.get(),sequenceNext.get()})
  if(b)b->setVisible(!fxPage);
 if(shapeOffset)shapeOffset->setVisible(!active);
 for(auto& slider:laneSliders)if(slider)slider->setVisible(!active);
 for(auto& b:laneTabs)if(b)b->setVisible(true);
}
void DISidechainEditor::resized() {
 // All child component bounds use design pixels. The parent editor, graph
 // renderer and pointer handling use the same scale to avoid mismatched hitboxes.
 graph={79,201,1428,176};
 stepStrip={78,392,1430,42};
 detailGraph={230,549,488,110};
 sequencerPanel={12,456,873,240};
 filterPanel={12,706,1110,232};
 outputPanel={1132,706,392,232};
 if(presetButton)presetButton->setBounds(538,30,395,45);
 if(abButtons[0])abButtons[0]->setBounds(940,36,36,32);
 if(abButtons[1])abButtons[1]->setBounds(980,36,36,32);
 if(abCopyButtons[0])abCopyButtons[0]->setBounds(1020,36,52,32);
 if(abCopyButtons[1])abCopyButtons[1]->setBounds(1076,36,52,32);
 if(setupButton)setupButton->setBounds(setupBounds);
 if(gridModeButton)gridModeButton->setBounds(917,124,124,30);
 for(int i=0;i<4;++i)if(sceneButtons[i])sceneButtons[i]->setBounds(873+i*62,179,57,22);
 if(copySceneButton)copySceneButton->setBounds(1124,179,113,22);
 if(duckDepthSlider)duckDepthSlider->setBounds(1224,586,284,30);
 if(duckLengthSlider)duckLengthSlider->setBounds(1224,638,284,30);
 if(reverbOnButton)reverbOnButton->setBounds(32,505,125,31);
 if(distortOnButton)distortOnButton->setBounds(32,505,125,31);
 if(reverseButton)reverseButton->setBounds(163,505,132,31);
 if(creativeModelBoxes[0])creativeModelBoxes[0]->setBounds(324,505,220,31);
 if(creativeModelBoxes[1])creativeModelBoxes[1]->setBounds(324,505,220,31);
 for(int i=0;i<7;++i)if(creativeSliders[i]){
  const int col=i==5?0:i==6?0:i<3?i:i-3;
  const int yy=i>=5?630:576;
  creativeSliders[i]->setBounds(36+col*283,yy,253,33);
 }
 for(int i=0;i<5;++i)if(cellControls[i])cellControls[i]->setBounds(1223+i*58,522,55,31);
 refreshCreativePage();
 if(sequenceToggle)sequenceToggle->setBounds(360,124,137,32);
 if(sequenceCount)sequenceCount->setBounds(501,124,118,30);
 if(combos.size()==3){
  combos[0]->setBounds(0,0,0,0); // filter mode uses premium segment buttons
  combos[1]->setBounds(376,769,204,30);
  combos[2]->setBounds(639,124,109,29);
 }
 if(sync)sync->setBounds(772,123,130,30);
 if(fxTypeBox)fxTypeBox->setBounds(866,124,124,30);
 if(fxToggle){fxToggle->setVisible(false);fxToggle->setBounds(0,0,0,0);}
 if(clipperButton)clipperButton->setBounds(1049,123,126,31);
 if(saturatorToggle)saturatorToggle->setBounds(1187,123,91,31);
 if(fxSliders[0])fxSliders[0]->setVisible(false);
 if(saturatorDriveSlider)saturatorDriveSlider->setBounds(1284,123,120,30);
 if(fxSliders[1])fxSliders[1]->setBounds(1412,123,105,30);
 if(sequencePrev)sequencePrev->setBounds(33,533,47,34);
 if(sequenceNext)sequenceNext->setBounds(156,533,47,34);
 if(copyStep)copyStep->setBounds(230,499,75,32);
 if(pasteStep)pasteStep->setBounds(310,499,75,32);
 if(resetStep)resetStep->setBounds(390,499,75,32);
 if(undoButton)undoButton->setBounds(470,499,75,32);
 if(redoButton)redoButton->setBounds(550,499,75,32);
 if(snapButton)snapButton->setBounds(630,499,85,32);
 if(shapePresetButton)shapePresetButton->setBounds(727,499,151,32);
 if(shapeOffset){shapeOffset->setBounds(1320,543,184,31);shapeOffset->setEnabled(selectedLane==0);}
 if(laneSliders[0])laneSliders[0]->setBounds(1320,593,184,31);
 if(laneSliders[1])laneSliders[1]->setBounds(1320,640,184,31);
 for(int i=0;i<5;++i)if(laneTabs[i]){
  laneTabs[i]->setBounds(920,500+i*36,99,27);
  laneTabs[i]->setToggleState(i==selectedLane,juce::dontSendNotification);
 }
 for(int i=0;i<4;++i)if(filterButtons[i])filterButtons[i]->setBounds(31+i*78,768,73,30);
 if(sliders.size()==8 && labels.size()==8){
  const int xs[]={1163,1290,28,144,260,376,492,608};
  for(int i=0;i<8;++i){
   labels[i]->setVisible(false); // titles are painted in the aligned premium panel
   sliders[i]->setBounds(xs[i],i<2?791:825,111,102);
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
 if(selectedLane>=3&&sequenceToggle&&sequenceToggle->getToggleState())return;
 // Continue editing the captured node beyond the graph edges; the vertical
 // value is clamped below. Starting a drag outside the graph remains ignored.
 if(!draggingShape&&!detailGraph.contains((e.position/uiScale).toInt()))return;
 const auto r=detailGraph;
 const auto point=e.position/uiScale;
 const int i=draggingShape&&dragNode>=0?dragNode:juce::jlimit(0,7,juce::roundToInt((point.x-r.getX())/juce::jmax(1,r.getWidth())*7.f));
 float v=juce::jlimit(0.f,1.f,(r.getBottom()-point.y)/(float)juce::jmax(1,r.getHeight()));
 if(sequenceToggle->getToggleState()&&selectedLane==0&&shapeOffset)
  v=juce::jlimit(0.f,1.f,v-(float)shapeOffset->getValue());
 if(snapButton&&snapButton->getToggleState())v=juce::jlimit(0.f,1.f,std::round(v*16.f)/16.f);
 if(sequenceToggle->getToggleState()){
  if(selectedLane==0&&selectedTrack<6&&proc.state.getRawParameterValue("creativeGridEnabled")->load()>0.5f)
   proc.setDuckCurvePoint(selectedTrack,i,v);
  else if(selectedLane==0)proc.setSequencePoint(selectedStep,i,v);
  else proc.setModulationPoint(selectedLane-1,selectedStep,i,v);
 }else proc.points[i]=v;
 repaint();
}
void DISidechainEditor::updateDetailPoint(const juce::MouseEvent& e){updatePoint(e);}
void DISidechainEditor::mouseDown(const juce::MouseEvent& e){
 const auto pos=(e.position/uiScale).toInt();
 // Six shape palette shortcuts mirror the dedicated shape preset browser.
 for(int i=0;i<9&&selectedLane<3;++i)if(juce::Rectangle<int>(734+(i%3)*48,552+(i/3)*42,42,36).contains(pos)){
  applyShapePreset(i);return;
 }
 if(juce::Rectangle<int>(24,117,153,43).contains(pos)){sequenceToggle->setToggleState(false,juce::sendNotificationSync);selectedLane=0;refreshCreativePage();repaint();return;}
 if(juce::Rectangle<int>(183,117,155,43).contains(pos)){sequenceToggle->setToggleState(true,juce::sendNotificationSync);refreshCreativePage();repaint();return;}
 // Each track cell is independently routed through the five cell switches.
 if(sequenceToggle->getToggleState()&&graph.contains(pos)){
  constexpr int labelWidth=116;
  const int left=graph.getX()+labelWidth,top=graph.getY()+23;
  if(pos.x>=left&&pos.y>=top){
   const int count=juce::jlimit(1,16,(int)sequenceCount->getValue());
   selectedStep=juce::jlimit(0,count-1,(pos.x-left)*count/juce::jmax(1,graph.getRight()-left));
   selectedTrack=juce::jlimit(0,5,(pos.y-top)*6/juce::jmax(1,graph.getBottom()-top));
   proc.setGridCell(selectedTrack,selectedStep,!proc.getGridCell(selectedTrack,selectedStep));
   if(gridModeButton&&!gridModeButton->getToggleState())
    gridModeButton->setToggleState(true,juce::sendNotificationSync);
   refreshDuckControls();
   refreshCreativePage();
   repaint();
  }
  return;
 }
 if(stepStrip.contains(pos)){
  const int count=juce::jlimit(1,16,(int)sequenceCount->getValue());
  selectedStep=juce::jlimit(0,count-1,(pos.x-stepStrip.getX())*count/juce::jmax(1,stepStrip.getWidth()));
  refreshCellControls();repaint();return;
 }
 if(selectedLane<3&&detailGraph.contains(pos)){
  draggingShape=true;
  dragNode=juce::jlimit(0,7,juce::roundToInt((pos.x-detailGraph.getX())*7.f/juce::jmax(1,detailGraph.getWidth())));
  dragStep=currentShapeKey();
  dragSequence=sequenceToggle->getToggleState();
  dragBefore=captureShape(dragStep,dragSequence);
 }
 updatePoint(e);
}
void DISidechainEditor::mouseDrag(const juce::MouseEvent& e){updatePoint(e);}
void DISidechainEditor::mouseUp(const juce::MouseEvent&) {
 if(!draggingShape)return;
 draggingShape=false;
 dragNode=-1;
 pushEdit(dragStep,dragSequence,dragBefore,captureShape(dragStep,dragSequence));
}

void DISidechainEditor::applyFactoryPreset(int index) {
 // SIDEFX factory programs drive the independent trigger grid, not the
 // legacy single-envelope bank inherited during development.
 index=juce::jlimit(0,5,index);
 const char* names[]={"Classic Duck","Deep Pump","Sidechain 4/4",
                      "Breakbeat Pump","Pump Rise","Wobble"};
 for(int track=0;track<8;++track)
  for(int step=0;step<16;++step)proc.setGridCell(track,step,false);
 constexpr float classic[8]={0.f,.05f,.22f,.55f,.83f,1.f,1.f,1.f};
 constexpr float deep[8]={0.f,0.f,.05f,.12f,.35f,.65f,.9f,1.f};
 constexpr float rise[8]={1.f,.9f,.7f,.5f,.3f,.12f,.02f,0.f};
 constexpr float wobble[8]={0.f,1.f,.1f,.9f,.2f,.8f,.15f,1.f};
 for(int track=0;track<6;++track){
  proc.setDuckTrackDepth(track,track==0?1.f:.55f);
  proc.setDuckTrackLength(track,1.f);
  for(int node=0;node<8;++node)
   proc.setDuckCurvePoint(track,node,index==1?deep[node]:index==4?rise[node]:index==5?wobble[node]:classic[node]);
 }
 for(int step=0;step<16;++step){
  bool kick=false,snare=false,ghost=false,extra=false;
  switch(index){
   case 0: kick=step%4==0;break;
   case 1: kick=step%2==0;break;
   case 2: kick=step%4==0;snare=step%8==4;break;
   case 3: kick=step%4==0;snare=step%8==4;ghost=step%8==3||step%8==7;extra=step==10||step==14;break;
   case 4: kick=step%4==0;break;
   default: kick=step%2==0;ghost=step%4==3;break;
  }
  proc.setGridCell(0,step,kick);
  proc.setGridCell(1,step,snare);
  proc.setGridCell(2,step,ghost);
  proc.setGridCell(3,step,extra);
 }
 presetName=names[index];
 proc.state.state.setProperty("presetName",presetName,nullptr);
 if(presetButton)presetButton->setButtonText("PRESETS  -  "+presetName);
 if(auto* p=proc.state.getParameter("sequenceEnabled")){
  p->beginChangeGesture();p->setValueNotifyingHost(1.f);p->endChangeGesture();
 }
 if(auto* p=proc.state.getParameter("creativeGridEnabled")){
  p->beginChangeGesture();p->setValueNotifyingHost(1.f);p->endChangeGesture();
 }
 selectedTrack=0;selectedStep=0;
 refreshDuckControls();refreshCreativePage();repaint();
 return;
}
void DISidechainEditor::applyShapePreset(int index) {
 index=juce::jlimit(0,8,index);
 const bool seq=sequenceToggle->getToggleState();
 // A preset must edit the SAME curve that the visible editor renders.
 // Volume in GRID mode is the track's own duck envelope (96+track),
 // while Filter/Pan retain independent per-step modulation curves.
 const int key=currentShapeKey();
 auto before=captureShape(key,seq),after=before;
 for(int i=0;i<8;++i)after[i]=di::curvePresets[(size_t)index][(size_t)i];
 if(seq&&selectedLane==0&&selectedTrack<6) {
  di::DynamicCurve dc;
  for(int i=0;i<8;++i)dc.y[(size_t)i]=after[(size_t)i];
  proc.setDynamicDuckCurve(selectedTrack,dc);
 }
 applyShape(key,seq,after);pushEdit(key,seq,before,after);
 repaint();
}
void DISidechainEditor::showPresetMenu() {
 juce::PopupMenu menu;const juce::StringArray names{"Classic Duck","Deep Pump","Sidechain 4/4","Breakbeat Pump","Pump Rise","Wobble"};
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
 juce::PopupMenu menu;const juce::StringArray names{"Smooth Rise","Linear Rise","Exponential Rise","Hard Gate","Reverse Ramp","Smooth Wobble","Sharp Duck","Double Pump","S-Curve"};
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
void DISidechainEditor::captureAB(int slot) {
 slot=juce::jlimit(0,1,slot);
 proc.getStateInformation(abSnapshots[(size_t)slot]);
 // A/B state is generated internally, but keep a hard bound so a malformed
 // future state extension cannot retain an unbounded editor-side allocation.
 if(abSnapshots[(size_t)slot].getSize()>4u*1024u*1024u){
  abSnapshots[(size_t)slot].reset();
  abStored[(size_t)slot]=false;
  return;
 }
 abStored[(size_t)slot]=true;
 if(abButtons[(size_t)slot]) abButtons[(size_t)slot]->setButtonText(slot==0?"A*":"B*");
}
void DISidechainEditor::recallAB(int slot) {
 slot=juce::jlimit(0,1,slot);
 if(!abStored[(size_t)slot]||abSnapshots[(size_t)slot].isEmpty()||
    abSnapshots[(size_t)slot].getSize()>4u*1024u*1024u||
    abSnapshots[(size_t)slot].getSize()>static_cast<size_t>(std::numeric_limits<int>::max())) return;
 proc.setStateInformation(abSnapshots[(size_t)slot].getData(),(int)abSnapshots[(size_t)slot].getSize());
 repaint();
}
void DISidechainEditor::copyAB(int from,int to) {
 from=juce::jlimit(0,1,from);to=juce::jlimit(0,1,to);
 if(!abStored[(size_t)from]) captureAB(from);
 if(!abStored[(size_t)from]) return;
 abSnapshots[(size_t)to]=abSnapshots[(size_t)from];
 abStored[(size_t)to]=true;
 if(abCopyButtons[(size_t)from]) abCopyButtons[(size_t)from]->setButtonText(from==0?"A→B ✓":"B→A ✓");
 if(abButtons[(size_t)to]) abButtons[(size_t)to]->setButtonText(to==0?"A*":"B*");
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
 const auto destination=juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("DI-Sidefx.disidefx");
 presetChooser=std::make_unique<juce::FileChooser>("Save DI SIDEFX preset",destination,"*.disidefx");
 juce::Component::SafePointer<DISidechainEditor> safe(this);
 presetChooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles,
  [safe](const juce::FileChooser& chooser){
   if(!safe)return;
   const auto file=chooser.getResult();
   if(file==juce::File())return;
   const auto name=file.getFileNameWithoutExtension();
   const auto oldName=safe->presetName;
   safe->proc.state.state.setProperty("presetName",name,nullptr);
   juce::MemoryBlock data;safe->proc.getStateInformation(data);
   if(file.replaceWithData(data.getData(),data.getSize())) {
    safe->presetName=name;
    safe->presetButton->setButtonText("PRESETS  -  "+name);
   } else {
    safe->proc.state.state.setProperty("presetName",oldName,nullptr);
    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
      "DI SIDEFX", "Could not save the preset file. Check folder permissions.");
   }
  });
}
void DISidechainEditor::loadUserPreset() {
 const auto folder=juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
 presetChooser=std::make_unique<juce::FileChooser>("Load DI SIDEFX preset",folder,"*.disidefx");
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
    safe->proc.state.state.setProperty("presetName",safe->presetName,nullptr);
    safe->presetButton->setButtonText("PRESETS  -  "+safe->presetName);
    safe->repaint();
   } else {
    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
      "DI SIDEFX", "Could not read this preset file.");
   }
  });
}
