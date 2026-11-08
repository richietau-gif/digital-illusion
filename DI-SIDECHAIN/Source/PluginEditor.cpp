#include "PluginEditor.h"
#include <cmath>

namespace {
// DI SIDECHAIN 3.0: only the envelope is orange; every interactive
// accent, spectrum highlight and status light is electric lime green.
const juce::Colour bg(0xff0a1011), panel(0xff172023), panelDark(0xff111a1d),
                   accent(0xffafff4d), curveAccent(0xffff962f),
                   pale(0xffeef4f0), muted(0xff92aaa5);
void box(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c, float radius=10.f) {
    g.setColour(c); g.fillRoundedRectangle(r, radius);
}
void text(juce::Graphics& g, const juce::String& s, float size,
          juce::Rectangle<float> r, juce::Colour c, juce::Justification align=juce::Justification::centredLeft) {
    g.setColour(c); g.setFont(juce::FontOptions(size, juce::Font::plain));
    g.drawFittedText(s, r.toNearestInt(), align, 1);
}
float xFromHz(float hz) {
    return 280.f + juce::jlimit(0.f,1.f,std::log(juce::jlimit(20.f,20000.f,hz)/20.f)/std::log(1000.f))*816.f;
}
float hzFromX(float x) {
    return 20.f*std::pow(1000.f,juce::jlimit(0.f,1.f,(x-280.f)/816.f));
}
}

