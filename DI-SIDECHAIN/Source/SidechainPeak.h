#pragma once
#include <cmath>
#include <algorithm>
namespace di {
// Sidechain samples may contain NaN/Inf from an upstream plugin.
// Ignore invalid channels rather than poisoning the peak follower.
inline float sidechainPeak(float left,float right,bool stereo) noexcept {
 const float l=std::isfinite(left)?std::abs(left):0.f;
 const float r=stereo&&std::isfinite(right)?std::abs(right):0.f;
 return std::max(l,r);
}
}
