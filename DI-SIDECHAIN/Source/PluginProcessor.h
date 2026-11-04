#pragma once
#include <JuceHeader.h>
#include <array>
class DISidechainProcessor : public juce::AudioProcessor {
public:
 DISidechainProcessor();
 void prepareToPlay(double,int) override;
 void releaseResources() override {}
 bool isBusesLayoutSupported(const BusesLayout&) const override;
 void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
 juce::AudioProcessorEditor* createEditor() override;
 bool hasEditor() const override {return true;}
 const juce::String getName() const override {return "DI SIDECHAIN";}
 bool acceptsMidi() const override {return true;}
 bool producesMidi() const override {return false;}
 bool isMidiEffect() const override {return false;}
 double getTailLengthSeconds() const override {return 0;}
 int getNumPrograms() override {return 1;}
 int getCurrentProgram() override {return 0;}
 void setCurrentProgram(int) override {}
 const juce::String getProgramName(int) override {return {};}
 void changeProgramName(int,const juce::String&) override {}
 void getStateInformation(juce::MemoryBlock&) override;
 void setStateInformation(const void*,int) override;
 juce::AudioProcessorValueTreeState state;
 std::array<std::atomic<float>,8> points;
 std::atomic<float> meter {0};
 // Same gain envelope evaluator is used by DSP and GUI rendering.
 float evaluateEnvelope(double normalizedPhase) const;
private:
 static juce::AudioProcessorValueTreeState::ParameterLayout makeParams();
 struct FilterChannel {juce::dsp::LinkwitzRileyFilter<float> low, high;};
 std::array<FilterChannel,2> filters;
 double sr=44100, phase=1, envelopeLength=0.25, detector=0;
 int refractory=0; bool armed=true; float smoothGain=1;
 float curve(double) const;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DISidechainProcessor)
};
