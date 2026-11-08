#pragma once
#include <cstdint>
namespace di {
// Sample positions are expected to advance exactly by the previous block size.
// Allow two samples of host rounding tolerance, but not a whole buffer:
// short Ableton loops and seeks must be recognized.
inline bool transportDiscontinuity(std::int64_t expected,std::int64_t actual) noexcept {
 if(expected<0)return false;
 // Subtraction in unsigned arithmetic avoids signed overflow at extreme positions.
 const auto a=static_cast<std::uint64_t>(expected);
 const auto b=static_cast<std::uint64_t>(actual);
 const auto difference=a>b?a-b:b-a;
 return difference>2u;
}
}
