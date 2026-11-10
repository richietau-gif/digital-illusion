#pragma once
#include <algorithm>
#include <cmath>

namespace di {
// DSP contract for full-band volume ducking. All envelopes and gain amounts
// are linear 0..1 values; never apply a second envelope in the output mixer.
inline float duckGainTarget(float envelope,float depth) noexcept {
 if(!std::isfinite(envelope))envelope=1.f;
 if(!std::isfinite(depth))depth=0.f;
 return 1.f-std::clamp(depth,0.f,1.f)*
             (1.f-std::clamp(envelope,0.f,1.f));
}
inline float applyDuckGain(float sample,float smoothedGain,float mix) noexcept {
 if(!std::isfinite(sample))return 0.f;
 const float amount=std::clamp(std::isfinite(mix)?mix:1.f,0.f,1.f);
 const float gain=std::clamp(std::isfinite(smoothedGain)?smoothedGain:1.f,0.f,1.f);
 return sample*(1.f+amount*(gain-1.f));
}
} // namespace di
