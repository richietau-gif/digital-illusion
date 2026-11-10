#include "../Source/EnvelopeFilter.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
static void check(bool test,const char* what){
 if(!test){std::cerr<<"FILTER AUDIO FAIL: "<<what<<"\n";std::exit(1);}
}
static float render(double tone,int mode,float cutoff,bool analog=false){
 di::EnvelopeFilter fx;fx.prepare(48000.);
 double energy=0.;int samples=0;
 for(int n=0;n<24000;++n){
  const float sample=static_cast<float>(.33*std::sin(6.28318530718*tone*n/48000.));
  const float output=fx.process(sample,cutoff,mode,.35f,analog);
  check(std::isfinite(output),"finite envelope filter signal");
  if(n>=8000){energy+=output*output;++samples;}
 }
 return std::sqrt((float)(energy/samples));
}
static float inputRms(double tone,bool hp,bool lp){
 di::InputCutFilter fx;fx.prepare(48000.);fx.set(220.f,1800.f);
 double power=0.;int count=0;
 for(int n=0;n<24000;++n){
  const float in=static_cast<float>(.33*std::sin(6.28318530718*tone*n/48000.));
  const float out=fx.process(in,hp,lp);
  check(std::isfinite(out),"finite input signal");
  if(n>=8000){power+=out*out;++count;}
 }
 return std::sqrt((float)(power/count));
}
int main(){
 const float low=di::EnvelopeFilter::frequency(0.f,1.f,95.f,11000.f,48000.f);
 const float high=di::EnvelopeFilter::frequency(1.f,1.f,95.f,11000.f,48000.f);
 check(std::abs(low-95.f)<.01f&&std::abs(high-11000.f)<.1f,"envelope maps to real cutoff");
 check(std::abs(di::EnvelopeFilter::frequency(0.f,0.f,95.f,11000.f,48000.f)-11000.f)<.1f,
       "zero depth is neutral");
 const float closed=render(2500.,0,low);
 const float open=render(2500.,0,high);
 check(closed<open*.20f,"LP sweep envelope audibly opens cutoff");
 const float hpClosed=render(2500.,1,high);
 const float hpOpen=render(2500.,1,low);
 check(hpClosed<hpOpen*.3f,"HP sweep envelope affects actual audio");
 const float analog=render(2500.,0,500.f,true);
 check(std::isfinite(analog),"analog ladder stable");
 check(render(1400.,2,800.f)>0.f,"bandpass produces nonzero signal");
 check(render(1400.,3,800.f)>0.f,"notch produces nonzero signal");
 const float unfilteredLow=inputRms(60.,false,false);
 const float filteredLow=inputRms(60.,true,false);
 check(filteredLow<unfilteredLow*.3f,"input HP independently removes low end");
 const float unfilteredHigh=inputRms(6600.,false,false);
 const float filteredHigh=inputRms(6600.,false,true);
 check(filteredHigh<unfilteredHigh*.5f,"input LP independently removes high end");
 check(std::abs(inputRms(60.,false,false)-unfilteredLow)<1e-6f,
       "both disabled keeps audio unchanged");
 check(std::isfinite(di::EnvelopeFilter::frequency(std::numeric_limits<float>::quiet_NaN(),
       1.f,80.f,12000.f,48000.f)),"malformed automation is sanitized");
 return 0;
}
