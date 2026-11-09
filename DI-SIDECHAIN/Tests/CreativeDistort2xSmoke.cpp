#include "../Source/CreativeDistort2x.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
static void check(bool ok,const char* message){if(!ok){std::cerr<<message<<"\n";std::exit(1);}}
int main() {
 di::CreativeDistort2x fx;
 for(int model=0;model<5;++model){
  fx.reset();
  float max=0.f;
  for(int i=0;i<8192;++i){
   const float sample=(i%2==0?.85f:-.85f);
   const float output=fx.process(sample,model,25.f);
   check(std::isfinite(output),"nonfinite oversampled output");
   max=std::max(max,std::abs(output));
  }
  check(max>.05f&&max<2.f,"unexpected oversampled output range");
 }
 fx.reset();
 for(int i=0;i<64;++i){
  const float out=fx.process(0.f,0,1.f);
  check(std::isfinite(out)&&std::abs(out)<1e-6f,"zero-input stability");
 }
 check(di::CreativeDistort2x::latencySamples==7,"dry alignment latency contract");
 return 0;
}
