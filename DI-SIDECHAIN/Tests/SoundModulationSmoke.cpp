#include "../Source/SoundModulation.h"
#include "../Source/DuckGain.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
static void check(bool x,const char* message){
 if(!x){std::cerr<<"MOD AUDIO: "<<message<<"\n";std::exit(1);}
}
int main(){
 const auto neutral=di::panModulation(.5f,1.f);
 check(std::abs(neutral.left-1.f)<1e-6f&&std::abs(neutral.right-1.f)<1e-6f,
       "Pan center preserves stereo");
 const auto left=di::panModulation(0.f,1.f);
 const auto right=di::panModulation(1.f,1.f);
 check(left.left>.999f&&left.right<.0001f,"left lane attenuates right channel");
 check(right.left<.0001f&&right.right>.999f,"right lane attenuates left channel");
 const auto bypass=di::panModulation(0.f,0.f);
 check(bypass.left==1.f&&bypass.right==1.f,"zero pan modulation amount has no audio effect");
 check(di::filterModulationBlend(1.f,1.f)==1.f,"filter lane high point retains full duck");
 check(di::filterModulationBlend(0.f,1.f)==0.f,"filter lane low point removes affected duck");
 check(di::filterModulationBlend(0.f,0.f)==1.f,"zero filter modulation is neutral");
 const float dry=.32f;
 const float attenuated=di::applyDuckGain(dry,.15f,1.f);
 const float filtered=dry+(attenuated-dry)*di::filterModulationBlend(0.f,1.f);
 check(std::abs(filtered-dry)<1e-6f,"Filter lane cancels selective duck correctly");
 const float panL=attenuated*left.left,panR=attenuated*left.right;
 check(panL>.001f&&std::abs(panR)<.0001f,"Pan lane changes actual stereo audio samples");
 return 0;
}
