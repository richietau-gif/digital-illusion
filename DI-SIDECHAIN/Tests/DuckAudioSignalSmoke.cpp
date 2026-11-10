#include "../Source/DuckGain.h"
#include "../Source/DuckVoiceEngine.h"
#include "../Source/CellFxRouting.h"
#include "../Source/ShapeSequencer.h"
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>

static void check(bool ok,const char* msg){
 if(!ok){std::cerr<<"DUCK AUDIO: "<<msg<<"\n";std::exit(1);}
}
// Render the EXACT duck voice, gain target and full-band mixer used by the
// processor, against constant-amplitude audio at 48kHz.
static float renderAudio(int mask,float depth,float wetMix) {
 di::DuckVoiceEngine engine;
 di::DuckVoiceEngine::TrackSettings settings;
 settings.depth=1.f;
 settings.duration=1.f;
 settings.useDynamic=true;
 for(int i=0;i<settings.dynamicCurve.count;++i)
  settings.dynamicCurve.y[(size_t)i]=0.f;
 engine.setTrack(0,settings);
 engine.reset();
 std::array<bool,6> cells{};
 std::array<int,6> flags{};
 cells[0]=true;flags[0]=mask;
 const auto route=di::routeCellEffects(cells,flags,true);
 const float input=.25f;
 double squared=0.;
 float gain=1.f;
 constexpr int samples=480;
 const float coeff=std::exp(-1.f/(.002f*48000.f));
 for(int sample=0;sample<samples;++sample){
  const float envelope=engine.process(0,route.duck,960.f,0);
  const float target=di::duckGainTarget(envelope,depth);
  gain=coeff*gain+(1.f-coeff)*target;
  const float output=di::applyDuckGain(input,gain,wetMix);
  check(std::isfinite(output),"sample must be finite");
  squared+=output*output;
 }
 return std::sqrt((float)(squared/samples));
}
int main(){
 const float dry=renderAudio(8,1.f,1.f);    // bypass only
 const float fullyWet=renderAudio(0,1.f,1.f);
 const float halfDepth=renderAudio(0,.5f,1.f);
 const float halfMix=renderAudio(0,1.f,.5f);
 const float zeroDepth=renderAudio(0,0.f,1.f);
 const float zeroMix=renderAudio(0,1.f,0.f);
 const float bypassWithFx=renderAudio(8|1|2|16,1.f,1.f);
 check(std::abs(dry-.25f)<.00001f,"bypassed cell must pass original audio");
 check(fullyWet<dry*.28f,"active duck must audibly attenuate output");
 check(halfDepth>fullyWet*2.f&&halfDepth<dry,"depth parameter changes audio");
 check(halfMix>fullyWet&&halfMix<dry,"wet mix changes actual audio");
 check(std::abs(zeroDepth-dry)<.00001f,"zero depth must pass dry");
 check(std::abs(zeroMix-dry)<.00001f,"zero mix must pass dry");
 check(std::abs(bypassWithFx-dry)<.00001f,
       "per-cell duck bypass must not duck even if FX sends are enabled");
 check(di::duckGainTarget(0.f,1.f)==0.f,"full duck gain floor");
 check(di::duckGainTarget(1.f,1.f)==1.f,"full envelope release gain");
 check(di::duckGainTarget(0.f,.4f)>.59f,"global depth scaling");
 return 0;
}
