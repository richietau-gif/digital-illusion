#include "../Source/OversampledFx2x.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
static void check(bool ok,const char* msg){if(!ok){std::cerr<<msg<<'\n';std::exit(1);}}
int main(){
 di::OversampledFx2x fx;
 double sum=0.;
 for(int i=0;i<512;++i){float y=fx.process(0.f,2.f,0.f);check(std::isfinite(y),"silence must be finite");sum+=std::abs(y);}
 check(sum<1e-6,"silence must remain silent");
 fx.reset();
 float steady=0.f;
 for(int i=0;i<512;++i)steady=fx.process(0.3f,1.f,1.f);
 check(std::isfinite(steady),"DC output finite");
 check(std::abs(steady-di::processFx(0.3f,1.f,1.f))<0.02f,"DC gain must settle");
 // Small-signal impulse response verifies the FIR path's nominal delay.
 fx.reset();
 int peakIndex=-1;float peakMagnitude=0.f;
 for(int i=0;i<32;++i){
  const float y=fx.process(i==0?0.0001f:0.f,1.f,0.f);
  if(std::abs(y)>peakMagnitude){peakMagnitude=std::abs(y);peakIndex=i;}
 }
 check(peakIndex==di::OversampledFx2x::latencySamples,"Oversampler group delay mismatch");
 check(peakMagnitude>0.f,"Impulse response must not be silent");
 fx.reset();
 for(int i=0;i<512;++i)check(std::isfinite(fx.process(i%2?1.f:-1.f,16.f,0.f)),"Nyquist input must stay finite");
 return 0;
}
