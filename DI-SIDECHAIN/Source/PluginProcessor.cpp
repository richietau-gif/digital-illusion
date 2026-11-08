#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
static juce::AudioProcessorValueTreeState::ParameterLayout layoutParams(){
 std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
 auto f=[&](const char* id,const char* name,float lo,float hi,float def){p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},name,juce::NormalisableRange<float>(lo,hi),def));};
 f("depth","Depth",0,1,1); f("mix","Mix",0,1,1);f("threshold","Trigger threshold dB",-60,0,-22);f("length","Envelope milliseconds",10,2000,250);f("smooth","Smoothing milliseconds",0.1,20,2);f("low","Low cutoff",25,1500,120);f("high","High cutoff",60,18000,400);f("retrigger","Trigger holdoff milliseconds",1,250,35);
 p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"mode",1},"Frequency mode",juce::StringArray{"Full","Low","High","Band"},0));
 p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"trigger",1},"Trigger mode",juce::StringArray{"Audio + MIDI","Audio only","MIDI only"},0));
 p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"sequenceEnabled",1},"Shape sequencer enabled",false));
 p.push_back(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{"sequenceSteps",1},"Sequence steps",1,32,16));
 p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"sync",1},"Tempo sync",true));
 p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"division",1},"Beat division",juce::StringArray{"1/32","1/16","1/8","1/4","1/2","1 bar","2 bars"},2));
 return {p.begin(),p.end()};
}
juce::AudioProcessorValueTreeState::ParameterLayout DISidechainProcessor::makeParams(){return layoutParams();}
DISidechainProcessor::DISidechainProcessor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withInput("Sidechain",juce::AudioChannelSet::stereo(),false).withOutput("Output",juce::AudioChannelSet::stereo(),true)),state(*this,nullptr,"PARAMS",makeParams()){
 const float defaults[8]={1,0.92f,0.08f,0.02f,0.32f,0.75f,0.95f,1};for(int i=0;i<8;++i)points[i]=defaults[i];
 for(auto& shape:sequencePoints)for(int i=0;i<8;++i)shape[i].store(defaults[i]);
}
void DISidechainProcessor::setSequencePoint(int step,int node,float value) noexcept {if(step>=0&&step<32&&node>=0&&node<8&&std::isfinite(value))sequencePoints[step][node].store(juce::jlimit(0.f,1.f,value),std::memory_order_relaxed);}
float DISidechainProcessor::getSequencePoint(int step,int node) const noexcept {return step>=0&&step<32&&node>=0&&node<8?sequencePoints[step][node].load(std::memory_order_relaxed):1.f;}
bool DISidechainProcessor::isBusesLayoutSupported(const BusesLayout& b) const {return b.getMainInputChannelSet()==juce::AudioChannelSet::stereo()&&b.getMainOutputChannelSet()==juce::AudioChannelSet::stereo()&&(b.inputBuses.size()<2||b.inputBuses[1].isDisabled()||b.inputBuses[1]==juce::AudioChannelSet::mono()||b.inputBuses[1]==juce::AudioChannelSet::stereo());}
void DISidechainProcessor::prepareToPlay(double sampleRate,int maxBlock){sr=sampleRate;phase=1;sequencer.reset();activeSequenceStep.store(0);detector=0;armed=true;refractory=0;smoothGain=1;juce::dsp::ProcessSpec spec{sampleRate,(juce::uint32)juce::jmax(1,maxBlock),1};for(auto& c:filters){c.low.prepare(spec);c.high.prepare(spec);c.low.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);c.high.setType(juce::dsp::LinkwitzRileyFilterType::highpass);c.low.reset();c.high.reset();}}
float DISidechainProcessor::curve(double t) const {t=juce::jlimit(0.0,1.0,t);double x=t*7;int i=juce::jmin(6,(int)x);float a=points[i].load(),b=points[i+1].load();float u=(float)(x-i);u=u*u*(3-2*u);return a+(b-a)*u;}
void DISidechainProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer& midi){
 juce::ScopedNoDenormals noDenormals;
 auto main=getBusBuffer(buffer,true,0);int n=main.getNumSamples();if(n<=0||main.getNumChannels()<2||!std::isfinite(sr)||sr<=0)return;
 auto* sideBus=getBus(true,1);bool hasSide=sideBus&&sideBus->isEnabled();
 // Keep sidechain as a non-owning view; never allocate or copy a buffer on the audio thread.
 juce::AudioBuffer<float> side; if(hasSide){auto view=getBusBuffer(buffer,true,1);side.setDataToReferTo(view.getArrayOfWritePointers(),view.getNumChannels(),view.getNumSamples());}
 auto get=[&](const char* id){return state.getRawParameterValue(id)->load();};
 int mode=(int)get("mode"),trig=(int)get("trigger");float depth=get("depth"),mix=get("mix"),lo=get("low"),hi=juce::jmax(lo+10,get("high"));
 for(auto& c:filters){c.low.setCutoffFrequency(lo);c.high.setCutoffFrequency(mode==3?hi:lo);}
 double bpm=120; if(auto* ph=getPlayHead()){if(auto pos=ph->getPosition()){if(auto b=pos->getBpm())bpm=*b;}}
 const double divisions[]={0.125,0.25,0.5,1,2,4,8};
 envelopeLength=get("sync")>0.5?60.0/juce::jmax(1.0,bpm)*divisions[juce::jlimit(0,6,(int)get("division"))]:get("length")/1000.0;
 const bool sequenceOn=get("sequenceEnabled")>0.5f;
 sequencer.setSteps((int)get("sequenceSteps"));
 if(sequenceOn)for(int k=0;k<32;++k){di::ShapeSequencer::Shape shape{};for(int j=0;j<8;++j)shape[j]=getSequencePoint(k,j);sequencer.setShape(k,shape);}
 double step=1.0/juce::jmax(1.0,envelopeLength*sr);int holdoff=(int)(get("retrigger")*.001*sr);float threshold=juce::Decibels::decibelsToGain(get("threshold"));float coeff=std::exp(-1.f/(float)(juce::jmax(.1f,get("smooth"))*.001*sr));
 juce::MidiBuffer::Iterator it(midi);juce::MidiMessage msg;int midiPos=0;bool hasEvent=it.getNextEvent(msg,midiPos);float peak=0;
 for(int i=0;i<n;++i){bool fire=false;
 while(hasEvent&&midiPos<=i){if(msg.isNoteOn()&&msg.getVelocity()>0&&trig!=1)fire=true;hasEvent=it.getNextEvent(msg,midiPos);}
 if(hasSide&&trig!=2&&side.getNumSamples()>=n){float v=0;for(int ch=0;ch<side.getNumChannels();++ch)v=juce::jmax(v,std::abs(side.getSample(ch,i)));detector=std::max((double)v,detector*.995);if(detector<threshold*.65)armed=true;if(armed&&detector>=threshold&&refractory==0){fire=true;armed=false;}}
 if(refractory>0)--refractory;if(fire){phase=0;if(sequenceOn)sequencer.trigger();refractory=holdoff;}
 float env;
 if(sequenceOn){env=sequencer.render(envelopeLength*sr);activeSequenceStep.store(sequencer.getCurrentStep(),std::memory_order_relaxed);}
 else {env=phase>=1?1:curve(phase);phase=juce::jmin(1.0,phase+step);}
 float target=1-depth*(1-env);smoothGain=coeff*smoothGain+(1-coeff)*target;
 for(int ch=0;ch<2;++ch){float dry=main.getSample(ch,i),wet=dry*smoothGain;
 if(mode!=0){auto& f=filters[ch];float low=f.low.processSample(0,dry),high=f.high.processSample(0,dry);float affected=mode==1?low:mode==2?high:dry-low-high;wet=dry+affected*(smoothGain-1);}
 float out=dry+(wet-dry)*mix;if(!std::isfinite(out))out=dry;main.setSample(ch,i,out);peak=juce::jmax(peak,std::abs(out));}
 }
 meter.store(peak);midi.clear();
}
void DISidechainProcessor::getStateInformation(juce::MemoryBlock& dest){auto tree=state.copyState();for(int i=0;i<8;++i)tree.setProperty("point"+juce::String(i),points[i].load(),nullptr);for(int k=0;k<32;++k)for(int j=0;j<8;++j)tree.setProperty("seq_"+juce::String(k)+"_"+juce::String(j),getSequencePoint(k,j),nullptr);if(auto xml=tree.createXml())copyXmlToBinary(*xml,dest);}
void DISidechainProcessor::setStateInformation(const void* data,int bytes){if(auto xml=getXmlFromBinary(data,bytes)){auto tree=juce::ValueTree::fromXml(*xml);if(tree.isValid()){for(int i=0;i<8;++i)points[i]=(float)tree.getProperty("point"+juce::String(i),points[i].load());for(int k=0;k<32;++k)for(int j=0;j<8;++j){auto key="seq_"+juce::String(k)+"_"+juce::String(j);if(tree.hasProperty(key))setSequencePoint(k,j,(float)tree.getProperty(key));}state.replaceState(tree);}}}
juce::AudioProcessorEditor* DISidechainProcessor::createEditor(){return new DISidechainEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new DISidechainProcessor();}
