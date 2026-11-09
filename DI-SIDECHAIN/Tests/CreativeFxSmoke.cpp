#include "../Source/CreativeFx.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
static void check(bool ok,const char* message){if(!ok){std::cerr<<message<<"\n";std::exit(1);}}
int main(){
 di::ReverseSend reverse;
 reverse.prepare(8000.);
 const int count=reverse.windowSamples();
 check(count>=64,"reverse window");
 for(int i=0;i<count;++i){
  float l=(float)i/count,r=-l;
  reverse.process(l,r,false);
 }
 for(int i=0;i<count;++i){
  float l=0.f,r=0.f;
  reverse.process(l,r,true);
  const float gain=std::clamp(std::min(i,count-1-i)/(float)std::max(1,std::min(256,count/4)),0.f,1.f);
  const float expected=(float)(count-1-i)/count*gain;
  check(std::abs(l-expected)<1e-5f&&std::abs(r+expected)<1e-5f,"reversed previous buffer");
 }
 // Switching the reverse window must not overrun buffers or replay stale data.
 reverse.setWindowSamples(count*2);
 for(int i=0;i<count*4;++i){
  float l=i%7==0?.8f:0.f,r=-l;
  reverse.process(l,r,true);
  check(std::isfinite(l)&&std::isfinite(r),"reverse resize stability");
 }
 check(reverse.windowSamples()==count*2,"reverse window adopts new size");
 // Predelay should delay the wet-only impulse without changing its amplitude.
 di::StereoPredelay predelay;
 predelay.prepare(8000.);
 for(int i=0;i<200;++i){
  float l=i==0?1.f:0.f,r=i==0?-.5f:0.f;
  predelay.process(l,r,160);
  if(i<160)check(std::abs(l)<1e-6f&&std::abs(r)<1e-6f,"predelay must be silent before read time");
  if(i==160)check(std::abs(l-1.f)<1e-6f&&std::abs(r+.5f)<1e-6f,"predelay returns captured input");
 }
 // Tone control must actually change distortion spectral balance.
 di::CreativeFx dark,bright;
 dark.prepare(48000.);bright.prepare(48000.);
 double darkPower=0.,brightPower=0.;
 for(int i=0;i<6000;++i){
  const float input=.25f*std::sin(2.f*3.14159265358979323846f*8000.f*i/48000.f);
  float dl=input,dr=input,bl=input,br=input;
  dark.process(dl,dr,true,false,0,16.f,1.f,0,0.f,2.f,.5f,false,0.f,.0f);
  bright.process(bl,br,true,false,0,16.f,1.f,0,0.f,2.f,.5f,false,0.f,1.f);
  if(i>2500){darkPower+=dl*dl;brightPower+=bl*bl;}
 }
 check(brightPower>darkPower*2.,"distortion tone parameter changes harmonic level");
 di::CreativeFx fx;
 fx.prepare(44100.);
 for(int model=0;model<5;++model){
  for(int i=0;i<1024;++i){
   float l=i==0?1.f:0.f,r=l;
   fx.process(l,r,true,true,model,24.f,.5f,model,.4f,3.f,.6f,i%2==0);
   check(std::isfinite(l)&&std::isfinite(r),"creative effects finite");
  }
 }
 fx.reset();
 // The 2x distortion path adds seven samples of latency to BOTH wet and dry.
 for(int i=0;i<di::CreativeDistort2x::latencySamples+2;++i){
  float l=i==0?.1f:0.f,r=i==0?-.2f:0.f;
  fx.process(l,r,false,false,0,0.f,0.f,0,0.f,3.f,.6f,false);
  if(i<di::CreativeDistort2x::latencySamples)
   check(std::abs(l)<1e-6f&&std::abs(r)<1e-6f,"latency-compensated dry");
  if(i==di::CreativeDistort2x::latencySamples)
   check(std::abs(l-.1f)<1e-5f&&std::abs(r+.2f)<1e-5f,"aligned dry after seven samples");
 }
 return 0;
}
