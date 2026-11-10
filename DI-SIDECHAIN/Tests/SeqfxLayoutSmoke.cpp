#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
static void expect(bool ok,const char* message){
 if(!ok){std::cerr<<message<<"\n";std::exit(1);}
}
int main(){
 // Design-space geometry shared by six musical rows and the transport row.
 constexpr int canvasW=1536,canvasH=950,left=195,right=1507;
 constexpr int gridTop=224,gridBottom=421,transportHeight=32;
 constexpr int cellPanelX=1129,cellPanelRight=1524;
 constexpr int laneStart=908,laneRight=1108;
 for(float scale:std::array<float,3>{.5f,.75f,1.f}){
  const int canvasPixels=std::lround(canvasW*scale);
  expect(std::lround(cellPanelRight*scale)<=canvasPixels,"cell routing clips horizontally");
  expect(std::lround(laneRight*scale)<std::lround(cellPanelX*scale),
         "modulation selector overlaps cell panel");
  expect(std::lround(laneStart*scale)>0,"modulation panel begins off-screen");
  for(int steps:std::array<int,4>{1,8,12,16}){
   int lastX=-1;
   for(int step=0;step<steps;++step){
    const int a=left+step*(right-left)/steps;
    const int b=left+(step+1)*(right-left)/steps;
    const int pxA=std::lround(a*scale),pxB=std::lround(b*scale);
    expect(pxA>=lastX&&pxB>pxA+8,"columns not monotonically clickable");
    lastX=pxB;
    const int mid=(a+b)/2;
    const int selected=(mid-left)*steps/(right-left);
    expect(selected==step,"transport selection and grid columns disagree");
   }
   expect(lastX==std::lround(right*scale),"transport ends short of main grid");
  }
  const int rowH=std::lround((gridBottom-gridTop-transportHeight)/6.f*scale);
  expect(rowH>=13,"six musical rows too short to select");
  expect(std::lround(canvasH*scale)>=std::lround(938*scale),"footer clipped");
 }
 return 0;
}
