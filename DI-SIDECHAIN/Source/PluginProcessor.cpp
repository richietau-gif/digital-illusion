#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <algorithm>
namespace { juce::StringArray filterNames(){ return {"LP12","LP24","LP48","HP12","HP24","HP48","Band Pass","Notch","Analog Ladder","SVF Morph","Comb","Formant/Vowel","Aggressive Bass"}; } }
juce::AudioProcessorValueTreeState::ParameterLayout DISidechainProcessor::makeParams(){
 std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
 auto f=[&](const char* id,const char* n,float lo,float hi,float d){p.push_back(std::make_unique<juce::AudioParameterFloat>(id,n,juce::NormalisableRange<float>(lo,hi),d));};
 f("depth","Depth",0,1,1); f("mix","Dry/Wet",0,1,1); f("threshold","Trigger Threshold",-60,0,-24); f("length","Envelope Length ms",10,2000,250); f("timeOffset","Time Offset",-1,1,0); f("curveAmount","Curve Tension",0,1,.5f); f("lookahead","Lookahead ms",0,200,20); f("attackProtection","Attack Protection",0,1,0); f("retrigger","Retrigger Holdoff ms",1,250,35); f("smooth","Gain Smoothing ms",0.1f,50,3); f("cutoff","Cutoff Hz",20,20000,800); f("resonance","Resonance",0,1,0.15f); f("filterDrive","Filter Drive",0,24,0); f("filterMod","Filter Envelope",-1,1,0); f("filterMix","Filter Mix",0,1,1); f("colorDrive","Color Drive",0,24,0); f("colorTone","Color Tone",-1,1,0); f("colorMix","Color Mix",0,1,0); f("output","Output dB",-24,12,0);
 p.push_back(std::make_unique<juce::AudioParameterChoice>("triggerMode","Trigger",juce::StringArray{"Audio","MIDI","Audio + MIDI"},0)); p.push_back(std::make_unique<juce::AudioParameterChoice>("detector","Detector",juce::StringArray{"Peak","RMS","Transient"},0)); p.push_back(std::make_unique<juce::AudioParameterChoice>("detectorSource","Detector Source",juce::StringArray{"Auto","Internal","External"},0)); p.push_back(std::make_unique<juce::AudioParameterChoice>("division","Sync Division",juce::StringArray{"1/16","1/8T","1/8","1/4T","1/4","1/2","1 bar"},2)); p.push_back(std::make_unique<juce::AudioParameterChoice>("filterModel","Filter Model",filterNames(),0)); p.push_back(std::make_unique<juce::AudioParameterChoice>("colorModel","Color",juce::StringArray{"Warm Saturation","Soft Clip","Hard Clip","Asymmetric Drive","Wavefold","Digital Crush"},0)); p.push_back(std::make_unique<juce::AudioParameterBool>("sync","BPM Sync",true)); p.push_back(std::make_unique<juce::AudioParameterBool>("filterEnabled","Filter Enabled",true));
 p.push_back(std::make_unique<juce::AudioParameterBool>("filterEnvelopeEnabled","Filter Envelope Enabled",true));
 p.push_back(std::make_unique<juce::AudioParameterBool>("colorEnabled","Color Enabled",true));
 p.push_back(std::make_unique<juce::AudioParameterBool>("bypass","Bypass",false)); p.push_back(std::make_unique<juce::AudioParameterChoice>("uiScale","UI Scale",juce::StringArray{"50%","75%","100%"},1)); return {p.begin(),p.end()}; }
