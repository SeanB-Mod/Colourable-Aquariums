// Included after existing slot, metadata, and live-state helpers.
#include "LightingMath.h"
#include "DefaultAquariumLight.h"
using P55GetLinearFn=F3*(*)(F3*,void*,const MethodInfo*);
static P55GetLinearFn g_p55OriginalLinear=nullptr;
static P54FloorSlotSnapshot g_p55Notified[16]{};
static void* g_p80NotifiedFishTint[16]{};
static void p51_write_abs_jump(u8*,void*);

static bool p55_live_pair(const StateList& live,const P54FloorSlotSnapshot& slot,bool requireStandard){
    if(!slot.active||!slot.floor||!p54_state_in_list(live,slot.state))return false;
    void* room=nullptr;void* floor=nullptr;
    if(!p48_floor_from_state(slot.state,room,floor)||floor!=slot.floor)return false;
    (void)requireStandard;return true; // Both standard and fish-tinted aquariums use this per-floor consumer.
}
static bool p55_same_tint(const P54FloorSlotSnapshot& a,const P54FloorSlotSnapshot& b){
    return a.active&&b.active&&a.state==b.state&&a.floor==b.floor&&p48_payload_equal(a.desired,b.desired);
}
static bool p55_refresh(const P54FloorSlotSnapshot& slot,const StateList& live){
    if(!p55_live_pair(live,slot,false))return true; // Old object gone: do not dereference it.
    return p48_mark_dirty(slot.floor,false);
}
static void p55_dispatch_refreshes(){
    if(!p55_consumer_ready())return;
    P54FloorSlotSnapshot desired[16]{};bool ready[16]{};bool any=false;
    for(int i=0;i<16;++i){ready[i]=p54_snapshot_slot(i,desired[i]);if((ready[i]&&desired[i].active)||g_p55Notified[i].active)any=true;}
    if(!any)return;
    StateList live=get_states();
    for(int i=0;i<16;++i){
        if(!ready[i])continue; // Busy publication is not removal.
        P54FloorSlotSnapshot next=desired[i];
        if(!p55_glow_on()||!p55_live_pair(live,next,true))next.active=false;
        void* fishTint=next.active?field_object(next.state,"_highestPriorityAquariumTintDefinition"):nullptr;
        P54FloorSlotSnapshot& previous=g_p55Notified[i];
        if(previous.active&&(!next.active||previous.state!=next.state||previous.floor!=next.floor)){
            if(!p55_refresh(previous,live))continue;
            previous=P54FloorSlotSnapshot{};g_p80NotifiedFishTint[i]=nullptr;
        }
        if(next.active&&(!p55_same_tint(previous,next)||g_p80NotifiedFishTint[i]!=fishTint)){
            if(p55_refresh(next,live)){
                previous=next;g_p80NotifiedFishTint[i]=fishTint;
            }
        }
    }
}
static F3* p55_linear_hook(F3* result,void* floor,const MethodInfo* method){
    // Windows x64 IL2CPP float3 return: RCX=sret, RDX=floor, R8=MethodInfo.
    F3* returned=g_p55OriginalLinear(result,floor,method);
    if(!p78_main())return returned;P78WorkScope work;
    if(!returned||!floor||!p55_glow_on()||!p55_consumer_ready())return returned;
    for(int i=0;i<16;++i){P54FloorSlotSnapshot slot{};
        if(!p54_snapshot_slot(i,slot)||!slot.active||slot.floor!=floor)continue;
        StateList live=get_states();if(!p55_live_pair(live,slot,true))return returned;
        P55RGB baseline{returned->x,returned->y,returned->z};
        void* activeFishTint=field_object(slot.state,"_highestPriorityAquariumTintDefinition");
        if(activeFishTint)baseline=p82_default_baseline(baseline);
        P55RGB tinted=p67_soft_glow(baseline,{slot.desired.r,slot.desired.g,slot.desired.b});
        // User-selected Dredge intensity: one quarter of the corrected normal baseline.
        if(activeFishTint){tinted.r*=0.25f;tinted.g*=0.25f;tinted.b*=0.25f;}
        returned->x=tinted.r;returned->y=tinted.g;returned->z=tinted.b;
        return returned;
    }
    return returned;
}
static bool p55_install_consumer_hook(){
    if(!p78_main())return false;
    Il2CppClass* cls=find_class("TPS.Game","CeilingLightPlacementManager");
    const MethodInfo* method=cls?find_method_name_params(cls,"GetLinearLightColor",1):nullptr;
    u8* target=method?(u8*)*(void* const*)method:nullptr;void* ga=find_loaded_module("GameAssembly.dll");
    static const u8 expected[15]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,0x24,0x10,0x48,0x89,0x7c,0x24,0x18};
    if(!p55_update_ready()||!target||!ptr_in_module(target,ga,32)){log_raw("0.7.81 CONSUMER: unavailable; standard glow disabled.\r\n");return false;}
    for(int i=0;i<15;++i)if(target[i]!=expected[i]){log_raw("0.7.81 CONSUMER: exact prologue mismatch; refusing patch.\r\n");return false;}
    p82_resolve(target,ga);
    u8* trampoline=(u8*)pVirtualAlloc(nullptr,64,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);if(!trampoline)return false;
    memcpy(trampoline,target,15);p51_write_abs_jump(trampoline+15,target+15);
    if(!p78_seal_trampoline(trampoline,29)){p78_free_trampoline(trampoline);return false;}
    g_p55OriginalLinear=(P55GetLinearFn)trampoline;
    DWORD old=0;if(!pVirtualProtect(target,15,PAGE_EXECUTE_READWRITE,&old)){g_p55OriginalLinear=nullptr;p78_free_trampoline(trampoline);return false;}
    p51_write_abs_jump(target,(void*)&p55_linear_hook);target[14]=0x90;
    pFlushInstructionCache(pGetCurrentProcess(),target,15);DWORD ignored=0;pVirtualProtect(target,15,old,&ignored);
    __atomic_store_n(&g_p55ConsumerReady,true,__ATOMIC_RELEASE);
    log_raw("0.7.81 CONSUMER: installed GetLinearLightColor hook; preserve retail luminance, no raw FloorArea override.\r\n");return true;
}
