#pragma once
// Pure, JUCE-independent timing rules shared by audio DSP and CTest.
#include <algorithm>
#include <cmath>
namespace precision_timing {
inline float finiteUnit(float value) noexcept {
 return std::isfinite(value)?std::clamp(value,0.f,1.f):0.f;
}
inline float finiteBipolar(float value) noexcept {
 return std::isfinite(value)?std::clamp(value,-1.f,1.f):0.f;
}
inline float shiftedPhase(float phase,float offset) noexcept {
 return finiteUnit(phase+finiteBipolar(offset));
}
inline float endPhase(float offset) noexcept {
 return 1.f+std::max(0.f,-finiteBipolar(offset));
}
inline bool active(float phase,float offset,bool triggered) noexcept {
 return triggered && std::isfinite(phase) && phase<endPhase(offset);
}
inline float mixDelayed(float delayed,float gain,float mix) noexcept {
 return delayed*(1.f-finiteUnit(mix))+delayed*gain*finiteUnit(mix);
}
}