void DIStudioSkin::drawRotarySlider(juce::Graphics& g,int x,int y,int width,int height,
                                   float position,float startAngle,float endAngle,juce::Slider&)
{
    const auto rect=juce::Rectangle<float>((float)x,(float)y,(float)width,(float)height);
    const auto cx=rect.getCentreX(),cy=rect.getCentreY();
    const float radius=juce::jmax(9.f,juce::jmin(rect.getWidth(),rect.getHeight())*.39f);
    const float start=startAngle,end=endAngle;
    const float angle=juce::jmap(juce::jlimit(0.f,1.f,position),start,end);
    const auto outer=juce::Colour(0xff233630);
    g.setColour(juce::Colour(0xff071011));
    g.fillEllipse(cx-radius-4,cy-radius-4,2*(radius+4),2*(radius+4));
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff364c49),cx-radius,cy-radius,
                                           juce::Colour(0xff101b1d),cx+radius,cy+radius,false));
    g.fillEllipse(cx-radius,cy-radius,2*radius,2*radius);
    juce::Path track;
    track.addCentredArc(cx,cy,radius+2,radius+2,0,start,end,true);
    g.setColour(outer.brighter(.25f));
    g.strokePath(track,juce::PathStrokeType(4.f,juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
    juce::Path active;
    active.addCentredArc(cx,cy,radius+2,radius+2,0,start,angle,true);
    g.setColour(juce::Colour(0xffafff4d).withAlpha(.22f));
    g.strokePath(active,juce::PathStrokeType(7.f));
    g.setColour(juce::Colour(0xffafff4d));
    g.strokePath(active,juce::PathStrokeType(3.3f,juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
    juce::Point<float> needle(cx+std::sin(angle)*(radius-9),cy-std::cos(angle)*(radius-9));
    g.setColour(juce::Colour(0xffedfff1));
    g.drawLine(cx,cy,needle.x,needle.y,2.2f);
    g.fillEllipse(needle.x-2,needle.y-2,4,4);
}
void DIStudioSkin::drawLinearSlider(juce::Graphics& g,int x,int y,int width,int height,
                                   float sliderPos,float,float,
                                   juce::Slider::SliderStyle style,juce::Slider& slider)
{
    if(style!=juce::Slider::LinearHorizontal)return;
    const float cy=y+height*.5f;
    const float left=(float)x+5.f,right=x+width-5.f;
    const auto active=slider.findColour(juce::Slider::thumbColourId);
    g.setColour(active.withAlpha(.18f));
    g.drawLine(left,cy,right,cy,4.f);
    g.setColour(active.withAlpha(.27f));
    g.drawLine(left,cy,sliderPos,cy,7.f);
    g.setColour(active);
    g.drawLine(left,cy,sliderPos,cy,3.f);
    g.setColour(active.brighter(.18f));
    g.fillEllipse(sliderPos-5,cy-5,10,10);
}
void DIStudioSkin::drawComboBox(juce::Graphics& g,int width,int height,bool,
                               int,int,int,int,juce::ComboBox&)
{
    const juce::Rectangle<float> rect(0,0,(float)width,(float)height);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff273a36),0,0,
                                           juce::Colour(0xff101b1d),0,(float)height,false));
    g.fillRoundedRectangle(rect.reduced(.5f),5.f);
    g.setColour(juce::Colour(0xff405950));
    g.drawRoundedRectangle(rect.reduced(.5f),5.f,1.f);
    juce::Path arrow;
    arrow.startNewSubPath(width-18.f,height*.43f);
    arrow.lineTo(width-13.f,height*.60f);
    arrow.lineTo(width-8.f,height*.43f);
    g.setColour(juce::Colour(0xffafff4d));
    g.strokePath(arrow,juce::PathStrokeType(1.6f));
}
DISidechainEditor::DISidechainEditor(DISidechainProcessor& p) : AudioProcessorEditor(&p), proc(p) {
    skin.setColour(juce::ComboBox::backgroundColourId, panel);
    skin.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff334741));
    skin.setColour(juce::ComboBox::textColourId, pale);
    skin.setColour(juce::PopupMenu::backgroundColourId, panel);
    skin.setColour(juce::PopupMenu::textColourId, pale);
    skin.setColour(juce::Slider::thumbColourId, accent);
    skin.setColour(juce::Slider::trackColourId, accent);
    skin.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff35473f));
    skin.setColour(juce::Slider::textBoxTextColourId, accent);
    skin.setColour(juce::Slider::textBoxBackgroundColourId, panelDark);
    skin.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setLookAndFeel(&skin);
    setWantsKeyboardFocus(true);

    const char* ids[] = {"depth","mix","threshold","length","smooth","low","high","retrigger","midiNote","midiChannel","detectorHP","outputFilterMin","outputFilterMax","outputFilterAmount","outputFilterResonance","outputFilterDrive","detectorAttackMs","detectorReleaseMs","multiLowDepth","multiMidDepth","multiHighDepth","envelopeOffset"};
    const char* titles[] = {"DEPTH","MIX","THRESHOLD dB","LENGTH ms","SMOOTH","LOW Hz","HIGH Hz","HOLDOFF","MIDI NOTE (-1 ANY)","MIDI CH (0 ANY)","TRIGGER HP Hz","FILTER MIN Hz","FILTER MAX Hz","FILTER AMOUNT","RESONANCE Q","FILTER DRIVE","TRIGGER ATTACK ms","TRIGGER RELEASE ms","MULTI LOW DEPTH","MULTI MID DEPTH","MULTI HIGH DEPTH","OFFSET ± %"};
    for (int i=0;i<22;++i) {
        auto s=std::make_unique<juce::Slider>();
        const bool rotary=(i==0||i==4||i==1||(i>=11&&i<=15));
        s->setSliderStyle(rotary?juce::Slider::RotaryHorizontalVerticalDrag:
                                 juce::Slider::LinearHorizontal);
        if(rotary)s->setTextBoxStyle(juce::Slider::TextBoxBelow,false,78,22);

        if(i>=8&&i<=12)s->setNumDecimalPlacesToDisplay(0);
        if(i>=13)s->setNumDecimalPlacesToDisplay(2);
        if(!rotary)s->setTextBoxStyle(juce::Slider::TextBoxRight,false,67,23);
        s->setColour(juce::Slider::textBoxTextColourId,pale);
        if(i==21){
            s->setComponentID("envelopeOffset");
            s->setColour(juce::Slider::thumbColourId,juce::Colour(0xff35dce7));
            s->setColour(juce::Slider::textBoxTextColourId,juce::Colour(0xff35dce7));
            s->setNumDecimalPlacesToDisplay(1);
            s->setDoubleClickReturnValue(true,0.);
        }
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
        btn->setColour(juce::TextButton::buttonColourId,juce::Colour(0xff283730));
        btn->setColour(juce::TextButton::buttonOnColourId,accent);
        btn->setColour(juce::TextButton::textColourOffId,pale);
        btn->setColour(juce::TextButton::textColourOnId,juce::Colour(0xff102012));
        btn->setClickingTogglesState(false);
        btn->onClick=[this,i]{setParameter("outputFilterType",(float)i);repaint();};
        addAndMakeVisible(*btn);
        filterTypeButtons[(size_t)i]=std::move(btn);
    }
    const char* slopeNames[]={"12 dB","24 dB"};
    for(int i=0;i<2;++i){
        auto btn=std::make_unique<juce::TextButton>(slopeNames[i]);
        btn->setComponentID("filter-slope-"+juce::String(i));
        btn->setColour(juce::TextButton::buttonColourId,juce::Colour(0xff283730));
        btn->setColour(juce::TextButton::buttonOnColourId,accent);
        btn->setColour(juce::TextButton::textColourOffId,pale);
        btn->setColour(juce::TextButton::textColourOnId,juce::Colour(0xff102012));
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
    const char* factory[]={"CLASSIC PUMP","TIGHT KICK","SMOOTH DUCK",
                           "GHOST KICK","BREAKBEAT","WOBBLE"};
    for(int i=0;i<6;++i){
        auto button=std::make_unique<juce::TextButton>(factory[i]);
        button->setComponentID("factory-preset-"+juce::String(i));
        button->setColour(juce::TextButton::buttonColourId,juce::Colour(0xff20312e));
        button->setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff493768));
        button->setColour(juce::TextButton::textColourOffId,pale);
        button->setColour(juce::TextButton::textColourOnId,juce::Colour(0xffc9b4ff));
        button->onClick=[this,i]{
            envelopePreset->setSelectedId(i+1,juce::sendNotificationSync);
        };
        addAndMakeVisible(*button);
        presetButtons[(size_t)i]=std::move(button);
    }

    // setSize invokes resized synchronously: do not move above child construction.
    setSize(1380,860);
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
    const int activePreset=envelopePreset->getSelectedId()-1;
    for(int i=0;i<6;++i)
        presetButtons[(size_t)i]->setToggleState(i==activePreset,juce::dontSendNotification);
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
    const auto dark=juce::Colour(0xff0c171b);
    const auto border=juce::Colour(0xff294d47);
    const auto cyan=juce::Colour(0xff31e9bf);
    auto section=[&](juce::Rectangle<float> area,const juce::String& name){
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff1c3033),
                           area.getX(),area.getY(),
                           juce::Colour(0xff101b20),area.getX(),area.getBottom(),false));
        g.fillRoundedRectangle(area,10.f);
        g.setColour(border.withAlpha(.80f));
        g.drawRoundedRectangle(area.reduced(.7f),10.f,1.f);
        g.setColour(juce::Colour(0xff244348));
        g.fillRoundedRectangle(area.removeFromTop(36),7.f);
        text(g,name,12,{area.getX()+14,area.getY()-29,area.getWidth()-22,21},pale);
    };
    // Panel groupings and clean hierarchy like studio-grade audio software.
    section({12,91,239,528},"SIDECHAIN INPUT");
    g.setColour(juce::Colour(0xff4ba8ec));g.fillRoundedRectangle({13,126,3,486},2.f);
    section({260,91,850,425},"SHAPE EDITOR");
    section({1120,91,247,186},"OUTPUT");
    section({1120,288,247,328},"FACTORY PRESETS");
    g.setColour(juce::Colour(0xffaf8bff));g.fillRoundedRectangle({1121,326,3,282},2.f);
    section({260,527,850,88},"FREQUENCY TARGET");
    section({12,628,908,220},"FILTER ENGINE");
    g.setColour(accent.withAlpha(.65f));g.fillRoundedRectangle({13,665,3,176},2.f);
    section({930,628,437,220},"SIDECHAIN CONTROLS");

    // Brand/navigation bar: orange reserved exclusively for envelope.
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff142a32),0,0,
                      juce::Colour(0xff071116),0,76,false));
    g.fillRect(0,0,1380,77);
    g.setColour(border);g.drawLine(0,77,1380,77,1.f);
    g.setColour(cyan);g.drawLine(20,41,28,41,2);
    const juce::Point<float> signal[]={{28,41},{34,27},{40,53},{47,19},{54,47},
                                        {60,33},{67,41},{76,41}};
    for(int i=1;i<8;++i)g.drawLine(juce::Line<float>(signal[i-1],signal[i]),2.f);
    text(g,"DI SIDECHAIN",23,{96,15,240,29},pale);
    text(g,"3.0",12,{297,21,55,23},accent);
    text(g,"D U C K   &   F I L T E R",10,{98,49,272,16},muted);
    box(g,{390,17,318,41},juce::Colour(0xff203438),8);
    text(g,"DIGITAL ILLUSION  ·  STUDIO SERIES",10,{409,23,284,28},
         pale,juce::Justification::centred);
    text(g,"AUDIO + MIDI",11,{907,27,156,23},muted,juce::Justification::centredRight);
    g.setColour(cyan.withAlpha(.7f));g.fillEllipse(1068,33,9,9);
    box(g,{1094,24,111,31},juce::Colour(0xff213935),9);
    text(g,triggerFlashTicks>0?"TRIGGERED":"TRIGGER READY",9,{1096,25,108,29},
         triggerFlashTicks>0?accent:muted,juce::Justification::centred);
    const bool enabled=proc.state.getRawParameterValue("outputFilterOn")->load()>.5f;
    box(g,{1230,17,137,42},enabled?juce::Colour(0xff28583c):juce::Colour(0xff263439),11);
    text(g,enabled?"FILTER ON":"FILTER OFF",12,{1240,22,119,29},
         enabled?accent:muted,juce::Justification::centred);

    // LEFT: input/detector. All five trigger dynamics controls live here.
    text(g,"AUDIO / MIDI TRIGGER",10,{27,134,205,19},muted);
    box(g,{29,161,206,36},juce::Colour(0xff13282a),7);
    text(g,"Use kick / percussion / gate as external sidechain",9,
         {34,164,192,30},pale,juce::Justification::centred);
    g.setColour(cyan.withAlpha(.8f));g.drawLine(27,211,234,211,1.f);
    text(g,"DETECTOR",10,{29,219,180,19},accent);
    // Mini signal display from the actual input FFT, updated from the audio history.
    box(g,{28,547,207,54},juce::Colour(0xff0d2224),7);
    for(int i=0;i<24;++i){
        const float h=3.f+spectrumBands[(size_t)i]*41.f;
        g.setColour(cyan.withAlpha(.22f+spectrumBands[(size_t)i]*.55f));
        g.drawLine(35.f+i*8.f,572.f-h*.5f,35.f+i*8.f,572.f+h*.5f,2.f);
    }
    text(g,"LIVE TRIGGER INPUT",9,{29,527,207,15},muted);

    // CENTER: preset / source / mode / length and huge editable orange curve.
    text(g,"ENVELOPE",12,{280,131,142,21},pale);
    text(g,"PRESET",9,{687,136,62,14},muted);
    text(g,"SOURCE",9,{278,165,175,14},muted);
    text(g,"MODE",9,{512,165,175,14},muted);
    text(g,"LENGTH",9,{746,165,175,14},muted);
    text(g,snapToGrid?"SNAP ON":"SNAP OFF",10,{1008,135,84,17},
         snapToGrid?accent:muted,juce::Justification::centredRight);

    const bool synced=proc.state.getRawParameterValue("sync")->load()>.5f;
    const int index=juce::jlimit(0,12,juce::roundToInt(
        proc.state.getRawParameterValue("division")->load()));
    const juce::String times[]={"1/32","1/16","1/8","1/4","1/2","1 BAR","2 BARS",
       "1/16T","1/8T","1/4T","1/16D","1/8D","1/4D"};
    const auto duration=synced?times[index]:
        juce::String(juce::roundToInt(proc.state.getRawParameterValue("length")->load()))+" ms";
    text(g,"CURRENT: "+duration,9,{947,180,146,17},muted,
        juce::Justification::centredRight);
    box(g,graph,juce::Colour(0xff101b24),4);
    for(int i=0;i<=8;++i){
        const float x=graph.getX()+graph.getWidth()*i/8.f;
        g.setColour(juce::Colour(0xff294048));
        g.drawVerticalLine(juce::roundToInt(x),graph.getY(),graph.getBottom());
    }
    for(int i=0;i<=6;++i){
        const float y=graph.getY()+graph.getHeight()*i/6.f;
        g.setColour(juce::Colour(0xff294048));
        g.drawHorizontalLine(juce::roundToInt(y),graph.getX(),graph.getRight());
        text(g,juce::String(-i*6)+" dB",9,{262,y-6,22,14},muted,
             juce::Justification::centredRight);
    }
    const float offsetPercent=proc.state.getRawParameterValue("envelopeOffset")->load();
    const float offsetPhase=offsetPercent*.01f;
    const bool loop=proc.state.getRawParameterValue("playbackMode")->load()>.5f;
    const auto timing=juce::Colour(0xff35dce7);
    if(std::abs(offsetPhase)>.00001f){
        // The ghost is the unshifted editable waveform. The bright orange
        // path is the actual timing used for gain and filter modulation.
        juce::Path reference;
        for(int i=0;i<=350;++i){
            const float t=(float)i/350.f;
            const float x=graph.getX()+t*graph.getWidth();
            const float y=graph.getBottom()-proc.evaluateEnvelope(t)*graph.getHeight();
            if(i==0)reference.startNewSubPath(x,y);else reference.lineTo(x,y);
        }
        const float dashes[]={4.f,6.f};
        juce::Path dashed;
        juce::PathStrokeType(1.3f).createDashedStroke(dashed,reference,dashes,2);
        g.setColour(timing.withAlpha(.50f));g.strokePath(dashed,juce::PathStrokeType(1.2f));
    }
    text(g,"OFFSET "+juce::String(offsetPercent>0?"+":"")+juce::String(offsetPercent,1)+"%",
        11,{925,158,166,20},timing,juce::Justification::centredRight);
    juce::Path curve;
    for(int i=0;i<=350;++i){
        const float phase=(float)i/350.f;
        const float x=graph.getX()+graph.getWidth()*phase;
        const float y=graph.getBottom()-proc.evaluateShiftedEnvelope(phase,loop)*graph.getHeight();
        if(i==0)curve.startNewSubPath(x,y);else curve.lineTo(x,y);
    }
    juce::Path fill(curve);
    fill.lineTo(graph.getRight(),graph.getBottom());
    fill.lineTo(graph.getX(),graph.getBottom());fill.closeSubPath();
    g.setGradientFill(juce::ColourGradient(curveAccent.withAlpha(.29f),
         graph.getCentreX(),graph.getY(),curveAccent.withAlpha(.02f),
         graph.getCentreX(),graph.getBottom(),false));
    g.fillPath(fill);
    g.setColour(curveAccent.withAlpha(.15f));
    g.strokePath(curve,juce::PathStrokeType(9.f));
    g.setColour(curveAccent);
    g.strokePath(curve,juce::PathStrokeType(3.0f));
    const float phase=juce::jlimit(0.f,1.f,proc.visualPhase.load());
    if(phase<1.f){
        const float cursor=graph.getX()+phase*graph.getWidth();
        g.setColour(cyan.withAlpha(.75f));g.drawLine(cursor,graph.getY(),
            cursor,graph.getBottom(),1.2f);
    }
    for(int i=0;i<proc.getEnvelopeNodeCount();++i){
        const auto node=proc.getEnvelopeNode(i);
        if(std::abs(offsetPhase)>.00001f){
            const float x0=graph.getX()+node.x*graph.getWidth();
            const float y0=graph.getBottom()-node.y*graph.getHeight();
            g.setColour(timing.withAlpha(.62f));g.drawEllipse(x0-4,y0-4,8,8,1.f);
        }
        float shifted=node.x+offsetPhase;
        if(loop){
            shifted=std::fmod(shifted,1.f);
            if(shifted<0)shifted+=1.f;
        }
        if(shifted<0.f||shifted>1.f)continue;
        const float x=graph.getX()+shifted*graph.getWidth();
        const float y=graph.getBottom()-node.y*graph.getHeight();
        g.setColour(curveAccent.withAlpha(.25f));g.fillEllipse(x-11,y-11,22,22);
        g.setColour(curveAccent);g.fillEllipse(x-7,y-7,14,14);
        g.setColour(juce::Colour(0xfffcfaf0));g.fillEllipse(x-3,y-3,6,6);
    }
    text(g,"1.1",9,{287,481,45,17},muted);
    text(g,"2.1",9,{669,481,45,17},muted);
    text(g,"END",9,{1040,481,40,17},muted,juce::Justification::centredRight);
    text(g,"ORANGE: ACTIVE CURVE  ·  CYAN DASH: ORIGINAL",
         9,{287,493,435,18},timing.withAlpha(.87f));
    text(g,"GR "+juce::String(proc.visualGainReductionDb.load(),1)+" dB",
         11,{955,136,127,18},accent,juce::Justification::centredRight);

    // RIGHT: output/mix/master rotary knobs and quick preset library.
    const char* masterNames[]={"DEPTH","SMOOTH","MIX"};
    for(int i=0;i<3;++i)
        text(g,masterNames[i],10,{1133.f+i*77.f,136,70,17},
             muted,juce::Justification::centred);
    text(g,"GREEN: ACTIVE CONTROL  ·  ORANGE: SHAPE",9,
         {1131,264,222,15},muted,juce::Justification::centred);
    text(g,"ONE CLICK TO LOAD A CURVE",9,{1134,323,217,16},juce::Colour(0xffb9a0ff));
    text(g,"Custom curves are saved with the Ableton project",9,
         {1136,590,214,18},muted,juce::Justification::centred);

    // Compact spectrum with actual incoming signal energy.
    for(int i=0;i<24;++i){
        const float amp=2.f+juce::jlimit(0.f,1.f,spectrumBands[(size_t)i])*26.f;
        g.setColour(cyan.withAlpha(.2f+.6f*spectrumBands[(size_t)i]));
        g.fillRoundedRectangle({280.f+i*33.f,567.f-amp*.5f,15.f,amp},3.f);
    }
    text(g,"20 Hz",9,{280,579,52,13},muted);
    text(g,"20 kHz",9,{1046,579,51,13},muted,juce::Justification::centredRight);
    const int mode=juce::jlimit(0,4,juce::roundToInt(
        proc.state.getRawParameterValue("mode")->load()));
    const float low=proc.state.getRawParameterValue("low")->load();
    const float high=proc.state.getRawParameterValue("high")->load();
    const float lowX=xFromHz(low);
    const float highX=xFromHz(high);
    if(mode!=0){
        const float l=mode==1?280.f:lowX;
        const float r=mode==1?lowX:mode==2?1096.f:highX;
        box(g,{l,545,juce::jmax(1.f,r-l),40},accent.withAlpha(.14f),2.f);
        g.setColour(accent);g.drawVerticalLine((int)l,545,584);
        g.drawVerticalLine((int)r,545,584);
    }
    const char* targetNames[]={"FULL","LOW","HIGH","BAND","MULTI"};
    for(int i=0;i<5;++i){
        const juce::Rectangle<float> area{279.f+i*165.f,593,155,19};
        box(g,area,mode==i?juce::Colour(0xff3b8e52):
                   juce::Colour(0xff203238),5);
        text(g,targetNames[i],9,area,mode==i?accent:pale,
             juce::Justification::centred);
    }

    // BOTTOM FILTER: interactive controls and an actual modeled response.
    const bool filterEnabled=proc.state.getRawParameterValue("outputFilterOn")->load()>.5f;
    text(g,"FILTER",11,{28,663,60,20},pale);
    text(g,filterEnabled?"ACTIVE":"BYPASSED",9,{85,664,79,18},
         filterEnabled?accent:muted);
    text(g,"TYPE",9,{172,645,56,14},muted);
    text(g,"SLOPE",9,{529,645,66,14},muted);
    const int filterType=juce::jlimit(0,3,
         juce::roundToInt(proc.state.getRawParameterValue("outputFilterType")->load()));
    const float minHz=proc.state.getRawParameterValue("outputFilterMin")->load();
    const float maxHz=proc.state.getRawParameterValue("outputFilterMax")->load();
    const float Q=proc.state.getRawParameterValue("outputFilterResonance")->load();
    const float fc=juce::jlimit(20.f,20000.f,proc.getVisualFilterCutoffHz());
    const juce::Rectangle<float> response{699,694,202,123};
    box(g,response,dark,5);
    for(int k=0;k<5;++k){
        const float x=response.getX()+k*response.getWidth()/4.f;
        g.setColour(border.withAlpha(.5f));g.drawVerticalLine((int)x,
                         response.getY(),response.getBottom());
    }
    // Frequency-response preview based on analog SVF biquad magnitude,
    // separate from the real DSP but reacting to its actual cutoff and Q.
    const float w0=juce::MathConstants<float>::twoPi;
    juce::Path responseLine;
    for(int j=0;j<180;++j){
        const float f=20.f*std::pow(1000.f,(float)j/179.f);
        const float ratio=f/juce::jmax(20.f,fc);
        const float k=1.f/juce::jmax(.5f,Q);
        const float residual=1.f-ratio*ratio;
        const float damping=k*ratio;
        const float den=std::sqrt(residual*residual+damping*damping);
        float num=1.f;
        if(filterType==1)num=ratio*ratio;
        if(filterType==2)num=ratio;
        if(filterType==3)num=std::sqrt(1.f+ratio*ratio*ratio*ratio);
        float mag=juce::jlimit(0.f,3.f,num/juce::jmax(.001f,den));
        const float db=juce::jlimit(-35.f,12.f,20.f*std::log10(juce::jmax(.0001f,mag)));
        const float x=response.getX()+(float)j/179.f*response.getWidth();
        const float y=response.getBottom()-5.f-(db+35.f)/47.f*(response.getHeight()-10.f);
        if(j==0)responseLine.startNewSubPath(x,y);else responseLine.lineTo(x,y);
    }
    g.setColour(accent.withAlpha(filterEnabled?.20f:.07f));
    g.strokePath(responseLine,juce::PathStrokeType(7.f));
    g.setColour(filterEnabled?accent:muted);
    g.strokePath(responseLine,juce::PathStrokeType(2.2f));
    text(g,""+juce::String(fc>=1000?fc/1000.f:fc,1)+
         (fc>=1000?" kHz":" Hz"),9,{775,820,128,18},accent,
         juce::Justification::centredRight);
    text(g,"DRIVE  ·  RESONANCE  ·  CUTOFF SWEEP",9,{32,825,635,15},muted);

    // BOTTOM RIGHT: all additional parameters remain accessible, compact.
    text(g,"AUDIO DETECTOR / MIDI / MULTIBAND",10,{942,663,410,16},muted);
    g.restoreState();
}
void DISidechainEditor::resized(){
    const float sc=(float)getWidth()/designWidth;
    const auto bounds=[sc](float x,float y,float w,float h){
        return juce::Rectangle<int>(juce::roundToInt(x*sc),juce::roundToInt(y*sc),
            juce::jmax(1,juce::roundToInt(w*sc)),juce::jmax(1,juce::roundToInt(h*sc)));};
    if(sliders.size()!=22||labels.size()!=22||combos.size()!=3||
       sync==nullptr||outputFilterToggle==nullptr||envelopePreset==nullptr||
       filterTypeButtons[0]==nullptr||filterSlopeButtons[0]==nullptr||
       presetButtons[0]==nullptr)return;
    // Main source, playback, tempo divisions and saved factory shapes.
    for(int i=0;i<3;++i)
        combos[(size_t)i]->setBounds(bounds(279.f+i*234.f,181.f,211,25));
    envelopePreset->setBounds(bounds(750,130,188,25));
    // Compact timing control below the curve, balanced around zero.
    labels[21]->setBounds(bounds(738,485,91,22));
    labels[21]->setVisible(true);
    sliders[21]->setBounds(bounds(837,485,254,24));
    sliders[21]->setVisible(true);
    sync->setBounds(bounds(1002,180,98,26));
    // Left independent audio transient detector and input.
    const int detector[]={2,7,10,16,17};
    for(int k=0;k<5;++k){
        const int i=detector[k];
        const float y=245.f+k*55.f;
        labels[(size_t)i]->setBounds(bounds(30,y,204,17));
        sliders[(size_t)i]->setBounds(bounds(29,y+18.f,205,29));
        labels[(size_t)i]->setVisible(true);sliders[(size_t)i]->setVisible(true);
    }
    // Right-hand output rotary controls remain prominent and editable.
    const int master[]={0,4,1};
    for(int k=0;k<3;++k){
        const int i=master[k];
        const float x=1132.f+k*78.f;
        labels[(size_t)i]->setBounds(bounds(x,137,73,17));
        sliders[(size_t)i]->setBounds(bounds(x,154,73,103));
        labels[(size_t)i]->setVisible(false);sliders[(size_t)i]->setVisible(true);
    }
    // Presets are accessible as one-click buttons, in addition to the dropdown.
    for(int k=0;k<6;++k)
        presetButtons[(size_t)k]->setBounds(bounds(1135,350.f+k*38.f,217,31));
    // Bottom filter signal chain: colored controls and real response display.
    outputFilterToggle->setBounds(bounds(85,651,105,25));
    for(int i=0;i<4;++i)
        filterTypeButtons[(size_t)i]->setBounds(bounds(202.f+i*79.f,662,74,23));
    for(int i=0;i<2;++i)
        filterSlopeButtons[(size_t)i]->setBounds(bounds(542.f+i*72.f,662,68,23));
    for(int k=0;k<5;++k){
        const int i=11+k;
        const float x=32.f+k*129.f;
        labels[(size_t)i]->setBounds(bounds(x,809,117,16));
        sliders[(size_t)i]->setBounds(bounds(x,701,117,105));
        labels[(size_t)i]->setVisible(true);sliders[(size_t)i]->setVisible(true);
    }
    // Compact but always visible technical controls under the 2nd right panel.
    const int extra[]={3,5,6,8,9,18,19,20};
    for(int k=0;k<8;++k){
        const int i=extra[k],row=k/2,col=k%2;
        const float x=944.f+col*204.f,y=686.f+row*38.f;
        labels[(size_t)i]->setBounds(bounds(x,y,198,14));
        sliders[(size_t)i]->setBounds(bounds(x,y+15,196,22));
        labels[(size_t)i]->setVisible(true);sliders[(size_t)i]->setVisible(true);
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
    const float offset=proc.state.getRawParameterValue("envelopeOffset")->load()*.01f;
    const bool loop=proc.state.getRawParameterValue("playbackMode")->load()>.5f;
    x-=offset;
    if(loop){x=std::fmod(x,1.f);if(x<0.f)x+=1.f;}
    x=juce::jlimit(0.f,1.f,x);
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
    if(q.x>=1005&&q.x<=1104&&q.y>=132&&q.y<=158){snapToGrid=!snapToGrid;repaint();return;}
    if(graph.contains(q)&&e.mods.isAltDown()){
        // Alt-drag the curve segment, with per-segment cubic shaping.
        float t=juce::jlimit(0.f,1.f,(q.x-graph.getX())/graph.getWidth());
        const float offset=proc.state.getRawParameterValue("envelopeOffset")->load()*.01f;
        const bool loop=proc.state.getRawParameterValue("playbackMode")->load()>.5f;
        t-=offset;
        if(loop){t=std::fmod(t,1.f);if(t<0.f)t+=1.f;}
        t=juce::jlimit(0.f,1.f,t);
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
            float shifted=node.x+
                proc.state.getRawParameterValue("envelopeOffset")->load()*.01f;
            if(proc.state.getRawParameterValue("playbackMode")->load()>.5f){
                shifted=std::fmod(shifted,1.f);
                if(shifted<0.f)shifted+=1.f;
            }
            if(shifted<0.f||shifted>1.f)continue;
            const float dx=q.x-(graph.getX()+shifted*graph.getWidth());
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
    if(q.y>=592&&q.y<=615&&q.x>=278&&q.x<=1110){
        const int index=juce::jlimit(0,4,(int)((q.x-279.f)/165.f));
        setParameter("mode",(float)index);
        repaint();
        return;
    }
    if(q.y>=543&&q.y<=587&&q.x>=279&&q.x<=1098){
        const int mode=juce::roundToInt(proc.state.getRawParameterValue("mode")->load());
        if(mode!=0){
            const float lo=xFromHz(proc.state.getRawParameterValue("low")->load());
            const float hi=xFromHz(proc.state.getRawParameterValue("high")->load());
            const float dl=std::abs(q.x-lo),dh=std::abs(q.x-hi);
            if(dl<=20.f&&(mode==1||mode==2||dl<=dh))bandHandle=0;
            else if(mode>=3&&dh<=20.f)bandHandle=1;
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
    const float offset=proc.state.getRawParameterValue("envelopeOffset")->load()*.01f;
    const bool loop=proc.state.getRawParameterValue("playbackMode")->load()>.5f;
    x-=offset;
    if(loop){x=std::fmod(x,1.f);if(x<0.f)x+=1.f;}
    x=juce::jlimit(0.f,1.f,x);
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
