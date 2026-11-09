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
  const float expected=(float)(count-1-i)/count;
  check(std::abs(l-expected)<1e-5f&&std::abs(r+expected)<1e-5f,"reversed previous buffer");
 }
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
 float l=.1f,r=-.2f;
 fx.process(l,r,false,false,0,0.f,0.f,0,0.f,3.f,.6f,false);
 check(std::abs(l-.1f)<1e-5f&&std::abs(r+.2f)<1e-5f,"bypass dry");
 return 0;
}
