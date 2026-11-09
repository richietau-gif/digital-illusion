#pragma once
#include <cstdint>
#include <limits>
namespace di {
// Return -1 when the host timeline is unknown or advancing the block
// would overflow a signed 64-bit sample position.
inline std::int64_t nextTransportSample(std::int64_t position,int blockSize) noexcept {
 if(position<0||blockSize<0)return -1;
 if(position>std::numeric_limits<std::int64_t>::max()-blockSize)return -1;
 return position+blockSize;
}

// Sample positions are expected to advance exactly by the previous block size.
// Allow two samples of host rounding tolerance, but not a whole buffer:
// short Ableton loops and seeks must be recognized.
inline bool transportDiscontinuity(std::int64_t expected,std::int64_t actual) noexcept {
 if(expected<0)return false;
 // The host may report a negative position only for an actual rewind. Avoid
 // converting opposite-signed values to unsigned: that wraps and can look
 // spuriously contiguous at the int64 limits.
 if (actual < 0) return true;
 const auto a = static_cast<std::uint64_t>(expected);
 const auto b = static_cast<std::uint64_t>(actual);
 const auto difference = a > b ? a - b : b - a;
 return difference > 2u;
}
}
