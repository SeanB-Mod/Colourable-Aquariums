#include <cassert>
#include <limits>
#include <cstdio>
struct F4{float x,y,z,w;};
void *p56_menu=(void*)1,*g_pickerWidget=(void*)2,*p56_inspector=(void*)3,*p56_room=(void*)4;
bool p56_closing=false,g_pickerOpen=true,mainThread=true,live=true,matching=true,g_pickerChanged=false;
F4 g_currentWaterColor{.1f,.2f,.3f,0};float g_manualHue,g_manualSat,g_manualVal;int writes=0,saves=0,visuals=0,glows=0,selections=0;
bool p78_main(){return mainThread;}struct P78WorkScope{};
bool p56_alive(void*){return live;}bool p56_opened(void*){return live;}bool p56_target_live(){return live;}bool p56_active(void*){return live;}
void* field_object(void*,const char*){return matching?p56_room:nullptr;}
void rgb_to_hsv(F4,float&,float&,float&){}
void update_active_water_color(F4 c){++writes;g_currentWaterColor=c;}
void update_manual_picker_visuals(){++visuals;}void p56_gradient(){++visuals;}
void p65_flush_glow(){++glows;}void persist_current_target(bool b){assert(b);++saves;}void p80_button_states(){++selections;}void log_raw(const char*){}
#include "ColourPickerBridge.h"
int main(){float c[4]{};
 assert(TPM_Aquarium_ColourBridgeV1((void*)99,1,c)==0&&writes==0);
 assert(TPM_Aquarium_ColourBridgeV1(p56_menu,0,c)==1&&c[2]==.3f&&c[3]==1&&writes==0);
 mainThread=false;assert(TPM_Aquarium_ColourBridgeV1(p56_menu,1,c)==-1);mainThread=true;
 p56_closing=true;assert(TPM_Aquarium_ColourBridgeV1(p56_menu,1,c)==-1);p56_closing=false;
 live=false;assert(TPM_Aquarium_ColourBridgeV1(p56_menu,1,c)==-1);live=true;
 matching=false;assert(TPM_Aquarium_ColourBridgeV1(p56_menu,1,c)==-1);matching=true;
 c[2]=std::numeric_limits<float>::quiet_NaN();assert(TPM_Aquarium_ColourBridgeV1(p56_menu,1,c)==-1);
 c[2]=1.1f;assert(TPM_Aquarium_ColourBridgeV1(p56_menu,1,c)==-1);
 c[2]=.75f;assert(TPM_Aquarium_ColourBridgeV1(p56_menu,1,c)==1);
 assert(writes==1&&saves==1&&visuals==2&&glows==1&&selections==1&&g_pickerChanged&&g_currentWaterColor.z==.75f);
 assert(TPM_Aquarium_ColourBridgeV1(p56_menu,2,c)==-1);
 puts("PASS: actual bridge ownership/read/apply, thread, close, stale room, range/NaN; exactly one aquarium update/persist; no ColourPicker dependency");}
