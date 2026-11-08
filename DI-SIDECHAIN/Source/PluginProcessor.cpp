#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "FxWaveshaper.h"
#include "PeakDetector.h"
#include <cmath>
static juce::AudioProcessorValueTreeState::ParameterLayout layoutParams(){
 std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
 auto f=[&](const char* id,const char* name,float lo,float hi,float def){p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},name,juce::NormalisableRange<float>(lo,hi),def));};
 f("depth","Depth",0,1,1); f("mix","Mix",0,1,1);f("threshold","Trigger threshold dB",-60,0,-22);f("length","Envelope milliseconds",10,2000,250);f("smooth","Smoothing milliseconds",0.1,20,2);f("low","Low cutoff",25,1500,120);f("high","High cutoff",60,18000,400);f("retrigger","Trigger holdoff milliseconds",1,250,35);f("shapeOffset","Shape vertical offset",-1,1,0);f("panMod","Pan modulation amount",-1,1,0);f("filterMod","Filter modulation amount",0,1,0);f("fxDrive","FX drive dB",0,24,0);f("fxMix","FX wet mix",0,1,1);
 p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"mode",1},"Frequency mode",juce::StringArray{"Full","Low","High","Band"},0));
 p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"trigger",1},"Trigger mode",juce::StringArray{"Audio + MIDI","Audio only","MIDI only"},0));
 p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"sequenceEnabled",1},"Shape sequencer enabled",false));
 p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"fxEnabled",1},"FX enabled",false));
 p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"fxType",1},"FX type",juce::StringArray{"Saturator","Soft Clipper"},0));
 p.push_back(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{"sequenceSteps",1},"Sequence steps",1,32,16));
 p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"sync",1},"Tempo sync",true));
 p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"division",1},"Beat division",juce::StringArray{"1/32","1/16","1/8","1/4","1/2","1 bar","2 bars"},2));
 return {p.begin(),p.end()};
}
juce::AudioProcessorValueTreeState::ParameterLayout DISidechainProcessor::makeParams(){return layoutParams();}
DISidechainProcessor::DISidechainProcessor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withInput("Sidechain",juce::AudioChannelSet::stereo(),false).withOutput("Output",juce::AudioChannelSet::stereo(),true)),state(*this,nullptr,"PARAMS",makeParams()){
 const float defaults[8]={1,0.92f,0.08f,0.02f,0.32f,0.75f,0.95f,1};for(int i=0;i<8;++i)points[i]=defaults[i];
 for(auto& shape:sequencePoints)for(int i=0;i<8;++i)shape[i].store(defaults[i]);
 for(int lane=0;lane<2;++lane)for(auto& shape:modulationPoints[lane])for(int i=0;i<8;++i)shape[i].store(lane==0?1.f:0.5f);
}
void DISidechainProcessor::setSequencePoint(int step,int node,float value) noexcept {if(step>=0&&step<32&&node>=0&&node<8&&std::isfinite(value))sequencePoints[step][node].store(juce::jlimit(0.f,1.f,value),std::memory_order_relaxed);}
float DISidechainProcessor::getSequencePoint(int step,int node) const noexcept {return step>=0&&step<32&&node>=0&&node<8?sequencePoints[step][node].load(std::memory_order_relaxed):1.f;}
void DISidechainProcessor::setModulationPoint(int lane,int step,int node,float value) noexcept {
 if(lane>=0&&lane<2&&step>=0&&step<32&&node>=0&&node<8&&std::isfinite(value))
  modulationPoints[lane][step][node].store(juce::jlimit(0.f,1.f,value),std::memory_order_relaxed);
}
float DISidechainProcessor::getModulationPoint(int lane,int step,int node) const noexcept {
 return lane>=0&&lane<2&&step>=0&&step<32&&node>=0&&node<8
  ?modulationPoints[lane][step][node].load(std::memory_order_relaxed):(lane==1?0.5f:1.f);
}
bool DISidechainProcessor::isBusesLayoutSupported(const BusesLayout& b) const {return b.getMainInputChannelSet()==juce::AudioChannelSet::stereo()&&b.getMainOutputChannelSet()==juce::AudioChannelSet::stereo()&&(b.inputBuses.size()<2||b.inputBuses[1].isDisabled()||b.inputBuses[1]==juce::AudioChannelSet::mono()||b.inputBuses[1]==juce::AudioChannelSet::stereo());}
void DISidechainProcessor::prepareToPlay(double sampleRate,int maxBlock){sr=sampleRate;phase=1;sequencer.reset();for(auto& lane:modulationSequencers)lane.reset();activeSequenceStep.store(0);detector=0;armed=true;refractory=0;smoothGain=1;expectedTransportSample=-1;wasTransportPlaying=false;fxBlend=state.getRawParameterValue("fxEnabled")->load()>0.5f?1.f:0.f;smoothedFxDrive=juce::Decibels::decibelsToGain(state.getRawParameterValue("fxDrive")->load());smoothedFxWet=state.getRawParameterValue("fxMix")->load();smoothedFxType=state.getRawParameterValue("fxType")->load();juce::dsp::ProcessSpec spec{sampleRate,(juce::uint32)juce::jmax(1,maxBlock),1};for(auto& c:filters){c.low.prepare(spec);c.high.prepare(spec);c.low.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);c.high.setType(juce::dsp::LinkwitzRileyFilterType::highpass);c.low.reset();c.high.reset();}}
float DISidechainProcessor::curve(double t) const {t=juce::jlimit(0.0,1.0,t);double x=t*7;int i=juce::jmin(6,(int)x);float a=points[i].load(),b=points[i+1].load();float u=(float)(x-i);u=u*u*(3-2*u);return a+(b-a)*u;}
void DISidechainProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer& midi){
 juce::ScopedNoDenormals noDenormals;
 auto main=getBusBuffer(buffer,true,0);int n=main.getNumSamples();if(n<=0||main.getNumChannels()<2||!std::isfinite(sr)||sr<=0)return;
 auto* sideBus=getBus(true,1);bool hasSide=sideBus&&sideBus->isEnabled();
 // Keep sidechain as a non-owning view; never allocate or copy a buffer on the audio thread.
 juce::AudioBuffer<float> side; if(hasSide){auto view=getBusBuffer(buffer,true,1);side.setDataToReferTo(view.getArrayOfWritePointers(),view.getNumChannels(),view.getNumSamples());}
 auto get=[&](const char* id){return state.getRawParameterValue(id)->load();};
 int mode=(int)get("mode"),trig=(int)get("trigger");float depth=get("depth"),mix=get("mix"),shapeOffset=get("shapeOffset"),panMod=get("panMod"),filterMod=get("filterMod"),fxDrive=get("fxDrive"),fxMix=get("fxMix"),lo=get("low"),hi=juce::jmax(lo+10,get("high"));
 const float filterRange=juce::jlimit(0.f,1.f,filterMod);
 const bool fxEnabled=get("fxEnabled")>0.5f;
 const int fxType=juce::jlimit(0,1,(int)get("fxType"));
 // Guard automation and restored host state before values enter persistent smoothers.
 const float drive=juce::Decibels::decibelsToGain(std::isfinite(fxDrive)?juce::jlimit(0.f,24.f,fxDrive):0.f);
 const float fxWet=std::isfinite(fxMix)?juce::jlimit(0.f,1.f,fxMix):0.f;
 const float fxTarget=fxEnabled?1.f:0.f;
 const float fxSmoothing=std::exp(-1.f/(float)(0.01*sr));
 const float fxParameterSmoothing=std::exp(-1.f/(float)(0.005*sr));
 for(auto& c:filters){c.low.setCutoffFrequency(lo);c.high.setCutoffFrequency(mode==3?hi:lo);}
 double bpm=120;
 double hostPpq=0.;bool hasPlayingPpq=false;
 if(auto* ph=getPlayHead()){
  if(auto pos=ph->getPosition()){
   if(auto b=pos->getBpm())bpm=*b;
   const bool playing=pos->getIsPlaying();
   if(playing){if(auto ppq=pos->getPpqPosition()){hostPpq=*ppq;hasPlayingPpq=std::isfinite(hostPpq);}}
   bool discontinuity=playing&&!wasTransportPlaying;
   if(playing){
    if(auto sample=pos->getTimeInSamples()){
     if(expectedTransportSample>=0&&std::abs(*sample-expectedTransportSample)>juce::jmax(2,n*2))discontinuity=true;
     expectedTransportSample=*sample+n;
    }else expectedTransportSample=-1;
   }else expectedTransportSample=-1;
   if(discontinuity){sequencer.reset();for(auto& lane:modulationSequencers)lane.reset();activeSequenceStep.store(0,std::memory_order_relaxed);phase=0;}
   wasTransportPlaying=playing;
  }
 }
 const double divisions[]={0.125,0.25,0.5,1,2,4,8};
 envelopeLength=get("sync")>0.5?60.0/juce::jmax(1.0,bpm)*divisions[juce::jlimit(0,6,(int)get("division"))]:get("length")/1000.0;
 const bool tempoSync=get("sync")>0.5f;
 const bool sequenceOn=get("sequenceEnabled")>0.5f;
 sequencer.setSteps((int)get("sequenceSteps"));
 for(auto& lane:modulationSequencers)lane.setSteps((int)get("sequenceSteps"));
 // Re-anchor each audio block to the DAW's PPQ timeline when tempo sync is enabled.
 // This corrects accumulated drift and aligns loop restarts to musical bars.
 if(sequenceOn&&tempoSync&&hasPlayingPpq){
  const double beatsPerStep=divisions[juce::jlimit(0,6,(int)get("division"))];
  const double position=hostPpq/beatsPerStep;
  sequencer.seekToStepPosition(position);
  for(auto& lane:modulationSequencers)lane.seekToStepPosition(position);
 }
 if(sequenceOn)for(int k=0;k<32;++k){
  di::ShapeSequencer::Shape shape{};for(int j=0;j<8;++j)shape[j]=getSequencePoint(k,j);sequencer.setShape(k,shape);
  for(int lane=0;lane<2;++lane){di::ShapeSequencer::Shape mod{};for(int j=0;j<8;++j)mod[j]=getModulationPoint(lane,k,j);modulationSequencers[lane].setShape(k,mod);}
 }
 double step=1.0/juce::jmax(1.0,envelopeLength*sr);int holdoff=(int)(get("retrigger")*.001*sr);float threshold=juce::Decibels::decibelsToGain(get("threshold"));float coeff=std::exp(-1.f/(float)(juce::jmax(.1f,get("smooth"))*.001*sr));
 const double detectorRelease=di::peakReleaseCoefficient(sr);
 juce::MidiBuffer::Iterator it(midi);juce::MidiMessage msg;int midiPos=0;bool hasEvent=it.getNextEvent(msg,midiPos);float peak=0;
 for(int i=0;i<n;++i){bool fire=false;
 fxBlend=fxTarget+(fxBlend-fxTarget)*fxSmoothing;
 if(!std::isfinite(smoothedFxDrive))smoothedFxDrive=drive;
 if(!std::isfinite(smoothedFxWet))smoothedFxWet=fxWet;
 smoothedFxDrive=drive+(smoothedFxDrive-drive)*fxParameterSmoothing;
 smoothedFxWet=fxWet+(smoothedFxWet-fxWet)*fxParameterSmoothing;
 smoothedFxType=(float)fxType+(smoothedFxType-(float)fxType)*fxSmoothing;
 while(hasEvent&&midiPos<=i){if(msg.isNoteOn()&&msg.getVelocity()>0&&trig!=1)fire=true;hasEvent=it.getNextEvent(msg,midiPos);}
 if(hasSide&&trig!=2&&side.getNumSamples()>=n){float v=0;for(int ch=0;ch<side.getNumChannels();++ch)v=juce::jmax(v,std::abs(side.getSample(ch,i)));detector=di::followPeak(detector,(double)v,detectorRelease);if(detector<threshold*.65)armed=true;if(armed&&detector>=threshold&&refractory==0){fire=true;armed=false;}}
 if(refractory>0)--refractory;
 if(fire){
  phase=0;
  // In tempo-sync mode, incoming kicks/notes trigger the ducking envelope,
  // not the song-position sequencer. Otherwise frequent notes pin it to step 1.
  // Free-running (millisecond) mode retains explicit retrigger behaviour.
  if(sequenceOn&&di::ShapeSequencer::shouldRestartOnTrigger(tempoSync)){
   sequencer.trigger();
   for(auto& lane:modulationSequencers)lane.trigger();
  }
  refractory=holdoff;
 }
 float env;float filterEnv=1.f,panEnv=0.5f;
 if(sequenceOn){env=sequencer.render(envelopeLength*sr);
  filterEnv=modulationSequencers[0].render(envelopeLength*sr);
  panEnv=modulationSequencers[1].render(envelopeLength*sr);
  activeSequenceStep.store(sequencer.getCurrentStep(),std::memory_order_relaxed);}
 else {env=phase>=1?1:curve(phase);phase=juce::jmin(1.0,phase+step);}
 if(sequenceOn)env=juce::jlimit(0.f,1.f,env+shapeOffset);
 float target=1-depth*(1-env);smoothGain=coeff*smoothGain+(1-coeff)*target;
 // Independent, opt-in modulation lanes; neutral defaults preserve legacy patches.
 const float pan=juce::jlimit(-1.f,1.f,panMod*(2.f*panEnv-1.f));
 const float panLeft=pan>0.f?std::cos(pan*juce::MathConstants<float>::halfPi):1.f;
 const float panRight=pan<0.f?std::cos(-pan*juce::MathConstants<float>::halfPi):1.f;
 const float filterLaneGain=1.f-filterRange*(1.f-filterEnv);
 for(int ch=0;ch<2;++ch){float dry=main.getSample(ch,i),wet=dry*smoothGain;
 if(mode!=0){auto& f=filters[ch];float low=f.low.processSample(0,dry),high=f.high.processSample(0,dry);float affected=mode==1?low:mode==2?high:dry-low-high;wet=dry+affected*(smoothGain-1);}
 float out=dry+(wet-dry)*mix;
 // Filter lane affects the selected frequency band, while Full mode remains full-band.
 if(filterRange>0.f&&mode!=0)out=dry+(out-dry)*filterLaneGain;
 if(panMod!=0.f)out*=ch==0?panLeft:panRight;
 if(fxBlend>0.00001f&&smoothedFxWet>0.f){const float saturated=di::processFx(out,smoothedFxDrive,smoothedFxType);
       out+=(smoothedFxWet*fxBlend)*(saturated-out);}
 if(!std::isfinite(out))out=dry;main.setSample(ch,i,out);peak=juce::jmax(peak,std::abs(out));}
 }
 meter.store(peak);midi.clear();
}
void DISidechainProcessor::getStateInformation(juce::MemoryBlock& dest){auto tree=state.copyState();for(int i=0;i<8;++i)tree.setProperty("point"+juce::String(i),points[i].load(),nullptr);for(int k=0;k<32;++k)for(int j=0;j<8;++j)tree.setProperty("seq_"+juce::String(k)+"_"+juce::String(j),getSequencePoint(k,j),nullptr);for(int lane=0;lane<2;++lane)for(int k=0;k<32;++k)for(int j=0;j<8;++j)tree.setProperty("mod_"+juce::String(lane)+"_"+juce::String(k)+"_"+juce::String(j),getModulationPoint(lane,k,j),nullptr);if(auto xml=tree.createXml())copyXmlToBinary(*xml,dest);}
void DISidechainProcessor::setStateInformation(const void* data,int bytes){if(auto xml=getXmlFromBinary(data,bytes)){auto tree=juce::ValueTree::fromXml(*xml);if(tree.isValid()){for(int i=0;i<8;++i)points[i]=(float)tree.getProperty("point"+juce::String(i),points[i].load());for(int k=0;k<32;++k)for(int j=0;j<8;++j){auto key="seq_"+juce::String(k)+"_"+juce::String(j);if(tree.hasProperty(key))setSequencePoint(k,j,(float)tree.getProperty(key));}for(int lane=0;lane<2;++lane)for(int k=0;k<32;++k)for(int j=0;j<8;++j){auto key="mod_"+juce::String(lane)+"_"+juce::String(k)+"_"+juce::String(j);if(tree.hasProperty(key))setModulationPoint(lane,k,j,(float)tree.getProperty(key));}state.replaceState(tree);}}}
juce::AudioProcessorEditor* DISidechainProcessor::createEditor(){return new DISidechainEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new DISidechainProcessor();}
