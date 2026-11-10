#include "../Source/CreativeDelay.h"
#include <cstdlib>
#include <cmath>
#include <iostream>
static void check(bool b,const char* m){if(!b){std::cerr<<m<<"\n";std::exit(1);}}
int main(){
 di::CreativeDelay fx;fx.prepare(8000.);
 bool heardFirst=false,heardPingPong=false;
 for(int i=0;i<420;++i){
  float l=i==0?1.f:0.f,r=0.f;
  fx.process(l,r,i==0,.8f,.65f,.01,true);
  check(std::isfinite(l)&&std::isfinite(r),"finite delay");
  if(i>=79&&i<=81&&l>.01f)heardFirst=true;
  if(i>=159&&i<=162&&r>.001f)heardPingPong=true;
 }
 check(heardFirst,"delayed impulse at correct time");
 check(heardPingPong,"pingpong repeat crosses to right");
 // Actual Wet semantics: 100% wet suppresses dry after the short
 // transition; 0% wet passes through, even with delay enabled.
 di::CreativeDelay fullWet,dryOnly;
 fullWet.prepare(8000.);dryOnly.prepare(8000.);
 for(int i=0;i<480;++i){
  float fl=1.f,fr=1.f,dl=1.f,dr=1.f;
  fullWet.process(fl,fr,i<280,1.f,0.f,.3,false);
  dryOnly.process(dl,dr,true,0.f,0.f,.3,false);
  if(i==240){
   check(std::abs(fl)<.01f&&std::abs(fr)<.01f,
         "100 percent wet must suppress original dry audio");
   check(std::abs(dl-1.f)<1e-6f&&std::abs(dr-1.f)<1e-6f,
         "zero wet must retain dry audio");
  }
  if(i==470)check(std::abs(fl-1.f)<.02f,
                   "dry input returns after delay cell closes");
 }
 fx.reset();
 for(int i=0;i<100;++i){float l=0.f,r=0.f;fx.process(l,r,false,1.f,.95f,.25,false);
  check(l==0.f&&r==0.f,"reset silent");}
 return 0;
}
