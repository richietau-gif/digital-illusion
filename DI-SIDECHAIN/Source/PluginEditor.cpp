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
    return 35.f + juce::jlimit(0.f, 1.f, std::log(juce::jlimit(20.f,20000.f,hz)/20.f)/std::log(1000.f))*658.f;
}
float hzFromX(float x) {
    return 20.f * std::pow(1000.f, juce::jlimit(0.f,1.f,(x-35.f)/658.f));
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

    const char* ids[] = {"depth","mix","threshold","length","smooth","low","high","retrigger","midiNote","midiChannel","detectorHP","outputFilterMin","outputFilterMax","outputFilterAmount","outputFilterResonance","outputFilterDrive","detectorAttackMs","detectorReleaseMs","multiLowDepth","multiMidDepth","multiHighDepth"};
    const char* titles[] = {"DEPTH","MIX","THRESHOLD dB","LENGTH ms","SMOOTH","LOW Hz","HIGH Hz","HOLDOFF","MIDI NOTE (-1 ANY)","MIDI CH (0 ANY)","TRIGGER HP Hz","FILTER MIN Hz","FILTER MAX Hz","FILTER AMOUNT","RESONANCE Q","FILTER DRIVE","TRIGGER ATTACK ms","TRIGGER RELEASE ms","MULTI LOW DEPTH","MULTI MID DEPTH","MULTI HIGH DEPTH"};
    for (int i=0;i<21;++i) {
        auto s=std::make_unique<juce::Slider>();
        s->setSliderStyle(juce::Slider::LinearHorizontal);
        if(i>=8&&i<=12)s->setNumDecimalPlacesToDisplay(0);
        if(i>=13)s->setNumDecimalPlacesToDisplay(2);
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

    outputFilterToggle=std::make_unique<juce::ToggleButton>("FILTER");
    outputFilterToggle->setColour(juce::ToggleButton::textColourId,accent);
    addAndMakeVisible(*outputFilterToggle);
    outputFilterAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.state,"outputFilterOn",*outputFilterToggle);
    const char* typeNames[]={"LP","HP","BP","NOTCH"};
    for(int i=0;i<4;++i){
        auto btn=std::make_unique<juce::TextButton>(typeNames[i]);
        btn->setComponentID("filter-type-"+juce::String(i));
        btn->setColour(juce::TextButton::buttonColourId,juce::Colour(0xff293240));
        btn->setColour(juce::TextButton::buttonOnColourId,accent);
        btn->setColour(juce::TextButton::textColourOffId,pale);
        btn->setClickingTogglesState(false);
        btn->onClick=[this,i]{setParameter("outputFilterType",(float)i);repaint();};
        addAndMakeVisible(*btn);
        filterTypeButtons[(size_t)i]=std::move(btn);
    }
    const char* slopeNames[]={"12 dB","24 dB"};
    for(int i=0;i<2;++i){
        auto btn=std::make_unique<juce::TextButton>(slopeNames[i]);
        btn->setComponentID("filter-slope-"+juce::String(i));
        btn->setColour(juce::TextButton::buttonColourId,juce::Colour(0xff293240));
        btn->setColour(juce::TextButton::buttonOnColourId,accent);
        btn->setColour(juce::TextButton::textColourOffId,pale);
        btn->setClickingTogglesState(false);
        btn->onClick=[this,i]{setParameter("outputFilterSlope",(float)i);repaint();};
        addAndMakeVisible(*btn);
        filterSlopeButtons[(size_t)i]=std::move(btn);
    }

    // Factory presets update the actual atomically stored envelope nodes.
    // The selection is a command, not a state parameter: user-edited curves
    // persist through the processor's ordinary Ableton state serialization.
    envelopePreset=std::make_unique<juce::ComboBox>();
    envelopePreset->setComponentID("envelopePreset");
    envelopePreset->addItemList({"Pump","Tight Kick","Smooth Duck",
                                 "Ghost Kick","Breakbeat","Wobble"},1);
    envelopePreset->setTextWhenNothingSelected("LOAD SHAPE...");
    envelopePreset->onChange=[this]{
        const int selection=envelopePreset->getSelectedId();
        if(selection>=1&&selection<=6){
            proc.applyEnvelopePreset(selection-1);
            selectedPoint=-1;
            repaint();
        }
    };
    addAndMakeVisible(*envelopePreset);
    // setSize invokes resized synchronously: do not move above child construction.
    setSize(1120,760);
    startTimerHz(15);
}
DISidechainEditor::~DISidechainEditor(){
    stopTimer();outputFilterAttachment.reset();ba.reset();ca.clear();sa.clear();
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
    // Host automation keeps all active filter modes highlighted.
    const int selectedType=juce::jlimit(0,3,
        juce::roundToInt(proc.state.getRawParameterValue("outputFilterType")->load()));
    const int selectedSlope=juce::jlimit(0,1,
        juce::roundToInt(proc.state.getRawParameterValue("outputFilterSlope")->load()));
    for(int i=0;i<4;++i)
        filterTypeButtons[(size_t)i]->setToggleState(i==selectedType,juce::dontSendNotification);
    for(int i=0;i<2;++i)
        filterSlopeButtons[(size_t)i]->setToggleState(i==selectedSlope,juce::dontSendNotification);
    const auto sequence=proc.triggerSequence.load(std::memory_order_relaxed);
    if(sequence!=lastTriggerSequence){lastTriggerSequence=sequence;triggerFlashTicks=3;}
    else if(triggerFlashTicks>0)--triggerFlashTicks;
    repaint();
}
void DISidechainEditor::paint(juce::Graphics& g){
    g.fillAll(bg);
    const float sc=(float)getWidth()/designWidth;
    g.saveState();
    g.addTransform(juce::AffineTransform::scale(sc));
    box(g,{0,0,1120,760},bg,8);
    // Header and transport bar
    text(g,"DI",19,{16,12,30,26},pale);
    text(g,"SIDECHAIN",19,{46,12,162,26},accent);
    text(g,"DIGITAL ILLUSION  /  AUDIO FX",9,{16,39,270,16},muted);
    box(g,{1001,17,104,23},triggerFlashTicks>0?juce::Colour(0xff256c3e):juce::Colour(0xff273445),10);
    text(g,triggerFlashTicks>0?"TRIGGERED":"TRIGGER READY",9,{1004,18,98,21},
        triggerFlashTicks>0?juce::Colour(0xff8ff4aa):muted,juce::Justification::centred);
    for(int i=0;i<3;++i){
        const float x=16.f+i*232.f;
        box(g,{x,67,224,47},juce::Colour(0xff242e3a),7);
        const char* headings[]={"TRIGGER SOURCE","PLAYBACK","LENGTH"};
        text(g,headings[i],9,{x+8,70,208,13},muted,juce::Justification::centred);
    }

    // Left: professional envelope editor
    box(g,{16,126,695,338},panel,9);
    text(g,"ENVELOPE",11,{29,132,114,23},pale);
    const bool synced=proc.state.getRawParameterValue("sync")->load()>.5f;
    const int division=juce::jlimit(0,12,juce::roundToInt(
        proc.state.getRawParameterValue("division")->load()));
    const juce::String divisions[]={"1/32","1/16","1/8","1/4","1/2","1 BAR",
      "2 BARS","1/16T","1/8T","1/4T","1/16D","1/8D","1/4D"};
    const auto duration=synced?divisions[division]:
        juce::String(juce::roundToInt(proc.state.getRawParameterValue("length")->load()))+" ms";
    const int playback=juce::roundToInt(proc.state.getRawParameterValue("playbackMode")->load());
    text(g,(playback==1?"LOOP · ":"ONE SHOT · ")+duration,10,{350,132,158,22},muted,
        juce::Justification::centred);
    text(g,snapToGrid?"SNAP ON":"SNAP OFF",10,{614,133,83,20},accent,
        juce::Justification::centredRight);
    for(int i=0;i<=8;++i){
        const float x=graph.getX()+graph.getWidth()*i/8.f;
        g.setColour(juce::Colour(0xff354151));
        g.drawVerticalLine(juce::roundToInt(x),graph.getY(),graph.getBottom());
    }
    for(int i=0;i<=4;++i){
        const float y=graph.getY()+graph.getHeight()*i/4.f;
        g.setColour(juce::Colour(0xff354151));
        g.drawHorizontalLine(juce::roundToInt(y),graph.getX(),graph.getRight());
    }
    juce::Path curve;
    for(int i=0;i<=300;++i){
        const float t=(float)i/300.f;
        const float x=graph.getX()+t*graph.getWidth();
        const float y=graph.getBottom()-proc.evaluateEnvelope(t)*graph.getHeight();
        if(i==0)curve.startNewSubPath(x,y);else curve.lineTo(x,y);
    }
    juce::Path fill(curve);
    fill.lineTo(graph.getRight(),graph.getBottom());
    fill.lineTo(graph.getX(),graph.getBottom());
    fill.closeSubPath();
    g.setColour(accent.withAlpha(.1f));g.fillPath(fill);
    g.setColour(accent);g.strokePath(curve,juce::PathStrokeType(3.f));
    const float phase=juce::jlimit(0.f,1.f,proc.visualPhase.load());
    if(phase<1.f){
        const float x=graph.getX()+phase*graph.getWidth();
        g.setColour(pale.withAlpha(.5f));
        g.drawLine(x,graph.getY(),x,graph.getBottom(),1);
    }
    for(int i=0;i<proc.getEnvelopeNodeCount();++i){
        const auto node=proc.getEnvelopeNode(i);
        const float x=graph.getX()+node.x*graph.getWidth();
        const float y=graph.getBottom()-node.y*graph.getHeight();
        g.setColour(accent);g.fillEllipse(x-7,y-7,14,14);
        g.setColour(pale);g.fillEllipse(x-3,y-3,6,6);
    }
    text(g,"0%",9,{35,434,50,16},muted);
    text(g,"100%",9,{638,434,55,16},muted,juce::Justification::centredRight);
    text(g,"DOUBLE-CLICK TO ADD POINT  ·  ALT-DRAG TO SHAPE",9,{178,434,393,17},
        muted,juce::Justification::centred);

    // Compact frequency overview; crossover handles keep their original audio routing.
    box(g,{16,473,695,111},panel,9);
    text(g,"FREQUENCY TARGET",11,{29,477,225,18},pale);
    text(g,"INPUT FFT",9,{609,477,90,18},muted,juce::Justification::centredRight);
    box(g,{34,502,660,43},juce::Colour(0xff202a38),6);
    for(int i=0;i<24;++i){
        const float h=3.f+juce::jlimit(0.f,1.f,spectrumBands[(size_t)i])*33.f;
        box(g,{43.f+i*26.8f,523.f-h*.5f,11,h},juce::Colour(0xff56657e),3);
    }
    const int mode=juce::jlimit(0,4,juce::roundToInt(proc.state.getRawParameterValue("mode")->load()));
    const float low=proc.state.getRawParameterValue("low")->load();
    const float high=proc.state.getRawParameterValue("high")->load();
    if(mode!=0){
        float l=mode==1?35.f:xFromHz(low);
        float r=mode==2?693.f:mode==1?xFromHz(low):xFromHz(high);
        if(mode==2)r=693.f;
        box(g,{l,502,juce::jmax(2.f,r-l),43},accent.withAlpha(.22f),4);
        g.setColour(accent);
        g.drawVerticalLine(juce::roundToInt(l),502,546);
        g.drawVerticalLine(juce::roundToInt(r),502,546);
    }
    text(g,"20 Hz",9,{35,548,70,15},muted);
    text(g,"20 kHz",9,{619,548,74,15},muted,juce::Justification::centredRight);
    const char* targets[]={"FULL","LOW","HIGH","BAND","MULTI"};
    for(int i=0;i<5;++i){
        const juce::Rectangle<float> rect{17.f+i*139.f,563,131,19};
        box(g,rect,i==mode?juce::Colour(0xffaa3948):juce::Colour(0xff293343),4);
        text(g,targets[i],9,rect,pale,juce::Justification::centred);
    }

    box(g,{16,593,695,152},panel,9);
    text(g,"MASTER",11,{29,600,172,20},pale);
    for(int i=0;i<3;++i){
        const char* names[]={"DEPTH","SMOOTH","MIX"};
        text(g,names[i],10,{29.f+i*228.f,636,220,17},muted,juce::Justification::centred);
        box(g,{29.f+i*228.f,660,217,53},juce::Colour(0xff222b39),6);
    }
    text(g,"GR "+juce::String(proc.visualGainReductionDb.load(),1)+" dB",
        10,{545,601,146,20},accent,juce::Justification::centredRight);

    // Right: always-open creative filter panel
    box(g,{723,67,382,332},panel,9);
    text(g,"OUTPUT FILTER",12,{739,77,223,22},pale);
    text(g,"TYPE",9,{741,125,106,17},muted);
    text(g,"SLOPE",9,{741,171,106,17},muted);
    text(g,"CUTOFF FOLLOWS THE ENVELOPE",9,{740,371,345,17},muted);
    // Right: always-open sidechain and multiband controls, even in bypass.
    box(g,{723,407,382,338},panelDark,9);
    text(g,"SIDECHAIN CONTROLS",12,{738,417,290,19},pale);
    text(g,"DETECTOR  ·  MIDI  ·  MULTIBAND",9,{740,440,340,17},muted);
    g.restoreState();
}
void DISidechainEditor::resized(){
    const float sc=(float)getWidth()/designWidth;
    const auto bounds=[sc](float x,float y,float w,float h){
        return juce::Rectangle<int>(juce::roundToInt(x*sc),juce::roundToInt(y*sc),
            juce::jmax(1,juce::roundToInt(w*sc)),juce::jmax(1,juce::roundToInt(h*sc)));};
    if(sliders.size()!=21||labels.size()!=21||combos.size()!=3||
       sync==nullptr||outputFilterToggle==nullptr||envelopePreset==nullptr||
       filterTypeButtons[0]==nullptr||filterSlopeButtons[0]==nullptr)return;
    for(int i=0;i<3;++i)combos[(size_t)i]->setBounds(bounds(25.f+i*232.f,86,202,24));
    sync->setBounds(bounds(990,53,110,22));
    envelopePreset->setBounds(bounds(140,133,198,23));
    outputFilterToggle->setBounds(bounds(970,77,120,24));
    // Main controls remain directly below the envelope editor.
    const int mainIndices[]={0,4,1};
    for(int j=0;j<3;++j){
        const int i=mainIndices[j];
        labels[(size_t)i]->setBounds(bounds(30+j*228.f,636,216,17));
        sliders[(size_t)i]->setBounds(bounds(32+j*228.f,667,210,32));
        labels[(size_t)i]->setVisible(false);
        sliders[(size_t)i]->setVisible(true);
    }
    for(int i=0;i<4;++i)
        filterTypeButtons[(size_t)i]->setBounds(bounds(741+i*87.f,144,80,24));
    for(int i=0;i<2;++i)
        filterSlopeButtons[(size_t)i]->setBounds(bounds(741+i*175.f,190,166,24));
    // Always-visible filter values; no advanced disclosure or resizing.
    for(int i=11;i<=15;++i){
        const int row=i-11;
        const float y=223.f+row*29.f;
        labels[(size_t)i]->setBounds(bounds(736,y,110,24));
        sliders[(size_t)i]->setBounds(bounds(847,y,242,25));
        labels[(size_t)i]->setVisible(true);
        sliders[(size_t)i]->setVisible(true);
    }
    // Detector, crossover, MIDI and MULTI controls use a compact two-column grid.
    const int extra[]={2,3,5,6,7,8,9,10,16,17,18,19,20};
    for(int k=0;k<13;++k){
        const int i=extra[k],row=k/2,col=k%2;
        const float x=739.f+col*181.f;
        const float y=464.f+row*39.f;
        labels[(size_t)i]->setBounds(bounds(x,y,177,15));
        sliders[(size_t)i]->setBounds(bounds(x,y+15,174,21));
        labels[(size_t)i]->setVisible(true);
        sliders[(size_t)i]->setVisible(true);
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
    if(q.x>=609&&q.x<=700&&q.y>=127&&q.y<=158){snapToGrid=!snapToGrid;repaint();return;}
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
    if(q.y>=563&&q.y<=583&&q.x>=17&&q.x<=712){
        const int index=juce::jlimit(0,4,(int)((q.x-17.f)/139.f));
        setParameter("mode",(float)index);
        repaint();
        return;
    }
    if(q.y>=500&&q.y<=548&&q.x>=35&&q.x<=693){
        const int mode=juce::roundToInt(proc.state.getRawParameterValue("mode")->load());
        if(mode!=0){
            const float lo=xFromHz(proc.state.getRawParameterValue("low")->load());
            const float hi=xFromHz(proc.state.getRawParameterValue("high")->load());
            const float dl=std::abs(q.x-lo),dh=std::abs(q.x-hi);
            if(dl<=18.f&&(mode==1||mode==2||dl<=dh))bandHandle=0;
            else if(mode>=3&&dh<=18.f)bandHandle=1;
            if(bandHandle>=0)updateBand(e);
        }
        return;
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
