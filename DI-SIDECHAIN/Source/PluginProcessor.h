#pragma once
#include <JuceHeader.h>
#include <array>
#include "ShapeSequencer.h"
#include "OversampledFx2x.h"
#include "FixedDelay.h"
#include "PrecisionModules.h"
class DISidechainProcessor : public juce::AudioProcessor {
public:
 DISidechainProcessor();
 void prepareToPlay(double,int) override;
 void releaseResources() override {}
 bool isBusesLayoutSupported(const BusesLayout&) const override;
 void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
 juce::AudioProcessorEditor* createEditor() override;
 bool hasEditor() const override {return true;}
 const juce::String getName() const override {return "PRECISION SIDECHAIN";}
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
 std::atomic<int> activeSequenceStep {0};
 std::array<std::array<std::atomic<float>,8>,32> sequencePoints;
 std::array<std::array<std::array<std::atomic<float>,8>,32>,2> modulationPoints;
 void setModulationPoint(int lane,int step,int node,float value) noexcept;
 float getModulationPoint(int lane,int step,int node) const noexcept;
 void setSequencePoint(int step,int node,float value) noexcept;
 float getSequencePoint(int step,int node) const noexcept;
private:
 static juce::AudioProcessorValueTreeState::ParameterLayout makeParams();
 struct FilterChannel {juce::dsp::LinkwitzRileyFilter<float> low, high;};
 std::array<FilterChannel,2> filters;
 std::array<di::OversampledFx2x,2> oversampledFx, saturatorFx;
 std::array<di::FixedDelay<di::OversampledFx2x::latencySamples>,2> fxDryDelay, saturatorDryDelay;
 di::ShapeSequencer sequencer;
 std::array<di::ShapeSequencer,2> modulationSequencers;
 double sr=44100, phase=1, envelopeLength=0.25, detector=0;
 int refractory=0; bool armed=true; float smoothGain=1, fxBlend=0.f, smoothedFxDrive=1.f, smoothedFxWet=1.f, smoothedFxType=0.f;
 juce::int64 expectedTransportSample=-1;
 bool wasTransportPlaying=false;
 float saturatorBlend=0.f;
 float curve(double) const;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DISidechainProcessor)
};
