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
 // Absolute generation must advance even when the visible step number is fixed.
 di::ShapeSequencer oneStep;
 oneStep.setSteps(1);oneStep.reset();
 check(oneStep.getStepGeneration()==0,"generation reset");
 for(int i=0;i<4;++i){
  oneStep.render(1.);
  check(oneStep.getCurrentStep()==0,"one-step modulo remains zero");
  check(oneStep.getStepGeneration()==static_cast<std::uint64_t>(i+1),"generation advances each cycle");
 }
 oneStep.seekToStepPosition(64.75);
 check(oneStep.getStepGeneration()==64,"generation anchors to PPQ");
 oneStep.render(4.);
 check(oneStep.getStepGeneration()==65,"generation advances after seek");
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
 // Host or automation corruption must not produce an invalid array index.
 di::ShapeSequencer::Shape finiteShape{};
 finiteShape.fill(0.4f);
 for(double position:{std::numeric_limits<double>::quiet_NaN(),
                      std::numeric_limits<double>::infinity(),
                      -std::numeric_limits<double>::infinity()}){
  const float output=di::ShapeSequencer::evaluate(finiteShape,position);
  check(std::isfinite(output)&&std::abs(output-0.4f)<1e-6f,
        "Nonfinite phase must produce a safe finite envelope");
 }
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
 // Musical position must dominate buffer duration: a tempo change changes
 // samples per step, not the song-position step chosen at the next block.
 di::ShapeSequencer tempoTimeline;
 tempoTimeline.setSteps(8);
 tempoTimeline.seekToStepPosition(3.25);
 check(tempoTimeline.getCurrentStep()==3,"Tempo sync initial musical step");
 for(int i=0;i<128;++i)tempoTimeline.render(24000.);
 tempoTimeline.seekToStepPosition(3.25+128.0/12000.0);
 check(tempoTimeline.getCurrentStep()==3,"Tempo change must preserve PPQ step");
 tempoTimeline.seekToStepPosition(1.5);
 check(tempoTimeline.getCurrentStep()==1,"Host loop rewind must seek to PPQ step");
 tempoTimeline.seekToStepPosition(8.0);
 check(tempoTimeline.getCurrentStep()==0,"Exact sequence loop boundary");
 tempoTimeline.seekToStepPosition(-8.5);
 check(tempoTimeline.getCurrentStep()==7,"Negative PPQ loop wrap");
 tempoTimeline.seekToStepPosition(1e12+2.75);
 check(tempoTimeline.getCurrentStep()==2,"Large PPQ timeline must wrap");
 // Regression: seek must select the correct step and output after DAW loop jumps.
 // Flat per-step envelopes make the expected sample independent of phase.
 di::ShapeSequencer seekCheck;
 seekCheck.setSteps(16);
 for(int k=0;k<16;++k){
  di::ShapeSequencer::Shape flatStep{};
  flatStep.fill(static_cast<float>(k)/15.f);
  seekCheck.setShape(k,flatStep);
 }
 for(double pos:{0.,0.5,1.,7.5,15.75,16.,17.25,-0.25,-16.5,1e12+2.75}){
  const double wrapped=std::fmod(std::fmod(pos,16.)+16.,16.);
  const int expectedStep=static_cast<int>(wrapped);
  seekCheck.seekToStepPosition(pos);
  check(seekCheck.getCurrentStep()==expectedStep,"DAW PPQ seek selected wrong step");
  check(std::abs(seekCheck.render(128.)-static_cast<float>(expectedStep)/15.f)<1e-6f,
        "DAW PPQ seek rendered wrong step envelope");
 }
 check(di::nextTransportSample(1000,128)==1128,"Transport block advance incorrect");
 check(di::nextTransportSample(-1,128)==-1,"Unknown transport position must stay unknown");
 check(di::nextTransportSample(1000,-1)==-1,"Negative block size must be rejected");
 check(di::nextTransportSample(std::numeric_limits<std::int64_t>::max()-128,128)
       ==std::numeric_limits<std::int64_t>::max(),"Transport limit boundary incorrect");
 check(di::nextTransportSample(std::numeric_limits<std::int64_t>::max()-127,128)
       ==-1,"Transport advance must reject signed overflow");
 check(!di::transportDiscontinuity(-1,0),"Unknown transport sample must not reset");
 check(!di::transportDiscontinuity(1024,1024),"Contiguous transport must not reset");
 check(!di::transportDiscontinuity(1024,1026),"Two-sample host tolerance");
 check(di::transportDiscontinuity(1024,1027),"Small seek must reset");
 check(di::transportDiscontinuity(1024,1000),"Short loop jump must reset");
 check(di::transportDiscontinuity(1024,0),"Rewind must reset");
 check(di::transportDiscontinuity(std::numeric_limits<std::int64_t>::max(),
                                  std::numeric_limits<std::int64_t>::min()),
       "Extreme host transport positions must not overflow");
 check(!di::transportDiscontinuity(std::numeric_limits<std::int64_t>::max()-1,
                                   std::numeric_limits<std::int64_t>::max()),
       "Near-limit transport advance must remain contiguous");

 return 0;
}
