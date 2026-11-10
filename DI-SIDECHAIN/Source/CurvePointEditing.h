#pragma once
#include "DynamicCurve.h"
#include <cmath>
#include <algorithm>
namespace di {
// Hit-testing is performed in unscaled design coordinates. A radius measured
// in PHYSICAL screen pixels is converted to design space via GUI scale.
struct CurveHit {
 static int nearest(const DynamicCurve& curve,float localX,float localY,
                    float width,float height,float uiScale,
                    bool deleting=false) noexcept {
  if(width<=0.f||height<=0.f||!std::isfinite(localX)||!std::isfinite(localY))
   return -1;
  const float scale=std::max(.25f,std::isfinite(uiScale)?uiScale:1.f);
  const float radius=(deleting?23.f:19.f)/scale;
  const float radiusSq=radius*radius;
  const int n=std::clamp(curve.count,2,DynamicCurve::capacity);
  int best=-1;float bestSq=radiusSq;
  // Pick the nearest visible handle, INCLUDING endpoints. If the nearest
  // handle is an endpoint, deletion is rejected rather than accidentally
  // deleting its adjacent interior point.
  for(int i=0;i<n;++i){
   const float dx=localX-curve.x[(size_t)i]*width;
   const float dy=localY-(1.f-curve.y[(size_t)i])*height;
   const float sq=dx*dx+dy*dy;
   if(sq<=bestSq){bestSq=sq;best=i;}
  }
  if(best==-1&&deleting){
   // At 50/75% nodes are difficult to hit vertically. Allow a narrower
   // horizontal handle band without selecting unrelated distant points.
   const float xTolerance=15.f/scale,yTolerance=34.f/scale;
   float bestX=xTolerance;
   for(int i=0;i<n;++i){
    const float dx=std::abs(localX-curve.x[(size_t)i]*width);
    const float dy=std::abs(localY-(1.f-curve.y[(size_t)i])*height);
    if(dx<=bestX&&dy<=yTolerance){bestX=dx;best=i;}
   }
  }
  if(deleting&&(best<=0||best>=n-1))return -1;
  return best;
 }
};
} // namespace di