DISidechainProcessor::DISidechainProcessor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withInput("Sidechain",juce::AudioChannelSet::stereo(),false).withOutput("Output",juce::AudioChannelSet::stereo(),true)),state(*this,nullptr,"PARAMS",makeParams()){
 std::array<EnvelopeNode,maxEnvelopeNodes> initial{};
 initial[0]={0.f,0.f,0.f}; initial[1]={.15f,.03f,.25f};
 initial[2]={.35f,.26f,-.15f}; initial[3]={.55f,.60f,.3f};
 initial[4]={.75f,.93f,.1f}; initial[5]={1.f,1.f,0.f};
 setEnvelopeNodes(initial,6);
 applyPreset(0);
}
bool DISidechainProcessor::isBusesLayoutSupported(const BusesLayout& b) const{return b.getMainInputChannelSet()==juce::AudioChannelSet::stereo()&&b.getMainOutputChannelSet()==juce::AudioChannelSet::stereo();}
void DISidechainProcessor::prepareToPlay(double s,int n){sr=s;maxBlock=n; delaySamples=(int)std::ceil(state.getRawParameterValue("lookahead")->load()*0.001f*sr); delaySamples=juce::jlimit(0,192000,delaySamples);setLatencySamples(delaySamples);delayWrite=0;colorToneLow={0.f,0.f};phase=1;gain=1;detector=0;holdoff=0;for(auto& f:filters){juce::dsp::ProcessSpec sp{s,(juce::uint32)n,1};f.prepare(sp);f.reset();for(auto* a:{&f.animated,&f.animated2,&f.animated3,&f.animated4}){a->setResonance(.707f);a->setCutoffFrequency(800.f);}}}
int DISidechainProcessor::getEnvelopeNodes(std::array<EnvelopeNode,maxEnvelopeNodes>& target) const noexcept
{
 const int count=juce::jlimit(2,maxEnvelopeNodes,envelopeNodeCount.load(std::memory_order_acquire));
 for(int i=0;i<count;++i)
  target[(size_t)i]={envelopeXs[(size_t)i].load(),envelopeYs[(size_t)i].load(),envelopeCurves[(size_t)i].load()};
 return count;
}
void DISidechainProcessor::setEnvelopeNodes(const std::array<EnvelopeNode,maxEnvelopeNodes>& source,int count) noexcept
{
 count=juce::jlimit(2,maxEnvelopeNodes,count);
 for(int i=0;i<count;++i) {
  const auto& v=source[(size_t)i];
  envelopeXs[(size_t)i].store(juce::jlimit(0.f,1.f,v.x),std::memory_order_relaxed);
  envelopeYs[(size_t)i].store(juce::jlimit(0.f,1.f,v.y),std::memory_order_relaxed);
  envelopeCurves[(size_t)i].store(juce::jlimit(-1.f,1.f,v.curve),std::memory_order_relaxed);
 }
 envelopeNodeCount.store(count,std::memory_order_release);
 envelopeRevision.fetch_add(1,std::memory_order_relaxed);
}
float DISidechainProcessor::getEnvelopeAt(float t) const noexcept
{
 const int count=juce::jlimit(2,maxEnvelopeNodes,envelopeNodeCount.load(std::memory_order_acquire));
 t=juce::jlimit(0.f,1.f,t);
 float prevX=envelopeXs[0].load(std::memory_order_relaxed);
 float prevY=envelopeYs[0].load(std::memory_order_relaxed);
 if(t<=prevX)return prevY;
 for(int i=1;i<count;++i) {
  const float x=envelopeXs[(size_t)i].load(std::memory_order_relaxed);
  const float y=envelopeYs[(size_t)i].load(std::memory_order_relaxed);
  if(t<=x||i==count-1) {
   const float span=juce::jmax(0.0001f,x-prevX);
   float u=juce::jlimit(0.f,1.f,(t-prevX)/span);
   const float globalCurve=state.getRawParameterValue("curveAmount")->load();
   const float tension=juce::jlimit(-1.f,1.f,envelopeCurves[(size_t)i-1].load(std::memory_order_relaxed)+(globalCurve-.5f)*1.4f);
   if(tension>=0.f) u=std::pow(u,1.f+3.f*tension);
   else u=1.f-std::pow(1.f-u,1.f-3.f*tension);
   return juce::jlimit(0.f,1.f,prevY+(y-prevY)*u);
  }
  prevX=x;prevY=y;
 }
 return prevY;
}
float DISidechainProcessor::envelope(float t) const noexcept {return getEnvelopeAt(t);}
float DISidechainProcessor::colour(float x,int m,float d) const noexcept {const float k=1.f+juce::Decibels::decibelsToGain(d)*4.f; switch(m){case 1:return std::tanh(x*k);case 2:return juce::jlimit(-1.f,1.f,x*k);case 3:return std::tanh((x>=0?x*k:x*k*0.65f));case 4:{float y=x*k;return 2.f*std::abs(y-std::round(y))-1.f;}case 5:return std::round(x*k*8.f)/8.f;default:return std::tanh(x*k*0.7f);}}
void DISidechainProcessor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer& midi){juce::ScopedNoDenormals nd;auto main=getBusBuffer(b,true,0);auto side=getBusBuffer(b,true,1);const int n=main.getNumSamples();if(n==0)return;auto val=[&](const char* id){return state.getRawParameterValue(id)->load();};const float depth=juce::jlimit(0.f,1.f,val("depth")),mix=juce::jlimit(0.f,1.f,val("mix")); const bool bypass=val("bypass")>0.5f;const int trig=(int)val("triggerMode");const float threshold=juce::Decibels::decibelsToGain(val("threshold"));const int hold=(int)(val("retrigger")*.001f*sr);float lengthSeconds=val("length")*.001f; if(val("sync")>0.5f){ double bpm=120.0; if(auto* ph=getPlayHead()) if(auto pos=ph->getPosition()) if(auto b=pos->getBpm()) if(std::isfinite(*b)&&*b>1.0) bpm=*b; static constexpr double beats[]={0.25,0.333333,0.5,0.666667,1.0,2.0,4.0}; lengthSeconds=(float)(60.0/bpm*beats[juce::jlimit(0,6,(int)val("division"))]); } const float len=juce::jmax(1.f,lengthSeconds*float(sr));const float offset=val("timeOffset");const float attack=val("attackProtection");const int model=(int)val("filterModel"), color=(int)val("colorModel");juce::MidiMessage mm;int mp=0;juce::MidiBuffer::Iterator mi(midi);bool has=mi.getNextEvent(mm,mp);float peak=0;const float blockCut=juce::jlimit(20.f,(float)(sr*.45),val("cutoff")); const float q=juce::jmax(.1f,1.f-val("resonance")*.9f);
 const bool filterOn=val("filterEnabled")>0.5f;
 const bool envOn=val("filterEnvelopeEnabled")>0.5f;
 const bool colorOn=val("colorEnabled")>0.5f;
 const float filterDepth=juce::jlimit(-1.f,1.f,val("filterMod"));
 const float filterMix=juce::jlimit(0.f,1.f,val("filterMix"));
 const float colorMix=juce::jlimit(0.f,1.f,val("colorMix"));
 const float filterDrive=juce::Decibels::decibelsToGain(val("filterDrive"));
 const float outputGain=juce::Decibels::decibelsToGain(val("output"));
 auto blockLp=juce::dsp::IIR::Coefficients<float>::makeLowPass(sr,blockCut,q); auto blockHp=juce::dsp::IIR::Coefficients<float>::makeHighPass(sr,blockCut,q); for(auto& f:filters){
 f.a.coefficients=(model==5?blockHp:blockLp);
 f.b.coefficients=(model==2?blockLp:blockHp);
 f.c.coefficients=(model==4||model==5?blockHp:blockLp);
 f.d.coefficients=(model==5?blockHp:blockLp);
} for(int i=0;i<n;++i){bool note=false;while(has&&mp<=i){note=mm.isNoteOn()&&mm.getVelocity()>0;has=mi.getNextEvent(mm,mp);}float sv=0;if((int)val("detectorSource")!=1 && side.getNumChannels()>0)sv=std::max(std::abs(side.getSample(0,i)),side.getNumChannels()>1?std::abs(side.getSample(1,i)):0.f); else sv=std::max(std::abs(main.getSample(0,i)),std::abs(main.getSample(1,i)));detector=std::max(sv,detector*0.995f);if(detector<threshold*.65f)gateArmed=true;bool audio=gateArmed&&detector>=threshold;if(audio)gateArmed=false;bool fire=(trig==0?audio:trig==1?note:(audio||note))&&holdoff<=0;if(fire){phase=0;holdoff=hold;triggerActivity.store(1.f);}if(holdoff>0)--holdoff;triggerActivity.store(triggerActivity.load()*.995f);float t=juce::jlimit(0.f,1.f,phase+offset);float e=phase>=1?1:envelope(t);if(attack>0)e=std::max(e,attack*(1.f-phase));float target=1.f-depth*(1.f-e);const float c=std::exp(-1.f/(std::max(.0001f,val("smooth"))*.001f*sr));gain=c*gain+(1-c)*target;phase=juce::jmin(1.f,phase+1.f/juce::jmax(1.f,len));for(int ch=0;ch<2;++ch){float dry=main.getSample(ch,i);const int readIndex=(delayWrite+192001-delaySamples)%192001; float delayed=(delaySamples==0?dry:delay[readIndex][ch]);delay[delayWrite][ch]=dry;float wet=delayed*gain;float out=bypass?delayed:delayed+(wet-delayed)*mix;if(!bypass){auto& f=filters[ch];
 float filtered=f.a.processSample(out);
 if(model==1)filtered=f.c.processSample(filtered);
 if(model==2){filtered=f.b.processSample(filtered);filtered=f.c.processSample(filtered);filtered=f.d.processSample(filtered);}
 if(filterOn && envOn && std::abs(filterDepth)>0.0001f)
 {
     const float modulatedHz=juce::jlimit(20.f,(float)(sr*.45),blockCut*std::exp2(4.f*filterDepth*(1.f-e)));
     const auto newType=model>=3&&model<=5?juce::dsp::StateVariableTPTFilterType::highpass:
                         (model==6||model==7)?juce::dsp::StateVariableTPTFilterType::bandpass:
                         juce::dsp::StateVariableTPTFilterType::lowpass;
     const float resonance=juce::jlimit(.15f,8.f,1.f+val("resonance")*7.f);
     for(auto* stage:{&f.animated,&f.animated2,&f.animated3,&f.animated4}){
      stage->setCutoffFrequency(modulatedHz);
      stage->setResonance(resonance);
      stage->setType(newType);
     }
     filtered=model==10?f.processComb(out,juce::roundToInt((float)sr/modulatedHz),val("resonance")*.9f):f.animated.processSample(0,out);
     if(model==7)filtered=out-filtered;
     if(model==1||model==4)filtered=f.animated2.processSample(0,filtered);
     if(model==2||model==5){
      filtered=f.animated2.processSample(0,filtered);
      filtered=f.animated3.processSample(0,filtered);
      filtered=f.animated4.processSample(0,filtered);
     }
 }
 else if(model>=3){float h=f.b.processSample(out);if(model>=3&&model<=5){
  filtered=h;
  if(model>=4)filtered=f.c.processSample(filtered);
  if(model==5){filtered=f.d.processSample(filtered);filtered=f.a.processSample(filtered);}
 } else if(model==6||model==7){
   f.animated.setCutoffFrequency(blockCut);
   f.animated.setResonance(juce::jlimit(.2f,8.f,1.f+val("resonance")*7.f));
   f.animated.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
   const float band=f.animated.processSample(0,out);
   filtered=model==6?band:out-band;
 } else if(model==8){ filtered=std::tanh(filtered*juce::Decibels::decibelsToGain(val("filterDrive"))); } else if(model==9){ filtered=std::tanh((filtered+h)*1.5f)-0.5f*h; } else if(model==10){ filtered=f.processComb(out,juce::roundToInt((float)sr/blockCut),val("resonance")*.9f); } else if(model==11){ filtered=std::tanh(filtered*3.f)+0.25f*h; } else if(model>=12){ filtered=std::tanh((filtered-h)*juce::Decibels::decibelsToGain(val("filterDrive"))); }}if(filterOn) {
 if(model<8 && filterDrive>1.001f)
  filtered=std::tanh(filtered*filterDrive)/std::tanh(filterDrive);
 out+=(filtered-out)*filterMix;
}
 float cd=val("colorDrive");
 if(colorOn&&colorMix>0.f){
  float coloured=colour(out,color,cd);
  const float tone=juce::jlimit(-1.f,1.f,val("colorTone"));
  const float alpha=1.f-std::exp(-2.f*juce::MathConstants<float>::pi*1800.f/(float)sr);
  colorToneLow[(size_t)ch]+=alpha*(coloured-colorToneLow[(size_t)ch]);
  const float low=colorToneLow[(size_t)ch],high=coloured-low;
  coloured=tone<0.f?coloured+tone*high:coloured+tone*low;
  out+=(coloured-out)*colorMix;
 }
 out*=outputGain;}if(!std::isfinite(out))out=0;main.setSample(ch,i,out);peak=std::max(peak,std::abs(out));}delayWrite=(delayWrite+1)%192001;}meter.store(peak);midi.clear();}
