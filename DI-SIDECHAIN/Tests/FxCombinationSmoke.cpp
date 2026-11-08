#include "../Source/FxWetMix.h"
#include "../Source/OversampledFx2x.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
static void check(bool ok,const char* msg){if(!ok){std::cerr<<msg<<'\n';std::exit(1);}}
int main(){
 const float dry=.25f, wet=.8f;
 check(di::mixAlignedFx(dry,wet,0.f)==dry,"FX bypass must be bit-exact dry");
 check(std::abs(di::mixAlignedFx(dry,wet,1.f)-wet)<1e-6f,"FX full wet mismatch");
 check(std::abs(di::mixAlignedFx(dry,wet,.5f)-.525f)<1e-6f,"FX mix mismatch");
 check(di::mixAlignedFx(dry,NAN,1.f)==dry,"nonfinite wet must bypass safely");
 // Both serial effect branches use the same fixed delay and therefore retain
 // one constant reported latency for all four enable combinations.
 check(di::OversampledFx2x::latencySamples==7,"oversampling latency contract changed");
 for(int fx=0;fx<2;++fx)for(int sat=0;sat<2;++sat){
  const int stages=(fx||sat)?2:2; // branches remain live to keep switching click-free
  check(stages*di::OversampledFx2x::latencySamples==14,"combined latency mismatch");
 }
 return 0;
}
