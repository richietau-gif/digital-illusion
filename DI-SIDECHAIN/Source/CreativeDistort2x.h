#pragma once
#include <array>
#include <algorithm>
#include <cmath>

namespace di {
// Fixed, deterministic 2x oversampled creative waveshaper.
// A 15-tap symmetric half-band interpolation/decimation filter produces
// seven samples of base-rate group delay. No allocations in process().
class CreativeDistort2x {
public:
 static constexpr int latencySamples=7;
 void reset() noexcept {
  up.fill(0.f);down.fill(0.f);upIndex=downIndex=0;
 }
 float process(float input,int model,float gain) noexcept {
  if(!std::isfinite(input))input=0.f;
  const float even=fir(up,upIndex,input,2.f);
  const float odd=fir(up,upIndex,0.f,2.f);
  const float wetEven=shape(even,model,gain);
  const float wetOdd=shape(odd,model,gain);
  const float result=fir(down,downIndex,wetEven,1.f);
  (void)fir(down,downIndex,wetOdd,1.f);
  return std::isfinite(result)?result:0.f;
 }
 static float shape(float input,int model,float gain) noexcept {
  constexpr float pi=3.14159265358979323846f;
  const float x=std::clamp((std::isfinite(input)?input:0.f)*
      std::clamp(std::isfinite(gain)?gain:1.f,1.f,32.f),-64.f,64.f);
  float y=0.f;
  switch(std::clamp(model,0,4)){
   case 0: y=std::tanh(x);break;                          // Warm
   case 1: y=(2.f/pi)*std::atan(x*1.6f);break;           // Acid
   case 2: y=std::clamp(x,-.8f,.8f)/.8f;break;           // Clip
   case 3: y=std::sin(x*1.5f);break;                     // Fold
   default:y=std::tanh(x+.35f*std::sin(4.f*x));break;    // Modern
  }
  return std::isfinite(y)?std::clamp(y,-1.f,1.f):0.f;
 }
private:
 static constexpr std::array<float,15> taps{{
   -.00365145f,0.f,.01617925f,0.f,-.06841178f,0.f,
   .30476398f,.50224000f,.30476398f,0.f,-.06841178f,
   0.f,.01617925f,0.f,-.00365145f
 }};
 static float fir(std::array<float,15>& history,int& index,float value,float gain) noexcept {
  history[static_cast<size_t>(index)]=value;
  float sum=0.f;int p=index;
  for(float tap:taps){sum+=tap*history[static_cast<size_t>(p)];p=p==0?14:p-1;}
  index=index==14?0:index+1;
  return sum*gain;
 }
 std::array<float,15> up{},down{};
 int upIndex=0,downIndex=0;
};
} // namespace di
