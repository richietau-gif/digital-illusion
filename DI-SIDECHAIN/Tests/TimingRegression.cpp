#include "../Source/PrecisionTiming.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

struct Timeline {
 int latency;
 int length;
 int offsetSamples(float pct) const {
  return static_cast<int>(std::lround(pct*length));
 }
 int outputForInput(int input) const {return input+latency;}
};

int main(){
 constexpr float eps=0.00001f;
 const auto near=[](float a,float b){return std::abs(a-b)<eps;};
 Timeline t{960,4800};
 for(float p: {-1.f,-.5f,0.f,.5f,1.f}){
  assert(std::abs(t.offsetSamples(p)-static_cast<int>(p*4800))<=1);
  assert(near(precision_timing::shiftedPhase(.5f,p),std::clamp(.5f+p,0.f,1.f)));
 }
 assert(t.outputForInput(1000)==1960);
 assert(near(precision_timing::shiftedPhase(.4f,0.f),.4f));
 assert(near(precision_timing::shiftedPhase(.4f,-.25f),.15f));
 assert(near(precision_timing::shiftedPhase(.4f,.25f),.65f));
 assert(near(precision_timing::endPhase(0.f),1.f));
 assert(near(precision_timing::endPhase(-1.f),2.f));
 assert(near(precision_timing::endPhase(-.3f),1.3f));
 assert(!precision_timing::active(.5f,-1.f,false)); // No trigger -> no duck.
 assert(precision_timing::active(1.25f,-.5f,true)); // Delayed curve must finish.
 assert(!precision_timing::active(1.5f,-.5f,true));
 assert(!precision_timing::active(1.f,0.f,true));
 assert(near(precision_timing::mixDelayed(.8f,.2f,0.f),.8f));
 assert(near(precision_timing::mixDelayed(.8f,.2f,1.f),.16f));
 assert(near(precision_timing::mixDelayed(.8f,.2f,.5f),.48f));
 assert(near(precision_timing::mixDelayed(.8f,.2f,.5f),precision_timing::mixDelayed(.8f,.2f,.5f)));
 const auto nan=std::numeric_limits<float>::quiet_NaN();
 assert(near(precision_timing::finiteBipolar(nan),0.f));
 assert(near(precision_timing::shiftedPhase(.5f,nan),.5f));
 for(int i=-100;i<=100;i++)
  assert(std::isfinite(precision_timing::shiftedPhase(.5f,(float)i/100.f)));
 std::cout<<"precision timing, offset, idle trigger and aligned dry/wet tests passed\n";
}