void DISidechainProcessor::setCurrentProgram(int p){currentPreset=juce::jlimit(0,31,p);applyPreset(currentPreset);}const juce::String DISidechainProcessor::getProgramName(int p){static const char* n[]={"Clean Kick Duck","Tight Drum Duck","Deep Wobble","Talking Bass","Growl Filter","Metallic Sweep","Aggressive Formant","Liquid Movement","Elastic Bass","Vowel Bounce","Glassy Growl","Warm Harsh","Digital Motion","Rhythmic Resonance","Reese Movement","Jungle Duck","Neuro Sweep","Rolling Pulse","Reverse Swell","Filter Drop","Resonant Impact","Distorted Pulse","Stereo Motion","Sub Safe Duck","Fast Breakbeat","Half Time Growl","Razor Resonance","Dark Movement","Formant Pulse","Precision Pump","Gate Bass","Future Duck"};return p>=0&&p<32?n[p]:"Custom";}
void DISidechainProcessor::applyPreset(int p){ auto set=[&](const char* id,float v){ if(auto* x=state.getParameter(id)) x->setValueNotifyingHost(x->convertTo0to1(v)); }; const int g=p/8; set("depth",0.35f+0.15f*(p%5)); set("mix",0.7f+0.05f*(p%5)); set("length",80.f+40.f*(p%10)); set("timeOffset",-1.f+0.5f*(p%5)); set("lookahead",5.f+5.f*(p%8)); set("attackProtection",(p%4)*0.25f); set("cutoff",120.f+220.f*(p%12)); set("resonance",0.05f+0.07f*(p%8)); set("filterDrive",(p%6)*3.f); set("filterMod",-0.8f+0.2f*(p%9)); set("filterMix",0.55f+0.08f*(p%5)); set("colorDrive",(p%7)*3.f); set("colorMix",(p%4)*0.2f); set("output",-3.f+g); set("filterModel",(float)(p%13)); set("colorModel",(float)(p%6)); set("triggerMode",(float)(p%3)); set("division",(float)(p%7));
 std::array<EnvelopeNode,maxEnvelopeNodes> points{};
 const int shape=p%6;
 int count=5;
 if(shape==0){ // Classic precision duck: immediate attenuation, smooth return.
  points[0]={0.f,0.f,0.f};points[1]={.12f,0.f,0.f};points[2]={.35f,.15f,.35f};
  points[3]={.74f,.96f,-.2f};points[4]={1.f,1.f,0.f};
 } else if(shape==1){ // Hard rhythmic gate.
  points[0]={0.f,0.f,0.f};points[1]={.24f,0.f,0.f};points[2]={.28f,1.f,0.f};
  points[3]={.79f,1.f,0.f};points[4]={1.f,1.f,0.f};
 } else if(shape==2){ // Single-cycle dubstep wobble, without step sequencer.
  count=7;
  points[0]={0.f,1.f,0.f};points[1]={.14f,.04f,.4f};points[2]={.29f,.98f,-.3f};
  points[3]={.46f,.08f,.3f};points[4]={.63f,.86f,-.3f};points[5]={.82f,.20f,.3f};
  points[6]={1.f,1.f,0.f};
 } else if(shape==3){ // Reverse swell.
  points[0]={0.f,1.f,0.f};points[1]={.20f,.85f,0.f};points[2]={.50f,.24f,.35f};
  points[3]={.78f,.03f,0.f};points[4]={1.f,1.f,0.f};
 } else if(shape==4){ // Double pump.
  count=7;
  points[0]={0.f,0.f,0.f};points[1]={.18f,.06f,0.f};points[2]={.35f,1.f,0.f};
  points[3]={.48f,.08f,0.f};points[4]={.64f,.09f,0.f};points[5]={.83f,.88f,0.f};
  points[6]={1.f,1.f,0.f};
 } else { // S-curve rise with protected attack.
  points[0]={0.f,0.f,0.f};points[1]={.15f,.03f,-.5f};points[2]={.43f,.38f,-.2f};
  points[3]={.80f,.95f,.5f};points[4]={1.f,1.f,0.f};
 }
 setEnvelopeNodes(points,count);
 }
