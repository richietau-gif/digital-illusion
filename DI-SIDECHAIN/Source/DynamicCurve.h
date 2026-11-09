#pragma once
#include <array>
#include <algorithm>
#include <cmath>

namespace di {
// Editor and DSP share the same variable-point curve. Endpoints are anchored
// to 0 and 1; interior points are freely editable in time and amplitude.
struct DynamicCurve {
 static constexpr int capacity=16;
 int count=8;
 std::array<float,capacity> x{},y{};
 DynamicCurve() noexcept {
  constexpr float defaults[8]={0.f,.02f,.2f,.65f,.9f,1.f,1.f,1.f};
  for(int i=0;i<8;++i){x[(size_t)i]=i/7.f;y[(size_t)i]=defaults[i];}
 }
 float evaluate(float t) const noexcept {
  t=std::clamp(std::isfinite(t)?t:0.f,0.f,1.f);
  const int n=std::clamp(count,2,capacity);
  for(int i=1;i<n;++i)if(t<=x[(size_t)i]||i==n-1){
   const float start=x[(size_t)(i-1)],end=x[(size_t)i];
   float q=std::clamp((t-start)/std::max(.00001f,end-start),0.f,1.f);
   q=q*q*(3.f-2.f*q);
   return y[(size_t)(i-1)]+(y[(size_t)i]-y[(size_t)(i-1)])*q;
  }
  return y[(size_t)(n-1)];
 }
 bool insert(float time,float level) noexcept {
  if(count>=capacity||!std::isfinite(time)||!std::isfinite(level))return false;
  time=std::clamp(time,.015f,.985f);
  int at=1;
  while(at<count-1&&x[(size_t)at]<time)++at;
  if(time-x[(size_t)(at-1)]<.015f||x[(size_t)at]-time<.015f)return false;
  for(int i=count;i>at;--i){x[(size_t)i]=x[(size_t)(i-1)];y[(size_t)i]=y[(size_t)(i-1)];}
  x[(size_t)at]=time;y[(size_t)at]=std::clamp(level,0.f,1.f);++count;return true;
 }
 bool erase(int index) noexcept {
  if(index<=0||index>=count-1||count<=2)return false;
  for(int i=index;i<count-1;++i){x[(size_t)i]=x[(size_t)(i+1)];y[(size_t)i]=y[(size_t)(i+1)];}
  --count;return true;
 }
 void move(int index,float time,float level) noexcept {
  if(index<0||index>=count||!std::isfinite(time)||!std::isfinite(level))return;
  y[(size_t)index]=std::clamp(level,0.f,1.f);
  if(index>0&&index<count-1)
   x[(size_t)index]=std::clamp(time,x[(size_t)(index-1)]+.015f,x[(size_t)(index+1)]-.015f);
 }
};
} // namespace di
