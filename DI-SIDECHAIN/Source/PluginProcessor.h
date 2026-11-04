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
 // Realtime-safe observability: written from the audio callback, read by the editor.
 std::atomic<float> visualPhase {1.f};
 std::atomic<float> visualGainReductionDb {0.f};
 std::atomic<uint32_t> triggerSequence {0};
 // Variable-time nodes use atomic storage; default legacy eight-node sessions remain readable.
 static constexpr int maxEnvelopeNodes = 32;
 struct EnvelopeNode { float x=0.f, y=1.f; };
 int getEnvelopeNodeCount() const noexcept;
 EnvelopeNode getEnvelopeNode(int index) const noexcept;
 void setEnvelopeNode(int index, float x, float y) noexcept;
 int insertEnvelopeNode(float x, float y) noexcept;
 bool removeEnvelopeNode(int index) noexcept;
 float evaluateEnvelope(double normalizedPhase) const noexcept;
private:
 static juce::AudioProcessorValueTreeState::ParameterLayout makeParams();
 struct FilterChannel {juce::dsp::LinkwitzRileyFilter<float> low, high;};
 std::array<FilterChannel,2> filters;
 std::array<std::atomic<float>,maxEnvelopeNodes> nodeX {};
 std::array<std::atomic<float>,maxEnvelopeNodes> nodeY {};
 std::atomic<int> activeNodeCount {0}; // 0 selects legacy 8-point envelope
 void adoptLegacyPoints() noexcept;
 double sr=44100, phase=1, envelopeLength=0.25, detector=0;
 int refractory=0; bool armed=true; float smoothGain=1;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DISidechainProcessor)
};
