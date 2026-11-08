#pragma once
#include <algorithm>
#include <cmath>

namespace di {
struct FilterCutoffs { float low; float high; };

// Keep both cutoffs in the supported audio band, even with malformed
// automation or unusual host sample rates. The caller can safely pass
// these values to a Linkwitz-Riley filter.
inline FilterCutoffs safeFilterCutoffs(double sampleRate,
                                      float low, float high) noexcept {
 const double rate = std::isfinite(sampleRate) && sampleRate > 0. ? sampleRate : 44100.;
 const float ceiling = static_cast<float>(std::max(1., rate * 0.45));
 low = std::isfinite(low) ? std::clamp(low, 25.f, 1500.f) : 120.f;
 high = std::isfinite(high) ? std::clamp(high, 60.f, 18000.f) : 400.f;
 low = std::min(low, ceiling);
 high = std::min(ceiling, std::max(low, high));
 return {low, high};
}
} // namespace di
