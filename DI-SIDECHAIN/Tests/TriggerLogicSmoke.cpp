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
 check(di::shouldConsumeAudioPeak(true,true,0),"Combined mode consumes audio crossing");
 check(di::shouldConsumeAudioPeak(true,true,1),"Audio-only consumes audio crossing");
 check(!di::shouldConsumeAudioPeak(true,true,2),"MIDI-only must not consume audio");
 check(!di::shouldConsumeAudioPeak(true,false,0),"Disarmed crossing must not consume");
 check(!di::shouldConsumeAudioPeak(false,true,0),"No crossing must not consume");
 // Reproduce a sustained audio pulse overlapping MIDI and the holdoff window.
 bool armed=true;
 int holdoff=4;
 int fires=0;
 for(int sample=0;sample<12;++sample){
  const bool high=sample>=1&&sample<=8;
  const bool midi=sample==1;
  if(!high)armed=true;
  const bool crossing=high&&armed;
  if(shouldFireTrigger(midi,crossing,armed,holdoff,0)){++fires;holdoff=4;}
  if(di::shouldConsumeAudioPeak(crossing,armed,0))armed=false;
  if(holdoff>0)--holdoff;
 }
 check(fires==1,"Sustained overlapping audio and MIDI must fire once");
 // A second kick arriving inside holdoff is consumed, not queued for later.
 armed=true;
 holdoff=10;
 fires=0;
 for(int sample=0;sample<24;++sample){
  const bool high=(sample>=1&&sample<=3)||(sample>=5&&sample<=7);
  if(!high)armed=true;
  const bool crossing=high&&armed;
  if(shouldFireTrigger(false,crossing,armed,holdoff,0)){++fires;holdoff=10;}
  if(di::shouldConsumeAudioPeak(crossing,armed,0))armed=false;
  if(holdoff>0)--holdoff;
 }
 check(fires==0,"Audio crossings inside holdoff must not queue delayed triggers");
 // Repeated short kicks outside holdoff must remain independently triggerable.
 armed=true;
 holdoff=0;
 fires=0;
 for(int sample=0;sample<32;++sample){
  const bool high=sample==1||sample==16;
  if(!high)armed=true;
  const bool crossing=high&&armed;
  if(shouldFireTrigger(false,crossing,armed,holdoff,0)){++fires;holdoff=8;}
  if(di::shouldConsumeAudioPeak(crossing,armed,0))armed=false;
  if(holdoff>0)--holdoff;
 }
 check(fires==2,"Separated audio kicks must retrigger independently");
 // MIDI-only must keep responding to dense notes even while audio holdoff runs.
 fires=0;
 holdoff=20;
 for(int sample=0;sample<32;++sample){
  const bool midi=sample==1||sample==2||sample==3||sample==20;
  if(shouldFireTrigger(midi,true,true,holdoff,2))++fires;
  if(holdoff>0)--holdoff;
 }
 check(fires==4,"MIDI-only dense notes must bypass audio holdoff");
 // Audio-only must never respond to MIDI notes, even with no holdoff.
 fires=0;
 for(int sample=0;sample<32;++sample)
  if(shouldFireTrigger(sample%2==0,false,true,0,1))++fires;
 check(fires==0,"Audio-only must ignore dense MIDI events");
 return 0;
}
