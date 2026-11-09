#include "../Source/CurveSelection.h"
#include <cstdlib>
#include <iostream>
static void check(bool ok,const char* message){if(!ok){std::cerr<<message<<"\n";std::exit(1);}}
int main(){
 check(di::resolveCurveKey(false,true,0,2,5)==0,"unsynced global editor key");
 check(di::resolveCurveKey(true,true,0,2,5)==98,"GRID curve preset must target selected duck Track 3");
 check(di::resolveCurveKey(true,true,0,5,14)==101,"GRID curve key must follow track, not step");
 check(di::resolveCurveKey(true,true,1,2,5)==37,"filter lane retains step-local shape");
 check(di::resolveCurveKey(true,true,2,2,5)==69,"pan lane retains step-local shape");
 check(di::resolveCurveKey(true,false,0,2,5)==5,"legacy mode uses step curve");
 check(di::resolveCurveKey(true,true,0,-1,5)==5,"invalid track falls back safely");
 check(di::resolveCurveKey(true,true,0,6,5)==5,"FX lane track has no duck voice");
 check(di::resolveCurveKey(true,false,1,0,40)==63,"invalid step clamped");
 return 0;
}
