#pragma once
#include <array>
#include <algorithm>
#include <cmath>
namespace precision {
struct EnvelopeEngine { static float interpolate(float a,float b,float t) noexcept { t=t<0?0:t>1?1:t; return a+(b-a)*t*t*(3.f-2.f*t); } };
struct TriggerDetector { float threshold=0.1f, hysteresis=0.65f, peak=0; bool armed=true; bool process(float x) noexcept { x=std::fabs(x); peak=std::max(x, peak*0.995f); if (peak<threshold*hysteresis) armed=true; if(armed&&peak>=threshold){armed=false;return true;} return false; } };
struct TimingEngine { static double divisionBeats(int i) noexcept { static constexpr double d[]={1.0/8,1.0/6,1.0/4,1.0/3,1.0/2,2.0/3,1,4}; return d[i<0?0:i>7?7:i]; } };
struct LookaheadBuffer { std::array<float,192000> data{}; size_t write=0; float push(float x,size_t delay) noexcept { data[write]=x; auto r=data[(write+data.size()-std::min(delay,data.size()-1))%data.size()]; write=(write+1)%data.size(); return r; } };
}
