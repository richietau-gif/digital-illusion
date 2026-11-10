#include "../Source/CreativeFx.h"
#include <cmath>
#include <cstdlib>
#include <iostream>

static void check(bool okay,const char* msg){
 if(!okay){std::cerr<<"SEQFX SOUND: "<<msg<<"\n";std::exit(1);}
}
int main(){
 // A/B processing confirms distortion actually changes output audio,
 // rather than only changing the button state or rendering a curve.
 di::CreativeFx dry,distorted;
 dry.prepare(48000.);distorted.prepare(48000.);
 double difference=0.,dryEnergy=0.;
 for(int i=0;i<12000;++i){
  const float sample=.3f*std::sin(6.28318530718f*440.f*i/48000.f);
  float a=sample,b=sample,c=sample,d=sample;
  dry.process(a,b,false,false,0,0.f,0.f,0,0.f,3.f,3.f,false);
  distorted.process(c,d,true,false,4,18.f,1.f,0,0.f,3.f,3.f,false);
  check(std::isfinite(a)&&std::isfinite(c),"distortion finite output");
  if(i>2000){difference+=(a-c)*(a-c);dryEnergy+=a*a;}
 }
 check(dryEnergy>1.0,"dry stereo audio present");
 check(difference>dryEnergy*.02,"distortion wet actually changes the waveform");

 // Reverb receives an impulse for the initial slice, then closes the
 // input while its internal delay lines must keep producing an audible tail.
 di::CreativeFx reverb;
 reverb.prepare(12000.);
 bool heardTail=false;
 for(int i=0;i<24000;++i){
  float l=i==0?.8f:0.f,r=l;
  reverb.process(l,r,false,i<128,0,0.f,0.f,2,1.f,5.f,4.f,false,0.f);
  check(std::isfinite(l)&&std::isfinite(r),"reverb finite");
  if(i>500&&i<20000&&(std::abs(l)>1e-6f||std::abs(r)>1e-6f))
   heardTail=true;
 }
 check(heardTail,"reverb must generate audio tail after input gate closes");

 // The complete creative chain must produce a synced-delay echo. The
 // first echo lands on the left; ping-pong feedback must reach the right.
 di::CreativeFx echo;
 echo.prepare(8000.);
 bool firstEcho=false,pong=false;
 for(int i=0;i<800;++i){
  float l=i==48?.7f:0.f,r=0.f;
  echo.process(l,r,false,false,0,0.f,0.f,0,0.f,3.f,3.f,false,
               0.f,1.f,i<150,1.f,.75f,.01,true);
  check(std::isfinite(l)&&std::isfinite(r),"delay finite");
  if(i>130&&i<147&&std::abs(l)>.0001f)firstEcho=true;
  if(i>210&&i<231&&std::abs(r)>.0001f)pong=true;
 }
 check(firstEcho,"delay produces delayed audio on first channel");
 check(pong,"ping-pong produces feedback on opposite channel");
 return 0;
}
