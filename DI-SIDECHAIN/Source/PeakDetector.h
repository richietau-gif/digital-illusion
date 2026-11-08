#pragma once
#include <algorithm>
#include <cmath>
namespace di {
// Sample-rate independent peak follower, designed for audio-thread use.
inline double peakReleaseCoefficient(double sampleRate, double releaseMs = 10.) noexcept {
 if (!std::isfinite(sampleRate) || sampleRate <= 0. || !std::isfinite(releaseMs) || releaseMs <= 0.)
  return 0.;
 return std::exp(-1. / (sampleRate * releaseMs * 0.001));
}
inline double followPeak(double previous, double inputPeak, double coefficient) noexcept {
 if (!std::isfinite(previous) || previous < 0.) previous = 0.;
 if (!std::isfinite(inputPeak) || inputPeak < 0.) inputPeak = 0.;
 if (!std::isfinite(coefficient)) coefficient = 0.;
 coefficient = std::clamp(coefficient, 0., 1.);
 return std::max(inputPeak, previous * coefficient);
}
}
