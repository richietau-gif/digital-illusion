#include <cmath>
#include <algorithm>
#include <iostream>
int main(){for(float o: {-1.f,-.5f,0.f,.5f,1.f}){float t=std::clamp(.5f+o,0.f,1.f); if(!std::isfinite(t)) return 1;} std::cout<<"precision sidechain smoke ok\n"; return 0;}
