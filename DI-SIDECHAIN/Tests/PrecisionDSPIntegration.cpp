#include "../Source/PluginProcessor.h"
#include <JuceHeader.h>
#include <array>
#include <cstdlib>
#include <cmath>
#include <iostream>

static void require(bool ok, const char* check, int line){
 if(!ok){std::cerr<<"Check failed at line "<<line<<": "<<check<<"\\n";std::exit(1);}
}
#define CHECK(expr) require(static_cast<bool>(expr),#expr,__LINE__)

namespace {
void param(DISidechainProcessor& plugin,const char* id,float value){
 auto* p=plugin.state.getParameter(id);
 CHECK(p!=nullptr);
 p->setValueNotifyingHost(p->convertTo0to1(value));
}
void fill(juce::AudioBuffer<float>& buffer){
 for(int ch=0;ch<buffer.getNumChannels();++ch)
  for(int i=0;i<buffer.getNumSamples();++i)
   buffer.setSample(ch,i,.1f*std::sin(.13f*(float)i+.12f*(float)ch));
}
void assertFinite(const juce::AudioBuffer<float>& buffer){
 for(int ch=0;ch<buffer.getNumChannels();++ch)
  for(int i=0;i<buffer.getNumSamples();++i)
   CHECK(std::isfinite(buffer.getSample(ch,i)));
}
}

int main(){
 juce::ScopedJuceInitialiser_GUI gui;
 DISidechainProcessor plugin;
 CHECK(plugin.getNumPrograms()==32);
 for(double rate:{44100.,48000.,96000.}){
  plugin.prepareToPlay(rate,128);
  juce::AudioBuffer<float> buffer(2,128);
  juce::MidiBuffer midi;
  // Bypass at 0 ms latency is strictly transparent (same phase in both channels).
  param(plugin,"lookahead",0.f);
  param(plugin,"bypass",1.f);
  plugin.prepareToPlay(rate,128);
  fill(buffer);
  juce::AudioBuffer<float> original;
  original.makeCopyOf(buffer);
  plugin.processBlock(buffer,midi);
  for(int ch=0;ch<2;++ch)
   for(int i=0;i<128;++i)
    CHECK(std::abs(buffer.getSample(ch,i)-original.getSample(ch,i))<1e-6f);
  // Dry/wet at 0% with zero lookahead and no colour/filter must remain dry.
  param(plugin,"bypass",0.f);
  param(plugin,"filterEnabled",0.f);
  param(plugin,"colorEnabled",0.f);
  param(plugin,"mix",0.f);
  param(plugin,"output",0.f);
  fill(buffer);original.makeCopyOf(buffer);
  plugin.processBlock(buffer,midi);
  for(int ch=0;ch<2;++ch)
   for(int i=0;i<128;++i)
    CHECK(std::abs(buffer.getSample(ch,i)-original.getSample(ch,i))<1e-5f);
  // Audible A/B regression: FILTER ENV must change cutoff response,
  // while FILTER ON remains active with modulation disabled.
  param(plugin,"triggerMode",1.f);
  param(plugin,"bypass",0.f);
  param(plugin,"depth",0.f);
  param(plugin,"filterEnabled",1.f);
  param(plugin,"filterModel",0.f);
  param(plugin,"filterMod",1.f);
  param(plugin,"filterMix",1.f);
  param(plugin,"cutoff",400.f);
  param(plugin,"resonance",.1f);
  param(plugin,"colorEnabled",0.f);
  param(plugin,"output",0.f);
  auto capture=[&](float enableEnvelope) {
   param(plugin,"filterEnvelopeEnabled",enableEnvelope);
   plugin.prepareToPlay(rate,128);
   fill(buffer);
   juce::MidiBuffer trigger;
   trigger.addEvent(juce::MidiMessage::noteOn(1,60,(juce::uint8)110),0);
   plugin.processBlock(buffer,trigger);
   juce::AudioBuffer<float> result;
   result.makeCopyOf(buffer);
   return result;
  };
  const auto staticFilter=capture(0.f);
  const auto modulatedFilter=capture(1.f);
  double responseDifference=0.;
  for(int i=0;i<128;++i)
   responseDifference+=std::abs(staticFilter.getSample(0,i)-modulatedFilter.getSample(0,i));
  CHECK(responseDifference>.001);

  // Audible A/B regression: COLOR ON/OFF must affect output samples.
  param(plugin,"filterEnabled",0.f);
  param(plugin,"mix",0.f);
  param(plugin,"colorModel",4.f);
  param(plugin,"colorMix",1.f);
  param(plugin,"colorDrive",12.f);
  param(plugin,"colorEnabled",0.f);
  fill(buffer);midi.clear();plugin.processBlock(buffer,midi);
  juce::AudioBuffer<float> colorOff; colorOff.makeCopyOf(buffer);
  param(plugin,"colorEnabled",1.f);
  fill(buffer);plugin.processBlock(buffer,midi);
  double colorDifference=0.;
  for(int i=0;i<128;++i)
   colorDifference+=std::abs(buffer.getSample(0,i)-colorOff.getSample(0,i));
  CHECK(colorDifference>.001);

  // Enumerate all named filter and colour models with envelope modulation on/off.
  for(int filter=0;filter<13;++filter){
   param(plugin,"filterModel",(float)filter);
   param(plugin,"filterEnabled",1.f);
   param(plugin,"cutoff",180.f);
   param(plugin,"filterMix",1.f);
   param(plugin,"resonance",.3f);
   for(int modulation=0;modulation<2;++modulation){
    param(plugin,"filterEnvelopeEnabled",(float)modulation);
    param(plugin,"filterMod",.6f);
    for(int color=0;color<6;++color){
     param(plugin,"colorEnabled",1.f);
     param(plugin,"colorModel",(float)color);
     param(plugin,"colorMix",.2f);
     fill(buffer);
     plugin.processBlock(buffer,midi);
     assertFinite(buffer);
    }
   }
  }
  param(plugin,"colorEnabled",0.f);
  param(plugin,"filterEnabled",0.f);
  param(plugin,"triggerMode",1.f);
  param(plugin,"timeOffset",-.5f);
  param(plugin,"depth",1.f);
  param(plugin,"mix",1.f);
  midi.addEvent(juce::MidiMessage::noteOn(1,60,(juce::uint8)100),0);
  fill(buffer);
  plugin.processBlock(buffer,midi);
  CHECK(plugin.triggerActivity.load()>0.f);
  assertFinite(buffer);
  // Verify the entire program bank can process, serialize and restore safely.
  for(int program=0;program<32;++program){
   plugin.setCurrentProgram(program);
   fill(buffer);
   plugin.processBlock(buffer,midi);
   assertFinite(buffer);
  }
  juce::MemoryBlock saved;
  plugin.getStateInformation(saved);
  CHECK(saved.getSize()>0);
  std::array<DISidechainProcessor::EnvelopeNode,DISidechainProcessor::maxEnvelopeNodes> nodes{};
  auto count=plugin.getEnvelopeNodes(nodes);
  CHECK(count>=2);
  plugin.setStateInformation(saved.getData(),(int)saved.getSize());
  CHECK(plugin.getEnvelopeNodes(nodes)==count);
  plugin.releaseResources();
 }
 std::cout<<"JUCE DSP integration: bypass, dry/wet, 13 filters, 6 colours, envelope gate, MIDI, presets, state and sample rates passed\\n";
}
