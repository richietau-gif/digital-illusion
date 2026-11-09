#include "../Source/DuckVoiceEngine.h"
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>

static void check(bool ok,const char* error){if(!ok){std::cerr<<error<<"\n";std::exit(1);}}
int main(){
 di::DuckVoiceEngine engine;
 std::array<bool,6> cells{};
 engine.reset();
 check(engine.process(0,cells,100.)==1.f,"empty grid must pass through");
 di::DuckVoiceEngine::TrackSettings kick;
 kick.depth=1.f;kick.duration=1.f;kick.shape.fill(0.f);
 engine.setTrack(0,kick);
 cells[0]=true;
 check(engine.process(1,cells,100.)==0.f,"kick trigger must duck");
 for(int i=0;i<100;++i)engine.process(1,cells,100.);
 check(engine.process(1,cells,100.)==1.f,"held step must not retrigger or latch");
 check(engine.process(2,cells,100.)==0.f,"new step must retrigger");
 engine.reset();
 di::DuckVoiceEngine::TrackSettings ghost;
 ghost.depth=.3f;ghost.duration=.5f;ghost.shape.fill(0.f);
 engine.setTrack(1,ghost);
 cells[0]=true;cells[1]=true;
 check(engine.process(0,cells,100.)==0.f,"strongest overlapping duck wins");
 engine.reset();
 cells[0]=false;cells[1]=true;
 check(std::abs(engine.process(0,cells,100.)-.7f)<1e-5f,"ghost independent depth");
 for(int i=0;i<50;++i)engine.process(0,cells,100.);
 check(engine.process(0,cells,100.)==1.f,"ghost independent length");
 // MIDI note can trigger mid-step without restarting the sequencer clock.
 engine.reset();
 engine.setTrack(0,di::DuckVoiceEngine::TrackSettings{});
 auto live=engine.getTrack(0);
 live.shape.fill(0.f);live.depth=1.f;live.duration=.2f;
 engine.setTrack(0,live);
 cells.fill(false);
 check(engine.process(0,cells,100.)==1.f,"no event before MIDI note");
 engine.triggerTrack(0,100.,.4f);
 check(std::abs(engine.process(0,cells,100.)-.6f)<1e-5f,"velocity-scaled MIDI duck");
 for(int n=0;n<20;++n)engine.process(0,cells,100.);
 check(engine.process(0,cells,100.)==1.f,"MIDI duck terminates in held step");
 kick.duration=std::nanf("");kick.depth=-3.f;
 engine.setTrack(0,kick);
 check(engine.getTrack(0).depth==0.f&&engine.getTrack(0).duration==1.f,"sanitize invalid settings");
 check(std::abs(di::DuckVoiceEngine::interpolate({0.f,0.f,0.f,0.f,1.f,1.f,1.f,1.f},.5f)-.5f)<1e-5f,"shape interpolation");
 return 0;
}
