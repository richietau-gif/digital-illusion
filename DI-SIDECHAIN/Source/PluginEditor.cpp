#include "PluginEditor.h"
#include <cmath>

namespace {
const juce::Colour bg(0xff0d1016), panel(0xff1b222e), panelDark(0xff151c27),
                   accent(0xffff686c), pale(0xfff4f5f8), muted(0xff9ca8b8);
void box(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c, float radius=10.f) {
    g.setColour(c); g.fillRoundedRectangle(r, radius);
}
void text(juce::Graphics& g, const juce::String& s, float size,
          juce::Rectangle<float> r, juce::Colour c, juce::Justification align=juce::Justification::centredLeft) {
    g.setColour(c); g.setFont(juce::FontOptions(size, juce::Font::plain));
    g.drawFittedText(s, r.toNearestInt(), align, 1);
}
float xFromHz(float hz) {
    return 33.f + juce::jlimit(0.f, 1.f, std::log(juce::jlimit(20.f,20000.f,hz)/20.f)/std::log(1000.f))*625.f;
}
float hzFromX(float x) {
    return 20.f * std::pow(1000.f, juce::jlimit(0.f,1.f,(x-33.f)/625.f));
}
}

DISidechainEditor::DISidechainEditor(DISidechainProcessor& p) : AudioProcessorEditor(&p), proc(p) {
    skin.setColour(juce::ComboBox::backgroundColourId, panel);
    skin.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff354050));
    skin.setColour(juce::ComboBox::textColourId, pale);
    skin.setColour(juce::PopupMenu::backgroundColourId, panel);
    skin.setColour(juce::PopupMenu::textColourId, pale);
    skin.setColour(juce::Slider::thumbColourId, accent);
    skin.setColour(juce::Slider::trackColourId, accent);
    skin.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff3a4455));
    skin.setColour(juce::Slider::textBoxTextColourId, pale);
    skin.setColour(juce::Slider::textBoxBackgroundColourId, panel);
    skin.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setLookAndFeel(&skin);

    const char* ids[] = {"depth","mix","threshold","length","smooth","low","high","retrigger"};
    const char* titles[] = {"DEPTH","MIX","THRESHOLD dB","LENGTH ms","SMOOTH","LOW Hz","HIGH Hz","HOLDOFF"};
    for (int i=0;i<8;++i) {
        auto s=std::make_unique<juce::Slider>();
        s->setSliderStyle(juce::Slider::LinearHorizontal);
        s->setTextBoxStyle(juce::Slider::TextBoxRight,false,67,23);
        s->setColour(juce::Slider::textBoxTextColourId,pale);
        auto l=std::make_unique<juce::Label>();
        l->setText(titles[i],juce::dontSendNotification);
        l->setColour(juce::Label::textColourId,muted);
        l->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(*l);addAndMakeVisible(*s);
        sa.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,ids[i],*s));
        sliders.push_back(std::move(s));labels.push_back(std::move(l));
    }
    const char* cids[]={"trigger","mode","division"};
    const juce::StringArray choices[]={
        {"Audio + MIDI","Audio only","MIDI only"},
        {"Full","Low","High","Band"},
        {"1/32","1/16","1/8","1/4","1/2","1 bar","2 bars"}};
    for(int i=0;i<3;++i) {
        auto c=std::make_unique<juce::ComboBox>();
        c->addItemList(choices[i],1);
        addAndMakeVisible(*c);
        ca.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,cids[i],*c));
        combos.push_back(std::move(c));
    }
    sync=std::make_unique<juce::ToggleButton>("SYNC");
    sync->setColour(juce::ToggleButton::textColourId,pale);
    addAndMakeVisible(*sync);
    ba=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"sync",*sync);

    // setSize invokes resized synchronously: do not move above child construction.
    setSize(700,760);
    startTimerHz(15);
}
DISidechainEditor::~DISidechainEditor(){
    stopTimer();ba.reset();ca.clear();sa.clear();
    setLookAndFeel(nullptr);
}
void DISidechainEditor::paint(juce::Graphics& g){
    g.fillAll(bg);
    const float sc=(float)getWidth()/designWidth;
    g.saveState();g.addTransform(juce::AffineTransform::scale(sc));
    const float h=(float)getHeight()/sc;
    box(g,{0,0,700.f,h},bg,18);
    text(g,"DI",19,{12,12,28,23},pale);
    text(g,"SIDECHAIN",19,{40,12,160,23},accent);
    text(g,"DIGITAL ILLUSION · AUDIO FX",9,{12,37,250,14},muted);
    box(g,{604,18,84,19},juce::Colour(0xff123e26),11);
    text(g,"EXT TRIGGER",10,{607,18,77,19},juce::Colour(0xff4fe58e),juce::Justification::centred);

    for(int i=0;i<3;++i) box(g,{11.f+i*226.f,57,219,40},juce::Colour(0xff242b37),7);
    const char* top[]={"TRIGGER","FREQUENCY MODE","LENGTH"};
    for(int i=0;i<3;++i) text(g,top[i],10,{17.f+i*226.f,60,207,16},pale,juce::Justification::centred);
    box(g,{11,108,678,365},panel,10);
    text(g,"VOLUME ENVELOPE",11,{19,119,260,20},pale);
    text(g,"8 POINTS · DRAG TO EDIT",9,{477,119,203,20},muted,juce::Justification::centredRight);
    for(int i=0;i<=8;++i){
        auto x=graph.getX()+graph.getWidth()*i/8.f;
        g.setColour(juce::Colour(0xff374253));g.drawVerticalLine(juce::roundToInt(x),graph.getY(),graph.getBottom());
    }
    for(int i=0;i<=4;++i){
        auto y=graph.getY()+graph.getHeight()*i/4.f;
        g.setColour(juce::Colour(0xff374253));g.drawHorizontalLine(juce::roundToInt(y),graph.getX(),graph.getRight());
    }
    // Match the reference's dominant coral envelope; all displayed nodes are DSP-backed.
    juce::Path line;
    for(int i=0;i<8;++i){
        float x=graph.getX()+graph.getWidth()*i/7.f;
        float y=graph.getBottom()-juce::jlimit(0.f,1.f,proc.points[i].load())*graph.getHeight();
        if(i==0)line.startNewSubPath(x,y);
        else {
            float prevX=graph.getX()+graph.getWidth()*(i-1)/7.f;
            float prevY=graph.getBottom()-juce::jlimit(0.f,1.f,proc.points[i-1].load())*graph.getHeight();
            float d=(x-prevX)*0.38f;
            line.cubicTo(prevX+d,prevY,x-d,y,x,y);
        }
    }
    juce::Path shaded(line);
    shaded.lineTo(graph.getRight(),graph.getBottom());
    shaded.lineTo(graph.getX(),graph.getBottom());shaded.closeSubPath();
    g.setColour(accent.withAlpha(.11f));g.fillPath(shaded);
    g.setColour(accent);g.strokePath(line,juce::PathStrokeType(4.f));
    for(int i=0;i<8;++i){
        float x=graph.getX()+graph.getWidth()*i/7.f;
        float y=graph.getBottom()-juce::jlimit(0.f,1.f,proc.points[i].load())*graph.getHeight();
        g.setColour(accent);g.fillEllipse(x-9,y-9,18,18);
        g.setColour(pale);g.fillEllipse(x-5,y-5,10,10);
    }
    text(g,"0",12,{36,421,50,20},muted);
    text(g,"100%",12,{598,421,57,20},muted,juce::Justification::centredRight);
    text(g,"0 dB",10,{19,445,90,18},muted);
    text(g,"DRAG POINTS TO EDIT",9,{241,445,218,18},accent,juce::Justification::centred);
    text(g,"-inf dB",10,{581,445,95,18},muted,juce::Justification::centredRight);
    text(g,"FREQUENCY TARGET",11,{11,488,255,22},pale);
    text(g,"ILLUSTRATIVE BARS · DSP CUTOFFS BELOW",9,{406,488,276,22},muted,juce::Justification::centredRight);
    box(g,{27,533,646,86},panel,11);
    for(int i=0;i<24;++i) {
        const float amp=14.f+std::abs(std::sin(i*0.43f+0.4f))*54.f;
        box(g,{36.f+i*26.f,576.f-amp*.5f,13,amp},juce::Colour(0xff475368),4);
    }
    const int mode=juce::jlimit(0,3,(int)proc.state.getRawParameterValue("mode")->load());
    const float low=proc.state.getRawParameterValue("low")->load();
    const float high=proc.state.getRawParameterValue("high")->load();
    float x1=mode==2?xFromHz(low):mode==0?33.f:xFromHz(low);
    float x2=mode==1?xFromHz(low):mode==0?658.f:mode==2?658.f:xFromHz(high);
    if(mode!=0){
        if(mode==1)x1=33.f;
        if(mode==2)x2=658.f;
        auto rect=juce::Rectangle<float>(x1,534.f,juce::jmax(2.f,x2-x1),84.f);
        box(g,rect,accent.withAlpha(.22f),3);
        g.setColour(accent);g.drawVerticalLine(juce::roundToInt(x1),528,623);
        g.drawVerticalLine(juce::roundToInt(x2),528,623);
    }
    text(g,"20 Hz",12,{27,621,90,20},muted);
    text(g,"20 kHz",12,{572,621,99,20},muted,juce::Justification::centredRight);
    const char* modes[]={"FULL","LOW","HIGH","BAND","MULTI"};
    for(int i=0;i<5;++i){
        juce::Rectangle<float> r{11.f+i*135.5f,647,129,25};
        box(g,r,i==mode?juce::Colour(0xffa9323c):juce::Colour(0xff29313f),5);
        text(g,i==4?"MULTI · SOON":modes[i],i==4?8.5f:10.f,r,i==4?muted:pale,juce::Justification::centred);
    }
    for(int i=0;i<3;++i)box(g,{11.f+i*226.f,683,219,59},juce::Colour(0xff232a36),6);
    text(g,"DEPTH",10,{11,688,219,15},muted,juce::Justification::centred);
    text(g,"SMOOTH",10,{237,688,219,15},muted,juce::Justification::centred);
    text(g,"MIX",10,{463,688,219,15},muted,juce::Justification::centred);
    text(g,advanced?"HIDE ADVANCED ↑":"ADVANCED CONTROLS ↓",10,{13,744,675,16},accent,juce::Justification::centred);
    if(advanced){
        box(g,{11,767,678,196},panelDark,9);
        text(g,"ADDITIONAL DSP SETTINGS",12,{25,774,420,20},pale);
        text(g,"THRESHOLD · LENGTH · LOW / HIGH CUTOFF · RETRIGGER",9,{25,941,620,15},muted);
    }
    g.restoreState();
}
void DISidechainEditor::resized(){
    const float sc=(float)getWidth()/designWidth;
    const auto bounds=[sc](float x,float y,float w,float h){
        return juce::Rectangle<int>(juce::roundToInt(x*sc),juce::roundToInt(y*sc),
            juce::jmax(1,juce::roundToInt(w*sc)),juce::jmax(1,juce::roundToInt(h*sc)));};
    if(sliders.size()!=8||labels.size()!=8||combos.size()!=3||sync==nullptr)return;
    for(int i=0;i<3;++i)combos[i]->setBounds(bounds(18.f+i*226.f,76,203,19));
    sync->setBounds(bounds(558,105,115,23));
    const int indices[]={0,4,1};
    for(int j=0;j<3;++j){
        int i=indices[j]; sliders[i]->setBounds(bounds(23+j*226.f,705,192,31));
        labels[i]->setBounds(bounds(11+j*226.f,684,219,18));
        labels[i]->setVisible(false);sliders[i]->setVisible(true);
    }
    const int extra[]={2,3,5,6,7};
    for(int k=0;k<5;++k){
        int i=extra[k], row=k/2,col=k%2;
        float x=29.f+col*335.f, y=803.f+row*46.f;
        labels[i]->setBounds(bounds(x,y,114,20));
        sliders[i]->setBounds(bounds(x+112,y,193,24));
        labels[i]->setVisible(advanced);sliders[i]->setVisible(advanced);
    }
}
juce::Point<float> DISidechainEditor::localDesignPoint(const juce::MouseEvent& e) const {
    const float sc=(float)getWidth()/designWidth;
    return e.position/sc;
}
void DISidechainEditor::setParameter(const char* id,float value){
    if(auto* p=proc.state.getParameter(id)){
        p->beginChangeGesture();
        p->setValueNotifyingHost(p->convertTo0to1(value));
        p->endChangeGesture();
    }
}
void DISidechainEditor::updatePoint(const juce::MouseEvent& e){
    auto q=localDesignPoint(e);
    if(!graph.contains(q))return;
    const int i=juce::jlimit(0,7,juce::roundToInt((q.x-graph.getX())/graph.getWidth()*7.f));
    proc.points[i].store(juce::jlimit(0.f,1.f,(graph.getBottom()-q.y)/graph.getHeight()));
    repaint();
}
void DISidechainEditor::updateBand(const juce::MouseEvent& e){
    if(bandHandle<0)return;
    auto q=localDesignPoint(e);
    const float hz=hzFromX(q.x);
    if(bandHandle==0)setParameter("low",juce::jlimit(25.f,1500.f,hz));
    if(bandHandle==1)setParameter("high",juce::jlimit(60.f,18000.f,hz));
    repaint();
}
void DISidechainEditor::mouseDown(const juce::MouseEvent& e){
    auto q=localDesignPoint(e);
    if(graph.contains(q)){updatePoint(e);return;}
    if(q.y>=647 && q.y<=674){
        int index=(int)((q.x-11.f)/135.5f);
        if(index>=0&&index<4){setParameter("mode",(float)index);repaint();}
        return;
    }
    if(q.y>=533&&q.y<=619){
        int mode=(int)proc.state.getRawParameterValue("mode")->load();
        if(mode!=0){
            float lo=xFromHz(proc.state.getRawParameterValue("low")->load());
            float hi=xFromHz(proc.state.getRawParameterValue("high")->load());
            bandHandle=(mode==1||mode==2)?(mode==1?0:0):(std::abs(q.x-lo)<std::abs(q.x-hi)?0:1);
            updateBand(e);
        }
        return;
    }
    if(q.y>=740&&q.y<=764){
        advanced=!advanced;setSize(700,advanced?975:760);repaint();
    }
}
void DISidechainEditor::mouseDrag(const juce::MouseEvent& e){
    if(bandHandle>=0)updateBand(e);else updatePoint(e);
}
