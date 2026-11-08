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
 // Pre-ducking mono audio history for the GUI FFT. Audio thread only writes
 // atomic samples; the UI does FFT off the realtime thread.
 static constexpr int analyzerSize = 2048;
 std::array<std::atomic<float>,analyzerSize> analyzerHistory {};
 std::atomic<uint64_t> analyzerWritten {0};
 // Variable-time nodes use atomic storage; default legacy eight-node sessions remain readable.
 static constexpr int maxEnvelopeNodes = 32;
 struct EnvelopeNode { float x=0.f, y=1.f; };
 int getEnvelopeNodeCount() const noexcept;
 EnvelopeNode getEnvelopeNode(int index) const noexcept;
 void setEnvelopeNode(int index, float x, float y) noexcept;
 int insertEnvelopeNode(float x, float y) noexcept;
 bool removeEnvelopeNode(int index) noexcept;
 float getEnvelopeSegmentCurve(int segment) const noexcept;
 void setEnvelopeSegmentCurve(int segment, float curve) noexcept;
 float evaluateEnvelope(double normalizedPhase) const noexcept;
 // Signed phase displacement: positive = delayed/right, negative = early/left.
 // ONE SHOT clamps to endpoints; LOOP wraps within the beat cycle.
 float evaluateShiftedEnvelope(double normalizedPhase,bool loop) const noexcept;
 // Filter's own editable A/B envelope banks; default LINK uses volume shape
 // so existing sessions retain their exact filter modulation.
 static constexpr int filterEnvelopePoints=8;
 float getFilterEnvelopePoint(int bank,int index) const noexcept;
 void setFilterEnvelopePoint(int bank,int index,float value) noexcept;
 float evaluateFilterEnvelope(double phase,bool loop) const noexcept;
 float evaluateFilterBank(int bank,double phase,bool loop) const noexcept;
 void copyVolumeShapeToFilterBanks() noexcept;
 float getVisualFollowerLevel() const noexcept {
     return visualFollowerLevel.load(std::memory_order_relaxed);
 }
 float getVisualFilterCutoffHz() const noexcept {
     return visualFilterCutoffHz.load(std::memory_order_relaxed);
 }
 // Factory envelope shapes; updates only envelope nodes, not filter or other parameters.
 void applyEnvelopePreset(int presetIndex) noexcept;
private:
 static juce::AudioProcessorValueTreeState::ParameterLayout makeParams();
 struct FilterChannel {
  juce::dsp::LinkwitzRileyFilter<float> low, high;
  juce::dsp::LinkwitzRileyFilter<float> bandLowPass, bandHighPass;
  juce::dsp::LinkwitzRileyFilter<float> phaseLowPass, phaseHighPass;
 };
 std::array<FilterChannel,2> filters;
 std::array<std::atomic<float>,maxEnvelopeNodes> nodeX {};
 std::array<std::atomic<float>,maxEnvelopeNodes> nodeY {};
 // Cubic Bezier easing factor per segment [-1, 1], 0 = smoothstep.
 std::array<std::atomic<float>,maxEnvelopeNodes> nodeCurve {};
 std::atomic<int> activeNodeCount {0}; // 0 selects legacy 8-point envelope
 std::array<std::array<std::atomic<float>,filterEnvelopePoints>,2> filterEnvelopeBanks {};
 std::atomic<float> visualFollowerLevel {0.f};
 std::atomic<bool> filterBanksInitialized {false};
 float followerLevel=0.f;
 void adoptLegacyPoints() noexcept;
 double sr=44100, phase=1, envelopeLength=0.25, detector=0;
 // Detector high-pass state is independent from audio-output crossover filters.
 double detectorHpPreviousInput=0., detectorHpPreviousOutput=0.;
 // Zero-delay-feedback topology-preserving state variable filter (TPT SVF).
 // Two independent stages permit 12/24 dB slopes while maintaining stereo.
 struct OutputSVFStage { float ic1=0.f,ic2=0.f; };
 std::array<std::array<OutputSVFStage,2>,2> outputFilterStages {};
 float outputFilterBlend=0.f;
 float smoothedFilterQ=.70710678f;
 float smoothedFilterDrive=0.f;
 std::array<float,2> drivePreviousInput {};

 std::atomic<float> visualFilterCutoffHz {20000.f};
 int refractory=0; bool armed=true; float smoothGain=1;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DISidechainProcessor)
};
