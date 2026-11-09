#pragma once
#include <algorithm>

namespace di {
// Centralized identity of the shape actually audible in the SIDEFX GRID.
// 0..31: legacy volume sequence; 32..63: filter; 64..95: pan;
// 96..101: independent duck envelopes for six tracks.
constexpr int resolveCurveKey(bool sequencer,bool grid,int lane,int track,int step) noexcept {
 if(!sequencer)return 0;
 lane=std::clamp(lane,0,2);
 step=std::clamp(step,0,31);
 if(grid&&lane==0&&track>=0&&track<6)return 96+track;
 return step+lane*32;
}
} // namespace di
