#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "CreativeDistort2x.h"
#include "FixedDelay.h"
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>

namespace di {
// Reverse-send is a pre-reverb effect. Input arrives in fixed windows, and the
// previous COMPLETED window is played backwards. This is intentional lookback,
// not a claim to zero-latency future audio. Dry audio remains undelayed.
class ReverseSend {
public:
 void prepare(double sampleRate) {
  const double safe=std::isfinite(sampleRate)?std::clamp(sampleRate,8000.,192000.):44100.;
  maxWindow=std::max(64,static_cast<int>(std::round(safe*2.0)));
  window=std::max(64,static_cast<int>(std::round(safe*0.125)));
  nextWindow=window;
  for(auto& channel:record)channel.assign(static_cast<size_t>(maxWindow),0.f);
  for(auto& channel:play)channel.assign(static_cast<size_t>(maxWindow),0.f);
  reset();
 }
 void setWindowSamples(int requested) noexcept {
  // Window changes apply at a capture boundary to avoid partial-buffer reads.
  nextWindow=std::clamp(requested,64,std::max(64,maxWindow));
 }
 void reset() noexcept {
  phase=0;filled=false;
  for(auto& channel:record)std::fill(channel.begin(),channel.end(),0.f);
  for(auto& channel:play)std::fill(channel.begin(),channel.end(),0.f);
 }
 void process(float& left,float& right,bool reversed) noexcept {
  if(window<=0||record[0].empty())return;
  const float inL=std::isfinite(left)?left:0.f;
  const float inR=std::isfinite(right)?right:0.f;
  record[0][static_cast<size_t>(phase)]=inL;
  record[1][static_cast<size_t>(phase)]=inR;
  if(reversed){
   // Taper both edges of each reversed block to prevent sharp loop clicks.
   const int fade=std::max(1,std::min(256,window/4));
   const float gain=std::clamp(std::min(phase,window-1-phase)/(float)fade,0.f,1.f);
   left=filled?play[0][static_cast<size_t>(window-1-phase)]*gain:0.f;
   right=filled?play[1][static_cast<size_t>(window-1-phase)]*gain:0.f;
  }
  if(++phase==window) {
   phase=0;
   record.swap(play);
   filled=nextWindow==window;
   if(nextWindow!=window){window=nextWindow;filled=false;}
  }
 }
 int windowSamples() const noexcept{return window;}
private:
 std::array<std::vector<float>,2> record,play;
 int maxWindow=0,window=0,nextWindow=0,phase=0;
 bool filled=false;
};

// Preallocated stereo predelay. Delay is applied to the wet reverb send only.
class StereoPredelay {
public:
 void prepare(double sr){
  const size_t n=static_cast<size_t>(std::ceil(std::clamp(sr,8000.,192000.)*.26))+8u;
  for(auto& channel:history)channel.assign(n,0.f);
  reset();
 }
 void reset() noexcept{
  writeIndex=0;
  for(auto& channel:history)std::fill(channel.begin(),channel.end(),0.f);
 }
 void process(float& left,float& right,int delaySamples) noexcept{
  const auto size=history[0].size();
  if(size==0)return;
  const auto delay=static_cast<size_t>(std::clamp(delaySamples,0,static_cast<int>(size)-2));
  const auto readIndex=(writeIndex+size-delay)%size;
  const float oldL=history[0][readIndex],oldR=history[1][readIndex];
  history[0][writeIndex]=left;history[1][writeIndex]=right;
  writeIndex=(writeIndex+1)%size;
  if(delay!=0){left=oldL;right=oldR;}
 }
private:
 std::array<std::vector<float>,2> history;
 size_t writeIndex=0;
};
class CreativeFx {
public:
 void prepare(double sr) {
  sampleRate=std::isfinite(sr)?std::clamp(sr,8000.,192000.):44100.;
  reverse.prepare(sampleRate);
  predelay.prepare(sampleRate);
  fadeCoeff=1.f-std::exp(-1.f/(float)(sampleRate*0.005));
  reverb.setSampleRate(sampleRate);
  for(auto& d:distortion)d.reset();
  for(auto& d:distortionDry)d.reset();
  reset();
 }
 void setReverseWindowSeconds(double seconds) noexcept {
  if(!std::isfinite(seconds))return;
  reverse.setWindowSamples(static_cast<int>(std::round(
   std::clamp(seconds,.03,2.0)*sampleRate)));
 }
 void reset() noexcept {
  reverb.reset();
  reverse.reset();
  predelay.reset();
  toneMemory.fill(0.f);
  for(auto& d:distortion)d.reset();
  for(auto& d:distortionDry)d.reset();
  lastModel=-1;
  lastLength=-1.f;
  lastDecay=-1.f;
  lastDistDriveDb=-1.f;distGain=1.f;
  distBlend=verbBlend=verbSend=0.f;
 }
 // Stage order: distort (optional) -> reverse send (optional) -> reverb.
 // Both effects have independent sends and never remove the original dry signal.
 void process(float& left,float& right,bool distActive,bool verbActive,
              int distModel,float distDrive,float distWet,
              int verbModel,float verbWet,float verbLength,float verbDecay,bool reversed,
              float verbPreDelayMs=0.f,float distortTone=1.f) noexcept {
  if(!std::isfinite(left))left=0.f;
  if(!std::isfinite(right))right=0.f;
  const float dryL=left,dryR=right;
  const float dw=std::clamp(distWet,0.f,1.f);
  distBlend+=fadeCoeff*((distActive?dw:0.f)-distBlend);
  verbSend+=fadeCoeff*((verbActive?1.f:0.f)-verbSend);
  verbBlend+=fadeCoeff*(std::clamp(verbWet,0.f,1.f)-verbBlend);
  // Always run both the oversampled and the seven-sample dry alignment
  // paths. Bypass and automation therefore preserve the reported latency.
  if(distDrive!=lastDistDriveDb){
   lastDistDriveDb=distDrive;
   distGain=std::clamp(std::pow(10.f,distDrive/20.f),1.f,32.f);
  }
  const float alignedL=distortionDry[0].process(dryL);
  const float alignedR=distortionDry[1].process(dryR);
  const float distortedL=distortion[0].process(dryL,distModel,distGain);
  const float distortedR=distortion[1].process(dryR,distModel,distGain);
  // Tone: 0 = mellow lowpass, 1 = open / modern high-frequency edge.
  const float tone=std::clamp(distortTone,0.f,1.f);
  const float hz=180.f*std::pow(100.f,tone);
  const float toneCoeff=std::exp(-6.28318530718f*hz/(float)sampleRate);
  toneMemory[0]=toneCoeff*toneMemory[0]+(1.f-toneCoeff)*distortedL;
  toneMemory[1]=toneCoeff*toneMemory[1]+(1.f-toneCoeff)*distortedR;
  left=alignedL+distBlend*(toneMemory[0]-alignedL);
  right=alignedR+distBlend*(toneMemory[1]-alignedR);
  const float originalL=left,originalR=right;
  float sendL=left,sendR=right;
  reverse.process(sendL,sendR,reversed);
  // Do not reset the reverb on inactive steps: the tail must ring out.
  // Reverb processes continuously, but receives no new input on inactive steps.
  sendL*=verbSend;sendR*=verbSend;
  const int delaySamples=static_cast<int>(std::round(
   std::clamp(verbPreDelayMs,0.f,250.f)*.001*sampleRate));
  predelay.process(sendL,sendR,delaySamples);
  const int model=std::clamp(verbModel,0,4);
  constexpr float sizes[5]={.2f,.45f,.7f,.88f,.98f};
  constexpr float damp[5]={.75f,.60f,.45f,.27f,.12f};
  if(model!=lastModel||verbLength!=lastLength||verbDecay!=lastDecay){
   juce::Reverb::Parameters p;
   p.roomSize=std::clamp(sizes[model]*(.55f+.45f*std::clamp(verbLength,.1f,8.f)/8.f)*(1.f-.5f*std::clamp(verbDecay,0.f,1.f)),0.f,1.f);
   p.damping=std::clamp(damp[model]*(1.f-.65f*std::clamp(verbDecay,0.f,1.f)),0.f,1.f);
   p.width=1.f;p.freezeMode=0.f;p.wetLevel=.33f;p.dryLevel=0.f;
   reverb.setParameters(p);
   lastModel=model;lastLength=verbLength;lastDecay=verbDecay;
  }
  reverb.processStereo(&sendL,&sendR,1);
  const float rw=verbBlend;
  left=originalL*(1.f-rw)+rw*sendL;
  right=originalR*(1.f-rw)+rw*sendR;
  if(!std::isfinite(left))left=0.f;
  if(!std::isfinite(right))right=0.f;
 }
private:
 double sampleRate=44100.;
 ReverseSend reverse;
 StereoPredelay predelay;
 std::array<float,2> toneMemory{};
 std::array<CreativeDistort2x,2> distortion;
 std::array<FixedDelay<CreativeDistort2x::latencySamples>,2> distortionDry;
 juce::Reverb reverb;
 int lastModel=-1;
 float lastLength=-1.f,lastDecay=-1.f;
 float lastDistDriveDb=-1.f,distGain=1.f;
 float distBlend=0.f,verbBlend=0.f,verbSend=0.f,fadeCoeff=.001f;
};
} // namespace di
