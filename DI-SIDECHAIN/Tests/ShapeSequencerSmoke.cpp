#include "../Source/ShapeSequencer.h"
#include "../Source/TransportContinuity.h"
#include <cstdlib>
#include <iostream>
static void check(bool condition, const char* message) { if (!condition) { std::cerr << message << '\n'; std::exit(1); } }
#include <cmath>
#include <limits>
int main() {
 di::ShapeSequencer seq;
 check(seq.getSteps()==16, "Sequencer check failed: seq.getSteps()==16");
 seq.setSteps(32);
 check(seq.getSteps()==32, "Sequencer check failed: seq.getSteps()==32");
 seq.setSteps(0);
 check(seq.getSteps()==1, "Sequencer check failed: seq.getSteps()==1");
 seq.setSteps(2);
 di::ShapeSequencer::Shape flat{};
 flat.fill(0.25f);
 seq.setShape(0,flat);
 check(std::abs(seq.render(4.)-0.25f)<1e-6f, "Sequencer check failed: std::abs(seq.render(4.)-0.25f)<1e-6f");
 for(int i=0;i<3;++i) seq.render(4.);
 check(seq.getCurrentStep()==1, "Sequencer check failed: seq.getCurrentStep()==1");
 seq.trigger();
 check(seq.getCurrentStep()==0, "Sequencer check failed: seq.getCurrentStep()==0");
 seq.setShape(0,di::ShapeSequencer::Shape{1,1,1,1,1,1,1,1});
 check(std::abs(seq.render(4.)-1.f)<1e-6f, "Sequencer check failed: std::abs(seq.render(4.)-1.f)<1e-6f");
 seq.reset();
 check(seq.getCurrentStep()==0, "Sequencer check failed: seq.getCurrentStep()==0");
 // Regression: all 32 steps must wrap cleanly, including after a transport restart.
 seq.setSteps(32);
 for(int k=0;k<32;++k){di::ShapeSequencer::Shape shape{};shape.fill((float)k/31.f);seq.setShape(k,shape);}
 seq.reset();
 for(int k=0;k<32;++k){
  check(seq.getCurrentStep()==k,"32-step order mismatch");
  check(std::abs(seq.render(1.)-(float)k/31.f)<1e-5f,"32-step output mismatch");
 }
 check(seq.getCurrentStep()==0,"32-step wrap failed");
 seq.render(1.);
 seq.trigger();
 check(seq.getCurrentStep()==0,"MIDI trigger must restart first step");
 check(std::abs(seq.render(1.))<1e-6f,"Trigger must restart first step value");
 // Regression: Pan modulation neutral midpoint must remain centered.
 di::ShapeSequencer pan;
 pan.setSteps(1);
 di::ShapeSequencer::Shape centered{};centered.fill(0.5f);
 pan.setShape(0,centered);
 for(int i=0;i<64;++i)check(std::abs(2.f*pan.render(8.)-1.f)<1e-6f,"Pan midpoint must be neutral");
 // Reject nonfinite automation values and clamp out-of-range envelope nodes.
 di::ShapeSequencer::Shape invalid{};invalid.fill(0.5f);
 invalid[0]=NAN;invalid[1]=-2.f;invalid[2]=3.f;
 pan.setShape(0,invalid);
 auto safe=pan.getShape(0);
 check(safe[0]==1.f&&safe[1]==0.f&&safe[2]==1.f,"Shape sanitization failed");
 check(!di::ShapeSequencer::shouldRestartOnTrigger(true),"Synced MIDI must not restart sequence");
 check(di::ShapeSequencer::shouldRestartOnTrigger(false),"Free-running MIDI should restart sequence");
 di::ShapeSequencer syncPattern;
 syncPattern.setSteps(4);
 syncPattern.render(1.);
 check(syncPattern.getCurrentStep()==1,"Synced sequence must advance");
 for(int note=0;note<12;++note){
  if(di::ShapeSequencer::shouldRestartOnTrigger(true))syncPattern.trigger();
  syncPattern.render(1.);
 }
 check(syncPattern.getCurrentStep()==1,"Dense triggers must not pin synced pattern to step zero");
 di::ShapeSequencer timeline;
 timeline.setSteps(16);
 timeline.seekToStepPosition(17.5);
 check(timeline.getCurrentStep()==1,"PPQ seek must wrap after 16 steps");
 timeline.seekToStepPosition(-0.25);
 check(timeline.getCurrentStep()==15,"Negative PPQ preroll must wrap");
 timeline.seekToStepPosition(4.0);
 check(timeline.getCurrentStep()==4,"Exact PPQ step boundary");
 timeline.seekToStepPosition(std::numeric_limits<double>::quiet_NaN());
 check(timeline.getCurrentStep()==4,"Invalid PPQ must not corrupt step");
 check(!di::transportDiscontinuity(-1,0),"Unknown transport sample must not reset");
 check(!di::transportDiscontinuity(1024,1024),"Contiguous transport must not reset");
 check(!di::transportDiscontinuity(1024,1026),"Two-sample host tolerance");
 check(di::transportDiscontinuity(1024,1027),"Small seek must reset");
 check(di::transportDiscontinuity(1024,1000),"Short loop jump must reset");
 check(di::transportDiscontinuity(1024,0),"Rewind must reset");
 return 0;
}
