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
    setWantsKeyboardFocus(true);

    const char* ids[] = {"depth","mix","threshold","length","smooth","low","high","retrigger","midiNote","midiChannel"};
    const char* titles[] = {"DEPTH","MIX","THRESHOLD dB","LENGTH ms","SMOOTH","LOW Hz","HIGH Hz","HOLDOFF","MIDI NOTE (-1 ANY)","MIDI CH (0 ANY)"};
    for (int i=0;i<10;++i) {
        auto s=std::make_unique<juce::Slider>();
        s->setSliderStyle(juce::Slider::LinearHorizontal);
        if(i>=8)s->setNumDecimalPlacesToDisplay(0);
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
    const char* cids[]={"trigger","playbackMode","division"};
    const juce::StringArray choices[]={
        {"Audio + MIDI","Audio only","MIDI only"},
        {"One Shot","Loop"},
        {"1/32","1/16","1/8","1/4","1/2","1 bar","2 bars","1/16T","1/8T","1/4T","1/16D","1/8D","1/4D"}};
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
void DISidechainEditor::updateSpectrum(){
    const uint64_t written=proc.analyzerWritten.load(std::memory_order_acquire);
    if(written<2048 || written==lastSpectrumPosition){
        for(auto& band:spectrumBands)band*=.92f;
        return;
    }
    lastSpectrumPosition=written;
    constexpr int N=2048;
    constexpr float twoPi=juce::MathConstants<float>::twoPi;
    for(int i=0;i<N;++i){
        const auto index=(written-N+(uint64_t)i)%N;
        const float v=proc.analyzerHistory[index].load(std::memory_order_relaxed);
        // Hann window on the message thread only; no FFT or allocations in DSP.
        const float window=.5f-.5f*std::cos(twoPi*(float)i/(N-1));
        spectrumWork[i]=(std::isfinite(v)?v:0.f)*window;
    }
    for(int i=N;i<2*N;++i)spectrumWork[i]=0.f;
    spectrumFft.performFrequencyOnlyForwardTransform(spectrumWork.data());
    const float sr=(float)juce::jmax(8000.0,proc.getSampleRate());
    const float nyquist=sr*.5f;
    for(int band=0;band<24;++band){
        const float leftHz=20.f*std::pow(1000.f,(float)band/24.f);
        const float rightHz=20.f*std::pow(1000.f,(float)(band+1)/24.f);
        const int a=juce::jlimit(1,N/2,juce::roundToInt(leftHz*N/sr));
        const int b=juce::jlimit(a,N/2,juce::roundToInt(juce::jmin(rightHz,nyquist)*N/sr));
        float peak=0.f;
        for(int bin=a;bin<=b;++bin)
            peak=juce::jmax(peak,spectrumWork[bin]);
        const float db=juce::Decibels::gainToDecibels(peak/(N*.5f),-90.f);
        const float level=juce::jlimit(0.f,1.f,(db+76.f)/72.f);
        // Fast attack and slower release give readable movement without GUI flicker.
        spectrumBands[band]=level>spectrumBands[band]
            ?(.65f*level+.35f*spectrumBands[band])
            :(.2f*level+.8f*spectrumBands[band]);
    }
}
void DISidechainEditor::timerCallback(){
    updateSpectrum();
    const auto sequence=proc.triggerSequence.load(std::memory_order_relaxed);
    if(sequence!=lastTriggerSequence){lastTriggerSequence=sequence;triggerFlashTicks=3;}
    else if(triggerFlashTicks>0)--triggerFlashTicks;
    repaint();
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
    box(g,{586,18,102,19},triggerFlashTicks>0?juce::Colour(0xff256c3e):juce::Colour(0xff253447),11);
    text(g,triggerFlashTicks>0?"TRIGGERED":"TRIGGER READY",9,{590,18,94,19},triggerFlashTicks>0?juce::Colour(0xff76f3a5):muted,juce::Justification::centred);

    for(int i=0;i<3;++i) box(g,{11.f+i*226.f,57,219,40},juce::Colour(0xff242b37),7);
    const char* top[]={"TRIGGER","MODE","LENGTH"};
    for(int i=0;i<3;++i) text(g,top[i],10,{17.f+i*226.f,60,207,16},pale,juce::Justification::centred);
    box(g,{11,108,678,365},panel,10);
    text(g,"VOLUME ENVELOPE",11,{19,119,260,20},pale);
    text(g,snapToGrid ? "SNAP: ON · CLICK TO TOGGLE" : "SNAP: OFF · CLICK TO TOGGLE",9,{460,119,220,20},accent,juce::Justification::centredRight);
    for(int i=0;i<=8;++i){
        auto x=graph.getX()+graph.getWidth()*i/8.f;
        g.setColour(juce::Colour(0xff374253));g.drawVerticalLine(juce::roundToInt(x),graph.getY(),graph.getBottom());
    }
    for(int i=0;i<=4;++i){
        auto y=graph.getY()+graph.getHeight()*i/4.f;
        g.setColour(juce::Colour(0xff374253));g.drawHorizontalLine(juce::roundToInt(y),graph.getX(),graph.getRight());
    }
    // The plotted curve is sampled from the SAME smoothstep interpolator as DSP.
    // Unlike a decorative Bezier approximation, this matches the actual gain shape.
    juce::Path line;
    constexpr int plotSamples = 256;
    for(int i=0;i<=plotSamples;++i){
        const double t=(double)i/plotSamples;
        const float x=graph.getX()+(float)t*graph.getWidth();
        const float y=graph.getBottom()-juce::jlimit(0.f,1.f,proc.evaluateEnvelope(t))*graph.getHeight();
        if(i==0)line.startNewSubPath(x,y);else line.lineTo(x,y);
    }
    juce::Path shaded(line);
    shaded.lineTo(graph.getRight(),graph.getBottom());
    shaded.lineTo(graph.getX(),graph.getBottom());shaded.closeSubPath();
    g.setColour(accent.withAlpha(.11f));g.fillPath(shaded);
    g.setColour(accent);g.strokePath(line,juce::PathStrokeType(4.f));
    const float phase=juce::jlimit(0.f,1.f,proc.visualPhase.load(std::memory_order_relaxed));
    if(phase<1.f){
        const float cursorX=graph.getX()+phase*graph.getWidth();
        g.setColour(pale.withAlpha(.55f));
        g.drawLine(cursorX,graph.getY(),cursorX,graph.getBottom(),1.5f);
    }
    const float grDb=proc.visualGainReductionDb.load(std::memory_order_relaxed);
    text(g,"GR "+juce::String(juce::jlimit(-120.f,0.f,grDb),1)+" dB",11,
         {550,142,126,17},accent,juce::Justification::centredRight);
    for(int i=0;i<proc.getEnvelopeNodeCount();++i){
        const auto node=proc.getEnvelopeNode(i);
        float x=graph.getX()+graph.getWidth()*node.x;
        float y=graph.getBottom()-node.y*graph.getHeight();
        g.setColour(accent);g.fillEllipse(x-(i==selectedPoint?11.f:9.f),y-(i==selectedPoint?11.f:9.f),i==selectedPoint?22.f:18.f,i==selectedPoint?22.f:18.f);
        g.setColour(pale);g.fillEllipse(x-5,y-5,10,10);
    }
    text(g,"0",12,{36,421,50,20},muted);
    text(g,"100%",12,{598,421,57,20},muted,juce::Justification::centredRight);
    text(g,"0 dB",10,{19,445,90,18},muted);
    text(g,"DOUBLE-CLICK: ADD · ALT-DRAG: CURVE",9,{206,445,295,18},accent,juce::Justification::centred);
    text(g,"-inf dB",10,{581,445,95,18},muted,juce::Justification::centredRight);
    text(g,"FREQUENCY TARGET",11,{11,488,255,22},pale);
    text(g,"LIVE INPUT SPECTRUM · 2048-POINT FFT",9,{400,488,282,22},muted,juce::Justification::centredRight);
    box(g,{27,533,646,86},panel,11);
    for(int i=0;i<24;++i) {
        const float amp=4.f+juce::jlimit(0.f,1.f,spectrumBands[i])*69.f;
        box(g,{36.f+i*26.f,576.f-amp*.5f,13,amp},juce::Colour(0xff55647b),4);
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
        box(g,{11,767,678,282},panelDark,9);
        text(g,"ADDITIONAL DSP SETTINGS",12,{25,774,420,20},pale);
        text(g,"THRESHOLD · LENGTH · CUTOFFS · HOLDOFF · MIDI NOTE / CHANNEL",9,{25,1025,620,15},muted);
    }
    g.restoreState();
}
void DISidechainEditor::resized(){
    const float sc=(float)getWidth()/designWidth;
    const auto bounds=[sc](float x,float y,float w,float h){
        return juce::Rectangle<int>(juce::roundToInt(x*sc),juce::roundToInt(y*sc),
            juce::jmax(1,juce::roundToInt(w*sc)),juce::jmax(1,juce::roundToInt(h*sc)));};
    if(sliders.size()!=10||labels.size()!=10||combos.size()!=3||sync==nullptr)return;
    for(int i=0;i<3;++i)combos[i]->setBounds(bounds(18.f+i*226.f,76,203,19));
    sync->setBounds(bounds(558,105,115,23));
    const int indices[]={0,4,1};
    for(int j=0;j<3;++j){
        int i=indices[j]; sliders[i]->setBounds(bounds(23+j*226.f,705,192,31));
        labels[i]->setBounds(bounds(11+j*226.f,684,219,18));
        labels[i]->setVisible(false);sliders[i]->setVisible(true);
    }
    const int extra[]={2,3,5,6,7,8,9};
    for(int k=0;k<7;++k){
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
    if(selectedPoint<0||selectedPoint>=proc.getEnvelopeNodeCount())return;
    const auto q=localDesignPoint(e);
    float x=juce::jlimit(0.f,1.f,(q.x-graph.getX())/graph.getWidth());
    float y=juce::jlimit(0.f,1.f,(graph.getBottom()-q.y)/graph.getHeight());
    if(snapToGrid){x=std::round(x*16.f)/16.f;y=std::round(y*16.f)/16.f;}
    proc.setEnvelopeNode(selectedPoint,x,y);
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
    if(q.x>=460&&q.x<=680&&q.y>=119&&q.y<=144){snapToGrid=!snapToGrid;repaint();return;}
    if(graph.contains(q)&&e.mods.isAltDown()){
        // Alt-drag the curve segment, with per-segment cubic shaping.
        const float t=juce::jlimit(0.f,1.f,(q.x-graph.getX())/graph.getWidth());
        curvatureSegment=proc.getEnvelopeNodeCount()-2;
        for(int i=0;i<proc.getEnvelopeNodeCount()-1;++i)
            if(t<=proc.getEnvelopeNode(i+1).x){curvatureSegment=i;break;}
        grabKeyboardFocus();
        return;
    }
    if(graph.contains(q)){
        // Only a node grabs the mouse. Clicking an empty part of the plot
        // does not alter a possibly distant node.
        selectedPoint=-1;
        float distanceSquared=14.f*14.f;
        for(int i=0;i<proc.getEnvelopeNodeCount();++i){
            const auto node=proc.getEnvelopeNode(i);
            const float dx=q.x-(graph.getX()+node.x*graph.getWidth());
            const float dy=q.y-(graph.getBottom()-node.y*graph.getHeight());
            const float d2=dx*dx+dy*dy;
            if(d2<distanceSquared){distanceSquared=d2;selectedPoint=i;}
        }
        if(e.mods.isRightButtonDown()&&selectedPoint>0&&selectedPoint<proc.getEnvelopeNodeCount()-1){
            proc.removeEnvelopeNode(selectedPoint);selectedPoint=-1;repaint();return;
        }
        if(selectedPoint>=0){grabKeyboardFocus();updatePoint(e);}
        return;
    }
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
        advanced=!advanced;setSize(700,advanced?1065:760);repaint();
    }
}
void DISidechainEditor::mouseDrag(const juce::MouseEvent& e){
    if(curvatureSegment>=0){
        const auto q=localDesignPoint(e);
        const float amount=juce::jlimit(-1.f,1.f,(graph.getCentreY()-q.y)/(graph.getHeight()*.5f));
        proc.setEnvelopeSegmentCurve(curvatureSegment,amount);
        repaint();return;
    }
    if(bandHandle>=0)updateBand(e);else updatePoint(e);
}
void DISidechainEditor::mouseUp(const juce::MouseEvent&){bandHandle=-1;curvatureSegment=-1;}
void DISidechainEditor::mouseDoubleClick(const juce::MouseEvent& e){
    const auto q=localDesignPoint(e);
    if(!graph.contains(q))return;
    float x=juce::jlimit(0.f,1.f,(q.x-graph.getX())/graph.getWidth());
    float y=juce::jlimit(0.f,1.f,(graph.getBottom()-q.y)/graph.getHeight());
    if(snapToGrid){x=std::round(x*16.f)/16.f;y=std::round(y*16.f)/16.f;}
    selectedPoint=proc.insertEnvelopeNode(x,y);
    if(selectedPoint>=0)grabKeyboardFocus();
    repaint();
}
bool DISidechainEditor::keyPressed(const juce::KeyPress& key){
    const int n=proc.getEnvelopeNodeCount();
    if(selectedPoint<0||selectedPoint>=n)return false;
    if(key.getKeyCode()==juce::KeyPress::deleteKey||
       key.getKeyCode()==juce::KeyPress::backspaceKey){
        if(proc.removeEnvelopeNode(selectedPoint)){
            selectedPoint=juce::jmin(selectedPoint,proc.getEnvelopeNodeCount()-1);
            repaint();
        }
        return true;
    }
    if(key.getKeyCode()==juce::KeyPress::leftKey&&key.getModifiers().isShiftDown()){
        const auto node=proc.getEnvelopeNode(selectedPoint);
        proc.setEnvelopeNode(selectedPoint,node.x-(snapToGrid?1.f/16.f:.005f),node.y);
        repaint();return true;
    }
    if(key.getKeyCode()==juce::KeyPress::rightKey&&key.getModifiers().isShiftDown()){
        const auto node=proc.getEnvelopeNode(selectedPoint);
        proc.setEnvelopeNode(selectedPoint,node.x+(snapToGrid?1.f/16.f:.005f),node.y);
        repaint();return true;
    }
    if(key.getKeyCode()==juce::KeyPress::leftKey){selectedPoint=juce::jmax(0,selectedPoint-1);repaint();return true;}
    if(key.getKeyCode()==juce::KeyPress::rightKey){selectedPoint=juce::jmin(n-1,selectedPoint+1);repaint();return true;}
    const int direction=key.getKeyCode()==juce::KeyPress::upKey?1:key.getKeyCode()==juce::KeyPress::downKey?-1:0;
    if(direction==0)return false;
    const auto node=proc.getEnvelopeNode(selectedPoint);
    proc.setEnvelopeNode(selectedPoint,node.x,node.y+direction*(snapToGrid?1.f/16.f:.01f));
    repaint();return true;
}
