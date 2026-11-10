#include "../Source/DuckVoiceEngine.h"
#include "../Source/CellFxRouting.h"
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
static void check(bool ok,const char* m){if(!ok){std::cerr<<m<<"\n";std::exit(1);}}
int main(){
 di::DuckVoiceEngine engine;
 di::DuckVoiceEngine::TrackSettings down,up;
 down.depth=1.f;down.duration=1.f;down.useDynamic=true;
 up=down;
 for(int i=0;i<down.dynamicCurve.count;++i){
  down.dynamicCurve.y[(size_t)i]=0.f;
  up.dynamicCurve.y[(size_t)i]=1.f;
 }
 engine.setStepTrack(0,0,down);
 engine.setStepTrack(0,1,up);
 engine.setStepTrack(1,0,up);
 std::array<bool,6> events{};events[0]=true;
 // The same TRACK has different envelopes in STEP 1 and STEP 2.
 engine.reset();
 check(engine.process(0,events,8.,100)<.001f,"track 1 / step 1 fully ducks");
 engine.reset();
 check(engine.process(1,events,8.,101)>.999f,"track 1 / step 2 plays different shape");
 // The same STEP may have a different curve on another TRACK.
 engine.reset();events[0]=false;events[1]=true;
 check(engine.process(0,events,8.,102)>.999f,"track 2 / step 1 has independent envelope");
 // Existing note's envelope stays snapshotted when the selected cell changes.
 engine.reset();events[0]=true;events[1]=false;
 check(engine.process(0,events,16.,103)<.001f,"voice starts with initial step envelope");
 engine.setStepTrack(0,0,up);
 events.fill(false);
 check(engine.process(1,events,16.,103)<.001f,"running voice does not morph after GUI edit");
 for(int i=0;i<20;++i)engine.process(1,events,16.,103);
 check(engine.process(1,events,16.,103)>.999f,"voice naturally finishes");
 // FX bypass is orthogonal to the envelope and works for individual cells.
 std::array<int,6> masks{};events[0]=true;masks[0]=8|1|16;
 const auto r=di::routeCellEffects(events,masks,true);
 check(!r.duck[0]&&r.reverb&&r.delay,"per-cell duck BYPASS keeps FX");
 return 0;
}
