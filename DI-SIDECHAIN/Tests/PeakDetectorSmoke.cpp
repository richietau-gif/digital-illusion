#include "../Source/PeakDetector.h"
#include "../Source/SidechainPeak.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
static void check(bool ok,const char* msg){if(!ok){std::cerr<<msg<<'\n';std::exit(1);}}
int main(){
 for(double rate:{44100.,48000.,96000.,192000.}){
  const double c=di::peakReleaseCoefficient(rate);
  check(c>0.&&c<1.,"valid release coefficient");
  double peak=1.;
  for(int i=0;i<static_cast<int>(rate*0.01);++i)peak=di::followPeak(peak,0.,c);
  check(std::abs(peak-std::exp(-1.))<0.0001,"10ms release must be sample-rate invariant");
 }
 check(di::followPeak(0.2,0.9,0.5)==0.9,"instant peak attack");
 check(di::followPeak(1.,0.,0.5)==0.5,"release decay");
 check(di::followPeak(std::numeric_limits<double>::quiet_NaN(),0.4,0.5)==0.4,"invalid previous");
 check(di::followPeak(0.4,std::numeric_limits<double>::infinity(),0.5)==0.2,"invalid input");
 check(di::sidechainPeak(-0.4f,0.7f,true)==0.7f,"Stereo peak");
 check(di::sidechainPeak(-0.4f,0.7f,false)==0.4f,"Mono peak");
 check(di::sidechainPeak(std::numeric_limits<float>::quiet_NaN(),0.6f,true)==0.6f,"NaN left ignored");
 check(di::sidechainPeak(0.3f,std::numeric_limits<float>::infinity(),true)==0.3f,"Inf right ignored");
 check(di::sidechainPeak(std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),true)==0.f,"Invalid stereo silence");
 return 0;
}
