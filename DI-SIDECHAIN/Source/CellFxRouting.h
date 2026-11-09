#pragma once
#include <array>
#include <cstdint>

namespace di {
// Pure, allocation-free event router shared by sequencer DSP and CTest.
// Bits: 0 = Reverb, 1 = Distort, 2 = Reverse into reverb,
//       3 = Bypass duck envelope, 4 = Delay.
struct CellFxRoute {
 std::array<bool,6> duck{};
 bool reverb=false,distort=false,reverse=false,delay=false;
};
inline CellFxRoute routeCellEffects(const std::array<bool,6>& events,
                                  const std::array<int,6>& masks,
                                  bool gated) noexcept {
 CellFxRoute result;
 if(!gated){
  result.duck.fill(false);
  result.reverb=result.distort=result.delay=true;
  return result;
 }
 for(size_t i=0;i<6;++i){
  if(!events[i])continue;
  const int bits=masks[i]&31;
  result.duck[i]=(bits&8)==0;
  result.reverb=result.reverb||((bits&1)!=0);
  result.distort=result.distort||((bits&2)!=0);
  result.delay=result.delay||((bits&16)!=0);
  result.reverse=result.reverse||((bits&5)==5);
 }
 return result;
}
} // namespace di
