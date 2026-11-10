#pragma once
#include <algorithm>
#include <cmath>

namespace di {
// Original analog-inspired nonlinear four-pole resonant low-pass filter.
// It is not an emulation/copy of any proprietary synth algorithm.
class AnalogLadder {
public:
 void prepare(double sampleRate) noexcept {
  sr=std::isfinite(sampleRate)?std::clamp(sampleRate,8000.,192000.):44100.;
  reset();setCutoff(400.f,.2f);
 }
 void reset() noexcept {for(auto& v:z)v=0.f;}
 void setCutoff(float hz,float resonance) noexcept {
  hz=std::isfinite(hz)?std::clamp(hz,25.f,static_cast<float>(sr*.42)):400.f;
  const float q=std::isfinite(resonance)?std::clamp(resonance,0.f,.95f):.2f;
  // Conservative resonance feedback keeps the nonlinear loop stable.
  feedback=q*2.9f;
  g=std::clamp(1.f-std::exp(-6.283185307f*hz/(float)sr),.0001f,.95f);
 }
 float process(float input) noexcept {
  float x=std::tanh(std::clamp(input,-16.f,16.f)-feedback*z[3]);
  for(float& state:z){
   state+=g*(std::tanh(x)-state);
   x=state;
  }
  return std::isfinite(z[3])?std::clamp(z[3],-4.f,4.f):0.f;
 }
private:
 double sr=44100.;
 float z[4]{},g=.05f,feedback=0.f;
};
} // namespace di
