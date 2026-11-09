#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "FxWaveshaper.h"
#include "FxWetMix.h"
#include "FilterCutoff.h"
#include "PeakDetector.h"
#include "SidechainPeak.h"
#include "TransportContinuity.h"
#include "TriggerLogic.h"
#include <cmath>
static juce::AudioProcessorValueTreeState::ParameterLayout layoutParams(){
 std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
 auto f=[&](const char* id,const char* name,float lo,float hi,float def){p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},name,juce::NormalisableRange<float>(lo,hi),def));};
 f("depth","Depth",0,1,1); f("mix","Mix",0,1,1);f("threshold","Trigger threshold dB",-60,0,-22);f("length","Envelope milliseconds",10,2000,250);f("smooth","Smoothing milliseconds",0.1,20,2);f("low","Low cutoff",25,1500,120);f("high","High cutoff",60,18000,400);f("retrigger","Trigger holdoff milliseconds",1,250,35);f("shapeOffset","Shape vertical offset",-1,1,0);f("panMod","Pan modulation amount",-1,1,0);f("filterMod","Filter modulation amount",0,1,0);f("fxDrive","Legacy FX drive dB",0,24,0);f("saturatorDrive","Saturator drive dB",0,24,0);f("fxMix","FX wet mix",0,1,1);
 p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"mode",1},"Frequency mode",juce::StringArray{"Full","Low","High","Band"},0));
 p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"trigger",1},"Trigger mode",juce::StringArray{"Audio + MIDI","Audio only","MIDI only"},0));
 p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"sequenceEnabled",1},"Shape sequencer enabled",true));
 p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"creativeGridEnabled",1},"Creative track sequencer enabled",true));
 p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"reverbEnabled",1},"Reverb enabled",false));
 p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"distortEnabled",1},"Distortion enabled",false));
 p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"reverbReverse",1},"Reverse before reverb",false));
 p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"reverbModel",1},"Reverb model",juce::StringArray{"Room","Plate","Hall","Shimmer-ish","Space"},0));
 p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"distortModel",1},"Distortion model",juce::StringArray{"Warm","Acid","Clip","Fold","Modern"},0));
 f("reverbWet","Reverb wet",0,1,.3f); f("reverbLength","Reverb length",.1f,8.f,3.f);
 f("reverbDecay","Reverb decay",0,1,.6f);
 f("distortWet","Distortion wet",0,1,.4f); f("distortDrive","Distortion drive dB",0,24,8);

 p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"fxEnabled",1},"FX enabled",false));
 p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"saturatorEnabled",1},"Independent saturator enabled",false));
 p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"clipperMode",1},"Clipper mode",juce::StringArray{"Off","Soft","Hard"},0));
 p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"fxType",1},"FX type",juce::StringArray{"Saturator","Soft Clipper"},0));
 p.push_back(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{"sequenceSteps",1},"Sequence steps",1,16,16));
 p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"sync",1},"Tempo sync",true));
 p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"division",1},"Beat division",juce::StringArray{"1/32","1/16","1/8","1/4","1/2","1 bar","2 bars"},2));
 // Editor-only settings persist in the host project and user .dipreset files.
 // New instances start at the compact 75% layout; restored host state still
 // overrides this default and therefore preserves existing projects.
 p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"uiScale",1},"UI scale",juce::StringArray{"50%","75%","100%"},1));
 p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"renderQuality",1},"Curve render quality",juce::StringArray{"Economy","Standard","High"},1));
 return {p.begin(),p.end()};
}
juce::AudioProcessorValueTreeState::ParameterLayout DISidechainProcessor::makeParams(){return layoutParams();}
DISidechainProcessor::DISidechainProcessor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withInput("Sidechain",juce::AudioChannelSet::stereo(),false).withOutput("Output",juce::AudioChannelSet::stereo(),true)),state(*this,nullptr,"PARAMS",makeParams()){
 const float defaults[8]={1,0.92f,0.08f,0.02f,0.32f,0.75f,0.95f,1};for(int i=0;i<8;++i)points[i]=defaults[i];
 for(auto& shape:sequencePoints)for(int i=0;i<8;++i)shape[i].store(defaults[i]);
 for(int lane=0;lane<2;++lane)for(auto& shape:modulationPoints[lane])for(int i=0;i<8;++i)shape[i].store(lane==0?1.f:0.5f);
 for(auto& scene:sceneCells)for(auto& track:scene)for(auto& cell:track)
  cell.store(false,std::memory_order_relaxed);
 for(auto& scene:sceneReverseCells)for(auto& cell:scene)
  cell.store(false,std::memory_order_relaxed);
 for(int t=0;t<6;++t){
  duckDepth[(size_t)t].store(1.f,std::memory_order_relaxed);
  duckLength[(size_t)t].store(1.f,std::memory_order_relaxed);
  constexpr float defaultShape[8]={0.f,.02f,.2f,.65f,.9f,1.f,1.f,1.f};
  for(int j=0;j<8;++j)duckCurve[(size_t)t][(size_t)j].store(defaultShape[j],std::memory_order_relaxed);
 }
}
void DISidechainProcessor::setDuckTrackDepth(int t,float v) noexcept {
 if(t>=0&&t<6&&std::isfinite(v))duckDepth[(size_t)t].store(juce::jlimit(0.f,1.f,v),std::memory_order_relaxed);
}
float DISidechainProcessor::getDuckTrackDepth(int t) const noexcept {
 return t>=0&&t<6?duckDepth[(size_t)t].load(std::memory_order_relaxed):1.f;
}
void DISidechainProcessor::setDuckTrackLength(int t,float v) noexcept {
 if(t>=0&&t<6&&std::isfinite(v))duckLength[(size_t)t].store(juce::jlimit(.05f,4.f,v),std::memory_order_relaxed);
}
float DISidechainProcessor::getDuckTrackLength(int t) const noexcept {
 return t>=0&&t<6?duckLength[(size_t)t].load(std::memory_order_relaxed):1.f;
}
void DISidechainProcessor::setDuckCurvePoint(int t,int j,float v) noexcept {
 if(t>=0&&t<6&&j>=0&&j<8&&std::isfinite(v))
  duckCurve[(size_t)t][(size_t)j].store(juce::jlimit(0.f,1.f,v),std::memory_order_relaxed);
}
float DISidechainProcessor::getDuckCurvePoint(int t,int j) const noexcept {
 return t>=0&&t<6&&j>=0&&j<8?duckCurve[(size_t)t][(size_t)j].load(std::memory_order_relaxed):1.f;
}
void DISidechainProcessor::setGridScene(int scene) noexcept {
 activeGridScene.store(juce::jlimit(0,gridScenes-1,scene),std::memory_order_release);
}
int DISidechainProcessor::getGridScene() const noexcept {
 return juce::jlimit(0,gridScenes-1,activeGridScene.load(std::memory_order_acquire));
}
void DISidechainProcessor::copyGridScene(int source,int destination) noexcept {
 if(source<0||source>=gridScenes||destination<0||destination>=gridScenes||source==destination)return;
 for(int t=0;t<gridTracks;++t)for(int k=0;k<gridSteps;++k)
  sceneCells[(size_t)destination][(size_t)t][(size_t)k].store(
   sceneCells[(size_t)source][(size_t)t][(size_t)k].load(std::memory_order_relaxed),std::memory_order_relaxed);
 for(int k=0;k<gridSteps;++k)
  sceneReverseCells[(size_t)destination][(size_t)k].store(
   sceneReverseCells[(size_t)source][(size_t)k].load(std::memory_order_relaxed),std::memory_order_relaxed);
}
void DISidechainProcessor::setGridReverse(int step,bool enabled) noexcept {
 if(step>=0&&step<gridSteps)
  sceneReverseCells[(size_t)getGridScene()][(size_t)step].store(enabled,std::memory_order_relaxed);
}
bool DISidechainProcessor::getGridReverse(int step) const noexcept {
 return step>=0&&step<gridSteps&&sceneReverseCells[(size_t)getGridScene()][(size_t)step].load(std::memory_order_relaxed);
}
void DISidechainProcessor::setGridCell(int track,int step,bool enabled) noexcept {
 if(track>=0&&track<gridTracks&&step>=0&&step<gridSteps)
  sceneCells[(size_t)getGridScene()][(size_t)track][(size_t)step].store(enabled,std::memory_order_relaxed);
}
bool DISidechainProcessor::getGridCell(int track,int step) const noexcept {
 return track>=0&&track<gridTracks&&step>=0&&step<gridSteps&&
  sceneCells[(size_t)getGridScene()][(size_t)track][(size_t)step].load(std::memory_order_relaxed);
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
void DISidechainProcessor::prepareToPlay(double sampleRate,int maxBlock){sr=sampleRate;creativeFx.prepare(sampleRate);duckVoices.reset();setLatencySamples(2*di::OversampledFx2x::latencySamples+di::CreativeDistort2x::latencySamples);for(auto& fx:oversampledFx)fx.reset();for(auto& fx:saturatorFx)fx.reset();for(auto& delay:fxDryDelay)delay.reset();for(auto& delay:saturatorDryDelay)delay.reset();saturatorBlend=state.getRawParameterValue("saturatorEnabled")->load()>0.5f?1.f:0.f;phase=1;sequencer.reset();for(auto& lane:modulationSequencers)lane.reset();activeSequenceStep.store(0);detector=0;armed=true;refractory=0;smoothGain=1;expectedTransportSample=-1;wasTransportPlaying=false;fxBlend=state.getRawParameterValue("fxEnabled")->load()>0.5f?1.f:0.f;const auto safeInitial=[this](const char* id,float low,float high,float fallback){const float value=state.getRawParameterValue(id)->load();return std::isfinite(value)?juce::jlimit(low,high,value):fallback;};smoothedFxDrive=juce::Decibels::decibelsToGain(safeInitial("fxDrive",0.f,24.f,0.f));smoothedFxWet=safeInitial("fxMix",0.f,1.f,1.f);smoothedFxType=safeInitial("fxType",0.f,1.f,0.f);juce::dsp::ProcessSpec spec{sampleRate,(juce::uint32)juce::jmax(1,maxBlock),1};for(auto& c:filters){c.low.prepare(spec);c.high.prepare(spec);c.low.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);c.high.setType(juce::dsp::LinkwitzRileyFilterType::highpass);c.low.reset();c.high.reset();}}
float DISidechainProcessor::curve(double t) const {if(!std::isfinite(t))t=0.;t=juce::jlimit(0.0,1.0,t);double x=t*7;int i=juce::jmin(6,(int)x);const auto safePoint=[this](int node){const float value=points[node].load(std::memory_order_relaxed);return std::isfinite(value)?juce::jlimit(0.f,1.f,value):1.f;};float a=safePoint(i),b=safePoint(i+1);float u=(float)(x-i);u=u*u*(3-2*u);return a+(b-a)*u;}
void DISidechainProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer& midi){
 juce::ScopedNoDenormals noDenormals;
 auto main=getBusBuffer(buffer,true,0);int n=main.getNumSamples();if(n<=0||main.getNumChannels()<2||!std::isfinite(sr)||sr<=0)return;
 auto* sideBus=getBus(true,1);bool hasSide=sideBus&&sideBus->isEnabled();
 // Keep sidechain as a non-owning view; never allocate or copy a buffer on the audio thread.
 juce::AudioBuffer<float> side; if(hasSide){auto view=getBusBuffer(buffer,true,1);side.setDataToReferTo(view.getArrayOfWritePointers(),view.getNumChannels(),view.getNumSamples());}
 auto get=[&](const char* id){return state.getRawParameterValue(id)->load();};
 // Validate host automation before integer casts, filter coefficients and smoothers.
 auto bounded=[&](const char* id,float low,float high,float fallback){
  const float value=get(id);
  return std::isfinite(value)?juce::jlimit(low,high,value):fallback;
 };
 const int mode=(int)bounded("mode",0.f,3.f,0.f);
 const int trig=(int)bounded("trigger",0.f,2.f,0.f);
 const float depth=bounded("depth",0.f,1.f,1.f);
 const float mix=bounded("mix",0.f,1.f,1.f);
 const float shapeOffset=bounded("shapeOffset",-1.f,1.f,0.f);
 const float panMod=bounded("panMod",-1.f,1.f,0.f);
 const float filterMod=bounded("filterMod",0.f,1.f,0.f);
 const float fxDrive=bounded("fxDrive",0.f,24.f,0.f);
 const float fxMix=bounded("fxMix",0.f,1.f,1.f);
 // Keep filter cutoffs below Nyquist at lower host sample rates (e.g. 22.05 kHz).
 const auto cutoffs=di::safeFilterCutoffs(sr,get("low"),get("high"));
 const float lo=cutoffs.low;
 const float hi=cutoffs.high;
 const float filterRange=juce::jlimit(0.f,1.f,filterMod);
 const bool fxEnabled=get("fxEnabled")>0.5f;
 const bool saturatorEnabled=get("saturatorEnabled")>0.5f;
 const bool creativeGrid=get("creativeGridEnabled")>0.5f;
 const bool reverbEnabled=get("reverbEnabled")>0.5f;
 const bool distortEnabled=get("distortEnabled")>0.5f;
 const bool reverseReverb=get("reverbReverse")>0.5f;
 const int reverbModel=(int)bounded("reverbModel",0.f,4.f,0.f);
 const int distortModel=(int)bounded("distortModel",0.f,4.f,0.f);
 const float reverbWet=bounded("reverbWet",0.f,1.f,.3f);
 const float reverbLength=bounded("reverbLength",.1f,8.f,3.f);
 const float reverbDecay=bounded("reverbDecay",0.f,1.f,.6f);
 const float distortWet=bounded("distortWet",0.f,1.f,.4f);
 const float distortDrive=bounded("distortDrive",0.f,24.f,8.f);

 const int fxType=(int)bounded("fxType",0.f,1.f,0.f);
 int clipperMode=(int)bounded("clipperMode",0.f,2.f,0.f);
 if(clipperMode==0 && fxEnabled) clipperMode=fxType>0?1:2;
 // Guard automation and restored host state before values enter persistent smoothers.
 const float drive=juce::Decibels::decibelsToGain(std::isfinite(fxDrive)?juce::jlimit(0.f,24.f,fxDrive):0.f);
 const float saturatorDriveDb=bounded("saturatorDrive",0.f,24.f,fxDrive);
 const float saturatorDrive=juce::Decibels::decibelsToGain(saturatorDriveDb);
 const float fxWet=std::isfinite(fxMix)?juce::jlimit(0.f,1.f,fxMix):0.f;
 const float fxTarget=clipperMode>0?1.f:0.f;
 const float saturatorTarget=saturatorEnabled?1.f:0.f;
 const float fxSmoothing=std::exp(-1.f/(float)(0.01*sr));
 const float fxParameterSmoothing=std::exp(-1.f/(float)(0.005*sr));
 for(auto& c:filters){c.low.setCutoffFrequency(lo);c.high.setCutoffFrequency(mode==3?hi:lo);}
 double bpm=120;
 double hostPpq=0.;bool hasPlayingPpq=false;
 if(auto* ph=getPlayHead()){
  if(auto pos=ph->getPosition()){
   if(auto b=pos->getBpm())if(std::isfinite(*b)&&*b>0.)bpm=*b;
   const bool playing=pos->getIsPlaying();
   if(playing){if(auto ppq=pos->getPpqPosition()){hostPpq=*ppq;hasPlayingPpq=std::isfinite(hostPpq);}}
   bool discontinuity=playing&&!wasTransportPlaying;
   if(playing){
    if(auto sample=pos->getTimeInSamples()){
     if(di::transportDiscontinuity(expectedTransportSample,*sample))discontinuity=true;
     expectedTransportSample=di::nextTransportSample(*sample,n);
    }else expectedTransportSample=-1;
   }else expectedTransportSample=-1;
   if(discontinuity){duckVoices.reset();sequencer.reset();for(auto& lane:modulationSequencers)lane.reset();activeSequenceStep.store(0,std::memory_order_relaxed);phase=0;}
   wasTransportPlaying=playing;
  }
 }
 const double divisions[]={0.125,0.25,0.5,1,2,4,8};
 const float divisionRaw=get("division");
 const int divisionIndex=std::isfinite(divisionRaw)?juce::jlimit(0,6,(int)divisionRaw):2;
 const float lengthRaw=get("length");
 const double lengthSeconds=std::isfinite(lengthRaw)?juce::jlimit(10.f,2000.f,lengthRaw)/1000.0:0.25;
 envelopeLength=get("sync")>0.5?60.0/bpm*divisions[divisionIndex]:lengthSeconds;
 const bool tempoSync=get("sync")>0.5f;
 const bool sequenceOn=get("sequenceEnabled")>0.5f;
 const int sequenceSteps=(int)bounded("sequenceSteps",1.f,16.f,16.f);
 sequencer.setSteps(sequenceSteps);
 for(auto& lane:modulationSequencers)lane.setSteps(sequenceSteps);
 // Re-anchor each audio block to the DAW's PPQ timeline when tempo sync is enabled.
 // This corrects accumulated drift and aligns loop restarts to musical bars.
 if(sequenceOn&&tempoSync&&hasPlayingPpq){
  const double beatsPerStep=divisions[divisionIndex];
  const double position=hostPpq/beatsPerStep;
  sequencer.seekToStepPosition(position);
  for(auto& lane:modulationSequencers)lane.seekToStepPosition(position);
 }
 if(sequenceOn)for(int k=0;k<sequenceSteps;++k){
  di::ShapeSequencer::Shape shape{};for(int j=0;j<8;++j)shape[j]=getSequencePoint(k,j);sequencer.setShape(k,shape);
  for(int lane=0;lane<2;++lane){di::ShapeSequencer::Shape mod{};for(int j=0;j<8;++j)mod[j]=getModulationPoint(lane,k,j);modulationSequencers[lane].setShape(k,mod);}
 }
 if(creativeGrid){
  for(int t=0;t<6;++t){
   di::DuckVoiceEngine::TrackSettings settings;
   settings.depth=getDuckTrackDepth(t);
   settings.duration=getDuckTrackLength(t);
   for(int node=0;node<8;++node)settings.shape[(size_t)node]=getDuckCurvePoint(t,node);
   duckVoices.setTrack(t,settings);
  }
 }
 double step=1.0/juce::jmax(1.0,envelopeLength*sr);int holdoff=(int)(bounded("retrigger",1.f,250.f,35.f)*.001*sr);float threshold=juce::Decibels::decibelsToGain(bounded("threshold",-60.f,0.f,-22.f));float coeff=std::exp(-1.f/(float)(bounded("smooth",.1f,20.f,2.f)*.001*sr));
 const double detectorRelease=di::peakReleaseCoefficient(sr);
 juce::MidiBuffer::Iterator it(midi);juce::MidiMessage msg;int midiPos=0;bool hasEvent=it.getNextEvent(msg,midiPos);float peak=0;
 for(int i=0;i<n;++i){bool midiNoteOn=false,audioPeak=false;
 fxBlend=fxTarget+(fxBlend-fxTarget)*fxSmoothing;
 saturatorBlend=saturatorTarget+(saturatorBlend-saturatorTarget)*fxSmoothing;
 if(!std::isfinite(smoothedFxDrive))smoothedFxDrive=drive;
 if(!std::isfinite(smoothedFxWet))smoothedFxWet=fxWet;
 smoothedFxDrive=drive+(smoothedFxDrive-drive)*fxParameterSmoothing;
 smoothedFxWet=fxWet+(smoothedFxWet-fxWet)*fxParameterSmoothing;
 smoothedFxType=(float)clipperMode+(smoothedFxType-(float)clipperMode)*fxSmoothing;
 while(hasEvent&&midiPos<=i){
  if(msg.isNoteOn()&&msg.getVelocity()>0){
   midiNoteOn=true;
   // MIDI notes 36..41 independently play six duck voices. Velocity scales depth.
   // MIDI events are handled at their exact in-block sample offset.
   if(sequenceOn&&creativeGrid){
    const int track=msg.getNoteNumber()-36;
    if(track>=0&&track<di::DuckVoiceEngine::tracks)
     duckVoices.triggerTrack(track,envelopeLength*sr,msg.getFloatVelocity());
   }
  }
  hasEvent=it.getNextEvent(msg,midiPos);
 }
 if(hasSide&&trig!=2&&side.getNumSamples()>=n){const float v=di::sidechainPeak(side.getSample(0,i),side.getNumChannels()>1?side.getSample(1,i):0.f,side.getNumChannels()>1);detector=di::followPeak(detector,(double)v,detectorRelease);if(detector<threshold*.65)armed=true;if(armed&&detector>=threshold)audioPeak=true;}
 const bool fire=di::shouldFireTrigger(midiNoteOn,audioPeak,armed,refractory,trig);
 // Consume a threshold crossing even during holdoff or a simultaneous MIDI note.
 // Otherwise a sustained kick can fire a delayed ghost trigger after holdoff.
 if(di::shouldConsumeAudioPeak(audioPeak,armed,trig))armed=false;
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
 if(sequenceOn&&creativeGrid){
  const int gridStep=activeSequenceStep.load(std::memory_order_relaxed);
  std::array<bool,di::DuckVoiceEngine::tracks> cells{};
  for(int t=0;t<di::DuckVoiceEngine::tracks;++t)cells[(size_t)t]=getGridCell(t,gridStep);
  // Every row has an independent envelope and length, with strongest-duck
  // overlap; a row triggers once as the transport enters its active step.
  env=duckVoices.process(gridStep,cells,envelopeLength*sr);
 }
 if(sequenceOn)env=juce::jlimit(0.f,1.f,env+shapeOffset);
 float target=1-depth*(1-env);smoothGain=coeff*smoothGain+(1-coeff)*target;
 // Independent, opt-in modulation lanes; neutral defaults preserve legacy patches.
 const float pan=juce::jlimit(-1.f,1.f,panMod*(2.f*panEnv-1.f));
 const float panLeft=pan>0.f?std::cos(pan*juce::MathConstants<float>::halfPi):1.f;
 const float panRight=pan<0.f?std::cos(-pan*juce::MathConstants<float>::halfPi):1.f;
 const float filterLaneGain=1.f-filterRange*(1.f-filterEnv);
 for(int ch=0;ch<2;++ch){float dry=main.getSample(ch,i);if(!std::isfinite(dry))dry=0.f;float wet=dry*smoothGain;
 if(mode!=0){auto& f=filters[ch];float low=f.low.processSample(0,dry),high=f.high.processSample(0,dry);float affected=mode==1?low:mode==2?high:dry-low-high;wet=dry+affected*(smoothGain-1);}
 float out=dry+(wet-dry)*mix;
 // Filter lane affects the selected frequency band, while Full mode remains full-band.
 if(filterRange>0.f&&mode!=0)out=dry+(out-dry)*filterLaneGain;
 if(panMod!=0.f)out*=ch==0?panLeft:panRight;
 // Run both branches continuously so automation/bypass does not reset FIR history.
 // Fixed 7-sample latency is reported to the host and shared by dry and wet.
 const float fxDry=fxDryDelay[ch].process(out);
 const float fxProcessed=oversampledFx[ch].process(out,smoothedFxDrive,smoothedFxType); // FX preserves original type and automation
 out=di::mixAlignedFx(fxDry,fxProcessed,smoothedFxWet*fxBlend);
 // Saturator is a separate, independently bypassable stage. Both oversampled
 // processors always run and both dry paths are latency-aligned to avoid clicks.
 const float satDry=saturatorDryDelay[ch].process(out);
 const float satProcessed=saturatorFx[ch].process(out,saturatorDrive,0.f);
 out=di::mixAlignedFx(satDry,satProcessed,smoothedFxWet*saturatorBlend);
 if(!std::isfinite(out))out=0.f;main.setSample(ch,i,out);peak=juce::jmax(peak,std::abs(out));}
 // Grid effect lanes: indices 6/7 are REVERB and DISTORT respectively.
 // In legacy mode, the new effects remain opt-in and ignore step cells.
 const int currentStep=activeSequenceStep.load(std::memory_order_relaxed);
 const bool verbCell=!creativeGrid||getGridCell(6,currentStep);
 const bool distCell=!creativeGrid||getGridCell(7,currentStep);
 float left=main.getSample(0,i),right=main.getSample(1,i);
 creativeFx.process(left,right,distortEnabled&&distCell,reverbEnabled&&verbCell,
                    distortModel,distortDrive,distortWet,
                    reverbModel,reverbEnabled?reverbWet:0.f,reverbLength,reverbDecay,(reverseReverb||getGridReverse(currentStep))&&verbCell);
 main.setSample(0,i,left);main.setSample(1,i,right);
 peak=juce::jmax(peak,juce::jmax(std::abs(left),std::abs(right)));
 }
 meter.store(peak);midi.clear();
}
void DISidechainProcessor::getStateInformation(juce::MemoryBlock& dest){auto tree=state.copyState();for(int i=0;i<8;++i)tree.setProperty("point"+juce::String(i),points[i].load(),nullptr);for(int k=0;k<32;++k)for(int j=0;j<8;++j)tree.setProperty("seq_"+juce::String(k)+"_"+juce::String(j),getSequencePoint(k,j),nullptr);for(int lane=0;lane<2;++lane)for(int k=0;k<32;++k)for(int j=0;j<8;++j)tree.setProperty("mod_"+juce::String(lane)+"_"+juce::String(k)+"_"+juce::String(j),getModulationPoint(lane,k,j),nullptr);tree.setProperty("gridScene",getGridScene(),nullptr);
 for(int scene=0;scene<gridScenes;++scene){
  for(int t=0;t<gridTracks;++t)for(int k=0;k<gridSteps;++k)
   tree.setProperty("scene_"+juce::String(scene)+"_"+juce::String(t)+"_"+juce::String(k),
    sceneCells[(size_t)scene][(size_t)t][(size_t)k].load(std::memory_order_relaxed),nullptr);
  for(int k=0;k<gridSteps;++k)
   tree.setProperty("reverse_"+juce::String(scene)+"_"+juce::String(k),
    sceneReverseCells[(size_t)scene][(size_t)k].load(std::memory_order_relaxed),nullptr);
 }
 for(int t=0;t<6;++t) {
  tree.setProperty("duckDepth_"+juce::String(t),getDuckTrackDepth(t),nullptr);
  tree.setProperty("duckLength_"+juce::String(t),getDuckTrackLength(t),nullptr);
  for(int j=0;j<8;++j)tree.setProperty("duckCurve_"+juce::String(t)+"_"+juce::String(j),getDuckCurvePoint(t,j),nullptr);
 }
 if(auto xml=tree.createXml())copyXmlToBinary(*xml,dest);}
