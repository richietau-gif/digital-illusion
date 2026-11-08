#pragma once
#include <algorithm>
#include <cmath>
namespace di {
// Mix already latency-aligned dry and oversampled wet signals.
// Preserve the exact dry sample for bypass and prevent nonfinite wet leakage.
inline float mixAlignedFx(float dry,float wet,float amount) noexcept {
 if(!std::isfinite(dry))return 0.f;
 if(!std::isfinite(amount)||amount<=0.f)return dry;
 if(!std::isfinite(wet))return dry;
 amount=std::min(amount,1.f);
 return dry+amount*(wet-dry);
}
}
