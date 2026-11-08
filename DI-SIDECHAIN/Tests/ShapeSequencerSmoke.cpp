#include "../Source/ShapeSequencer.h"
#include <cstdlib>
#include <iostream>
static void check(bool condition, const char* message) { if (!condition) { std::cerr << message << '\n'; std::exit(1); } }
#include <cmath>
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
 return 0;
}
