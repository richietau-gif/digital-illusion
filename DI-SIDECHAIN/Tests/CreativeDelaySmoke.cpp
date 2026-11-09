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
 fx.reset();
 for(int i=0;i<100;++i){float l=0.f,r=0.f;fx.process(l,r,false,1.f,.95f,.25,false);
  check(l==0.f&&r==0.f,"reset silent");}
 return 0;
}
