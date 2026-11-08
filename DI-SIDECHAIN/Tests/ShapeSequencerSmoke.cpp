#include "../Source/ShapeSequencer.h"
#include <cassert>
#include <cmath>
int main() {
 di::ShapeSequencer seq;
 assert(seq.getSteps()==16);
 seq.setSteps(32);
 assert(seq.getSteps()==32);
 seq.setSteps(0);
 assert(seq.getSteps()==1);
 seq.setSteps(2);
 di::ShapeSequencer::Shape flat{};
 flat.fill(0.25f);
 seq.setShape(0,flat);
 assert(std::abs(seq.render(4.)-0.25f)<1e-6f);
 for(int i=0;i<3;++i) seq.render(4.);
 assert(seq.getCurrentStep()==1);
 seq.trigger();
 assert(seq.getCurrentStep()==0);
 seq.setShape(0,di::ShapeSequencer::Shape{1,1,1,1,1,1,1,1});
 assert(std::abs(seq.render(4.)-1.f)<1e-6f);
 seq.reset();
 assert(seq.getCurrentStep()==0);
 return 0;
}
