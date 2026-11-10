#pragma once
#include <array>
#include <algorithm>
namespace di {
// Single source of truth for the shape-preset thumbnail and actual DSP data.
inline constexpr std::array<std::array<float,8>,9> curvePresets{{
 {{1.f,.82f,.4f,.05f,.13f,.42f,.79f,1.f}},
 {{0.f,.04f,.14f,.32f,.68f,.92f,1.f,1.f}},
 {{1.f,1.f,.98f,.9f,.7f,.42f,.15f,0.f}},
 {{0.f,0.f,0.f,0.f,1.f,1.f,1.f,1.f}},
 {{1.f,.8f,.4f,.1f,.1f,.4f,.8f,1.f}},
 {{0.f,.14f,.28f,.43f,.57f,.72f,.86f,1.f}},
 {{1.f,.75f,.25f,.05f,.05f,.25f,.75f,1.f}},
 {{1.f,.2f,.8f,.15f,.85f,.2f,.8f,1.f}},
 {{0.f,.1f,.35f,.7f,.9f,.9f,.98f,1.f}}
}};
inline float sampleCurvePreset(int index,float t) noexcept {
 index=std::clamp(index,0,8);
 const auto& p=curvePresets[(size_t)index];
 const float f=std::clamp(t,0.f,1.f)*7.f;
 const int i=std::min(6,static_cast<int>(f));
 float u=f-i;u=u*u*(3.f-2.f*u);
 return p[(size_t)i]+(p[(size_t)(i+1)]-p[(size_t)i])*u;
}
} // namespace di
