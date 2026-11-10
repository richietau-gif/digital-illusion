#include "PluginEditor.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace {
constexpr float W = 1100.f, H = 740.f;
constexpr float gx0=78.f, gx1=956.f, gy0=135.f, gy1=293.f;
const auto cyan=juce::Colour(0xff05d9e5);
const auto orange=juce::Colour(0xffff9019);
const auto ink=juce::Colour(0xff09121c);
const auto panel=juce::Colour(0xff111c28);
const auto pale=juce::Colour(0xffe1eef3);
const auto muted=juce::Colour(0xff8ca5b7);
struct Knob {
 const char* id; const char* label; int x,y,w,h; const char* suffix; int digits; bool percent;
};
constexpr std::array<Knob,18> knobSpecs{{
 {"threshold","Threshold",34,435,94,66," dB",1,false},
 {"retrigger","Holdoff",151,435,94,66," ms",0,false},
 {"length","Length",276,435,84,66," ms",0,false},
 {"timeOffset","Offset",365,435,84,66," %",0,true},
 {"lookahead","Lookahead",454,435,84,66," ms",1,false},
 {"attackMs","Attack",552,435,79,66," ms",1,false},
 {"releaseMs","Release",637,435,79,66," ms",1,false},
 {"depth","Depth",723,435,79,66," %",0,true},
 {"mix","Dry / Wet",831,435,110,66," %",0,true},
 {"output","Output",953,435,110,66," dB",1,false},
 {"cutoff","Cutoff",30,614,103,67," Hz",0,false},
 {"resonance","Resonance",140,614,103,67,"",2,false},
 {"filterDrive","Drive",250,614,103,67," dB",1,false},
 {"filterMod","ENV AMOUNT",425,662,146,19," %",0,true},
 {"filterMix","FILTER MIX",425,694,146,19," %",0,true},
 {"colorDrive","Drive",685,614,107,67," dB",1,false},
 {"colorTone","Tone",808,614,107,67," %",0,true},
 {"colorMix","Mix",929,614,107,67," %",0,true}
}};
juce::Rectangle<float> rect(float x,float y,float w,float h){return {x,y,w,h};}
float clamp(float x){return juce::jlimit(0.f,1.f,x);}
}

// A real JUCE child component owns the envelope hit area. Its mouse events
// are translated back to editor coordinates (including JUCE's 75% scale).
// This avoids accidental interception by any future neighbouring GUI widgets.
class EnvelopeMouseLayer final : public juce::Component {
public:
 explicit EnvelopeMouseLayer(DISidechainEditor& owner) : editor(owner) {
  setName("Envelope point editor");
  setInterceptsMouseClicks(true,false);
  setMouseCursor(juce::MouseCursor::CrosshairCursor);
 }
 void mouseDown(const juce::MouseEvent& e) override { editor.mouseDown(e.getEventRelativeTo(&editor)); }
 void mouseDrag(const juce::MouseEvent& e) override { editor.mouseDrag(e.getEventRelativeTo(&editor)); }
 void mouseUp(const juce::MouseEvent& e) override { editor.mouseUp(e.getEventRelativeTo(&editor)); }
private:
 DISidechainEditor& editor;
};

PrecisionLookAndFeel::PrecisionLookAndFeel() {
 setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff111f2d));
 setColour(juce::ComboBox::outlineColourId,juce::Colour(0xff365064));
 setColour(juce::ComboBox::textColourId,pale);
 setColour(juce::ComboBox::arrowColourId,cyan);
 setColour(juce::TextEditor::backgroundColourId,ink);
 setColour(juce::TextEditor::textColourId,pale);
 setColour(juce::TextEditor::outlineColourId,juce::Colour(0xff365064));
 setColour(juce::TextButton::textColourOffId,pale);
 setColour(juce::TextButton::textColourOnId,juce::Colours::black);
 setColour(juce::ToggleButton::textColourId,pale);
}
void PrecisionLookAndFeel::drawRotarySlider(juce::Graphics& g,int x,int y,int width,int height,float pos,float a,float b,juce::Slider& slider) {
 const float diameter=(float)juce::jmin(width,height);
 const float radius=diameter*.34f,cx=x+width*.5f,cy=y+height*.43f;
 const float angle=a+pos*(b-a);
 juce::Path base,progress;
 base.addCentredArc(cx,cy,radius,radius,0,a,b,true);
 progress.addCentredArc(cx,cy,radius,radius,0,a,angle,true);
 g.setColour(juce::Colour(0xff263743));
 g.strokePath(base,juce::PathStrokeType(4.6f));
 g.setColour(slider.getName()=="timeOffset"?orange:cyan);
 g.strokePath(progress,juce::PathStrokeType(4.6f));
 g.setColour(juce::Colour(0xff101c29));
 g.fillEllipse(cx-radius*.78f,cy-radius*.78f,radius*1.56f,radius*1.56f);
 g.setColour(pale);
 g.drawLine(cx+std::sin(angle)*radius*.25f,cy-std::cos(angle)*radius*.25f,
            cx+std::sin(angle)*radius*.75f,cy-std::cos(angle)*radius*.75f,2.f);
}
void PrecisionLookAndFeel::drawButtonBackground(juce::Graphics& g,juce::Button& button,const juce::Colour&,bool hovered,bool pressed) {
 const bool on=button.getToggleState();
 const bool power=button.getButtonText()=="ON"||button.getButtonText()=="ENV ON";
 auto bounds=button.getLocalBounds().toFloat().reduced(1);
 g.setColour(on?(power?juce::Colour(0xff9ff52e):juce::Colour(0xff11c9d5)):juce::Colour(hovered?0xff294357:0xff192939));
 g.fillRoundedRectangle(bounds,6.f);
 g.setColour(on?(power?juce::Colour(0xffb4ff49):cyan):juce::Colour(0xff355368));
 g.drawRoundedRectangle(bounds,6.f,1.f);
 if(pressed){g.setColour(juce::Colours::white.withAlpha(.08f));g.fillRoundedRectangle(bounds,6.f);}
}

