#include "../Source/TriggerLogic.h"
#include <cstdlib>
#include <iostream>
static void check(bool ok,const char* msg){if(!ok){std::cerr<<msg<<'\n';std::exit(1);}}
int main(){
 using di::shouldFireTrigger;
 check(shouldFireTrigger(true,false,false,0,0),"Combined mode accepts MIDI");
 check(shouldFireTrigger(true,false,false,100,0),"Dense MIDI bypasses audio holdoff");
 check(shouldFireTrigger(false,true,true,0,0),"Combined mode accepts audio");
 check(!shouldFireTrigger(false,true,true,1,0),"Audio holdoff must last entire count");
 check(shouldFireTrigger(false,true,true,0,1),"Audio-only accepts audio");
 check(!shouldFireTrigger(true,false,true,0,1),"Audio-only ignores MIDI");
 check(shouldFireTrigger(true,true,true,10,2),"MIDI-only accepts MIDI");
 check(!shouldFireTrigger(false,true,true,0,2),"MIDI-only ignores audio");
 check(!shouldFireTrigger(false,true,false,0,0),"Audio requires armed detector");
 check(shouldFireTrigger(true,true,true,0,0),"Simultaneous sources yield one trigger");
 check(!shouldFireTrigger(false,false,true,0,0),"Silence does not trigger");
 return 0;
}
