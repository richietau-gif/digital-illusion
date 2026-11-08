#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

// Stable antiderivative of tanh(x); used for first-order ADAA saturation.
static float logCoshStable(float x) noexcept {
 const float a=std::abs(x);
 return a+std::log1p(std::exp(-2.f*a))-std::log(2.f);
}
static juce::AudioProcessorValueTreeState::ParameterLayout layoutParams(){
 std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
 auto f=[&](const char* id,const char* name,float lo,float hi,float def){p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},name,juce::NormalisableRange<float>(lo,hi),def));};
 f("envelopeOffset","Envelope timing offset percent",-50,50,0);
 p.push_back(std::make_unique<juce::AudioParameterBool>(
     juce::ParameterID{"filterEnvelopeLink",1},"Link filter envelope to volume",true));
 f("filterEnvelopeMorph","Filter curve A/B morph",0,1,0);
 f("filterEnvelopeFollower","Audio envelope follower modulation",-1,1,0);
 f("depth","Depth",0,1,1); f("mix","Mix",0,1,1);f("threshold","Trigger threshold dB",-60,0,-22);f("length","Envelope milliseconds",10,2000,250);f("smooth","Smoothing milliseconds",0.1,20,2);f("low","Low cutoff",25,1500,120);f("high","High cutoff",60,18000,400);f("retrigger","Trigger holdoff milliseconds",1,250,35);
 f("midiNote","MIDI trigger note (-1 = any)",-1,127,-1);
 f("midiChannel","MIDI trigger channel (0 = any)",0,16,0);
 f("detectorHP","Audio trigger HP Hz (0 = off)",0,2000,0);
 f("detectorAttackMs","Audio trigger attack ms",0,30,0);
 f("detectorReleaseMs","Audio trigger release ms",1,250,4.16f);
 f("multiLowDepth","MULTI low band ducking",0,1,1);
 f("multiMidDepth","MULTI mid band ducking",0,1,1);
 f("multiHighDepth","MULTI high band ducking",0,1,1);
 // Logarithmic knob response gives precision at 40–500 Hz for bass wobbles.
 juce::NormalisableRange<float> cutoffRange(20.f,20000.f);
 cutoffRange.setSkewForCentre(1000.f);
 p.push_back(std::make_unique<juce::AudioParameterFloat>(
    juce::ParameterID{"outputFilterMin",1},"Output filter minimum Hz",cutoffRange,120.f));
 p.push_back(std::make_unique<juce::AudioParameterFloat>(
    juce::ParameterID{"outputFilterMax",1},"Output filter maximum Hz",cutoffRange,16000.f));
 f("outputFilterAmount","Output filter modulation",0,1,1);
 f("outputFilterResonance","Output filter resonance Q",0.5,12,0.70710678f);
 f("outputFilterDrive","Output filter drive",0,1,0);
 p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"outputFilterSlope",1},
     "Output filter slope",juce::StringArray{"12 dB","24 dB"},0));
 p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"outputFilterOn",1},"Output filter enabled",false));
 p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"outputFilterType",1},"Output filter type",juce::StringArray{"Low Pass","High Pass","Band Pass","Notch"},0));
 p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"mode",1},"Frequency mode",juce::StringArray{"Full","Low","High","Band","Multi"},0));
 p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"trigger",1},"Trigger mode",juce::StringArray{"Audio + MIDI","Audio only","MIDI only"},0));
 p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"playbackMode",1},"Envelope playback",juce::StringArray{"One Shot","Loop"},0));
 p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"sync",1},"Tempo sync",true));
 p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"division",1},"Beat division",juce::StringArray{"1/32","1/16","1/8","1/4","1/2","1 bar","2 bars","1/16T","1/8T","1/4T","1/16D","1/8D","1/4D"},2));
 return {p.begin(),p.end()};
}
juce::AudioProcessorValueTreeState::ParameterLayout DISidechainProcessor::makeParams(){return layoutParams();}
DISidechainProcessor::DISidechainProcessor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withInput("Sidechain",juce::AudioChannelSet::stereo(),false).withOutput("Output",juce::AudioChannelSet::stereo(),true)),state(*this,nullptr,"PARAMS",makeParams()){
 const float defaults[8]={1,0.92f,0.08f,0.02f,0.32f,0.75f,0.95f,1};for(int i=0;i<8;++i)points[i]=defaults[i];
 for(int i=0;i<filterEnvelopePoints;++i){
     filterEnvelopeBanks[0][i].store(defaults[i],std::memory_order_relaxed);
     filterEnvelopeBanks[1][i].store(i%2==0?1.f:.04f,std::memory_order_relaxed);
 }
}
bool DISidechainProcessor::isBusesLayoutSupported(const BusesLayout& b) const {return b.getMainInputChannelSet()==juce::AudioChannelSet::stereo()&&b.getMainOutputChannelSet()==juce::AudioChannelSet::stereo()&&(b.inputBuses.size()<2||b.inputBuses[1].isDisabled()||b.inputBuses[1]==juce::AudioChannelSet::mono()||b.inputBuses[1]==juce::AudioChannelSet::stereo());}
void DISidechainProcessor::prepareToPlay(double sampleRate,int maxBlock){
 sr=sampleRate;phase=1;detector=0;armed=true;refractory=0;smoothGain=1;
 detectorHpPreviousInput=0.;detectorHpPreviousOutput=0.;
 for(auto& channel:outputFilterStages)
  for(auto& stage:channel)stage={};
 outputFilterBlend=0.f;smoothedFilterQ=.70710678f;smoothedFilterDrive=0.f;
 drivePreviousInput.fill(0.f);
 followerLevel=0.f;visualFollowerLevel.store(0.f,std::memory_order_relaxed);
 visualFilterCutoffHz.store(20000.f);
 juce::dsp::ProcessSpec spec{sampleRate,(juce::uint32)juce::jmax(1,maxBlock),1};
 for(auto& c:filters){
  auto init=[&](juce::dsp::LinkwitzRileyFilter<float>& filter,
                juce::dsp::LinkwitzRileyFilterType type){
   filter.prepare(spec);filter.setType(type);filter.reset();
  };
  const auto lp=juce::dsp::LinkwitzRileyFilterType::lowpass;
  const auto hp=juce::dsp::LinkwitzRileyFilterType::highpass;
  init(c.low,lp);init(c.high,hp);
  init(c.bandLowPass,lp);init(c.bandHighPass,hp);
  init(c.phaseLowPass,lp);init(c.phaseHighPass,hp);
 }
}
int DISidechainProcessor::getEnvelopeNodeCount() const noexcept {
    const int n=activeNodeCount.load(std::memory_order_acquire);
    return n >= 2 ? juce::jlimit(2,maxEnvelopeNodes,n) : 8;
}
DISidechainProcessor::EnvelopeNode DISidechainProcessor::getEnvelopeNode(int index) const noexcept {
    const int count=activeNodeCount.load(std::memory_order_acquire);
    const int n=count>=2 ? juce::jlimit(2,maxEnvelopeNodes,count) : 8;
    index=juce::jlimit(0,n-1,index);
    if(count<2) return {(float)index/7.f,juce::jlimit(0.f,1.f,points[index].load())};
    const float x=nodeX[index].load(std::memory_order_relaxed);
    const float y=nodeY[index].load(std::memory_order_relaxed);
    return {juce::jlimit(0.f,1.f,std::isfinite(x)?x:0.f),
            juce::jlimit(0.f,1.f,std::isfinite(y)?y:1.f)};
}
void DISidechainProcessor::adoptLegacyPoints() noexcept {
    if(activeNodeCount.load(std::memory_order_acquire)>=2)return;
    for(int i=0;i<8;++i){
        nodeCurve[i].store(0.f,std::memory_order_relaxed);
        nodeX[i].store((float)i/7.f,std::memory_order_relaxed);
        nodeY[i].store(juce::jlimit(0.f,1.f,points[i].load()),std::memory_order_relaxed);
    }
    activeNodeCount.store(8,std::memory_order_release);
}
void DISidechainProcessor::applyEnvelopePreset(int presetIndex) noexcept {
    // Seven time/value keyframes per shape; explicit 0/1 endpoints keep
    // One Shot and Loop deterministic and avoid zero-duration segments.
    static constexpr float factory[6][7][2] = {
        // Pump: immediate ducking with a long smooth release.
        {{0.f,1.f},{.04f,.02f},{.19f,.02f},{.4f,.18f},
         {.65f,.63f},{.84f,.93f},{1.f,1.f}},
        // Tight Kick: very short dip with fast recovery.
        {{0.f,1.f},{.012f,0.f},{.08f,.03f},{.19f,.4f},
         {.34f,.93f},{.6f,1.f},{1.f,1.f}},
        // Smooth Duck: soft onset and gentle recovery.
        {{0.f,1.f},{.12f,.35f},{.26f,.2f},{.5f,.32f},
         {.73f,.7f},{.9f,.98f},{1.f,1.f}},
        // Ghost Kick: subtle groove for bus ducking.
        {{0.f,1.f},{.03f,.15f},{.14f,.15f},{.34f,.45f},
         {.53f,.7f},{.77f,.95f},{1.f,1.f}},
        // Breakbeat: three irregular ducks in a single cycle.
        {{0.f,1.f},{.07f,.08f},{.22f,1.f},{.41f,.15f},
         {.56f,1.f},{.79f,.08f},{1.f,1.f}},
        // Wobble: three cyclic sweeps; works with FILTER and Depth 0.
        {{0.f,1.f},{.15f,.05f},{.33f,1.f},{.49f,.05f},
         {.67f,1.f},{.83f,.05f},{1.f,1.f}}
    };
    if(presetIndex<0 || presetIndex>=6)return;
    activeNodeCount.store(0,std::memory_order_release);
    for(int i=0;i<7;++i){
        nodeX[(size_t)i].store(factory[presetIndex][i][0],std::memory_order_relaxed);
        nodeY[(size_t)i].store(factory[presetIndex][i][1],std::memory_order_relaxed);
        nodeCurve[(size_t)i].store(0.f,std::memory_order_relaxed);
    }
    activeNodeCount.store(7,std::memory_order_release);
}

