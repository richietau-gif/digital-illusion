#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
struct Timeline { int latency; int length; int offsetSamples(float pct) const { return static_cast<int>(std::lround(pct * length)); } int outputForInput(int input) const { return input + latency; } };
int main(){ Timeline t{960,4800}; for(float p: {-1.f,-.5f,0.f,.5f,1.f}) assert(std::abs(t.offsetSamples(p)-static_cast<int>(p*4800))<=1); assert(t.outputForInput(1000)==1960); for(float x=-100.f;x<=100.f;x+=1.f) assert(std::isfinite(x)); std::cout<<"timing regression ok\n"; }
