#pragma once
#include <array>
#include <algorithm>
#include <cmath>

// Realtime-safe envelope sequencer core. No allocations, locks or I/O in render().
namespace di {
class ShapeSequencer {
public:
 static constexpr int maxSteps = 32;
 static constexpr int nodesPerStep = 8;
 // Tempo-synced sequences follow the host transport, not incoming trigger density.
 static constexpr bool shouldRestartOnTrigger(bool tempoSync) noexcept { return !tempoSync; }
 using Shape = std::array<float, nodesPerStep>;
 ShapeSequencer() noexcept {
   const Shape initial{{1.f, .92f, .08f, .02f, .32f, .75f, .95f, 1.f}};
   shapes.fill(initial);
 }
 void setSteps(int count) noexcept { steps = std::clamp(count, 1, maxSteps); currentStep %= steps; }
 int getSteps() const noexcept { return steps; }
 int getCurrentStep() const noexcept { return currentStep; }
 void setShape(int step, const Shape& shape) noexcept {
   if (step < 0 || step >= maxSteps) return;
   for (int i = 0; i < nodesPerStep; ++i)
     shapes[step][i] = std::isfinite(shape[i]) ? std::clamp(shape[i], 0.f, 1.f) : 1.f;
 }
 Shape getShape(int step) const noexcept {
   return shapes[std::clamp(step, 0, maxSteps - 1)];
 }
 void reset() noexcept { currentStep = 0; phase = 0.; }
 void trigger() noexcept { currentStep = 0; phase = 0.; }
 void advanceStep() noexcept { currentStep = (currentStep + 1) % steps; phase = 0.; }
 void seekToStepPosition(double stepPosition) noexcept {
   if (!std::isfinite(stepPosition)) return;
   const double wrapped = std::fmod(std::fmod(stepPosition, static_cast<double>(steps)) + steps, static_cast<double>(steps));
   currentStep = std::clamp(static_cast<int>(wrapped), 0, steps - 1);
   phase = wrapped - static_cast<double>(currentStep);
 }

 // samplesPerStep may be derived from BPM and musical division in the processor.
 float render(double samplesPerStep) noexcept {
   if (!std::isfinite(samplesPerStep) || samplesPerStep < 1.) samplesPerStep = 1.;
   const float result = evaluate(shapes[currentStep], phase);
   phase += 1. / samplesPerStep;
   while (phase >= 1.) { phase -= 1.; currentStep = (currentStep + 1) % steps; }
   return result;
 }
 static float evaluate(const Shape& shape, double position) noexcept {
   const double x = std::clamp(position, 0., 1.) * (nodesPerStep - 1);
   const int i = std::min(nodesPerStep - 2, static_cast<int>(x));
   float t = static_cast<float>(x - i);
   t = t * t * (3.f - 2.f * t);
   return shape[i] + (shape[i + 1] - shape[i]) * t;
 }
private:
 std::array<Shape, maxSteps> shapes{};
 int steps = 16, currentStep = 0;
 double phase = 0.;
};
} // namespace di