void DISidechainProcessor::setStateInformation(const void* data,int bytes){
 if(auto xml=getXmlFromBinary(data,bytes)){
  auto tree=juce::ValueTree::fromXml(*xml);
  if(!tree.isValid()||tree.getType()!=state.state.getType())return;
  // Missing properties in older presets must restore defaults, not leak
  // curve values from a previously loaded preset in this processor instance.
  constexpr float volumeDefaults[8]={1.f,.92f,.08f,.02f,.32f,.75f,.95f,1.f};
  const auto restored=[&](const juce::String& key,float fallback){
   if(!tree.hasProperty(key))return fallback;
   const float value=(float)tree.getProperty(key);
   return std::isfinite(value)?juce::jlimit(0.f,1.f,value):fallback;
  };
  for(int j=0;j<8;++j)
   points[j].store(restored("point"+juce::String(j),volumeDefaults[j]),std::memory_order_relaxed);
  for(int k=0;k<32;++k)for(int j=0;j<8;++j)
   setSequencePoint(k,j,restored("seq_"+juce::String(k)+"_"+juce::String(j),volumeDefaults[j]));
  for(int lane=0;lane<2;++lane)for(int k=0;k<32;++k)for(int j=0;j<8;++j)
   setModulationPoint(lane,k,j,restored("mod_"+juce::String(lane)+"_"+juce::String(k)+"_"+juce::String(j),lane==0?1.f:.5f));
  for(int scene=0;scene<gridScenes;++scene){
   for(int t=0;t<gridTracks;++t)for(int k=0;k<gridSteps;++k){
    const auto key="scene_"+juce::String(scene)+"_"+juce::String(t)+"_"+juce::String(k);
    sceneCells[(size_t)scene][(size_t)t][(size_t)k].store((bool)tree.getProperty(key,false),std::memory_order_relaxed);
   }
   for(int k=0;k<gridSteps;++k){
    const auto key="reverse_"+juce::String(scene)+"_"+juce::String(k);
    sceneReverseCells[(size_t)scene][(size_t)k].store((bool)tree.getProperty(key,false),std::memory_order_relaxed);
   }
  }
  setGridScene((int)tree.getProperty("gridScene",0));
  for(int t=0;t<6;++t){
   setDuckTrackDepth(t,(float)tree.getProperty("duckDepth_"+juce::String(t),1.f));
   setDuckTrackLength(t,(float)tree.getProperty("duckLength_"+juce::String(t),1.f));
   constexpr float shape[8]={0.f,.02f,.2f,.65f,.9f,1.f,1.f,1.f};
   for(int j=0;j<8;++j)setDuckCurvePoint(t,j,(float)tree.getProperty("duckCurve_"+juce::String(t)+"_"+juce::String(j),shape[j]));
  }
  duckVoices.reset();
  state.replaceState(tree);
 }
}
juce::AudioProcessorEditor* DISidechainProcessor::createEditor(){return new DISidechainEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new DISidechainProcessor();}
