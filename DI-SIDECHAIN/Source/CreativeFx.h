#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
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
  window=std::max(64,static_cast<int>(std::round(safe*0.125)));
  for(auto& channel:record)channel.assign(static_cast<size_t>(window),0.f);
  for(auto& channel:play)channel.assign(static_cast<size_t>(window),0.f);
  reset();
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
   filled=true;
  }
 }
 int windowSamples() const noexcept{return window;}
private:
 std::array<std::vector<float>,2> record,play;
 int window=0,phase=0;
 bool filled=false;
};

class CreativeFx {
public:
 void prepare(double sr) {
  sampleRate=std::isfinite(sr)?std::clamp(sr,8000.,192000.):44100.;
  reverse.prepare(sampleRate);
  fadeCoeff=1.f-std::exp(-1.f/(float)(sampleRate*0.005));
  reverb.setSampleRate(sampleRate);
  reset();
 }
 void reset() noexcept {
  reverb.reset();
  reverse.reset();
  lastModel=-1;
  lastLength=-1.f;
  lastDecay=-1.f;
  distBlend=verbBlend=verbSend=0.f;
 }
 static float distort(float input,int model,float drive) noexcept {
  const float x=std::clamp(std::isfinite(input)?input:0.f,-8.f,8.f)
              *std::clamp(std::isfinite(drive)?drive:1.f,1.f,32.f);
  constexpr float pi=3.14159265358979323846f;
  float y=0.f;
  switch(std::clamp(model,0,4)) {
   case 0: y=std::tanh(x);break;                        // warm
   case 1: y=2.f/pi*std::atan(2.f*x);break;             // soft acid
   case 2: y=std::clamp(x,-.8f,.8f)/.8f;break;          // clip
   case 3: y=std::sin(x*1.7f);break;                    // fold
   default: y=std::tanh(x+0.35f*std::sin(x*4.f));break; // aggressive
  }
  return std::isfinite(y)?std::clamp(y,-1.f,1.f):0.f;
 }
 // Stage order: distort (optional) -> reverse send (optional) -> reverb.
 // Both effects have independent sends and never remove the original dry signal.
 void process(float& left,float& right,bool distActive,bool verbActive,
              int distModel,float distDrive,float distWet,
              int verbModel,float verbWet,float verbLength,float verbDecay,bool reversed) noexcept {
  if(!std::isfinite(left))left=0.f;
  if(!std::isfinite(right))right=0.f;
  const float dryL=left,dryR=right;
  const float dw=std::clamp(distWet,0.f,1.f);
  distBlend+=fadeCoeff*((distActive?dw:0.f)-distBlend);
  verbSend+=fadeCoeff*((verbActive?1.f:0.f)-verbSend);
  verbBlend+=fadeCoeff*(std::clamp(verbWet,0.f,1.f)-verbBlend);
  if(distBlend>0.000001f) {
   const float gain=std::clamp(std::pow(10.f,distDrive/20.f),1.f,32.f);
   left=dryL+distBlend*(distort(dryL,distModel,gain)-dryL);
   right=dryR+distBlend*(distort(dryR,distModel,gain)-dryR);
  }
  const float originalL=left,originalR=right;
  float sendL=left,sendR=right;
  reverse.process(sendL,sendR,reversed);
  // Do not reset the reverb on inactive steps: the tail must ring out.
  // Reverb processes continuously, but receives no new input on inactive steps.
  sendL*=verbSend;sendR*=verbSend;
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
 juce::Reverb reverb;
 int lastModel=-1;
 float lastLength=-1.f,lastDecay=-1.f;
 float distBlend=0.f,verbBlend=0.f,verbSend=0.f,fadeCoeff=.001f;
};
} // namespace di
