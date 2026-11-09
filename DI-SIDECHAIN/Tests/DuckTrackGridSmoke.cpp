#include "../Source/DuckTrackGrid.h"
#include <cstdlib>
#include <cmath>
#include <iostream>
static void check(bool ok,const char* why){if(!ok){std::cerr<<why<<"\n";std::exit(1);}}
int main(){
 di::DuckTrackGrid grid;
 check(grid.stepCount()==16,"default step count");
 check(grid.triggerMask(0)==0,"empty pattern is silent");
 grid.setCell(0,0,{true,1.f,1.f,0.f});
 grid.setCell(1,0,{true,.5f,.75f,.25f});
 grid.setCell(2,4,{true,1.f,1.f,0.f});
 check(grid.triggerMask(0)==3,"independent simultaneous events");
 check(grid.triggerMask(4)==4,"other step trigger");
 grid.setMuted(1,true);
 check(grid.triggerMask(0)==1,"muting is independent");
 grid.setMuted(1,false);
 grid.setStepCount(4);
 check(grid.triggerMask(4)==0,"step-count hides events beyond active range");
 grid.setStepCount(16);
 grid.copyScene(0,1);
 grid.selectScene(1);
 check(grid.triggerMask(0)==3,"scene copy");
 grid.setCell(0,0,{false,1.f,1.f,0.f});
 check(grid.triggerMask(0)==2,"scene independence");
 grid.selectScene(0);
 check(grid.triggerMask(0)==3,"original scene intact");
 grid.setCell(3,2,{true,2.f,-1.f,10.f});
 auto cell=grid.getCell(3,2);
 check(cell.velocity==1.f&&cell.length==0.f&&cell.shift==.5f,"cell parameter bounds");
 grid.setCell(3,2,{true,NAN,NAN,NAN});
 cell=grid.getCell(3,2);
 check(cell.velocity==1.f&&cell.length==1.f&&cell.shift==0.f,"non-finite sanitization");
 check(grid.triggerMask(-1)==0&&grid.triggerMask(16)==0,"out-of-range steps");
 grid.clear();
 check(grid.triggerMask(0)==0&&grid.sceneIndex()==0,"reset");
 return 0;
}
