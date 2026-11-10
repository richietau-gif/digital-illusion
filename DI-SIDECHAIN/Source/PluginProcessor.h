#pragma once
#include <JuceHeader.h>
#include <array>
class DISidechainProcessor : public juce::AudioProcessor {
public:
 DISidechainProcessor();
 void prepareToPlay(double,int) override; void releaseResources() override {}
 bool isBusesLayoutSupported(const BusesLayout&) const override;
 void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
 juce::AudioProcessorEditor* createEditor() override; bool hasEditor() const override { return true; }
 const juce::String getName() const override { return "PRECISION SIDECHAIN"; }
 bool acceptsMidi() const override { return true; } bool producesMidi() const override { return false; }
 bool isMidiEffect() const override { return false; } double getTailLengthSeconds() const override { return 0; }
 int getNumPrograms() override { return 32; } int getCurrentProgram() override { return currentPreset; }
 void setCurrentProgram(int) override; const juce::String getProgramName(int) override; void changeProgramName(int,const juce::String&) override {}
 void getStateInformation(juce::MemoryBlock&) override; void setStateInformation(const void*,int) override;
 juce::AudioProcessorValueTreeState state;
 std::atomic<float> meter{0.f}, triggerActivity{0.f};
 static juce::AudioProcessorValueTreeState::ParameterLayout makeParams();
private:
 struct Filter
 {
  juce::dsp::IIR::Filter<float> a,b,c,d;
  juce::dsp::StateVariableTPTFilter<float> animated;
  void prepare(const juce::dsp::ProcessSpec& s) {a.prepare(s);b.prepare(s);c.prepare(s);d.prepare(s);animated.prepare(s);}
  void reset() {a.reset();b.reset();c.reset();d.reset();animated.reset();}
 };
 double sr=44100.; int maxBlock=0; int currentPreset=0; int delaySamples=0; int delayWrite=0; bool gateArmed=true; int holdoff=0; float detector=0.f,phase=1.f,gain=1.f;
 std::array<std::array<float,2>,192001> delay{}; std::array<Filter,2> filters;
 float envelope(float) const noexcept; float colour(float,int,float) const noexcept; void applyPreset(int);
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DISidechainProcessor)
};