void DISidechainProcessor::getStateInformation(juce::MemoryBlock& d)
{
 if(auto xml=state.copyState().createXml()){
  std::array<EnvelopeNode,maxEnvelopeNodes> nodes{};
  const int count=getEnvelopeNodes(nodes);
  xml->setAttribute("currentPreset",currentPreset);
  auto* envelopeXml=xml->createNewChildElement("PRECISION_ENVELOPE");
  envelopeXml->setAttribute("count",count);
  for(int i=0;i<count;++i){
   auto* node=envelopeXml->createNewChildElement("NODE");
   node->setAttribute("x",(double)nodes[(size_t)i].x);
   node->setAttribute("y",(double)nodes[(size_t)i].y);
   node->setAttribute("curve",(double)nodes[(size_t)i].curve);
  }
  copyXmlToBinary(*xml,d);
 }
}
void DISidechainProcessor::setStateInformation(const void* data,int size)
{
 if(auto xml=getXmlFromBinary(data,size)){
  currentPreset=juce::jlimit(0,31,xml->getIntAttribute("currentPreset",0));
  xml->removeAttribute("currentPreset");
  if(auto* envelopeXml=xml->getChildByName("PRECISION_ENVELOPE")){
   std::array<EnvelopeNode,maxEnvelopeNodes> nodes{};
   int count=0;
   for(auto* node=envelopeXml->getFirstChildElement();node!=nullptr&&count<maxEnvelopeNodes;node=node->getNextElement()){
    if(node->hasTagName("NODE"))
     nodes[(size_t)count++]={(float)node->getDoubleAttribute("x",0.0),
                              (float)node->getDoubleAttribute("y",1.0),
                              (float)node->getDoubleAttribute("curve",0.0)};
   }
   if(count>=2)setEnvelopeNodes(nodes,count);
   xml->removeChildElement(envelopeXml,true);
  }
  state.replaceState(juce::ValueTree::fromXml(*xml));
 }
}
juce::AudioProcessorEditor* DISidechainProcessor::createEditor(){return new DISidechainEditor(*this);}juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new DISidechainProcessor();}
