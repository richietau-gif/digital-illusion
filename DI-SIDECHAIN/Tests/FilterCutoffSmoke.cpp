#include "../Source/FilterCutoff.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
static void check(bool ok,const char* message) {
 if(!ok){std::cerr<<message<<'\n';std::exit(1);}
}
int main(){
 for(double sr:{8000.,22050.,44100.,48000.,96000.,192000.}){
  for(float low:{25.f,120.f,1500.f,18000.f}){
   for(float high:{60.f,400.f,18000.f}){
    const auto c=di::safeFilterCutoffs(sr,low,high);
    check(std::isfinite(c.low)&&std::isfinite(c.high),"Nonfinite cutoff");
    check(c.low>0.f&&c.high>=c.low,"Inverted or invalid band");
    check(c.high<=static_cast<float>(sr*.45)+.01f,"Cutoff exceeds safe Nyquist margin");
   }
  }
 }
 const float nan=std::numeric_limits<float>::quiet_NaN();
 const float inf=std::numeric_limits<float>::infinity();
 for(float value:{nan,inf,-inf}){
  const auto c=di::safeFilterCutoffs(48000.,value,value);
  check(std::isfinite(c.low)&&std::isfinite(c.high),"Invalid automation not sanitized");
 }
 const auto fallback=di::safeFilterCutoffs(std::numeric_limits<double>::quiet_NaN(),nan,inf);
 check(fallback.low==120.f&&fallback.high==400.f,"Fallback defaults changed");
 return 0;
}
