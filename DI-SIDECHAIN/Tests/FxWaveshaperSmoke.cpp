#include "../Source/FxWaveshaper.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
static void check(bool ok,const char* msg){if(!ok){std::cerr<<msg<<'\n';std::exit(1);}}
int main(){
 for(float x=-2.f;x<=2.f;x+=0.005f){
  check(std::isfinite(di::softClip(x)),"soft clip finite");
  check(di::softClip(x)<=1.f&&di::softClip(x)>=-1.f,"soft clip bounded");
  check(std::abs(di::softClip(x)+di::softClip(-x))<1e-5f,"soft clip odd symmetry");
  for(float type: {0.f,0.25f,0.5f,0.75f,1.f}){
   const float v=di::processFx(x,4.f,type);
   check(std::isfinite(v),"fx output finite");
   check(std::abs(v+di::processFx(-x,4.f,type))<1e-5f,"fx odd symmetry");
  }
 }
 check(std::abs(di::processFx(0.f,4.f,0.5f))<1e-6f,"zero input silence");
 // A zero-dB Drive setting must not disable the selected waveshaper.
 check(std::abs(di::processFx(0.8f,1.f,1.f)-0.8f)>0.01f,"Soft Clipper must process at 0 dB Drive");
 check(std::abs(di::processFx(0.8f,1.f,0.f)-0.8f)>0.01f,"Saturator must process at 0 dB Drive");

 check(std::abs(di::processFx(0.7f,4.f,0.f)-di::saturate(0.7f,4.f))<1e-6f,"saturator endpoint");
 check(std::abs(di::processFx(0.7f,4.f,1.f)-di::softClip(2.8f))<1e-6f,"clipper endpoint");
 check(di::processFx(std::numeric_limits<float>::quiet_NaN(),4.f,0.f)==0.f,"NaN input sanitized");
 check(std::isfinite(di::processFx(0.5f,std::numeric_limits<float>::infinity(),0.5f)),"infinite drive sanitized");
 return 0;
}
