#pragma once
#include <array>
#include <cmath>
namespace di {
// Fixed integer delay for matching the oversampled FX path.
// No allocations or branches dependent on the host block size.
template<int Samples> class FixedDelay {
 static_assert(Samples>0,"Delay must be positive");
public:
 void reset() noexcept { history.fill(0.f); index=0; }
 float process(float sample) noexcept {
  if(!std::isfinite(sample))sample=0.f;
  const float delayed=history[index];
  history[index]=sample;
  index=(index+1)%Samples;
  return delayed;
 }
private:
 std::array<float,Samples> history{};
 int index=0;
};
}
