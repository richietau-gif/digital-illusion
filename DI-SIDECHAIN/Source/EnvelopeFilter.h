#pragma once
#include <algorithm>
#include <cmath>
#include "AnalogLadder.h"

namespace di {
// A realtime-safe, continuously modulated 2-pole filter with no allocations.
// Mode: 0 low-pass, 1 high-pass, 2 band-pass, 3 notch.
// A separate four-pole input stage is used by the fixed HP/LP cleanup rack.
class EnvelopeFilter {
public:
 void prepare(double sampleRate) noexcept {
  sr=std::clamp(std::isfinite(sampleRate)?(float)sampleRate:44100.f,8000.f,192000.f);
  ladder.prepare(sr);
  reset();
 }
 void reset() noexcept {
  low1=low2=bandLow=0.f;lastCutoff=0.f;counter=0;ladder.reset();
 }
 static float frequency(float normalized,float amount,float minHz,float maxHz,float sampleRate) noexcept {
  const float nyquist=std::clamp(sampleRate*.42f,50.f,18000.f);
  const float bottom=std::clamp(std::isfinite(minHz)?minHz:90.f,25.f,nyquist);
  const float top=std::clamp(std::isfinite(maxHz)?maxHz:11000.f,bottom,nyquist);
  const float depth=std::clamp(std::isfinite(amount)?amount:0.f,0.f,1.f);
  const float env=std::clamp(std::isfinite(normalized)?normalized:1.f,0.f,1.f);
  return top*std::pow(bottom/top,depth*(1.f-env));
 }
 float process(float input,float cutoff,int mode,float resonance=0.f,bool analog=false) noexcept {
  if(!std::isfinite(input))return 0.f;
  const float maxCut=sr*.42f;
  const float hz=std::clamp(std::isfinite(cutoff)?cutoff:1000.f,25.f,maxCut);
  // Recalculate only if the frequency really changes; all state is per channel.
  // Update the nonlinear ladder coefficient at audio-rate/16. This is a
  // sub-millisecond control interval, avoiding expensive exp per sample.
  if(counter++%16==0||lastCutoff==0.f){
   lastCutoff=hz;
   g=std::clamp(1.f-std::exp(-6.283185307179586f*hz/sr),.00005f,.97f);
   bandG=std::clamp(g*.24f,.00005f,.97f);
   ladder.setCutoff(hz,resonance);
  }
  const float signal=analog?std::tanh(input*1.2f):input;
  low1+=g*(signal-low1);
  low2+=g*(low1-low2);
  bandLow+=bandG*(signal-bandLow);
  // Conservative feedforward peak emphasizes transitions without feedback instability.
  const float boost=std::clamp(std::isfinite(resonance)?resonance:0.f,0.f,.95f)*.25f;
  const float lp=std::clamp(low2+boost*(low1-low2),-8.f,8.f);
  const float analogLow=analog?ladder.process(signal):0.f;
  const float selectedLow=analog?analogLow:lp;
  const float hp=signal-selectedLow;
  const float bp=(analog?analogLow:low1)-bandLow;
  float result=mode==1?hp:mode==2?bp:mode==3?signal-bp:selectedLow;
  return std::isfinite(result)?result:0.f;
 }
private:
 float sr=44100.f,lastCutoff=0.f,g=.2f,bandG=.05f;
 float low1=0.f,low2=0.f,bandLow=0.f;
 unsigned int counter=0;
 AnalogLadder ladder;
};

// Static 12 dB/oct input clean-up HP and LP. Controls are *independent* of the
// rhythmic FILTER lane and apply even when no sequencer cells are active.
class InputCutFilter {
public:
 void prepare(double sampleRate) noexcept {
  sr=std::clamp(std::isfinite(sampleRate)?(float)sampleRate:44100.f,8000.f,192000.f);
  reset();set(35.f,17000.f);
 }
 void reset() noexcept { hp1=hp2=lp1=lp2=0.f; }
 void set(float hpHz,float lpHz) noexcept {
  const float cap=sr*.42f;
  const float hi=std::clamp(std::isfinite(hpHz)?hpHz:35.f,20.f,cap);
  const float lo=std::clamp(std::isfinite(lpHz)?lpHz:17000.f,20.f,cap);
  hg=std::clamp(1.f-std::exp(-6.2831853f*hi/sr),.00005f,.97f);
  lg=std::clamp(1.f-std::exp(-6.2831853f*lo/sr),.00005f,.97f);
 }
 float process(float sample,bool hpOn,bool lpOn) noexcept {
  if(!std::isfinite(sample))return 0.f;
  hp1+=hg*(sample-hp1);
  const float high=sample-hp1;
  hp2+=hg*(high-hp2);
  const float hp=high-hp2;
  const float x=hpOn?hp:sample;
  lp1+=lg*(x-lp1);
  lp2+=lg*(lp1-lp2);
  return lpOn?lp2:x;
 }
private:
 float sr=44100.f,hg=.001f,lg=.9f;
 float hp1=0.f,hp2=0.f,lp1=0.f,lp2=0.f;
};
} // namespace di
