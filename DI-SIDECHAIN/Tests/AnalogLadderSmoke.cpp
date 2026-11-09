#include "../Source/AnalogLadder.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
static void check(bool b,const char* m){if(!b){std::cerr<<m<<"\n";std::exit(1);}}
int main(){
 di::AnalogLadder ladder;ladder.prepare(48000.);
 ladder.setCutoff(1200.f,.55f);
 for(int i=0;i<48000;++i){
  const float in=std::sin(i*.21f)*.8f;
  const float out=ladder.process(in);
  check(std::isfinite(out)&&std::abs(out)<4.f,"stable ladder even with resonance");
 }
 ladder.reset();ladder.setCutoff(50.f,.85f);
 for(int i=0;i<48000;++i)check(std::isfinite(ladder.process(i==0?1.f:0.f)),"low cutoff stability");
 return 0;
}
