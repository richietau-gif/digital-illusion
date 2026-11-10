#include "../Source/DynamicCurve.h"
#include "../Source/CurvePresets.h"
#include <cstdlib>
#include <cmath>
#include <iostream>
static void check(bool b,const char* m){if(!b){std::cerr<<m<<"\n";std::exit(1);}}
int main(){
 di::DynamicCurve c;
 check(c.count==8,"initial eight nodes");
 check(c.insert(.45f,.3f)&&c.count==9,"add node");
 int inserted=-1;for(int i=0;i<c.count;++i)if(std::abs(c.x[(size_t)i]-.45f)<.001f)inserted=i;
 check(inserted>0,"node inserted by time");
 c.move(inserted,.48f,.9f);
 check(std::abs(c.y[(size_t)inserted]-.9f)<.001f,"move y");
 check(c.x[(size_t)(inserted-1)]<c.x[(size_t)inserted],"time order");
 check(c.erase(inserted)&&c.count==8,"remove node");
 check(!c.erase(0)&&!c.erase(c.count-1),"endpoints anchored");
 for(int k=0;k<8;++k) {
  const auto& shape=di::curvePresets[(size_t)k];
  for(int i=0;i<8;++i)c.y[(size_t)i]=shape[(size_t)i];
  for(int i=0;i<=100;++i){
   const float v=c.evaluate(i/100.f);
   check(std::isfinite(v)&&v>=0.f&&v<=1.f,"interpolation bounded");
   check(std::abs(v-di::sampleCurvePreset(k,i/100.f))<.0001f,"UI preset matches actual curve");
  }
 }
 check(c.insert(.22f,.4f),"new structural edit");
 c.move(1,.95f,1.f);
 check(c.x[1]<c.x[2],"no node crossing");
 return 0;
}
