#pragma once
#include <cmath>
#include <string>

namespace di {
// Compact UI frequency readout. No long float tails in HP/LP or sweep knobs.
// <1000 Hz: whole Hz. >=1000 Hz: kHz with at most ONE decimal.
inline std::string shortFrequency(float hz) {
 if(!std::isfinite(hz)||hz<0.f)hz=0.f;
 const int whole=static_cast<int>(std::lround(hz));
 if(whole<1000)return std::to_string(whole)+" Hz";
 const int tenths=static_cast<int>(std::lround(hz/100.f));
 if(tenths%10==0)return std::to_string(tenths/10)+" kHz";
 return std::to_string(tenths/10)+"."+std::to_string(tenths%10)+" kHz";
}
} // namespace di