void DISidechainProcessor::setEnvelopeNode(int index,float x,float y) noexcept {
    adoptLegacyPoints();
    const int n=getEnvelopeNodeCount();
    if(index<0||index>=n)return;
    if(!std::isfinite(x)||!std::isfinite(y))return;
    if(index==0)x=0.f;
    else if(index==n-1)x=1.f;
    else {
        const float left=nodeX[index-1].load(std::memory_order_relaxed);
        const float right=nodeX[index+1].load(std::memory_order_relaxed);
        x=juce::jlimit(left+0.001f,right-0.001f,x);
    }
    nodeX[index].store(x,std::memory_order_relaxed);
    nodeY[index].store(juce::jlimit(0.f,1.f,y),std::memory_order_relaxed);
}
int DISidechainProcessor::insertEnvelopeNode(float x,float y) noexcept {
    if(!std::isfinite(x)||!std::isfinite(y))return -1;
    adoptLegacyPoints();
    const int n=getEnvelopeNodeCount();
    if(n>=maxEnvelopeNodes)return -1;
    x=juce::jlimit(0.001f,0.999f,x);
    int index=1;
    while(index<n&&nodeX[index].load(std::memory_order_relaxed)<x)++index;
    const float left=nodeX[index-1].load(std::memory_order_relaxed);
    const float right=nodeX[index].load(std::memory_order_relaxed);
    if(right-left<0.002f)return -1;
    x=juce::jlimit(left+0.001f,right-0.001f,x);
    for(int i=n;i>index;--i){
        nodeX[i].store(nodeX[i-1].load(std::memory_order_relaxed),std::memory_order_relaxed);
        nodeY[i].store(nodeY[i-1].load(std::memory_order_relaxed),std::memory_order_relaxed);
        nodeCurve[i].store(nodeCurve[i-1].load(std::memory_order_relaxed),std::memory_order_relaxed);
    }
    nodeCurve[index].store(0.f,std::memory_order_relaxed);
    nodeX[index].store(x,std::memory_order_relaxed);
    nodeY[index].store(juce::jlimit(0.f,1.f,y),std::memory_order_relaxed);
    activeNodeCount.store(n+1,std::memory_order_release);
    return index;
}
bool DISidechainProcessor::removeEnvelopeNode(int index) noexcept {
    adoptLegacyPoints();
    const int n=getEnvelopeNodeCount();
    if(index<=0||index>=n-1||n<=2)return false;
    // Reduce the visible count first so a concurrent reader cannot see a stale final slot.
    activeNodeCount.store(n-1,std::memory_order_release);
    for(int i=index;i<n-1;++i){
        nodeX[i].store(nodeX[i+1].load(std::memory_order_relaxed),std::memory_order_relaxed);
        nodeY[i].store(nodeY[i+1].load(std::memory_order_relaxed),std::memory_order_relaxed);
        nodeCurve[i].store(nodeCurve[i+1].load(std::memory_order_relaxed),std::memory_order_relaxed);
    }
    return true;
}
float DISidechainProcessor::getEnvelopeSegmentCurve(int segment) const noexcept {
    const int n=getEnvelopeNodeCount();
    if(segment<0||segment>=n-1)return 0.f;
    if(activeNodeCount.load(std::memory_order_acquire)<2)return 0.f;
    const float v=nodeCurve[segment].load(std::memory_order_relaxed);
    return std::isfinite(v)?juce::jlimit(-1.f,1.f,v):0.f;
}
void DISidechainProcessor::setEnvelopeSegmentCurve(int segment,float value) noexcept {
    if(!std::isfinite(value))return;
    adoptLegacyPoints();
    const int n=getEnvelopeNodeCount();
    if(segment<0||segment>=n-1)return;
    nodeCurve[segment].store(juce::jlimit(-1.f,1.f,value),std::memory_order_relaxed);
}
static float cubicEnvelopeEase(float u,float curve) noexcept {
    // Bezier y(t) with P0=(0,0) and P3=(1,1).
    // Monotone, bounded control values avoid DSP overshoot.
    const float c1=curve>0.f?curve*.8f:0.f;
    const float c2=curve<0.f?1.f+curve*.8f:1.f;
    const float inv=1.f-u;
    return 3.f*inv*inv*u*c1+3.f*inv*u*u*c2+u*u*u;
}
float DISidechainProcessor::evaluateEnvelope(double t) const noexcept {
    if(!std::isfinite(t))return 1.f;
    t=juce::jlimit(0.0,1.0,t);
    const int count=activeNodeCount.load(std::memory_order_acquire);
    if(count<2){
        const double x=t*7.0;
        const int i=juce::jmin(6,(int)x);
        const float a=points[i].load(std::memory_order_relaxed);
        const float b=points[i+1].load(std::memory_order_relaxed);
        float u=(float)(x-i);u=u*u*(3.f-2.f*u);
        return juce::jlimit(0.f,1.f,a+(b-a)*u);
    }
    const int n=juce::jlimit(2,maxEnvelopeNodes,count);
    auto left=getEnvelopeNode(0);
    for(int i=1;i<n;++i){
        auto right=getEnvelopeNode(i);
        if(t<=right.x||i==n-1){
            const float span=right.x-left.x;
            if(span<=0.000001f)return right.y;
            float u=juce::jlimit(0.f,1.f,((float)t-left.x)/span);
            u=cubicEnvelopeEase(u,getEnvelopeSegmentCurve(i-1));
            return juce::jlimit(0.f,1.f,left.y+(right.y-left.y)*u);
        }
        left=right;
    }
    return left.y;
}
float DISidechainProcessor::evaluateShiftedEnvelope(double t,bool loop) const noexcept {
    if(!std::isfinite(t))return 1.f;
    const float offset=juce::jlimit(-50.f,50.f,
        state.getRawParameterValue("envelopeOffset")->load(std::memory_order_relaxed));
    if(std::abs(offset)<.0001f)return evaluateEnvelope(t); // exact legacy behavior
    double shifted=t-(double)offset*.01;
    if(loop){
        shifted=std::fmod(shifted,1.0);
        if(shifted<0.0)shifted+=1.0;
    }else{
        // Past either edge, hold the neutral endpoints. No circular wrap
        // in ONE SHOT so a delayed kick cannot leak a previous cycle.
        if(shifted<0.0)return 1.f;
        if(shifted>=1.0)return 1.f;
    }
    return evaluateEnvelope(shifted);
}
float DISidechainProcessor::getFilterEnvelopePoint(int bank,int index) const noexcept {
    if(bank<0||bank>=2||index<0||index>=filterEnvelopePoints)return 1.f;
    return filterEnvelopeBanks[(size_t)bank][(size_t)index].load(std::memory_order_relaxed);
}
void DISidechainProcessor::setFilterEnvelopePoint(int bank,int index,float y) noexcept {
    if(bank<0||bank>=2||index<0||index>=filterEnvelopePoints||!std::isfinite(y))return;
    filterEnvelopeBanks[(size_t)bank][(size_t)index].store(
        juce::jlimit(0.f,1.f,y),std::memory_order_relaxed);
}
float DISidechainProcessor::evaluateFilterBank(int bank,double t,bool loop) const noexcept {
    if(!std::isfinite(t))return 1.f;
    const double offset=(double)juce::jlimit(-50.f,50.f,
       state.getRawParameterValue("envelopeOffset")->load(std::memory_order_relaxed))*.01;
    t-=offset;
    if(loop){
        t=std::fmod(t,1.0);
        if(t<0.0)t+=1.0;
    }else if(t<0.0||t>=1.0){
        return 1.f;
    }
    t=juce::jlimit(0.0,1.0,t);
    const double position=t*(filterEnvelopePoints-1);
    const int i=juce::jmin(filterEnvelopePoints-2,(int)position);
    float u=(float)(position-i);
    u=u*u*(3.f-2.f*u);
    const float left=getFilterEnvelopePoint(bank,i),right=getFilterEnvelopePoint(bank,i+1);
    return juce::jlimit(0.f,1.f,left+(right-left)*u);
}
float DISidechainProcessor::evaluateFilterEnvelope(double t,bool loop) const noexcept {
    if(state.getRawParameterValue("filterEnvelopeLink")->load(std::memory_order_relaxed)>.5f)
        return evaluateShiftedEnvelope(t,loop);
    const float morph=juce::jlimit(0.f,1.f,
        state.getRawParameterValue("filterEnvelopeMorph")->load(std::memory_order_relaxed));
    const float a=evaluateFilterBank(0,t,loop),b=evaluateFilterBank(1,t,loop);
    return a+(b-a)*morph;
}
void DISidechainProcessor::copyVolumeShapeToFilterBanks() noexcept {
    // On unlink, preserve the sound by sampling the volume curve.
    for(int i=0;i<filterEnvelopePoints;++i){
        const float sample=evaluateEnvelope((double)i/(filterEnvelopePoints-1));
        filterEnvelopeBanks[0][i].store(sample,std::memory_order_relaxed);
        filterEnvelopeBanks[1][i].store(sample,std::memory_order_relaxed);
    }
}
void DISidechainProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer& midi){
 juce::ScopedNoDenormals noDenormals;
 auto main=getBusBuffer(buffer,true,0);int n=main.getNumSamples();if(n<=0||main.getNumChannels()<2||!std::isfinite(sr)||sr<=0)return;
 auto* sideBus=getBus(true,1);bool hasSide=sideBus&&sideBus->isEnabled();
 // Keep sidechain as a non-owning view; never allocate or copy a buffer on the audio thread.
 juce::AudioBuffer<float> side; if(hasSide){auto view=getBusBuffer(buffer,true,1);side.setDataToReferTo(view.getArrayOfWritePointers(),view.getNumChannels(),view.getNumSamples());}
 auto get=[&](const char* id){return state.getRawParameterValue(id)->load();};
 int mode=(int)get("mode"),trig=(int)get("trigger");float depth=get("depth"),mix=get("mix");
 const float multiLowDepth=juce::jlimit(0.f,1.f,get("multiLowDepth"));
 const float multiMidDepth=juce::jlimit(0.f,1.f,get("multiMidDepth"));
 const float multiHighDepth=juce::jlimit(0.f,1.f,get("multiHighDepth"));
 const int playbackMode=juce::jlimit(0,1,juce::roundToInt(get("playbackMode")));
 const float detectorHP=juce::jlimit(0.f,2000.f,get("detectorHP"));
 const float detectorAttackMs=juce::jlimit(0.f,30.f,get("detectorAttackMs"));
 const float detectorReleaseMs=juce::jlimit(1.f,250.f,get("detectorReleaseMs"));
 const double attackCoeff=detectorAttackMs<=.0001f ? 0.0 :
       std::exp(-1.0/(juce::jmax(1.0,(double)detectorAttackMs*.001*sr)));
 const double releaseCoeff=std::exp(-1.0/
       juce::jmax(1.0,(double)detectorReleaseMs*.001*sr));
 const bool filterEnabled=get("outputFilterOn")>.5f;
 const int outputFilterType=juce::jlimit(0,3,juce::roundToInt(get("outputFilterType")));
 const int outputFilterSlope=juce::jlimit(0,1,juce::roundToInt(get("outputFilterSlope")));
 const float targetFilterQ=juce::jlimit(.5f,12.f,get("outputFilterResonance"));
 const float targetFilterDrive=juce::jlimit(0.f,1.f,get("outputFilterDrive"));
 const float filterAmount=juce::jlimit(0.f,1.f,get("outputFilterAmount"));
 const float followerAmount=juce::jlimit(-1.f,1.f,get("filterEnvelopeFollower"));
 const bool filterLinked=get("filterEnvelopeLink")>.5f;
 const double followerAttack=std::exp(-1.0/(sr*.010));
 const double followerRelease=std::exp(-1.0/(sr*.080));
 const float cutoffMin=juce::jlimit(20.f,(float)(sr*.45),get("outputFilterMin"));
 const float cutoffMax=juce::jlimit(cutoffMin,(float)(sr*.45),
                                    juce::jmax(cutoffMin,get("outputFilterMax")));
 const float filterLogRange=std::log(juce::jmax(1.0001f,cutoffMax/cutoffMin));
 const float filterFadeStep=1.f/(float)juce::jmax(1.0,sr*.005);
 const float filterParameterCoefficient=1.f-std::exp(-1.f/(float)(sr*.005));
 const double hpCoefficient=detectorHP>1.f ? std::exp(-juce::MathConstants<double>::twoPi*(double)detectorHP/sr) : 0.;
 const int midiNote=juce::jlimit(-1,127,juce::roundToInt(get("midiNote")));
 const int midiChannel=juce::jlimit(0,16,juce::roundToInt(get("midiChannel")));
 // Set both crossovers independently. BAND is HP(low) -> LP(high), not a
 // subtraction of two unrelated filters, which leaked incorrect frequencies.
 const float maxCutoff=juce::jmax(100.f,(float)(sr*.5-100.));
 const float lo=juce::jlimit(25.f,maxCutoff-20.f,get("low"));
 const float hi=juce::jlimit(lo+10.f,maxCutoff,juce::jmax(lo+10.f,get("high")));
 for(auto& c:filters){
  c.low.setCutoffFrequency(lo);
  c.high.setCutoffFrequency(lo);
  c.bandLowPass.setCutoffFrequency(hi);
  c.bandHighPass.setCutoffFrequency(hi);
  c.phaseLowPass.setCutoffFrequency(hi);
  c.phaseHighPass.setCutoffFrequency(hi);
 }
 double bpm=120;
 double hostPpq=0;
 bool hostPpqAvailable=false,hostRunning=true;
 if(auto* ph=getPlayHead()){
  if(auto pos=ph->getPosition()){
   if(auto b=pos->getBpm())bpm=*b;
   if(auto beats=pos->getPpqPosition()){hostPpq=*beats;hostPpqAvailable=std::isfinite(hostPpq);}
   hostRunning=pos->getIsPlaying();
  }
 }
 if(!std::isfinite(bpm)||bpm<=0)bpm=120;
 const double divisions[]={0.125,0.25,0.5,1,2,4,8,1.0/6.0,1.0/3.0,2.0/3.0,0.375,0.75,1.5};
 envelopeLength=get("sync")>0.5?60.0/juce::jmax(1.0,bpm)*divisions[juce::jlimit(0,12,(int)get("division"))]:get("length")/1000.0;
 double step=1.0/juce::jmax(1.0,envelopeLength*sr);int holdoff=(int)(get("retrigger")*.001*sr);float threshold=juce::Decibels::decibelsToGain(get("threshold"));float coeff=std::exp(-1.f/(float)(juce::jmax(.1f,get("smooth"))*.001*sr));
 juce::MidiBuffer::Iterator it(midi);juce::MidiMessage msg;int midiPos=0;bool hasEvent=it.getNextEvent(msg,midiPos);float peak=0;
 const uint64_t analyzerStart=analyzerWritten.load(std::memory_order_relaxed);
 for(int i=0;i<n;++i){bool fire=false;
 // Capture unprocessed input BEFORE the gain/filters alter the stereo bus.
 const float inputMono=.5f*(main.getSample(0,i)+main.getSample(1,i));
 analyzerHistory[(analyzerStart+(uint64_t)i)%analyzerSize].store(
     std::isfinite(inputMono)?inputMono:0.f,std::memory_order_relaxed);
 while(hasEvent&&midiPos<=i){
  if(msg.isNoteOn()&&msg.getVelocity()>0&&trig!=1
     &&(midiNote<0||msg.getNoteNumber()==midiNote)
     &&(midiChannel==0||msg.getChannel()==midiChannel))
    fire=true; // Exact host-provided sample offset, including irregular patterns.
  hasEvent=it.getNextEvent(msg,midiPos);
 }
 if(hasSide&&trig!=2&&side.getNumSamples()>=n){
  float v=0.f;
  // Peak detection across mono or stereo sidechain, without leaking the
  // external audio to the main signal. Process signed audio before rectifying.
  float signedInput=0.f;
  for(int ch=0;ch<side.getNumChannels();++ch){
   const float sample=side.getSample(ch,i);
   if(std::abs(sample)>std::abs(signedInput))signedInput=sample;
   v=juce::jmax(v,std::abs(sample));
  }
  if(detectorHP>1.f){
   const double filtered=hpCoefficient*(detectorHpPreviousOutput+
                                         (double)signedInput-detectorHpPreviousInput);
   detectorHpPreviousInput=signedInput;
   detectorHpPreviousOutput=filtered;
   v=std::isfinite(filtered)?(float)std::abs(filtered):0.f;
  }else{
   detectorHpPreviousInput=0.;
   detectorHpPreviousOutput=0.;
  }
  // Attack/release peak follower. ATTACK 0 is instantaneous (legacy).
  // The release default ~4.16 ms matches the legacy 0.995 at 48 kHz.
  const double coefficient=v>=detector ? attackCoeff : releaseCoeff;
  detector=coefficient*detector+(1.0-coefficient)*(double)v;
  if(detector<threshold*.65)armed=true;
  if(armed&&detector>=threshold&&refractory==0){fire=true;armed=false;}
 }
 if(refractory>0)--refractory;
 if(fire){phase=0;refractory=holdoff;triggerSequence.fetch_add(1,std::memory_order_relaxed);}
 float env=1.f;
 double filterPhase=phase;
 if(playbackMode==0){
  // ONE SHOT: every accepted MIDI/audio event starts the envelope from zero.
  filterPhase=phase;
  env=phase>=1?1.f:evaluateShiftedEnvelope(phase,false);
  phase=juce::jmin(1.0,phase+step);
 }else if(hostRunning){
  // LOOP: use the host's musical position when available, so each beat is
  // repeatable on the Ableton timeline, including seek and loop playback.
  if(hostPpqAvailable&&get("sync")>.5f){
   const double beatsPerCycle=juce::jmax(.0001,envelopeLength*bpm/60.);
   double beats=hostPpq+(double)i*bpm/(60.0*sr);
   double cycle=std::fmod(beats,beatsPerCycle);
   if(cycle<0)cycle+=beatsPerCycle;
   phase=cycle/beatsPerCycle;
  }
  filterPhase=phase;
  env=evaluateShiftedEnvelope(phase,true);
  phase=hostPpqAvailable&&get("sync")>.5f ? phase : std::fmod(phase+step,1.0);
 }else{
  phase=1;
  filterPhase=1;
  env=1.f;
 }
 float target=1-depth*(1-env);smoothGain=coeff*smoothGain+(1-coeff)*target;
 // Filter has a separate A/B curve when unlinked from volume ducking.
 // Audio follower can additionally modulate cutoff with a signed amount.
 // It reads external sidechain when connected, otherwise the program input.
 float detectorSample=std::abs(inputMono);
 if(hasSide&&side.getNumSamples()>=n){
    detectorSample=0.f;
    for(int channel=0;channel<side.getNumChannels();++channel)
       detectorSample=juce::jmax(detectorSample,std::abs(side.getSample(channel,i)));
 }
 const double followCoeff=detectorSample>=followerLevel?followerAttack:followerRelease;
 followerLevel=(float)(followCoeff*followerLevel+(1.-followCoeff)*detectorSample);
 if(i==n-1)visualFollowerLevel.store(followerLevel,std::memory_order_relaxed);
 const float filterEnv=filterLinked?env:(filterPhase>=1.0?1.f:
     evaluateFilterEnvelope(filterPhase,playbackMode==1));
 const float followerMod=followerAmount*(juce::jlimit(0.f,1.f,followerLevel*2.f)-.5f);
 const float shaped=1.f-filterAmount*(1.f-
       juce::jlimit(0.f,1.f,filterEnv+followerMod));
 const float cutoff=cutoffMin*std::exp(filterLogRange*shaped);
 // Topology-preserving bilinear transform coefficients.
 // 0.45 * sample rate prevents tan(pi/2) singularity near Nyquist.
 const float g=std::tan(juce::MathConstants<float>::pi*cutoff/(float)sr);
 smoothedFilterQ+=filterParameterCoefficient*(targetFilterQ-smoothedFilterQ);
 smoothedFilterDrive+=filterParameterCoefficient*(targetFilterDrive-smoothedFilterDrive);
 const float k=1.f/juce::jmax(.5f,smoothedFilterQ);
 const float a1=1.f/(1.f+g*(g+k));
 const float a2=g*a1;
 const float a3=g*a2;
 outputFilterBlend=juce::jlimit(0.f,1.f,outputFilterBlend+
    (filterEnabled?filterFadeStep:-filterFadeStep));
 if(i==n-1)visualFilterCutoffHz.store(cutoff,std::memory_order_relaxed);
 for(int ch=0;ch<2;++ch){
  const float dry=main.getSample(ch,i);
  auto& f=filters[ch];
  // Complementary Linkwitz-Riley outputs have matching phase. Never
  // subtract unrelated low/high filter phases from the dry signal.
  const float low=f.low.processSample(0,dry);
  const float high=f.high.processSample(0,dry);
  const float mid=f.bandLowPass.processSample(0,high);
  const float upper=f.bandHighPass.processSample(0,high);
  // Apply the second crossover's allpass response to the low branch,
  // matching the phase of the mid+upper branches.
  const float lowAligned=f.phaseLowPass.processSample(0,low)
                         +f.phaseHighPass.processSample(0,low);
  const float keep=mode==1?high:mode==2?low:lowAligned+upper;
  // Interpolate from EXACT dry at no gain reduction to the
  // phase-aligned, frequency-selected output at full reduction.
  // MULTI: every band must be reconstructed in the SAME crossover phase
  // domain before applying independent band attenuation. Subtracting
  // phase-shifted low/mid/high components from the original dry signal
  // causes incomplete cancellation, especially below the first crossover.
  const float multiProcessed=(1.f-multiLowDepth)*lowAligned+
                             (1.f-multiMidDepth)*mid+
                             (1.f-multiHighDepth)*upper;
  const float wet=mode==0?dry*smoothGain:
     mode==4?dry+(1.f-smoothGain)*(multiProcessed-dry):
             dry+(1.f-smoothGain)*(keep-dry);
  const float outUnclamped=dry+(wet-dry)*mix;
  float out=std::isfinite(outUnclamped)?outUnclamped:dry;
  // Pre-filter soft saturation: DRIVE 0 is exactly transparent. At higher
  // settings the soft-clipping signal excites the resonant filter harmonics.
  float filteredInput=out;
  const float driveGain=1.f+15.f*smoothedFilterDrive*smoothedFilterDrive;
  const float drivenInput=juce::jlimit(-128.f,128.f,out*driveGain);
  auto& previousDrive=drivePreviousInput[(size_t)ch];
  // First-order antiderivative anti-aliasing (ADAA) of tanh:
  // (log cosh(x[n]) - log cosh(x[n-1])) / (x[n] - x[n-1]).
  // Stable around equal inputs and large drive; unlike raw tanh it
  // attenuates alias products without allocation or adding host latency.
  const float delta=drivenInput-previousDrive;
  const float saturated=std::abs(delta)<.001f ?
    std::tanh(.5f*(drivenInput+previousDrive)) :
    (logCoshStable(drivenInput)-logCoshStable(previousDrive))/delta;
  previousDrive=drivenInput;
  if(smoothedFilterDrive>.00001f){
   const float normalized=saturated/juce::jmax(.001f,std::tanh(driveGain));
   filteredInput=out+smoothedFilterDrive*(normalized-out);
  }
  auto processSVF=[&](OutputSVFStage& stage,float x){
   const float v3=x-stage.ic2;
   const float v1=a1*stage.ic1+a2*v3;
   const float v2=stage.ic2+a2*stage.ic1+a3*v3;
   stage.ic1=2.f*v1-stage.ic1;
   stage.ic2=2.f*v2-stage.ic2;
   const float lp=v2;
   const float hp=x-k*v1-v2;
   if(outputFilterType==0)return lp;
   if(outputFilterType==1)return hp;
   if(outputFilterType==2)return v1;
   return lp+hp; // Notch
  };
  auto& svf=outputFilterStages[(size_t)ch];
  float filtered=processSVF(svf[0],filteredInput);
  // Keep the second stage continuously conditioned so switching 12/24 dB
  // doesn't activate an uninitialized or stale filter memory.
  const float secondStage=processSVF(svf[1],filtered);
  if(outputFilterSlope==1)filtered=secondStage;
  out+=outputFilterBlend*(filtered-out);
  if(!std::isfinite(out)){
   out=dry;
   for(auto& stage:svf)stage={};
  }
  main.setSample(ch,i,out);
  peak=juce::jmax(peak,std::abs(out));
 }
 }
 analyzerWritten.store(analyzerStart+(uint64_t)n,std::memory_order_release);
 meter.store(peak);visualPhase.store((float)phase,std::memory_order_relaxed);visualGainReductionDb.store(juce::Decibels::gainToDecibels(juce::jmax(0.000001f,smoothGain),-120.f),std::memory_order_relaxed);midi.clear();
}
void DISidechainProcessor::getStateInformation(juce::MemoryBlock& dest){
    auto tree=state.copyState();
    for(int i=0;i<8;++i)tree.setProperty("point"+juce::String(i),points[i].load(),nullptr);
    for(int bank=0;bank<2;++bank)
      for(int i=0;i<filterEnvelopePoints;++i)
        tree.setProperty("filterEnv"+juce::String(bank)+"_"+juce::String(i),
                         getFilterEnvelopePoint(bank,i),nullptr);
    const int count=activeNodeCount.load(std::memory_order_acquire);
    if(count>=2){
        tree.setProperty("envelopeVersion",2,nullptr);
        tree.setProperty("envelopeCount",count,nullptr);
        for(int i=0;i<juce::jmin(count,maxEnvelopeNodes);++i){
            const auto node=getEnvelopeNode(i);
            tree.setProperty("envX"+juce::String(i),node.x,nullptr);
            tree.setProperty("envY"+juce::String(i),node.y,nullptr);
            tree.setProperty("envCurve"+juce::String(i),getEnvelopeSegmentCurve(i),nullptr);
        }
    }
    if(auto xml=tree.createXml())copyXmlToBinary(*xml,dest);
}
void DISidechainProcessor::setStateInformation(const void* data,int bytes){
    if(auto xml=getXmlFromBinary(data,bytes)){
        auto tree=juce::ValueTree::fromXml(*xml);
        if(tree.isValid()){
            // Legacy sessions have no envelopeCount and retain the original eight points.
            activeNodeCount.store(0,std::memory_order_release);
            for(int i=0;i<8;++i)
                points[i].store(juce::jlimit(0.f,1.f,(float)tree.getProperty("point"+juce::String(i),points[i].load())),std::memory_order_relaxed);
            const int count=(int)tree.getProperty("envelopeCount",0);
            if(count>=2&&count<=maxEnvelopeNodes){
                bool valid=tree.hasProperty("envX0")&&tree.hasProperty("envY0");
                float previous=-1.f;
                for(int i=0;i<count&&valid;++i){
                    const float x=(float)tree.getProperty("envX"+juce::String(i),-1.f);
                    const float y=(float)tree.getProperty("envY"+juce::String(i),-1.f);
                    valid=std::isfinite(x)&&std::isfinite(y)&&x>=0.f&&x<=1.f&&
                          y>=0.f&&y<=1.f&&(i==0||x>previous);
                    if(valid){
                        nodeX[i].store(x,std::memory_order_relaxed);
                        nodeY[i].store(y,std::memory_order_relaxed);
                        const float rawCurve=(float)tree.getProperty("envCurve"+juce::String(i),0.f);
                        nodeCurve[i].store(std::isfinite(rawCurve)?juce::jlimit(-1.f,1.f,rawCurve):0.f,std::memory_order_relaxed);
                    }
                    previous=x;
                }
                if(valid)activeNodeCount.store(count,std::memory_order_release);
            }
            for(int bank=0;bank<2;++bank)
              for(int i=0;i<filterEnvelopePoints;++i){
                const auto key="filterEnv"+juce::String(bank)+"_"+juce::String(i);
                if(tree.hasProperty(key)){
                    const float v=(float)tree.getProperty(key);
                    if(std::isfinite(v))setFilterEnvelopePoint(bank,i,v);
                }
              }
            state.replaceState(tree);
        }
    }
}
juce::AudioProcessorEditor* DISidechainProcessor::createEditor(){return new DISidechainEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new DISidechainProcessor();}