DISidechainEditor::DISidechainEditor(DISidechainProcessor& p):AudioProcessorEditor(&p),proc(p) {
 setLookAndFeel(&lookAndFeel);
 for(const auto& spec:knobSpecs){
  auto s=std::make_unique<juce::Slider>();
  s->setName(spec.id);
  s->setSliderStyle((juce::String(spec.id)=="filterMod"||juce::String(spec.id)=="filterMix")
                    ?juce::Slider::LinearHorizontal:juce::Slider::RotaryHorizontalVerticalDrag);
  s->setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
  s->setColour(juce::Slider::textBoxTextColourId,pale);
  s->setColour(juce::Slider::textBoxBackgroundColourId,ink);
  s->setColour(juce::Slider::textBoxOutlineColourId,juce::Colour(0xff25384a));
  s->setColour(juce::Slider::trackColourId,cyan);
  s->setColour(juce::Slider::thumbColourId,cyan);
  s->setNumDecimalPlacesToDisplay(spec.digits);
  s->setTooltip(juce::String(spec.label));
  if(spec.percent) {
   const bool bipolar=(juce::String(spec.id)=="timeOffset"||juce::String(spec.id)=="filterMod"||juce::String(spec.id)=="colorTone");
   s->textFromValueFunction=[bipolar](double v){return juce::String(v*100.,0)+" %";};
   s->valueFromTextFunction=[](const juce::String& t){return t.getDoubleValue()/100.;};
   juce::ignoreUnused(bipolar);
  } else s->setTextValueSuffix(spec.suffix);
  addAndMakeVisible(*s);
  sliderAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.state,spec.id,*s));
  // JUCE SliderAttachment installs its own parameter text conversion.
  // Reapply compact formatting afterwards to avoid 0.28600000-style values.
  if(spec.percent){
   s->textFromValueFunction=[](double v){return juce::String(v*100.0,0)+"%";};
   s->valueFromTextFunction=[](const juce::String& t){return t.getDoubleValue()/100.0;};
  } else {
   const juce::String suffix(spec.suffix);
   const int digits=spec.digits;
   s->textFromValueFunction=[suffix,digits](double v){return juce::String(v,digits)+suffix;};
   s->valueFromTextFunction=[](const juce::String& t){return t.getDoubleValue();};
  }
  sliders.emplace_back(std::move(s));
 }
 struct Choice{const char* id;juce::StringArray names;};
 const std::array<Choice,7> choices{{
  {"triggerMode",{"Audio","MIDI","Audio + MIDI"}},
  {"detectorSource",{"Auto","INT","EXT"}},
  {"detector",{"Peak","RMS","Transient"}},
  {"division",{"1/16","1/8T","1/8","1/4T","1/4","1/2","1 bar"}},
  {"filterModel",{"LP12","LP24","LP48","HP12","HP24","HP48","Band Pass","Notch","Analog Ladder","SVF Morph","Comb","Formant / Vowel","Aggressive Bass"}},
  {"colorModel",{"Warm Saturation","Soft Clip","Hard Clip","Asymmetric Drive","Wavefold","Digital Crush"}},
  {"uiScale",{"50%","75%","100%"}}
 }};
 for(const auto& choice:choices) {
  auto c=std::make_unique<juce::ComboBox>();
  c->addItemList(choice.names,1);
  addAndMakeVisible(*c);
  comboAttachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(proc.state,choice.id,*c));
  combos.emplace_back(std::move(c));
 }
 gridPicker=std::make_unique<juce::ComboBox>();
 gridPicker->addItemList({"Off","1/4","1/8","1/16"},1);
 gridPicker->setTooltip("Snap envelope points to this rhythmic editing grid. Off enables free movement.");
 addAndMakeVisible(*gridPicker);
 gridAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(proc.state,"editGrid",*gridPicker);
 syncLengthSlider=std::make_unique<juce::Slider>();
 syncLengthSlider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
 syncLengthSlider->setName("syncLength");
 syncLengthSlider->setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
 syncLengthSlider->setColour(juce::Slider::textBoxTextColourId,pale);
 syncLengthSlider->setColour(juce::Slider::textBoxBackgroundColourId,ink);
 syncLengthSlider->setColour(juce::Slider::textBoxOutlineColourId,juce::Colour(0xff25384a));
 syncLengthSlider->textFromValueFunction=[](double value) {
  const juce::StringArray labels{"1/16","1/8T","1/8","1/4T","1/4","1/2","1 Bar"};
  return labels[juce::jlimit(0,6,juce::roundToInt(value))];
 };
 syncLengthSlider->valueFromTextFunction=[](const juce::String& text){
  const juce::StringArray labels{"1/16","1/8T","1/8","1/4T","1/4","1/2","1 Bar"};
  int index=0;
  for(int i=0;i<labels.size();++i) {
   if(labels[i].equalsIgnoreCase(text.trim())){index=i;break;}
  }
  return (double)juce::jmax(0,index);
 };
 addAndMakeVisible(*syncLengthSlider);
 syncLengthAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.state,"division",*syncLengthSlider);
  curveAmountSlider=std::make_unique<juce::Slider>();
 curveAmountSlider->setSliderStyle(juce::Slider::LinearHorizontal);
 curveAmountSlider->setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
 curveAmountSlider->setColour(juce::Slider::trackColourId,cyan);
 curveAmountSlider->setColour(juce::Slider::thumbColourId,cyan);
 curveAmountSlider->textFromValueFunction=[](double value){return juce::String(value*100.,0)+"%";};
 curveAmountSlider->valueFromTextFunction=[](const juce::String& text){return text.getDoubleValue()/100.;};
 addAndMakeVisible(*curveAmountSlider);
 curveAmountAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.state,"curveAmount",*curveAmountSlider);
  presetBrowser=std::make_unique<juce::ComboBox>();
 for(int i=0;i<proc.getNumPrograms();++i) presetBrowser->addItem(proc.getProgramName(i),i+1);
 presetBrowser->setSelectedId(proc.getCurrentProgram()+1,juce::dontSendNotification);
 presetBrowser->onChange=[this] {
  if(controlsReady){
   int selected=presetBrowser->getSelectedId()-1;
   if(selected>=0&&selected<proc.getNumPrograms())proc.setCurrentProgram(selected);
  }
 };
 addAndMakeVisible(*presetBrowser);
 auto button=[this](std::unique_ptr<juce::TextButton>& target,const juce::String& text,bool toggle=false) {
  target=std::make_unique<juce::TextButton>(text);
  target->setClickingTogglesState(toggle);
  addAndMakeVisible(*target);
 };
 button(prevPreset,"<");
 button(nextPreset,">");
 button(aButton,"A",true);
 button(bButton,"B",true);
 button(copyAB,"A > B");
 button(savePreset,"SAVE");
 button(undoButton,"↶");
 button(redoButton,"↷");
 button(filterPower,"ON",true);
 button(filterEnvelopePower,"ENV ON",true);
 button(colorPower,"ON",true);
 button(curveShapeButton,"SHAPES");
 button(settingsButton,"SETUP");
 button(powerButton,"ON",true);
 button(sourceInt,"INT",true);
 button(sourceExt,"EXT",true);
 button(sourceMidi,"MIDI",true);
 auto setDiscreteParameter=[this](const char* id,float value) {
  if(auto* parameter=proc.state.getParameter(id)){
   parameter->beginChangeGesture();
   parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
   parameter->endChangeGesture();
  }
 };
 sourceInt->onClick=[setDiscreteParameter](){setDiscreteParameter("triggerMode",0.f);setDiscreteParameter("detectorSource",1.f);};
 sourceExt->onClick=[setDiscreteParameter](){setDiscreteParameter("triggerMode",0.f);setDiscreteParameter("detectorSource",2.f);};
 sourceMidi->onClick=[setDiscreteParameter](){setDiscreteParameter("triggerMode",1.f);};

 powerButton->setTooltip("Enable/disable entire PRECISION SIDECHAIN processing (inverted Bypass)");
 powerButton->setToggleState(true,juce::dontSendNotification);
 settingsButton->onClick=[this]{
  juce::PopupMenu menu;
  menu.addItem(1,"UI 50%");menu.addItem(2,"UI 75%");menu.addItem(3,"UI 100%");
  menu.addSeparator();menu.addItem(4,"Load user preset...");
  juce::Component::SafePointer<DISidechainEditor> safe(this);
  menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(settingsButton.get()),[safe](int selection){
   if(safe==nullptr)return;
   if(selection>=1&&selection<=3){
    safe->combos[6]->setSelectedId(selection,juce::sendNotificationSync);
   } else if(selection==4) {
    safe->presetLoadChooser=std::make_unique<juce::FileChooser>(
     "Load PRECISION SIDECHAIN preset",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),"*.preset");
    safe->presetLoadChooser->launchAsync(
     juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,
     [safe](const juce::FileChooser& chooser) {
      if(safe==nullptr)return;
      const auto file=chooser.getResult();
      juce::MemoryBlock data;
      if(file.existsAsFile()&&file.getSize()>0&&file.getSize()<4*1024*1024&&file.loadFileAsData(data)){
       safe->proc.setStateInformation(data.getData(),(int)data.getSize());
       safe->pullNodes();
       safe->snapshots[safe->activeSnapshot]=safe->proc.state.copyState();
       safe->snapshotCurves[safe->activeSnapshot]=safe->curve;
      }
     });
   }
  });
 };
 powerButton->onClick=[this]{
  const bool isOn=powerButton->getToggleState();
  if(auto* p=proc.state.getParameter("bypass")){
   p->beginChangeGesture();
   p->setValueNotifyingHost(p->convertTo0to1(isOn?0.f:1.f));
   p->endChangeGesture();
  }
 };
 button(selectTool,"EDIT",true);
 button(bendTool,"BEND",true);
 button(eraseTool,"ERASE",true);
 button(clearCurve,"RESET");
 selectTool->setTooltip("Add points and drag existing points");
 bendTool->setTooltip("Drag an envelope point vertically to alter its segment curvature");
 eraseTool->setTooltip("Click an internal point to remove it");
 clearCurve->setTooltip("Reset the envelope to a simple duck shape");
 selectTool->onClick=[this]{selectEditTool(EditTool::select);};
 bendTool->onClick=[this]{selectEditTool(EditTool::bend);};
 eraseTool->onClick=[this]{selectEditTool(EditTool::erase);};
 clearCurve->onClick=[this]{
  recordUndo();
  curve.count=4;
  curve.nodes[0]={0.f,0.f,0.f};curve.nodes[1]={.18f,.02f,0.f};
  curve.nodes[2]={.63f,.86f,.1f};curve.nodes[3]={1.f,1.f,0.f};
  pushNodes();
 };
 selectEditTool(EditTool::select);
 prevPreset->onClick=[this]{movePreset(-1);};
 nextPreset->onClick=[this]{movePreset(1);};
 aButton->onClick=[this]{setSnapshot(0);};
 bButton->onClick=[this]{setSnapshot(1);};
 copyAB->onClick=[this]{copySnapshot(0,1);};
 undoButton->onClick=[this]{undo();};
 redoButton->onClick=[this]{redo();};
 filterAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.state,"filterEnabled",*filterPower);
 filterEnvelopeAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.state,"filterEnvelopeEnabled",*filterEnvelopePower);
 colorAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.state,"colorEnabled",*colorPower);

 sync=std::make_unique<juce::ToggleButton>("BPM SYNC");
 bypass=std::make_unique<juce::ToggleButton>("BYPASS");
 attackProtectToggle=std::make_unique<juce::ToggleButton>("ATTACK PROT.");
 addAndMakeVisible(*sync);addAndMakeVisible(*bypass);addAndMakeVisible(*attackProtectToggle);
 syncAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.state,"sync",*sync);
 bypassAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.state,"bypass",*bypass);
 attackProtectAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.state,"attackProtection",*attackProtectToggle);

 curveShapeButton->onClick=[this] {
  juce::PopupMenu menu;
  menu.addItem(1,"Classic Duck");menu.addItem(2,"Hard Gate");
  menu.addItem(3,"Double Pump");menu.addItem(4,"Smooth Wobble");
  menu.addItem(5,"Reverse");
  juce::Component::SafePointer<DISidechainEditor> safe(this);
  menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(curveShapeButton.get()),[safe](int result) {
   if(safe==nullptr||result<1)return;
   safe->recordUndo();
   auto& c=safe->curve;
   c.count= result==3?6:result==4?7:4;
   const float x6[]={0.f,.16f,.38f,.61f,.83f,1.f,1.f};
   const float x7[]={0.f,.15f,.30f,.48f,.65f,.82f,1.f};
   for(int i=0;i<c.count;i++) {
    float x=result==3?x6[i]:result==4?x7[i]:(float)i/(float)(c.count-1);
    float y=result==1?x*x*(3.f-2.f*x):
            result==2?(x<.3f?0.f:1.f):
            result==3?((i==1||i==3)?0.f:1.f):
            result==4?(i%2==0?1.f:0.05f):1.f-x;
    c.nodes[(size_t)i]={x,y,0.f};
   }
   safe->pushNodes();
  });
 };
 savePreset->onClick=[this] {
  presetFileChooser=std::make_unique<juce::FileChooser>("Save PRECISION SIDECHAIN preset",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("PrecisionSidechain.preset"),"*.preset");
  juce::Component::SafePointer<DISidechainEditor> safe(this);
  presetFileChooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles,
    [safe](const juce::FileChooser& chooser){
     if(safe==nullptr)return;
     auto path=chooser.getResult();
     if(path!=juce::File()) {
      juce::MemoryBlock state;
      safe->proc.getStateInformation(state);
      path.replaceWithData(state.getData(),state.getSize());
     }
    });
 };
 combos.back()->onChange=[this]{if(controlsReady)changeScale();};
 pullNodes();
 snapshots[0]=proc.state.copyState();snapshots[1]=snapshots[0].createCopy();
 snapshotCurves[0]=curve;snapshotCurves[1]=curve;
 aButton->setToggleState(true,juce::dontSendNotification);
 bButton->setToggleState(false,juce::dontSendNotification);
 envelopeMouseLayer=std::make_unique<EnvelopeMouseLayer>(*this);
 addAndMakeVisible(*envelopeMouseLayer);
 controlsReady=true;
 setResizable(true,true);
 setResizeLimits(550,370,1400,940);
 changeScale();
 startTimerHz(25);
}
DISidechainEditor::~DISidechainEditor() {
 stopTimer();controlsReady=false;
 envelopeMouseLayer.reset();
 filterAttachment.reset();filterEnvelopeAttachment.reset();colorAttachment.reset();curveAmountAttachment.reset();gridAttachment.reset();syncLengthAttachment.reset();attackProtectAttachment.reset();
 syncAttachment.reset();bypassAttachment.reset();sliderAttachments.clear();comboAttachments.clear();
 setLookAndFeel(nullptr);
}
void DISidechainEditor::changeScale() {
 const auto* p=proc.state.getRawParameterValue("uiScale");
 const int i=p?juce::jlimit(0,2,juce::roundToInt(p->load())):1;
 const int sizes[][2]={{550,370},{825,555},{1100,740}};
 setSize(sizes[i][0],sizes[i][1]);
}
juce::Rectangle<int> DISidechainEditor::scaled(int x,int y,int w,int h) const {
 const float f=juce::jmin(getWidth()/W,getHeight()/H);
 const float ox=(getWidth()-W*f)*.5f,oy=(getHeight()-H*f)*.5f;
 return {juce::roundToInt(ox+x*f),juce::roundToInt(oy+y*f),juce::roundToInt(w*f),juce::roundToInt(h*f)};
}
void DISidechainEditor::resized() {
 if(!controlsReady||sliders.size()!=knobSpecs.size()||combos.size()!=7||!presetBrowser)return;
 const auto b=[this](juce::Component* component,int x,int y,int w,int h){component->setBounds(scaled(x,y,w,h));};
 envelopeMouseLayer->setBounds(scaled((int)gx0,(int)gy0,(int)(gx1-gx0),(int)(gy1-gy0)));
 // The mouse layer was added last; don't reorder components from resized().
 b(prevPreset.get(),327,19,30,31);b(presetBrowser.get(),365,18,225,33);
 b(nextPreset.get(),597,19,30,31);b(savePreset.get(),636,19,52,31);
 b(aButton.get(),700,19,32,31);b(bButton.get(),737,19,32,31);
 b(copyAB.get(),775,19,52,31);b(undoButton.get(),834,19,30,31);b(redoButton.get(),869,19,30,31);
 b(bypass.get(),909,22,72,27);b(settingsButton.get(),992,19,52,31);b(powerButton.get(),1049,19,36,31);
 combos[6]->setVisible(false);
 b(selectTool.get(),140,96,54,30);
 b(bendTool.get(),199,96,54,30);
 b(eraseTool.get(),258,96,62,30);
 b(clearCurve.get(),325,96,59,30);
 b(curveShapeButton.get(),43,99,77,29);b(gridPicker.get(),725,91,122,32);
 b(combos[3].get(),399,515,132,24);
 b(syncLengthSlider.get(),276,435,84,66);
 b(attackProtectToggle.get(),551,522,120,19);
 b(curveAmountSlider.get(),858,93,135,29);
 b(combos[0].get(),33,406,118,27);
 b(sourceInt.get(),154,406,29,27);
 b(sourceExt.get(),184,406,29,27);
 b(sourceMidi.get(),214,406,38,27);
 combos[1]->setVisible(false);
 b(combos[2].get(),31,521,100,21);
 b(sync.get(),274,522,112,19);
 b(filterPower.get(),104,564,51,26);b(combos[4].get(),162,564,162,27);
 b(filterEnvelopePower.get(),335,564,83,27);
 b(colorPower.get(),734,564,52,27);b(combos[5].get(),795,563,255,29);
 for(size_t i=0;i<sliders.size();++i) {
  const auto& k=knobSpecs[i];
  b(sliders[i].get(),k.x,k.y,k.w,k.h);
  if(juce::String(k.id)=="length")sliders[i]->setVisible(proc.state.getRawParameterValue("sync")->load()<=.5f);
 }
}
void DISidechainEditor::timerCallback() {
 if(!controlsReady)return;
 const bool synced=proc.state.getRawParameterValue("sync")->load()>.5f;
 powerButton->setToggleState(proc.state.getRawParameterValue("bypass")->load()<=.5f,juce::dontSendNotification);
 const int triggerMode=(int)proc.state.getRawParameterValue("triggerMode")->load();
 const int detectorSource=(int)proc.state.getRawParameterValue("detectorSource")->load();
 sourceInt->setToggleState(triggerMode!=1&&detectorSource==1,juce::dontSendNotification);
 sourceExt->setToggleState(triggerMode!=1&&detectorSource==2,juce::dontSendNotification);
 sourceMidi->setToggleState(triggerMode==1,juce::dontSendNotification);
 syncLengthSlider->setVisible(synced);
 for(size_t i=0;i<knobSpecs.size();++i)
  if(juce::String(knobSpecs[i].id)=="length")sliders[i]->setVisible(!synced);
 combos[3]->setVisible(false);
 if(proc.envelopeRevision.load()!=knownRevision&&draggedNode<0)pullNodes();
 if(!presetBrowser->isPopupActive())presetBrowser->setSelectedId(proc.getCurrentProgram()+1,juce::dontSendNotification);
 repaint();
}
void DISidechainEditor::pullNodes() {curve.count=proc.getEnvelopeNodes(curve.nodes);knownRevision=proc.envelopeRevision.load();}
void DISidechainEditor::pushNodes() {proc.setEnvelopeNodes(curve.nodes,curve.count);knownRevision=proc.envelopeRevision.load();repaint();}
void DISidechainEditor::recordUndo() {
 if(undoHistory.size()>=32)undoHistory.erase(undoHistory.begin());
 undoHistory.push_back(curve);redoHistory.clear();
}
void DISidechainEditor::undo() {
 if(undoHistory.empty())return;
 redoHistory.push_back(curve);curve=undoHistory.back();undoHistory.pop_back();pushNodes();
}
void DISidechainEditor::redo() {
 if(redoHistory.empty())return;
 undoHistory.push_back(curve);curve=redoHistory.back();redoHistory.pop_back();pushNodes();
}
float DISidechainEditor::graphX(float x) const {
 const auto area=scaled((int)gx0,(int)gy0,(int)(gx1-gx0),(int)(gy1-gy0));
 return (float)area.getX()+juce::jlimit(0.f,1.f,x)*(float)area.getWidth();
}
float DISidechainEditor::graphY(float y) const {
 const auto area=scaled((int)gx0,(int)gy0,(int)(gx1-gx0),(int)(gy1-gy0));
 return (float)area.getBottom()-juce::jlimit(0.f,1.f,y)*(float)area.getHeight();
}
bool DISidechainEditor::graphContains(juce::Point<int> p) const {
 return scaled((int)gx0,(int)gy0,(int)(gx1-gx0),(int)(gy1-gy0)).expanded(8).contains(p);
}
void DISidechainEditor::mouseDown(const juce::MouseEvent& e) {
 if(!graphContains(e.getPosition()))return;
 const auto p=e.getPosition();
 bool insertedOnMouseDown=false;
 int nearest=-1;float best=juce::jlimit(9.f,18.f,14.f*juce::jmin(getWidth()/W,getHeight()/H));
 for(int i=0;i<curve.count;++i) {
  const float dx=(float)p.x-graphX(curve.nodes[(size_t)i].x);
  const float dy=(float)p.y-graphY(curve.nodes[(size_t)i].y);
  const float d=std::sqrt(dx*dx+dy*dy);
  if(d<best){best=d;nearest=i;}
 }
 if(e.mods.isRightButtonDown()||editTool==EditTool::erase) {
  if(nearest>0&&nearest<curve.count-1) {
   recordUndo();
   for(int i=nearest;i<curve.count-1;i++)curve.nodes[(size_t)i]=curve.nodes[(size_t)i+1];
   --curve.count;pushNodes();
  }
  return;
 }
 if(nearest<0&&editTool==EditTool::select&&curve.count<maxNodes) {
  const auto bounds=scaled((int)gx0,(int)gy0,(int)(gx1-gx0),(int)(gy1-gy0));
  float x=clamp((p.x-bounds.getX())/(float)bounds.getWidth());
  const int grid=(int)proc.state.getRawParameterValue("editGrid")->load();
  if(grid>0&&!e.mods.isAltDown()) {
   const int divisions=4<<(grid-1);
   x=std::round(x*(float)divisions)/(float)divisions;
  }
  const float y=clamp(1.f-(p.y-bounds.getY())/(float)bounds.getHeight());
  if(x<=.015f||x>=.985f)return;
  for(int i=0;i<curve.count;++i)
   if(std::abs(curve.nodes[(size_t)i].x-x)<.015f)return;
  recordUndo();
  int insert=1;
  while(insert<curve.count&&curve.nodes[(size_t)insert].x<x)++insert;
  for(int j=curve.count;j>insert;--j)curve.nodes[(size_t)j]=curve.nodes[(size_t)j-1];
  curve.nodes[(size_t)insert]={x,y,0.f};++curve.count;nearest=insert;
  insertedOnMouseDown=true;
  pushNodes();
 }
 draggedNode=nearest;
 changeCurve=e.mods.isShiftDown()||editTool==EditTool::bend;
 dragGrabOffset={0.f,0.f};
 if(nearest>=0) {
  if(!insertedOnMouseDown)recordUndo();
  dragStartCurve=curve.nodes[(size_t)nearest].curve;
  dragGrabOffset={(float)p.x-graphX(curve.nodes[(size_t)nearest].x),
                  (float)p.y-graphY(curve.nodes[(size_t)nearest].y)};
 }
}
void DISidechainEditor::mouseDrag(const juce::MouseEvent& e) {
 if(draggedNode<0)return;
 auto& node=curve.nodes[(size_t)draggedNode];
 if(changeCurve||e.mods.isShiftDown()) {
  node.curve=juce::jlimit(-1.f,1.f,dragStartCurve-e.getDistanceFromDragStartY()*.008f);
 } else {
  const auto bounds=scaled((int)gx0,(int)gy0,(int)(gx1-gx0),(int)(gy1-gy0));
  float x=clamp(((float)e.x-dragGrabOffset.x-(float)bounds.getX())/(float)bounds.getWidth());
  const int grid=(int)proc.state.getRawParameterValue("editGrid")->load();
  if(grid>0&&!e.mods.isAltDown()){
   const int divisions=4<<(grid-1);
   x=std::round(x*(float)divisions)/(float)divisions;
  }
  if(draggedNode==0)x=0.f;
  else if(draggedNode==curve.count-1)x=1.f;
  else {
   const float left=curve.nodes[(size_t)draggedNode-1].x+.015f;
   const float right=curve.nodes[(size_t)draggedNode+1].x-.015f;
   x=left<=right?juce::jlimit(left,right,x):node.x;
  }
  node.x=x;
  node.y=clamp(1.f-((float)e.y-dragGrabOffset.y-(float)bounds.getY())/(float)bounds.getHeight());
 }
 pushNodes();
}
void DISidechainEditor::mouseUp(const juce::MouseEvent&) {draggedNode=-1;changeCurve=false;dragGrabOffset={0.f,0.f};}
void DISidechainEditor::selectEditTool(EditTool tool){
 editTool=tool;
 selectTool->setToggleState(tool==EditTool::select,juce::dontSendNotification);
 bendTool->setToggleState(tool==EditTool::bend,juce::dontSendNotification);
 eraseTool->setToggleState(tool==EditTool::erase,juce::dontSendNotification);
}
void DISidechainEditor::movePreset(int delta) {
 proc.setCurrentProgram((proc.getCurrentProgram()+delta+proc.getNumPrograms())%proc.getNumPrograms());
 presetBrowser->setSelectedId(proc.getCurrentProgram()+1,juce::dontSendNotification);
}
void DISidechainEditor::setSnapshot(int slot) {
 if(slot==activeSnapshot)return;
 snapshots[activeSnapshot]=proc.state.copyState();
 snapshotCurves[activeSnapshot]=curve;
 if(snapshots[slot].isValid())proc.state.replaceState(snapshots[slot].createCopy());
 curve=snapshotCurves[slot];pushNodes();
 activeSnapshot=slot;
 aButton->setToggleState(slot==0,juce::dontSendNotification);
 bButton->setToggleState(slot==1,juce::dontSendNotification);
}
void DISidechainEditor::copySnapshot(int source,int destination) {
 if(source==activeSnapshot){snapshots[source]=proc.state.copyState();snapshotCurves[source]=curve;}
 snapshots[destination]=snapshots[source].createCopy();snapshotCurves[destination]=snapshotCurves[source];
 if(destination==activeSnapshot){proc.state.replaceState(snapshots[destination].createCopy());curve=snapshotCurves[destination];pushNodes();}
}
void DISidechainEditor::paint(juce::Graphics& g) {
 g.fillAll(ink);
 const float scale=juce::jmin(getWidth()/W,getHeight()/H);
 const float ox=(getWidth()-W*scale)*.5f,oy=(getHeight()-H*scale)*.5f;
 g.addTransform(juce::AffineTransform::scale(scale).translated(ox,oy));
 const auto panelBox=[&](float x,float y,float w,float h){
  g.setColour(panel);g.fillRoundedRectangle(rect(x,y,w,h),9.f);
  g.setColour(juce::Colour(0xff2b4656));g.drawRoundedRectangle(rect(x,y,w,h),9.f,1.f);
 };
 const auto label=[&](juce::String s,float x,float y,float width,float size,juce::Colour color,bool bold=false){
  g.setColour(color);
  g.setFont(juce::Font(juce::FontOptions(size).withStyle(bold?"Bold":"Regular")));
  g.drawText(s,juce::Rectangle<float>(x,y,width,size+6.f),juce::Justification::centredLeft);
 };
 const auto get=[this](const char* id,float def){
  auto* p=proc.state.getRawParameterValue(id);return p?p->load():def;
 };
 g.setColour(juce::Colour(0xff102330));g.fillRect(0.f,0.f,W,67.f);
 // Vector logo - original Digital Illusion pulse icon, no image dependency.
 juce::Path logo;logo.startNewSubPath(15,34);
 const float ly[]={34,34,27,44,17,48,32,34,25,40,34,34};
 for(int i=1;i<12;i++)logo.lineTo(15.f+i*4.f,ly[i]);
 g.setColour(cyan);g.strokePath(logo,juce::PathStrokeType(2.2f));
 label("PRECISION SIDECHAIN",74,9,250,19,pale,true);
 label("D I G I T A L   I L L U S I O N",75,37,250,9,muted);
 panelBox(13,73,1074,284);
 label("ENVELOPE EDITOR",30,77,285,12,cyan,true);
 label("LEFT CLICK: ADD / DRAG    RIGHT: DELETE    SHIFT+DRAG: BEND",322,82,390,10,muted);
 label("GRID",687,92,40,10,muted);label("CURVE "+juce::String(juce::roundToInt(get("curveAmount",.5f)*100.f))+"%",851,78,125,11,muted);
 label("TRIGGER",80,307,70,10,muted);
 label("1/4",294,307,60,10,muted);
 label("1/2",528,307,60,10,muted);
 label("3/4",754,307,60,10,muted);
 label("1 BAR",927,307,60,10,muted);
 // Large envelope editing graph.
 g.setColour(juce::Colour(0xff0b1723));
 g.fillRoundedRectangle(rect(gx0,gy0,gx1-gx0,gy1-gy0),6);
 for(int i=0;i<=8;i++){
  float x=gx0+(gx1-gx0)*(float)i/8.f;
  g.setColour(juce::Colour(0xff283847));g.drawLine(x,gy0,x,gy1,.7f);
 }
 for(int i=0;i<=4;i++){
  float y=gy0+(gy1-gy0)*(float)i/4.f;
  g.setColour(juce::Colour(0xff283847));g.drawLine(gx0,y,gx1,y,.7f);
  label(juce::String(1.f-(float)i*.25f,2),36,y-5,34,9,muted);
 }

 const float depth=clamp(get("depth",1.f));
 const float offset=juce::jlimit(-1.f,1.f,get("timeOffset",0.f));
 const float attack=clamp(get("attackProtection",0.f));
 // Editable SOURCE curve and processed output are intentionally separate.
 // At -100% offset the processed signal can be flat even though the user
 // should always be able to grab the SOURCE envelope's nodes.
 juce::Path editable, result;
 for(int i=0;i<=320;i++){
  const float phase=(float)i/320.f;
  const float source=proc.getEnvelopeAt(phase);
  const float processed=proc.getEnvelopeAt(clamp(phase+offset));
  const float protectedEnv=juce::jmax(processed,attack*(1.f-phase));
  const float finalGain=1.f-depth*(1.f-protectedEnv);
  const float x=gx0+(gx1-gx0)*phase;
  const float ey=gy1-(gy1-gy0)*source;
  const float oy=gy1-(gy1-gy0)*finalGain;
  if(i==0){editable.startNewSubPath(x,ey);result.startNewSubPath(x,oy);}
  else {editable.lineTo(x,ey);result.lineTo(x,oy);}
 }
 g.setColour(orange.withAlpha(.14f));
 juce::Path fill=editable;fill.lineTo(gx1,gy1);fill.lineTo(gx0,gy1);fill.closeSubPath();g.fillPath(fill);
 g.setColour(cyan.withAlpha(.55f));
 g.strokePath(result,juce::PathStrokeType(1.6f));
 g.setColour(orange);
 g.strokePath(editable,juce::PathStrokeType(3.3f));
 for(int i=0;i<curve.count;i++){
  const float x=gx0+(gx1-gx0)*curve.nodes[(size_t)i].x;
  const float y=gy1-(gy1-gy0)*curve.nodes[(size_t)i].y;
  g.setColour(juce::Colour(0xff060d16));g.fillEllipse(x-7,y-7,14,14);
  g.setColour(juce::Colours::white);g.fillEllipse(x-5,y-5,10,10);
  g.setColour(orange);g.fillEllipse(x-4,y-4,8,8);
 }
 g.setColour(cyan.withAlpha(.7f));g.drawLine(gx0,gy0,gx0,gy1,1.3f);
 label("OFFSET "+juce::String(juce::roundToInt(offset*100.f))+"%",575,97,112,10,orange,true);
 // ENV and OUT meter, sourced from actual plugin atomics.
 label("ENV",999,90,41,11,muted);label("OUT",1043,90,38,11,muted);
 const float envLevel=clamp(proc.gainReductionMeter.load());
 const float outputL=clamp(proc.outputMeterL.load()),outputR=clamp(proc.outputMeterR.load());
 const float levels[]={envLevel,outputL,outputR};
 for(int bar=0;bar<3;++bar){
  const float x=bar==0?998.f:bar==1?1032.f:1055.f;
  const float width=bar==0?19.f:15.f;
  g.setColour(juce::Colour(0xff142e36));
  g.fillRoundedRectangle(rect(x,131,width,165),3.f);
  const float level=levels[bar];
  g.setColour(bar==0?juce::Colour(0xff9cf44b):cyan);
  if(level>0.001f)
   g.fillRoundedRectangle(rect(x,296-165.f*level,width,165.f*level),2.f);
 }
 // Mid row: Trigger, Timing, Envelope, Mix.
 panelBox(14,365,240,178);panelBox(262,365,276,178);
 panelBox(546,365,260,178);panelBox(814,365,272,178);
 label("TRIGGER",30,372,212,14,cyan,true);
 label("TIMING",278,372,250,14,cyan,true);
 label("ENVELOPE",558,372,244,14,cyan,true);
 label("MIX",830,372,245,14,cyan,true);
 const auto valueText=[&](const Knob& spec)->juce::String {
  const float value=get(spec.id,0.f);
  if(spec.percent)return juce::String(juce::roundToInt(value*100.f))+"%";
  if(juce::String(spec.id)=="cutoff" && value>=1000.f)
   return juce::String(value/1000.f,2)+" kHz";
  return juce::String(value,spec.digits)+juce::String(spec.suffix);
 };
 const auto valueBadge=[&](juce::String value,float x,float y,float w) {
  g.setColour(juce::Colour(0xff0b1725));
  g.fillRoundedRectangle(rect(x,y,w,18),4.f);
  g.setColour(juce::Colour(0xffd5e8f1));
  g.setFont(juce::FontOptions(12.f));
  g.drawText(value,rect(x,y,w,18),juce::Justification::centred);
 };
 for(int i=0;i<10;i++){
  const auto& spec=knobSpecs[(size_t)i];
  label(spec.label,spec.x+4,spec.y-21,spec.w,12,pale);
  if(juce::String(spec.id)=="length" && get("sync",1.f)>.5f){
   const juce::StringArray rates{"1/16","1/8T","1/8","1/4T","1/4","1/2","1 Bar"};
   const int division=juce::jlimit(0,6,(int)get("division",2.f));
   valueBadge(rates[division],spec.x,spec.y+68,spec.w);
  } else valueBadge(valueText(spec),spec.x,spec.y+68,spec.w);
 }
 label("SOURCE",158,389,80,9,muted);
 // The detector selector includes its own current mode; avoid covering the value badge.
 
 // Bottom row: large filter and COLOR.
 panelBox(14,551,628,177);panelBox(651,551,435,177);
 label("FILTER",30,559,81,13,cyan,true);
 label("ENV CUT-OFF",427,566,150,10,muted);
 label("COLOR",668,559,63,13,cyan,true);
 for(int i=10;i<18;i++){
  const auto& spec=knobSpecs[(size_t)i];
  if(i==13||i==14)continue;
  label(spec.label,spec.x+5,spec.y-20,spec.w,12,pale);
  valueBadge(valueText(spec),spec.x+2,spec.y+72,spec.w-4);
 }
 label("ENV",368,658,55,11,muted,true);
 label("MIX",368,689,55,11,muted,true);
 label(valueText(knobSpecs[13]),580,660,55,10,pale);
 label(valueText(knobSpecs[14]),580,692,55,10,pale);
 // Approximate visual frequency response. Real audio uses the selected DSP model.
 const float cutoff=juce::jlimit(20.f,20000.f,get("cutoff",800.f));
 const int model=(int)get("filterModel",0.f);
 const bool filterEnabled=get("filterEnabled",1.f)>.5f;
 const bool envelopeFilter=get("filterEnvelopeEnabled",1.f)>.5f;
 juce::Path filterPath;
 for(int i=0;i<=128;i++){
  const float x=445.f+(float)i/128.f*171.f;
  const float frequency=20.f*std::pow(1000.f,(float)i/128.f);
  const float ratio=frequency/cutoff;
  float magnitude=1.f/std::sqrt(1.f+std::pow(ratio,model==0||model==3?2.f:model==1||model==4?4.f:6.f));
  if(model>=3&&model<=5)magnitude=1.f-magnitude;
  if(model==6)magnitude=std::exp(-2.5f*std::abs(std::log(juce::jmax(.001f,ratio))));
  if(model==7)magnitude=1.f-.85f*std::exp(-5.f*std::abs(std::log(juce::jmax(.001f,ratio))));
  const float y=642.f-magnitude*46.f;
  if(i==0)filterPath.startNewSubPath(x,y);else filterPath.lineTo(x,y);
 }
 g.setColour(cyan.withAlpha(filterEnabled?1.f:.35f));
 g.strokePath(filterPath,juce::PathStrokeType(2.4f));
 if(filterEnabled&&envelopeFilter) {
  g.setColour(juce::Colour(0xffa4f465));g.fillEllipse(612,563,6,6);
 }
 label("20",446,644,30,9,muted);label("1k",516,644,30,9,muted);
 label("20k",585,644,40,9,muted);
 label("DIGITAL ILLUSION - PRECISION SIDECHAIN 1.0",16,728,500,9,muted);
}
