#pragma once
#include <JuceHeader.h>
#include <array>
#include "ShapeSequencer.h"
#include "OversampledFx2x.h"
#include "FixedDelay.h"
#include "CreativeFx.h"
#include "DuckVoiceEngine.h"
#include "EnvelopeFilter.h"
class DISidechainProcessor : public juce::AudioProcessor {
public:
 DISidechainProcessor();
 void prepareToPlay(double,int) override;
 void releaseResources() override {}
 bool isBusesLayoutSupported(const BusesLayout&) const override;
 void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
 juce::AudioProcessorEditor* createEditor() override;
 bool hasEditor() const override {return true;}
 const juce::String getName() const override {return "DI SEQFX";}
 bool acceptsMidi() const override {return true;}
 bool producesMidi() const override {return false;}
 bool isMidiEffect() const override {return false;}
 double getTailLengthSeconds() const override {return 12.;}
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
 std::atomic<float> duckReduction {0}; // actual post-smoothing gain reduction, 0..1
 std::atomic<float> inputLevel {0}; // pre-FX audio input peak; diagnoses silent MIDI/audio routing
 std::atomic<int> activeSequenceStep {0};
 static constexpr int gridTracks=6, gridSteps=16;
 static constexpr int gridScenes=4;
 std::atomic<int> activeGridScene{0};
 std::array<std::array<std::array<std::atomic<bool>,gridSteps>,gridTracks>,gridScenes> sceneCells{};
 std::array<std::array<std::atomic<bool>,gridSteps>,gridScenes> sceneReverseCells{};
 // A cell chooses independently whether to apply ducking and which FX sends
 // are activated. Bit 0 = reverb, bit 1 = distortion, bit 2 = pre-reverb
 // reverse, bit 3 = bypass duck envelope only, bit 4 = delay send.
 std::array<std::array<std::array<std::atomic<int>,gridSteps>,6>,gridScenes> sceneCellFx{};
 void setCellFxFlag(int track,int step,int flag,bool enabled) noexcept;
 bool getCellFxFlag(int track,int step,int flag) const noexcept;
 int getCellFxMask(int track,int step) const noexcept;
 void setGridScene(int scene) noexcept;
 int getGridScene() const noexcept;
 void copyGridScene(int source,int destination) noexcept;
 std::array<std::atomic<float>,6> duckDepth{},duckLength{};
 std::array<std::array<std::atomic<float>,8>,6> duckCurve{};
 // A unique, scene-local 2-16-point envelope belongs to EVERY GRID cell:
 // scene A-D x six musical tracks x sixteen steps. No shared track curve.
 using StepPointArray=std::array<std::atomic<float>,di::DynamicCurve::capacity>;
 using StepPointGrid=std::array<std::array<std::array<StepPointArray,gridSteps>,gridTracks>,gridScenes>;
 std::array<std::array<std::array<std::atomic<int>,gridSteps>,gridTracks>,gridScenes> duckDynamicCount{};
 StepPointGrid duckDynamicX{},duckDynamicY{};
 di::DynamicCurve getDynamicDuckCurve(int track,int step,int scene=-1) const noexcept;
 void setDynamicDuckCurve(int track,int step,const di::DynamicCurve& curve,int scene=-1) noexcept;
 void setDuckTrackDepth(int track,float value) noexcept;
 void setDuckTrackLength(int track,float value) noexcept;
 float getDuckTrackDepth(int track) const noexcept;
 float getDuckTrackLength(int track) const noexcept;
 void setDuckCurvePoint(int track,int node,float value) noexcept;
 float getDuckCurvePoint(int track,int node) const noexcept;
 void setGridReverse(int step,bool enabled) noexcept;
 bool getGridReverse(int step) const noexcept;
 void setGridCell(int track,int step,bool enabled) noexcept;
 bool getGridCell(int track,int step) const noexcept;

 std::array<std::array<std::atomic<float>,8>,32> sequencePoints;
 std::array<std::array<std::array<std::atomic<float>,8>,32>,2> modulationPoints;
 void setModulationPoint(int lane,int step,int node,float value) noexcept;
 float getModulationPoint(int lane,int step,int node) const noexcept;
 void setSequencePoint(int step,int node,float value) noexcept;
 float getSequencePoint(int step,int node) const noexcept;
private:
 static juce::AudioProcessorValueTreeState::ParameterLayout makeParams();
 std::array<di::EnvelopeFilter,2> envelopeFilters;
 std::array<di::InputCutFilter,2> inputFilters;
 std::array<di::OversampledFx2x,2> oversampledFx, saturatorFx;
 std::array<di::FixedDelay<di::OversampledFx2x::latencySamples>,2> fxDryDelay, saturatorDryDelay;
 di::CreativeFx creativeFx;
 di::DuckVoiceEngine duckVoices;
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
