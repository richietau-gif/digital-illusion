#include "../Source/CurvePointEditing.h"
#include "../Source/FrequencyDisplay.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

static void require(bool b,const char* msg){
 if(!b){std::cerr<<"CURVE HIT: "<<msg<<"\n";std::exit(1);}
}
static void verify(int nodes,float scale){
 di::DynamicCurve c;
 c.count=nodes;
 for(int i=0;i<nodes;++i){
  c.x[(size_t)i]=static_cast<float>(i)/(nodes-1);
  c.y[(size_t)i]=(i%2)?0.85f:0.15f;
 }
 constexpr float width=488.f,height=110.f;
 for(int i=1;i<nodes-1;++i){
  const float x=c.x[(size_t)i]*width;
  const float y=(1.f-c.y[(size_t)i])*height;
  // Simulates 5-pixel hand/cursor error at 50%, 75% and 100% UI scale.
  const float dx=5.f/scale,dy=3.f/scale;
  require(di::CurveHit::nearest(c,x+dx,y+dy,width,height,scale,true)==i,
          "right-click must select the intended interior node");
  auto copy=c;
  const int target=di::CurveHit::nearest(copy,x+dx,y+dy,width,height,scale,true);
  require(copy.erase(target)&&copy.count==nodes-1,"point deletion must reduce count");
  require(copy.x[(size_t)(target-1)]<copy.x[(size_t)target],
          "deletion must preserve ordered times");
 }
 for(int i:{0,nodes-1}){
  const float x=c.x[(size_t)i]*width,y=(1.f-c.y[(size_t)i])*height;
  require(di::CurveHit::nearest(c,x,y,width,height,scale,true)==-1,
          "anchored endpoints cannot be deleted");
  require(di::CurveHit::nearest(c,x,y,width,height,scale,false)==i,
          "endpoints remain selectable for vertical adjustment");
 }
 require(di::CurveHit::nearest(c,-300.f,-300.f,width,height,scale,true)==-1,
         "right-click away from graph must not delete an unrelated node");
}
int main(){
 for(float scale:{.5f,.75f,1.f})
  for(int count:{2,3,8,12,16})verify(count,scale);
 require(di::shortFrequency(35.f)=="35 Hz","highpass shows whole Hertz");
 require(di::shortFrequency(950.f)=="950 Hz","lowpass below 1k shows whole Hertz");
 require(di::shortFrequency(1000.f)=="1 kHz","integer kHz has no decimal");
 require(di::shortFrequency(1250.f)=="1.3 kHz","fractional kHz has one decimal");
 require(di::shortFrequency(17000.f)=="17 kHz","upper lowpass removes decimal tail");
 require(di::shortFrequency(12535.69f)=="12.5 kHz","automated float value shortened");
 return 0;
}
