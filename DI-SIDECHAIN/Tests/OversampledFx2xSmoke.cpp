#include "../Source/OversampledFx2x.h"
#include "../Source/FixedDelay.h"
#include "../Source/FxWetMix.h"
#include <limits>
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
  const float mixed=di::mixAlignedFx(drySignal,processed,0.f);
  check(std::isfinite(processed),"Oversampled FX must remain finite");
  check(mixed==drySignal,"Zero-wet bypass must equal delayed dry exactly");
 }
 check(di::mixAlignedFx(0.25f,0.75f,0.f)==0.25f,"Bypass preserves dry");
 check(di::mixAlignedFx(0.25f,0.75f,1.f)==0.75f,"Full wet preserves processed");
 check(di::mixAlignedFx(0.25f,0.75f,0.5f)==0.5f,"Half wet interpolates");
 check(di::mixAlignedFx(0.25f,std::numeric_limits<float>::quiet_NaN(),1.f)==0.25f,"Nonfinite wet must not leak");
 check(di::mixAlignedFx(0.25f,0.75f,std::numeric_limits<float>::quiet_NaN())==0.25f,"Nonfinite mix must bypass");
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
 // Abrupt automation and nonfinite inputs must not poison FIR state.
 fx.reset();
 for(int i=0;i<4096;++i){
  const float input=(i%127==0)?std::numeric_limits<float>::quiet_NaN()
                   :0.8f*std::sin(static_cast<float>(i)*0.19f);
  const float drive=(i%113==0)?std::numeric_limits<float>::infinity()
                    :(i%2==0?1.f:16.f);
  const float type=(i%3==0)?0.f:1.f;
  check(std::isfinite(fx.process(input,drive,type)),
        "FX automation must not poison oversampler state");
 }
 // Quantitative aliasing regression: at 48 kHz, a 10 kHz tone's 30 kHz
 // third harmonic folds to 18 kHz without adequate oversampling filtering.
 fx.reset();
 constexpr int count=8192;
 constexpr double sampleRate=48000., fundamental=10000., alias=18000.;
 constexpr double pi=3.14159265358979323846;
 double plainReal=0.,plainImag=0.,oversampledReal=0.,oversampledImag=0.;
 for(int i=0;i<count;++i){
  const float input=0.8f*std::sin(2.*pi*fundamental*i/sampleRate);
  const float plain=di::processFx(input,8.f,1.f);
  const float oversampled=fx.process(input,8.f,1.f);
  if(i<256)continue; // exclude FIR startup transient
  const double angle=2.*pi*alias*i/sampleRate;
  const double c=std::cos(angle),sn=std::sin(angle);
  plainReal+=plain*c;plainImag+=plain*sn;
  oversampledReal+=oversampled*c;oversampledImag+=oversampled*sn;
 }
 const double plainAlias=std::hypot(plainReal,plainImag);
 const double oversampledAlias=std::hypot(oversampledReal,oversampledImag);
 check(plainAlias>1.,"Aliasing test tone must produce measurable harmonics");
 check(oversampledAlias<plainAlias*0.4,"2x FIR must attenuate 18kHz folded alias by at least 8dB");
 return 0;
}
