// Recover the same default lighting definition read by the guarded retail
// GetLinearLightColor implementation. Read-only: no temporary floor/state edits.
static void* (*p82_definitions)(const MethodInfo*)=nullptr;
static P47FieldOffsetFn p82_offset=nullptr;
static const MethodInfo* p82_gamma=nullptr;
static void p82_resolve(u8* target,void* module){
    // RCX=null; call definitions singleton; load floor room and default light.
    static const u8 before[]={0x33,0xc9,0x44,0x0f,0x29,0x44,0x24,0x20,0x48,0x8b,0xda,0xe8};
    static const u8 after[]={0x4c,0x8b,0x83,0xa8,0,0,0,0x48,0x8b,0xb8,0xf8,0x02,0,0};
    if(!ptr_in_module(target,module,0x40))return;
    for(unsigned i=0;i<sizeof(before);++i)if(target[0x22+i]!=before[i])return;
    for(unsigned i=0;i<sizeof(after);++i)if(target[0x32+i]!=after[i])return;
    int displacement=0;memcpy(&displacement,target+0x2e,4);
    u8* getter=target+0x32+displacement;if(!ptr_in_module(getter,module,32))return;
    p82_offset=(P47FieldOffsetFn)resolve_export(module,"il2cpp_field_get_offset");
    Il2CppClass* math=find_class("UnityEngine","Mathf");
    p82_gamma=math?find_method_param1(math,"GammaToLinearSpace","System.Single"):nullptr;
    if(!p82_offset||!p82_gamma)return;
    p82_definitions=(decltype(p82_definitions))getter;
    log_raw("DREDGE BASELINE: guarded retail default-light reader ready; no game field writes.\r\n");
}
static P55RGB p82_default_baseline(P55RGB retail){
    if(!p82_definitions||!p82_offset||!p82_gamma)return retail;
    void* definitions=p82_definitions(nullptr);if(!definitions)return retail;
    Il2CppClass* cls=il2cpp_object_get_class((Il2CppObject*)definitions);
    auto field=p48_field_at_offset(cls,0x2f8,p82_offset);if(!field)return retail;
    void* light=il2cpp_field_get_value_object(field,definitions);if(!light)return retail;
    cls=il2cpp_object_get_class((Il2CppObject*)light);
    field=p48_field_at_offset(cls,0x44,p82_offset);if(!field)return retail;
    // Require the full managed Color type before unboxing its four floats.
    const Il2CppType* type=il2cpp_field_get_type(field);char* name=type?il2cpp_type_get_name(type):nullptr;
    bool color=name&&streq(name,"UnityEngine.Color");if(name)il2cpp_free(name);if(!color)return retail;
    F4 gamma=read_boxed_f4(il2cpp_field_get_value_object(field,light));
    P55RGB result{};float channels[3]={gamma.x,gamma.y,gamma.z};float* values[3]={&result.r,&result.g,&result.b};
    for(int i=0;i<3;++i){void* args[]={&channels[i]};auto box=invoke(p82_gamma,nullptr,args);if(!box)return retail;*values[i]=boxed_float(box,-1.0f);}
    return p55_valid_rgb(result)&&p55_luminance(result)>0.000001f?result:retail;
}
