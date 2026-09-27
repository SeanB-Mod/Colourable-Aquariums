#include <cassert>
#include <cstdio>
void *p56_menu=(void*)1,*p56_inspector=(void*)2,*p56_room=(void*)3;
void *p67_swatchWidget=(void*)4,*p80_add=(void*)5,*p80_delete=(void*)6;
bool p80_externalPalette=false,p56_closing=false,g_pickerOpen=true,mainThread=true,live=true,matching=true;
int p80_selected=4,refreshes=0,hides=0,shows=0;
bool p78_main(){return mainThread;}struct P78WorkScope{};
bool p56_alive(void* p){return p&&live;}bool p56_opened(void*){return live;}bool p56_target_live(){return live;}bool p56_active(void*){return live;}
void* field_object(void*,const char*){return matching?p56_room:nullptr;}
void* p56_go(void* p){return p;}void p56_setActive(void*,bool on){if(on)++shows;else ++hides;}
void p80_refresh_swatches(){++refreshes;}void log_raw(const char*){}
#include "PaletteBridge.h"
int main(){
 assert(!p80_externalPalette); // No caller: standalone is unchanged.
 assert(TPM_Aquarium_PaletteBridgeV1((void*)99,1)==0&&!p80_externalPalette);
 assert(TPM_Aquarium_PaletteBridgeV1(p56_menu,0)==1&&!p80_externalPalette&&hides==0);
 mainThread=false;assert(TPM_Aquarium_PaletteBridgeV1(p56_menu,1)==-1);mainThread=true;
 matching=false;assert(TPM_Aquarium_PaletteBridgeV1(p56_menu,1)==-1);matching=true;
 live=false;assert(TPM_Aquarium_PaletteBridgeV1(p56_menu,1)==-1);live=true;
 assert(TPM_Aquarium_PaletteBridgeV1(p56_menu,3)==-1);
 assert(TPM_Aquarium_PaletteBridgeV1(p56_menu,1)==1&&p80_externalPalette&&p80_selected==-1&&hides==3);
 assert(TPM_Aquarium_PaletteBridgeV1(p56_menu,2)==1&&!p80_externalPalette&&refreshes==1&&shows==2);
 assert(TPM_Aquarium_PaletteBridgeV1(p56_menu,2)==1&&refreshes==1);
 assert(TPM_Aquarium_PaletteBridgeV1(p56_menu,1)==1);p56_closing=true;
 assert(TPM_Aquarium_PaletteBridgeV1(p56_menu,2)==1&&!p80_externalPalette&&refreshes==1);
 p56_closing=false;g_pickerOpen=false;assert(TPM_Aquarium_PaletteBridgeV1(p56_menu,1)==-1);
 puts("PASS: optional query/acquire/release, ownership/thread/stale-target gates, standalone default, idempotent release and closing does not resurrect UI.");
}
