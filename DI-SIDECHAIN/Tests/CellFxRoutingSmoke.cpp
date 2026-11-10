#include "../Source/CellFxRouting.h"
#include <array>
#include <cstdlib>
#include <iostream>
static void check(bool ok,const char* message){
 if(!ok){std::cerr<<message<<"\n";std::exit(1);}
}
int main(){
 std::array<bool,6> cells{};
 std::array<int,6> masks{};
 auto route=di::routeCellEffects(cells,masks,true);
 check(!route.reverb&&!route.distort&&!route.delay&&!route.reverse&&!route.filter,"empty grid is silent");
 for(bool enabled:route.duck)check(!enabled,"inactive duck must be off");
 cells[0]=true;
 route=di::routeCellEffects(cells,masks,true);
 check(route.duck[0]&&!route.reverb&&!route.distort&&!route.delay,"plain duck cell");
 masks[0]=8|1|2|4|16; // BYP + RVB + DST + REV + DELAY
 route=di::routeCellEffects(cells,masks,true);
 check(!route.duck[0],"duck bypass works");
 check(route.reverb&&route.distort&&route.reverse&&route.delay,"bypass preserves FX sends");
 cells[1]=true;masks[1]=0;
 route=di::routeCellEffects(cells,masks,true);
 check(!route.duck[0]&&route.duck[1],"independent duck row survives bypass");
 cells[0]=false;
 route=di::routeCellEffects(cells,masks,true);
 check(!route.reverb&&!route.distort&&!route.delay&&!route.reverse&&!route.filter,
       "inactive cell must not leak FX sends");
 cells.fill(false);masks.fill(0);
 route=di::routeCellEffects(cells,masks,false);
 check(route.reverb&&route.distort&&route.delay,"sidechain mode keeps FX available");
 masks[2]=4;cells[2]=true;
 route=di::routeCellEffects(cells,masks,true);
 check(!route.reverse,"reverse flag alone cannot create reverb send");
 // New FLT bit is genuinely independent of the DUCK envelope and FX sends.
 cells.fill(false);masks.fill(0);
 cells[0]=true;masks[0]=32;
 route=di::routeCellEffects(cells,masks,true);
 check(route.filter&&route.duck[0],"FILTER lane can coexist with DUCK");
 masks[0]=32|8; // FLT + BYP
 route=di::routeCellEffects(cells,masks,true);
 check(route.filter&&!route.duck[0],"BYP affects ducking but not FILTER FX");
 cells[0]=false;
 route=di::routeCellEffects(cells,masks,true);
 check(!route.filter,"inactive cell gates off its FILTER envelope");

 return 0;
}
