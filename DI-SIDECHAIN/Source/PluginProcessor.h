#pragma once
#include <JuceHeader.h>
#include <array>
class DISidechainProcessor : public juce::AudioProcessor, private juce::AsyncUpdater {
public:
 DISidechainProcessor();
 ~DISidechainProcessor() override { cancelPendingUpdate(); }
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
 struct EnvelopeNode { float x = 0.f, y = 1.f, curve = 0.f; };
 static constexpr int maxEnvelopeNodes = 12;
 int getEnvelopeNodes(std::array<EnvelopeNode,maxEnvelopeNodes>& target) const noexcept;
 void setEnvelopeNodes(const std::array<EnvelopeNode,maxEnvelopeNodes>& source, int count) noexcept;
 float getEnvelopeAt(float phase) const noexcept;
 std::atomic<unsigned> envelopeRevision{0};
 juce::AudioProcessorValueTreeState state;
 std::atomic<float> meter{0.f}, triggerActivity{0.f}, gainReductionMeter{0.f}, outputMeterL{0.f}, outputMeterR{0.f};
 static juce::AudioProcessorValueTreeState::ParameterLayout makeParams();
private:
 void handleAsyncUpdate() override;
 std::atomic<int> pendingLatencySamples{0};
 int previousDelaySamples=0;
 int delayTransitionSamples=0;
 static constexpr int delayTransitionLength=128;
 struct Filter
 {
  juce::dsp::IIR::Filter<float> a,b,c,d;
  juce::dsp::StateVariableTPTFilter<float> animated,animated2,animated3,animated4;
  static constexpr int combSize=8192;
  std::array<float,combSize> combBuffer{};
  int combWrite=0;
  float processComb(float input,int delay,float feedback) noexcept {
   delay=juce::jlimit(1,combSize-1,delay);
   const int read=(combWrite+combSize-delay)%combSize;
   const float previous=combBuffer[(size_t)read];
   combBuffer[(size_t)combWrite]=std::tanh(input+juce::jlimit(0.f,.92f,feedback)*previous);
   if(++combWrite>=combSize)combWrite=0;
   return .5f*input+.5f*previous;
  }
  void prepare(const juce::dsp::ProcessSpec& s) {a.prepare(s);b.prepare(s);c.prepare(s);d.prepare(s);animated.prepare(s);animated2.prepare(s);animated3.prepare(s);animated4.prepare(s);}
  void reset() {a.reset();b.reset();c.reset();d.reset();animated.reset();animated2.reset();animated3.reset();animated4.reset();combBuffer.fill(0.f);combWrite=0;}
 };
 std::array<std::atomic<float>,maxEnvelopeNodes> envelopeXs{}, envelopeYs{}, envelopeCurves{};
 std::atomic<int> envelopeNodeCount{2};
  std::array<float,2> colorToneLow{{0.f,0.f}};
  double sr=44100.; int maxBlock=0; int currentPreset=0; int delaySamples=0; int delayWrite=0; bool gateArmed=true; int holdoff=0; float detector=0.f, rmsEnergy=0.f, transientFast=0.f, transientSlow=0.f,phase=1.f,gain=1.f;
 std::array<std::array<float,2>,192001> delay{}; std::array<Filter,2> filters;
 float envelope(float) const noexcept; float colour(float,int,float) const noexcept; void applyPreset(int);
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DISidechainProcessor)
};
