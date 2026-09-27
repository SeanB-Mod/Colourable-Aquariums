#include <cassert>
#include <cstdio>
void *p56_menu=(void*)1,*p56_inspector=(void*)2,*p56_room=(void*)3;
void *p67_swatchWidget=(void*)4,*p80_add=(void*)5,*p80_delete=(void*)6;
bool p80_externalPalette=false,p56_closing=false,g_pickerOpen=true,mainThread=true,live=true,matching=true;
int p80_selected=4,refreshes=0,hides=0,shows=0;
bool g_pickerSampling=false,g_manualMouseWasDown=false;int g_pickerDragMode=2;
bool p78_main(){return mainThread;}struct P78WorkScope{};
bool p56_alive(void* p){return p&&live;}bool p56_opened(void*){return live;}bool p56_target_live(){return live;}bool p56_active(void*){return live;}
void* field_object(void*,const char*){return matching?p56_room:nullptr;}
void* p56_go(void* p){return p;}void p56_setActive(void*,bool on){if(on)++shows;else ++hides;}
void p80_refresh_swatches(){++refreshes;}void log_raw(const char*){}
#include "SamplingBridge.h"
int main(){assert(!g_pickerSampling);assert(TPM_Aquarium_SamplingBridgeV1((void*)99,1)==0);assert(TPM_Aquarium_SamplingBridgeV1(p56_menu,0)==1&&!g_pickerSampling);mainThread=false;assert(TPM_Aquarium_SamplingBridgeV1(p56_menu,1)==-1);mainThread=true;matching=false;assert(TPM_Aquarium_SamplingBridgeV1(p56_menu,1)==-1);matching=true;assert(TPM_Aquarium_SamplingBridgeV1(p56_menu,1)==1&&g_pickerSampling&&g_pickerDragMode==0&&g_manualMouseWasDown);p56_closing=true;assert(TPM_Aquarium_SamplingBridgeV1(p56_menu,2)==1&&!g_pickerSampling);assert(TPM_Aquarium_SamplingBridgeV1(p56_menu,1)==-1);puts("PASS: optional sampling query/pause/resume, thread and ownership gates, drag cancellation and safe resume during close.");}
