#pragma once
#include <algorithm>
#include <cmath>
namespace di {
inline float softClip(float x) noexcept {
 if (!std::isfinite(x)) return 0.f;
 if (x <= -1.5f) return -1.f;
 if (x >= 1.5f) return 1.f;
 return x - (4.f / 27.f) * x * x * x;
}
inline float hardClip(float x) noexcept {
 if (!std::isfinite(x)) return 0.f;
 return std::clamp(x,-1.f,1.f);
}
inline float saturate(float x, float drive) noexcept {
 if (!std::isfinite(x) || !std::isfinite(drive)) return 0.f;
 drive = std::max(1.f,drive);
 return std::tanh(x * drive) / std::tanh(drive);
}
inline float processFx(float input, float drive, float type) noexcept {
 if (!std::isfinite(input)) return 0.f;
 drive = std::isfinite(drive) ? std::max(1.f,drive) : 1.f;
 type = std::isfinite(type) ? std::clamp(type,0.f,2.f) : 0.f;
 const float driven=input*drive;
 const float a=saturate(input,drive);
 const float b=softClip(driven);
 if(type>=1.5f)return hardClip(driven);
 return a+(b-a)*type;
}
}
