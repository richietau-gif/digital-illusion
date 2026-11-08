#pragma once
#include <array>
#include <cmath>
#include <algorithm>
#include "FxWaveshaper.h"
namespace di {
// Streaming 2x oversampled waveshaper with linear-phase 15-tap half-band FIR.
// 7 samples of base-rate latency (3.5 up + 3.5 down); compensate dry externally.
// No heap allocation, locks, or parameter changes in the audio callback.
class OversampledFx2x {
public:
 static constexpr int latencySamples=7;
 void reset() noexcept { up.fill(0.f); down.fill(0.f); upIndex=downIndex=0; }
 float process(float input,float drive,float type) noexcept {
  if(!std::isfinite(input))input=0.f;
  // Zero stuffing; interpolation FIR has gain 2 at DC.
  const float a=filter(up,upIndex,input,2.f);
  const float b=filter(up,upIndex,0.f,2.f);
  const float processedA=processFx(a,drive,type);
  const float processedB=processFx(b,drive,type);
  filter(down,downIndex,processedA,1.f);
  return filter(down,downIndex,processedB,1.f);
 }
private:
 // Windowed-sinc lowpass at pi/2, normalized to unity DC.
 static constexpr std::array<float,15> taps{
  -0.00365145f,0.f,0.01617925f,0.f,-0.06841178f,0.f,
  0.30476398f,0.50224000f,0.30476398f,0.f,-0.06841178f,
  0.f,0.01617925f,0.f,-0.00365145f};
 static float filter(std::array<float,15>& history,int& index,float input,float gain) noexcept {
  history[index]=input;
  float sum=0.f;
  int p=index;
  for(float tap:taps){sum+=tap*history[p];p=p==0?14:p-1;}
  index=index==14?0:index+1;
  return sum*gain;
 }
 std::array<float,15> up{},down{};
 int upIndex=0,downIndex=0;
};
}
