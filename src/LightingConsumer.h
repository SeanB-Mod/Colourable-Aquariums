// Included after existing slot, metadata, and live-state helpers.
#include "LightingMath.h"
using P55GetLinearFn=F3*(*)(F3*,void*,const MethodInfo*);
static P55GetLinearFn g_p55OriginalLinear=nullptr;
static P54FloorSlotSnapshot g_p55Notified[16]{};
static P54FloorSlotSnapshot g_p55Logged[16]{};
static volatile u64 g_p55Calls=0,g_p55Tinted=0,g_p55Refreshes=0,g_p55RefreshFailures=0;
static unsigned g_p55SampleLogs=0;
static void p51_write_abs_jump(u8*,void*);

static bool p55_live_pair(const StateList& live,const P54FloorSlotSnapshot& slot,bool requireStandard){
    if(!slot.active||!slot.floor||!p54_state_in_list(live,slot.state))return false;
    void* room=nullptr;void* floor=nullptr;
    if(!p48_floor_from_state(slot.state,room,floor)||floor!=slot.floor)return false;
    return !requireStandard||!field_object(slot.state,"_highestPriorityAquariumTintDefinition");
}
static bool p55_same_tint(const P54FloorSlotSnapshot& a,const P54FloorSlotSnapshot& b){
    return a.active&&b.active&&a.state==b.state&&a.floor==b.floor&&p48_payload_equal(a.desired,b.desired);
}
static bool p55_refresh(const P54FloorSlotSnapshot& slot,const StateList& live){
    if(!p55_live_pair(live,slot,false))return true; // Old object gone: do not dereference it.
    if(p48_mark_dirty(slot.floor,false)){__atomic_fetch_add(&g_p55Refreshes,1ull,__ATOMIC_RELAXED);return true;}
    __atomic_fetch_add(&g_p55RefreshFailures,1ull,__ATOMIC_RELAXED);return false;
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
        P54FloorSlotSnapshot& previous=g_p55Notified[i];
        if(previous.active&&(!next.active||previous.state!=next.state||previous.floor!=next.floor)){
            if(!p55_refresh(previous,live))continue;
            previous=P54FloorSlotSnapshot{};
        }
        if(next.active&&!p55_same_tint(previous,next)){
            if(p55_refresh(next,live))previous=next;
        }
    }
}
static F3* p55_linear_hook(F3* result,void* floor,const MethodInfo* method){
    // Windows x64 IL2CPP float3 return: RCX=sret, RDX=floor, R8=MethodInfo.
    F3* returned=g_p55OriginalLinear(result,floor,method);
    if(!p78_main())return returned;P78WorkScope work;
    __atomic_fetch_add(&g_p55Calls,1ull,__ATOMIC_RELAXED);
    if(!returned||!floor||!p55_glow_on()||!p55_consumer_ready())return returned;
    for(int i=0;i<16;++i){P54FloorSlotSnapshot slot{};
        if(!p54_snapshot_slot(i,slot)||!slot.active||slot.floor!=floor)continue;
        StateList live=get_states();if(!p55_live_pair(live,slot,true))return returned;
        P55RGB baseline{returned->x,returned->y,returned->z};
        P55RGB tinted=p67_soft_glow(baseline,{slot.desired.r,slot.desired.g,slot.desired.b});
        returned->x=tinted.r;returned->y=tinted.g;returned->z=tinted.b;
        __atomic_fetch_add(&g_p55Tinted,1ull,__ATOMIC_RELAXED);
        if(g_p55SampleLogs<128&&!p55_same_tint(g_p55Logged[i],slot)){
            g_p55Logged[i]=slot;++g_p55SampleLogs;char b[720];
            psprintf(b,"0.7.81 LIGHT SAMPLE floor=%p retailLinear=(%.5f,%.5f,%.5f) tintedLinear=(%.5f,%.5f,%.5f) luminanceBefore=%.5f luminanceAfter=%.5f. No FloorArea write.\r\n",floor,(double)baseline.r,(double)baseline.g,(double)baseline.b,(double)tinted.r,(double)tinted.g,(double)tinted.b,(double)p55_luminance(baseline),(double)p55_luminance(tinted));log_raw(b);
        }
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
static void p55_status(){
    char b[520];psprintf(b,"0.7.81 CONSUMER STATUS: hook=%s calls=%llu tinted=%llu changeRefreshes=%llu refreshFailures=%llu standardPayloadWrites=0.\r\n",p55_consumer_ready()?"INSTALLED":"OFF",(unsigned long long)__atomic_load_n(&g_p55Calls,__ATOMIC_RELAXED),(unsigned long long)__atomic_load_n(&g_p55Tinted,__ATOMIC_RELAXED),(unsigned long long)__atomic_load_n(&g_p55Refreshes,__ATOMIC_RELAXED),(unsigned long long)__atomic_load_n(&g_p55RefreshFailures,__ATOMIC_RELAXED));log_raw(b);
}
