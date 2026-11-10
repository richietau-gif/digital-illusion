#pragma once
#include <algorithm>
#include <cmath>
namespace di {
struct StereoPanGain {float left=1.f,right=1.f;};
// Equal-power attenuation pan modulation around a neutral lane midpoint.
inline StereoPanGain panModulation(float lane,float amount) noexcept {
 if(!std::isfinite(lane))lane=.5f;
 if(!std::isfinite(amount))amount=0.f;
 const float v=std::clamp(amount*(2.f*std::clamp(lane,0.f,1.f)-1.f),-1.f,1.f);
 constexpr float halfPi=1.57079632679489661923f;
 return {v>0.f?std::cos(v*halfPi):1.f,
         v<0.f?std::cos(-v*halfPi):1.f};
}
inline float filterModulationBlend(float envelope,float amount) noexcept {
 const float env=std::clamp(std::isfinite(envelope)?envelope:1.f,0.f,1.f);
 const float depth=std::clamp(std::isfinite(amount)?amount:0.f,0.f,1.f);
 return 1.f-depth*(1.f-env);
}
} // namespace di
