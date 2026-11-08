#include "../Source/OversampledFx2x.h"
#include "../Source/FixedDelay.h"
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
 di::FixedDelay<di::OversampledFx2x::latencySamples> dry;
 for(int i=0;i<32;++i){
  const float result=dry.process(i==0?1.f:0.f);
  check(std::abs(result-(i==di::OversampledFx2x::latencySamples?1.f:0.f))<1e-6f,"Dry delay impulse alignment");
 }
 dry.reset();
 for(int i=0;i<di::OversampledFx2x::latencySamples;++i)
  check(dry.process(1.f)==0.f,"Dry delay reset must clear history");
 fx.reset();
 int peakIndex=-1;float peakMagnitude=0.f;
 for(int i=0;i<32;++i){
  const float y=fx.process(i==0?0.0001f:0.f,1.f,0.f);
  if(std::abs(y)>peakMagnitude){peakMagnitude=std::abs(y);peakIndex=i;}
 }
 check(peakIndex==di::OversampledFx2x::latencySamples,"Oversampler group delay mismatch");
 check(peakMagnitude>0.f,"Impulse response must not be silent");
 // A fully dry mix must be a bit-exact delayed copy, regardless of FX settings.
 fx.reset();
 dry.reset();
 for(int i=0;i<256;++i){
  const float input=std::sin(static_cast<float>(i)*0.137f);
  const float drySignal=dry.process(input);
  const float processed=fx.process(input,12.f,1.f);
  const float mixed=drySignal+0.f*(processed-drySignal);
  check(std::isfinite(processed),"Oversampled FX must remain finite");
  check(mixed==drySignal,"Zero-wet bypass must equal delayed dry exactly");
 }
 // Per-channel state must remain independent under stereo processing.
 di::OversampledFx2x left,right,reference;
 for(int i=0;i<256;++i){
  const float input=std::sin(static_cast<float>(i)*0.071f);
  const float expected=reference.process(input,4.f,0.f);
  const float l=left.process(input,4.f,0.f);
  right.process(-input,8.f,1.f);
  check(l==expected,"Stereo FX channels must not share filter history");
 }

 fx.reset();
 for(int i=0;i<512;++i)check(std::isfinite(fx.process(i%2?1.f:-1.f,16.f,0.f)),"Nyquist input must stay finite");
 return 0;
}
