#include "InputHookGuard.h"
// Included after the existing colour, live-state and native-hook helpers.
#include "AquariumSelection.h"
using P56Void=void(*)(void*,const MethodInfo*);
using P56Close=void(*)(void*,void*,const MethodInfo*);
static P56Void p56_originalUpdate,p56_originalRefresh,p56_originalInput,p56_originalPress,p56_originalForce;
static P56Close p56_originalClose;
static bool p56_ready=false,p56_busy=false,p56_closing=false;
static unsigned p57_diagnostics=0;
static void p57_once(unsigned bit,const char* message){if(!(p57_diagnostics&bit)){p57_diagnostics|=bit;log_raw(message);}}
static void *p56_inspector=nullptr,*p56_button=nullptr,*p56_menu=nullptr;
static void *p56_room=nullptr,*p56_state=nullptr,*p56_water=nullptr,*p56_entity=nullptr;
static void *p56_savedOpen=nullptr,*p56_savedClosed=nullptr,*p56_preview=nullptr;
static int p56_resetState=0;
static bool p67_inspectorInteractive=true,p67_changedInspector=false;
static RectState p65_stockPickerRect{};
static float p56_savedHue=0,p56_savedSat=0,p56_savedVal=0,p56_lastHue=-1,p56_lastSat=-1;
static AquariumGCHandle p56_buttonHandle=0,p56_holds[32]{};static int p56_holdCount=0;
static const MethodInfo *p56_aliveMethod=nullptr,*p56_baseOpen=nullptr,*p56_baseClose=nullptr,*p56_baseCloseRequest=nullptr,*p56_baseInput=nullptr;
struct P56Hidden{void* go;bool active;};
static P56Hidden p56_hidden[4]{};static int p56_hiddenCount=0;
static u64 (*p56_clock)()=nullptr;
static void p56_tick();
static void p56_end(bool returnToRoom,bool save);
static void* p56_go(void* component){return component?invoke(M_Component_get_gameObject,component):nullptr;}
static void* p56_tr(void* component){void* go=p56_go(component);return go?invoke(M_GameObject_get_transform,go):nullptr;}
static bool p56_alive(void* obj){void* a[]={obj};return obj&&p56_aliveMethod&&boxed_bool(invoke(p56_aliveMethod,nullptr,a));}
static bool p56_active(void* component){void* go=p56_go(component);return go&&boxed_bool(invoke(M_GameObject_get_activeInHierarchy,go));}
static bool p57_scene_object(void* component){
    void* go=p56_go(component);if(!go)return false;
    auto getter=find_method0_hierarchy(C_GameObject,"get_scene");
    Il2CppObject* scene=invoke(getter,go);if(!scene)return false;
    auto valid=find_method0_hierarchy(il2cpp_object_get_class(scene),"IsValid");
    // Scene is a value type: runtime_invoke receives its unboxed instance.
    return valid&&boxed_bool(invoke(valid,il2cpp_object_unbox(scene)));
}
static void p56_setActive(void* go,bool on){if(p56_alive(go)){void* a[]={&on};invoke(M_GameObject_SetActive,go,a);}}
static void p56_hold(void* obj){if(obj&&p56_holdCount<32)p56_holds[p56_holdCount++]=il2cpp_gchandle_new((Il2CppObject*)obj,false);}
static void p56_release(){log_raw("0.7.81 UI CLOSE: releasing pointer-width runtime handles.\r\n");for(int i=0;i<p56_holdCount;++i)if(p56_holds[i])il2cpp_gchandle_free(p56_holds[i]);p56_holdCount=0;}
static bool p56_opened(void* menu){
    if(!p56_alive(menu))return false;
    auto m=find_method0_hierarchy(il2cpp_object_get_class((Il2CppObject*)menu),"get_IsOpeningOrOpen");
    return m&&boxed_bool(invoke(m,menu));
}
static bool p56_call(const MethodInfo* m,void* obj,void** a=nullptr){
    if(!m)return false;Il2CppObject* ex=nullptr;il2cpp_runtime_invoke(m,obj,a,&ex);
    if(ex){log_raw("0.7.81 UI: managed call rejected; operation stopped.\r\n");return false;}return true;
}
static void* p56_live_state(void* room){
    if(!room)return nullptr;StateList live=get_states();
    for(int i=0;i<live.count;++i)if(field_object(live.states[i],"<AquariumRoomInstance>k__BackingField")==room)return live.states[i];
    return nullptr;
}
static int p56_bind(void* room,void*& state){
    state=p56_live_state(room);if(!state)return -1;
    F3 focus{};void* floor=nullptr;if(!p53_state_focus(state,focus,floor))return -1;
    refresh_water_targets();P56Bounds bounds[32]{};
    int count=g_waterCount<32?g_waterCount:32;
    for(int i=0;i<count;++i){Bounds6 b{};if(get_renderer_bounds(g_water[i].renderer,b))bounds[i]={b.center.x,b.center.y,b.center.z,b.extents.x,b.extents.y,b.extents.z};}
    return p56_unique_water(focus.x,focus.y,focus.z,bounds,count);
}
static bool p56_target_live(){
    if(!p56_state||p56_live_state(p56_room)!=p56_state)return false;
    // Refresh may reorder renderers. Always recover the locked object by identity.
    if(!p56_alive(p56_water))return false;
    for(int i=0;i<g_waterCount;++i)if(g_water[i].renderer==p56_water){g_waterTarget=i;return true;}
    return false;
}
static bool p64_tick_target_live(u64 now){
    static void* checkedState=nullptr;
    static u64 checkedAt=0;
    static bool valid=false;
    // Every tick: reject a changed inspector selection or destroyed renderer.
    if(!p56_state||!p56_active(p56_inspector)||!p56_opened(p56_inspector)||field_object(p56_inspector,"_room")!=p56_room||!p56_alive(p56_water))return false;
    // The full state walk allocates boxed entries; keep it off the input hot path.
    if(checkedState!=p56_state||now-checkedAt>=250){
        checkedState=p56_state;checkedAt=now;valid=p56_target_live();
    }
    return valid;
}
static void* p59_load_stock_inspector(){
    // Use the game's own cached factory. It registers and retains the inspector
    // without selecting an entity or opening the inspector window.
    auto cls=find_class("TPS.Game.UI","InspectorUI");if(!cls)return nullptr;
    auto create=find_method_name_params(cls,"CreateDynamicInspectorMenu",1);if(!create)return nullptr;
    void* type=il2cpp_type_get_object(il2cpp_class_get_type(cls));void* a[]={type};
    auto arr=(Il2CppArray*)invoke(M_Resources_FindAll,nullptr,a);if(!arr)return nullptr;
    static int attemptedManager=0;static bool hasAttempted=false;
    for(uptr i=0;i<arr->max_length;++i){void* manager=arr->vector[i];
        if(!p56_alive(manager)||!p56_active(manager))continue;
        if(!field_object(manager,"_dynamicInspectors"))continue; // Awake has not completed.
        void* asset=field_object(manager,"_inspectorMenuEnhancerItemRef");if(!asset)continue;
        int id=object_id(manager);if(hasAttempted&&id==attemptedManager)return nullptr;
        attemptedManager=id;hasAttempted=true;
        log_raw("0.7.81 UI LOAD: requesting stock enhancer inspector from InspectorUI cached factory.\r\n");
        void* args[]={asset};Il2CppObject* exception=nullptr;
        void* inspector=il2cpp_runtime_invoke(create,manager,args,&exception);
        if(exception){log_raw("0.7.81 UI LOAD: native factory raised a managed exception; no repeated load attempts this session.\r\n");return nullptr;}
        if(!p56_alive(inspector)){log_raw("0.7.81 UI LOAD: factory returned no live inspector.\r\n");return nullptr;}
        void* button=field_object(inspector,"_buttonColorPicker"),*menu=field_object(inspector,"_itemColorPickerMenu");
        char b[320];psprintf(b,"0.7.81 UI LOAD: inspector=%p colourButton=%p pickerMenu=%p.\r\n",inspector,button,menu);log_raw(b);
        if(p56_alive(button)&&p56_alive(menu))return inspector;
        return nullptr;
    }
    return nullptr;
}
static void* p56_item_inspector(){
    // Enhancer, exhibit and ordinary-item inspectors are siblings, not subclasses
    // of InspectorMenuItem. Scan their shared base and resolve the named fields.
    auto c=find_class("TPS.Game.UI","InspectorMenu");if(!c)return nullptr;
    void* type=il2cpp_type_get_object(il2cpp_class_get_type(c));void* a[]={type};
    auto arr=(Il2CppArray*)invoke(M_Resources_FindAll,nullptr,a);if(!arr)return nullptr;
    auto rootMethod=find_method0_hierarchy(C_Transform,"get_root");
    void* ownTr=p56_alive(p56_inspector)?p56_tr(p56_inspector):nullptr;
    void* ownRoot=ownTr?invoke(rootMethod,ownTr):nullptr;
    int aliveCount=0,fieldCount=0,sceneCount=0;
    for(uptr i=0;i<arr->max_length;++i){void* o=arr->vector[i];
        if(!p56_alive(o))continue;++aliveCount;
        void* button=field_object(o,"_buttonColorPicker"),*menu=field_object(o,"_itemColorPickerMenu");
        if(!p56_alive(button)||!p56_alive(menu))continue;++fieldCount;
        void* tr=p56_tr(o);void* root=tr?invoke(rootMethod,tr):nullptr;
        bool sameHUD=ownRoot&&root==ownRoot;
        if(!sameHUD&&!p57_scene_object(o))continue;++sceneCount;
        if(!(p57_diagnostics&512)){
            p57_diagnostics|=512;char cls[160],b[420];class_full(il2cpp_object_get_class((Il2CppObject*)o),cls,sizeof(cls));
            psprintf(b,"0.7.81 UI DONOR: using %s; sameHUD=%s button=%p menu=%p.\r\n",cls,sameHUD?"yes":"no",button,menu);log_raw(b);
        }
        return o;
    }
    static int previousAlive=-1,previousFields=-1,previousScenes=-1;
    if(aliveCount!=previousAlive||fieldCount!=previousFields||sceneCount!=previousScenes){
        previousAlive=aliveCount;previousFields=fieldCount;previousScenes=sceneCount;
        char b[360];psprintf(b,"0.7.81 UI DONOR: inspectors=%llu alive=%d withButtonAndMenu=%d validSceneOrHUD=%d ownRoot=%p.\r\n",(unsigned long long)arr->max_length,aliveCount,fieldCount,sceneCount,ownRoot);log_raw(b);
    }
    return p59_load_stock_inspector();
}
static void p56_hide(void* component){
    void* go=p56_go(component);if(!p56_alive(go)||p56_hiddenCount==4)return;
    p56_hidden[p56_hiddenCount++]={go,boxed_bool(invoke(M_GameObject_get_activeSelf,go))};p56_hold(go);p56_setActive(go,false);
}
static void p80_update_selection();
static void p80_reticle(void* widget,F4 color){
    void* image=field_object(widget,"_reticleImage");if(!p56_alive(image))return;
    color.w=1.0f;void* a[]={&color};
    invoke(find_method1_hierarchy(il2cpp_object_get_class((Il2CppObject*)image),"set_color","UnityEngine.Color"),image,a);
}
static float p80_lastValue=-1;
static void p56_gradient(){
    if(!p56_menu||!p56_alive(g_pickerWidget))return;
    bool hueChanged=p56_lastHue!=g_manualHue||p56_lastSat!=g_manualSat;
    if(!hueChanged&&p80_lastValue==g_manualVal)return;
    p80_lastValue=g_manualVal;p56_lastHue=g_manualHue;p56_lastSat=g_manualSat;
    set_field_float(g_pickerWidget,"_hue",g_manualHue);set_field_float(g_pickerWidget,"_saturation",g_manualSat);set_field_float(g_pickerWidget,"_value",g_manualVal);
    if(hueChanged)invoke(find_method0_hierarchy(il2cpp_object_get_class((Il2CppObject*)g_pickerWidget),"UpdateValueGradient"),g_pickerWidget);
    p80_reticle(g_pickerWidget,hsv_to_rgb(g_manualHue,g_manualSat,g_manualVal));
    p80_update_selection();
}
static bool p62_clear_click_guard(void* button);
#include "AquariumSwatches.h"

static bool p62_clear_click_guard(void* button){
    // The captured crash is the indirect hasClickAction call in UIButton.PrimaryPress.
    // Use its managed property API, not a raw backing-field write.
    auto cls=il2cpp_object_get_class((Il2CppObject*)button);
    auto setter=find_method1_hierarchy(cls,"set_hasClickAction","System.Func`1<System.Boolean>");
    if(!setter)setter=find_method_name_params(find_class("TPS.Core.UI","UINavigationFocus"),"set_hasClickAction",1);
    auto getter=find_method0_hierarchy(cls,"get_hasClickAction");
    if(!setter||!getter)return false;
    void* args[]={nullptr};
    if(!p56_call(setter,button,args))return false;
    Il2CppObject* ex=nullptr;
    void* guard=il2cpp_runtime_invoke(getter,button,nullptr,&ex);
    if(ex||guard){log_raw("0.7.81 CLICK GUARD: null readback failed; button disabled.\r\n");return false;}
    p57_once(1024,"0.7.81 CLICK GUARD: managed setter completed; getter confirms null callback.\r\n");
    return true;
}
static void p56_add_button(void* inspector){
    if(!p56_ready||!p56_alive(inspector))return;
    p57_once(1,"0.7.81 UI DISCOVERY: reached room inspector button setup.\r\n");
    void* room=field_object(inspector,"_room");bool aquarium=p56_live_state(room)!=nullptr;
    if(aquarium)p57_once(2,"0.7.81 UI DISCOVERY: selected room matches a live AquariumState.\r\n");
    if(p56_inspector!=inspector||!p56_alive(p56_button)){
        if(p56_alive(p56_button))p56_setActive(p56_go(p56_button),false);
        if(p56_buttonHandle)il2cpp_gchandle_free(p56_buttonHandle);
        p56_buttonHandle=0;p56_button=nullptr;p56_inspector=inspector;
        if(!aquarium){p57_once(4,"0.7.81 UI DISCOVERY: room is not yet bound to a live aquarium; retry remains enabled.\r\n");return;}
        void* item=p56_item_inspector();void* donor=item?field_object(item,"_buttonColorPicker"):nullptr;
        void* group=field_object(inspector,"_buttonGroup");void* root=group?field_object(group,"Root"):nullptr;
        if(!donor){p57_once(8,"0.7.81 UI DISCOVERY: stock item colour button not found in a valid scene; retry remains enabled.\r\n");return;}
        if(!root){p57_once(16,"0.7.81 UI DISCOVERY: room button-group root is not ready; retry remains enabled.\r\n");return;}
        // Clone the button component and its GameObject, never the item inspector/controller.
        void* a[]={donor};p56_button=invoke(M_Object_Instantiate,nullptr,a);if(!p56_alive(p56_button)){p57_once(32,"0.7.81 UI DISCOVERY: Object.Instantiate did not return a live button.\r\n");p56_button=nullptr;return;}
        p56_buttonHandle=il2cpp_gchandle_new((Il2CppObject*)p56_button,false);
        const char* events[]={"onClick","onSecondaryClick","onClickWithContext","onSecondaryClickWithContext"};
        for(auto name:events)set_field_ref(p56_button,name,nullptr);
        bool world=false;void* pa[]={root,&world};invoke(M_Transform_SetParent,p56_tr(p56_button),pa);
        auto rename=find_method1_hierarchy(C_Object,"set_name","System.String");void* na[]={il2cpp_string_new("TPM_Aquarium_ChooseColour")};invoke(rename,p56_go(p56_button),na);
        log_raw("0.7.81 UI: cloned stock colour-wheel button into room action group.\r\n");
    }
    if(!p56_button)return;
    void* group=field_object(inspector,"_buttonGroup");void* root=group?field_object(group,"Root"):nullptr;
    if(root){bool world=false;void* a[]={root,&world};invoke(M_Transform_SetParent,p56_tr(p56_button),a);}
    // SetupButton registers the new button in the normal visible-button list and supplies its tooltip.
    auto cls=find_class("TPS.Game.UI","InspectorMenu");auto setup=find_method_name_params(cls,"SetupButton",6);
    bool yes=true,no=false;void* args[]={p56_button,&yes,&aquarium,&no,nullptr,il2cpp_string_new("Choose Colour")};
    if(!p56_call(setup,inspector,args)){p57_once(64,"0.7.81 UI DISCOVERY: native SetupButton failed.\r\n");return;}
    if(!p62_clear_click_guard(p56_button)){p56_setActive(p56_go(p56_button),false);return;}
    invoke(find_method_name_params(cls,"RefreshButtonGridLayout",0),inspector);
    if(aquarium){p56_setActive(p56_go(p56_button),true);p57_once(128,"0.7.81 UI DISCOVERY: native SetupButton completed and colour button activated.\r\n");}
}
static void p57_reconcile(){
    if(!p56_ready||p56_menu||!p56_clock)return;
    // Keep wrappers rooted, validate native Unity lifetime each poll, and
    // release destroyed candidates. RefreshButtons already handles new UI.
    static void* cached[128]{};static AquariumGCHandle roots[128]{};
    static unsigned count=0;static u64 nextScan=0;
    for(unsigned i=0;i<count;){
        if(!p56_alive(cached[i])){
            if(roots[i])il2cpp_gchandle_free(roots[i]);
            --count;cached[i]=cached[count];roots[i]=roots[count];cached[count]=nullptr;roots[count]=0;
        }else ++i;
    }
    auto reconcile=[](void* inspector){
        if(!p56_active(inspector)||!p56_opened(inspector))return false;
        if(!p56_live_state(field_object(inspector,"_room")))return false;
        if(inspector==p56_inspector&&p56_alive(p56_button)&&p56_active(p56_button))return true;
        p56_originalRefresh(inspector,nullptr);p56_add_button(inspector);return true;
    };
    for(unsigned i=0;i<count;++i)if(reconcile(cached[i]))return;
    u64 now=p56_clock();if(now<nextScan)return;nextScan=now+(count?5000:1000);
    auto cls=find_class("TPS.Game.UI","InspectorMenuRoom");if(!cls)return;
    void* type=il2cpp_type_get_object(il2cpp_class_get_type(cls));void* a[]={type};
    auto arr=(Il2CppArray*)invoke(M_Resources_FindAll,nullptr,a);if(!arr||arr->max_length>128)return;
    auto arrayRoot=il2cpp_gchandle_new((Il2CppObject*)arr,false);if(!arrayRoot)return;
    for(uptr i=0;i<arr->max_length;++i){void* inspector=arr->vector[i];
        if(!p56_alive(inspector))continue;bool known=false;
        for(unsigned j=0;j<count;++j)if(cached[j]==inspector)known=true;
        if(!known&&count<128){auto root=il2cpp_gchandle_new((Il2CppObject*)inspector,false);
            if(root){cached[count]=inspector;roots[count++]=root;}}
    }
    il2cpp_gchandle_free(arrayRoot);
    for(unsigned i=0;i<count;++i)if(reconcile(cached[i]))return;
}

static bool p63_menu_callback(void* menu,const char* setter,const char* getter,void* value){
    auto cls=find_class("TPS.Game.UI","MenuBase");
    auto set=find_method_name_params(cls,setter,1);
    auto get=find_method_name_params(cls,getter,0);
    if(!set||!get)return false;
    void* args[]={value};if(!p56_call(set,menu,args))return false;
    Il2CppObject* ex=nullptr;
    void* actual=il2cpp_runtime_invoke(get,menu,nullptr,&ex);
    return !ex&&actual==value;
}
#include "AquariumWindowHost.h"
static void p56_begin(){
    if(!p70_game_foreground()||!p70_sync_scope()){log_raw("0.7.81 UI: waiting for focused game and valid museum storage.\r\n");return;}

    log_raw("0.7.81 UI OPEN 02: checking inspector and target.\r\n");
    if(p56_menu||g_pickerOpen){log_raw("0.7.81 UI OPEN blocked: editor already tracked as open.\r\n");return;}
    if(!p56_opened(p56_inspector)){log_raw("0.7.81 UI OPEN blocked: aquarium inspector is not open.\r\n");return;}
    void* room=field_object(p56_inspector,"_room"),*state=nullptr;
    int index=p56_bind(room,state);if(index<0){log_raw("0.7.81 UI: selected room has no unique live water target; no colour changed.\r\n");return;}
    void* item=p56_item_inspector();void* menu=item?field_object(item,"_itemColorPickerMenu"):nullptr;
    if(!p56_alive(menu)||!p56_baseOpen||!p56_baseClose){log_raw("0.7.81 UI OPEN blocked: picker or base API unavailable.\r\n");return;}
    if(p56_opened(menu)){log_raw("0.7.81 UI OPEN blocked: stock picker already open.\r\n");return;}
    void* widget=field_object(menu,"_widgetColorPicker");if(!p56_alive(widget))return;
    log_raw("0.7.81 UI OPEN 03: target and stock picker found; preparing callbacks.\r\n");
    auto menuClass=find_class("TPS.Game.UI","MenuBase");
    p56_savedOpen=invoke(find_method_name_params(menuClass,"get_OnOpen",0),menu);
    p56_savedClosed=invoke(find_method_name_params(menuClass,"get_OnClosed",0),menu);
    p56_hold(p56_savedOpen);p56_hold(p56_savedClosed);
    if(!p63_menu_callback(menu,"set_OnOpen","get_OnOpen",nullptr)||!p63_menu_callback(menu,"set_OnClosed","get_OnClosed",nullptr)){
        p63_menu_callback(menu,"set_OnOpen","get_OnOpen",p56_savedOpen);
        p63_menu_callback(menu,"set_OnClosed","get_OnClosed",p56_savedClosed);
        p56_release();log_raw("0.7.81 UI OPEN stopped: menu callback verification failed.\r\n");return;
    }
    capture_rect_state(p56_tr(widget),p65_stockPickerRect);
    p56_menu=menu;p56_room=room;p56_state=state;p56_water=g_water[index].renderer;
    p56_entity=field_object(p56_inspector,"<InspectedEntity>k__BackingField");
    p56_hold(menu);p56_hold(room);p56_hold(state);p56_hold(p56_water);p56_hold(p56_entity);p56_hold(p56_inspector);
    log_raw("0.7.81 UI OPEN 04: callbacks verified; preparing preview and controls.\r\n");
    p56_hiddenCount=0;
    p56_hide(field_object(menu,"_widgetRenderPrefab"));p56_hide(field_object(menu,"_widgetColorSwatch"));
    p56_hide(field_object(menu,"_buttonAdd"));p56_hide(field_object(menu,"_buttonDelete"));
    p67_open_swatches(menu);
    // The inspector icon is driven by its owning controller. A detached clone
    // has no populated texture and renders a white rectangle. Omit that preview.
    log_raw("0.7.81 UI OPEN 05: preview prepared; setting reset control.\r\n");
    void* reset=field_object(menu,"_buttonReset");p56_resetState=field_i32(reset,"<CurrentState>k__BackingField");
    if(reset){bool no=false;void* a[]={&no,&no};invoke(find_method_name_params(il2cpp_object_get_class((Il2CppObject*)reset),"SetButtonStatus",2),reset,a);}
    g_waterTarget=index;
    // Prepare the proven manual wheel in place BEFORE opening the native window.
    g_pickerWidget=widget;g_pickerWidgetGO=p56_go(widget);g_pickerWidgetTransform=p56_tr(widget);
    g_pickerOriginalParent=invoke(M_Transform_get_parent,g_pickerWidgetTransform);
    g_pickerHostParent=g_pickerOriginalParent;
    g_pickerOriginalActiveSelf=boxed_bool(invoke(M_GameObject_get_activeSelf,g_pickerWidgetGO));
    g_pickerOriginalCallback=field_object(widget,"OnColorPicked");
    p56_savedHue=field_float(widget,"_hue");p56_savedSat=field_float(widget,"_saturation");p56_savedVal=field_float(widget,"_value");p56_lastHue=p56_lastSat=-1;
    log_raw("0.7.81 UI OPEN 06: preparing wheel interaction.\r\n");
    neutralize_picker_interaction();p56_setActive(g_pickerWidgetGO,true);
    if(!resolve_picker_manual_visuals()){p56_end(false,false);return;}
    WaterTarget& wt=g_water[index];g_currentWaterColor=wt.hasCustom?wt.customColor:read_mat_color(wt.originalMaterial,il2cpp_string_new("_Albedo"));
    rgb_to_hsv(g_currentWaterColor,g_manualHue,g_manualSat,g_manualVal);
    g_pickerChanged=false;g_pickerDragMode=0;g_manualMouseWasDown=true;g_pickerOpen=true;
    // Keep the room inspector from competing for input while its editor is open.
    p67_inspectorInteractive=boxed_bool(field_object(p56_inspector,"_isInteractive"),true);
    bool notInteractive=false;void* interactiveArgs[]={&notInteractive};
    p67_changedInspector=p56_call(find_method_name_params(find_class("TPS.Game.UI","InspectorMenu"),"SetInteractive",1),p56_inspector,interactiveArgs);
    if(!p68_prepare_window(menu)){log_raw("0.7.81 UI OPEN stopped: no active window host.\r\n");p56_end(false,false);return;}
    // Deliberately call MenuBaseUIPanel.OpenMenu, not the item-specific overload.
    log_raw("0.7.81 UI OPEN 07: opening native panel.\r\n");
    if(!p56_call(p56_baseOpen,menu)){p56_end(false,false);return;}
    // A fresh session may not yet have run the disabled widget's Start. Its verified
    // implementation builds textures only; it does not edit an item or invoke a colour callback.
    if(!field_object(widget,"_wheelTexture")||!field_object(widget,"_valueTexture")){
        if(!p56_call(find_method0_hierarchy(il2cpp_object_get_class((Il2CppObject*)widget),"Start"),widget)){p56_end(false,false);return;}
    }
    // Panel animation may modify navigation state; keep only manual wheel/slider input active.
    bool off=false;void* disabled[]={&off};invoke(M_Behaviour_set_enabled,widget,disabled);
    for(int i=0;i<g_pickerBehaviourCount;++i)invoke(M_Behaviour_set_enabled,g_pickerBehaviours[i].behaviour,disabled);
    log_raw("0.7.81 UI OPEN 08: panel open; refreshing wheel visuals.\r\n");
    if(!p56_active(menu)||!p56_active(widget)){log_raw("0.7.81 UI OPEN stopped: window remained invisible; clearing editor state.\r\n");p56_end(false,false);return;}
    p56_gradient();update_manual_picker_visuals();
    char status[320];psprintf(status,"0.7.81 UI OPEN STATE: menuActive=%d menuOpened=%d widgetActive=%d selectedRoomMatches=%d.\r\n",(int)p56_active(menu),(int)p56_opened(menu),(int)p56_active(widget),(int)(field_object(p56_inspector,"_room")==p56_room));log_raw(status);
    log_raw("0.7.81 UI: native Choose Colour frame opened for selected aquarium; no colour applied at open.\r\n");
}
static void p56_end(bool returnToRoom,bool save){
    if(!p56_menu||p56_closing)return;p56_closing=true;g_pickerSampling=false;
    void* menu=p56_menu;bool live=p56_target_live();
    if(!save||!live){g_pickerChanged=false;p65_glowPending=false;}
    else p65_flush_glow();
    // Close native panels before restoring the wheel's retail input behaviours.
    if(p56_alive(menu))p56_call(p56_baseClose,menu);
    if(p56_alive(g_pickerWidget)){
        set_field_float(g_pickerWidget,"_hue",p56_savedHue);set_field_float(g_pickerWidget,"_saturation",p56_savedSat);set_field_float(g_pickerWidget,"_value",p56_savedVal);
        invoke(find_method0_hierarchy(il2cpp_object_get_class((Il2CppObject*)g_pickerWidget),"UpdateValueGradient"),g_pickerWidget);
        p80_reticle(g_pickerWidget,hsv_to_rgb(p56_savedHue,p56_savedSat,p56_savedVal));
    }
    if(p56_alive(g_pickerWidget)){
        void* widgetTransform=g_pickerWidgetTransform;
        // The native widget was never detached. Do not use the retired world-space reparenting cleanup.
        g_pickerOriginalParent=nullptr;
        close_stock_picker(true);
        restore_rect_state(widgetTransform,p65_stockPickerRect);
    }else {g_pickerOpen=false;g_pickerChanged=false;g_pickerBehaviourCount=0;}
    p65_stockPickerRect=RectState{};
    for(int i=0;i<p56_hiddenCount;++i)p56_setActive(p56_hidden[i].go,p56_hidden[i].active);
    p56_hiddenCount=0;
    p67_close_swatches();
    if(p56_alive(p56_preview)){void* a[]={p56_go(p56_preview)};invoke(find_method_param1(C_Object,"Destroy","UnityEngine.Object"),nullptr,a);}p56_preview=nullptr;
    if(p56_alive(menu)){void* reset=field_object(menu,"_buttonReset");if(p56_alive(reset)){void* a[]={&p56_resetState};invoke(find_method_name_params(il2cpp_object_get_class((Il2CppObject*)reset),"SetButtonStatus",1),reset,a);}}
    if(p56_alive(menu)){
        bool openRestored=p63_menu_callback(menu,"set_OnOpen","get_OnOpen",p56_savedOpen);
        bool closedRestored=p63_menu_callback(menu,"set_OnClosed","get_OnClosed",p56_savedClosed);
        if(!openRestored||!closedRestored)log_raw("0.7.81 UI CLOSE: menu callback restoration failed.\r\n");
    }
    p68_restore_window(menu);
    p56_menu=nullptr;
    // Do not reopen/reselect the room: OpenMenu recentres the camera.
    (void)returnToRoom;
    if(p67_changedInspector&&p56_alive(p56_inspector)){
        void* a[]={&p67_inspectorInteractive};
        p56_call(find_method_name_params(find_class("TPS.Game.UI","InspectorMenu"),"SetInteractive",1),p56_inspector,a);
    }
    p67_changedInspector=false;
    p56_room=p56_state=p56_water=p56_entity=nullptr;p56_savedOpen=p56_savedClosed=nullptr;p56_release();p56_closing=false;
    log_raw("0.7.81 UI: aquarium editor closed; retail item menu restored.\r\n");
}
static void p56_reset(){
    if(!p56_target_live())return;
    void* water=p56_water;
    // Reset closes the editor; invalidate its dirty flag so closing cannot resurrect the colour.
    g_pickerChanged=false;p48_clear_standard_floor_sink_if_selected(true);
    // Existing Dredge definitions are shared; only invoke that route for a special aquarium.
    if(field_object(p56_state,"_highestPriorityAquariumTintDefinition"))restore_live_glow_tints(true);
    p56_end(true,false);
    refresh_water_targets();
    for(int i=0;i<g_waterCount;++i)if(g_water[i].renderer==water){g_waterTarget=i;reset_water_target();return;}
    log_raw("0.7.81 RESET: selected water disappeared; no other tank was reset.\r\n");
}
static bool p56_press(void* self){
    if(!p56_ready)return false;
    if(self&&self==p56_button){
        log_raw("0.7.81 UI OPEN 01: button handler reached.\r\n");
        if(p56_active(self))p56_begin();
        return true;
    }
    if(p80_edit_press(self))return true;
    if(p67_swatch_press(self))return true;
    if(p56_menu&&self==field_object(p56_menu,"_buttonReset")){p56_reset();return true;}
    if(p56_menu&&self==field_object(p56_menu,"_buttonClose")){log_raw("0.7.81 UI CLOSE reason: X button.\r\n");p56_end(true,true);return true;}
    return false;
}
static void p56_press_hook(void* self,const MethodInfo* m){if(!p78_main()){p56_originalPress(self,m);return;}P78WorkScope work;if(!p56_press(self))p56_originalPress(self,m);}
static void p56_force_hook(void* self,const MethodInfo* m){if(!p78_main()){p56_originalForce(self,m);return;}P78WorkScope work;if(!p56_press(self))p56_originalForce(self,m);}
static void p56_refresh_hook(void* self,const MethodInfo* m){
    p56_originalRefresh(self,m);
    if(!p78_main())return;P78WorkScope work;
    void* room=field_object(self,"_room");
    if(p56_menu&&!p56_closing&&room&&room!=p56_room){log_raw("0.7.81 UI CLOSE reason: inspector changed rooms.\r\n");p56_end(false,true);}
    p56_add_button(self);
}
static void p56_update_hook(void* self,const MethodInfo* m){p56_originalUpdate(self,m);if(p78_main()){P78WorkScope work;if(p56_ready)p56_tick();}}
static void p56_input_hook(void* self,const MethodInfo* m){
    if(!p78_main()){p56_originalInput(self,m);return;}P78WorkScope work;
    if(self==p56_menu){p56_call(p56_baseInput,self);return;}p56_originalInput(self,m);
}
static void p56_close_hook(void* self,void* requester,const MethodInfo* m){
    if(!p78_main()){p56_originalClose(self,requester,m);return;}P78WorkScope work;
    if(self==p56_menu){
        if(!p56_closing){log_raw("0.7.81 UI CLOSE reason: native CloseMenu request.\r\n");p56_end(false,true);}
        else {void* a[]={requester};p56_call(p56_baseCloseRequest,self,a);}
        return;
    }p56_originalClose(self,requester,m);
}
struct P56Hook{const char* ns;const char* cls;const char* method;int argc;const u8* bytes;int size;void* replacement;void** original;bool ripR10;u8* target;u8* trampoline;u8 originalBytes[16];};
static bool p56_install(){
    if(!p78_main())return false;
    p56_aliveMethod=find_method_param1(C_Object,"op_Implicit","UnityEngine.Object");
    auto base=find_class("TPS.Game.UI","MenuBaseUIPanel");p56_baseOpen=find_method_name_params(base,"OpenMenu",0);p56_baseClose=find_method_name_params(base,"CloseMenuImmediately",0);
    p56_baseCloseRequest=find_method_name_params(base,"CloseMenu",1);
    p56_baseInput=find_method_name_params(find_class("TPS.Game.UI","MenuBase"),"UpdateInput",0);
    p56_clock=(u64(*)())resolve_export(find_loaded_module("kernel32.dll"),"GetTickCount64");
    if(!p56_aliveMethod||!p56_baseOpen||!p56_baseClose||!p56_baseCloseRequest||!p56_baseInput||!p56_clock)return false;
    static const u8 update[]={0x48,0x89,0x6c,0x24,0x18,0x57,0x48,0x83,0xec,0x20,0x48,0x8b,0x11,0x48,0x8b,0xf9};
    static const u8 refresh[]={0x48,0x89,0x5c,0x24,0x10,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57};
    static const u8 close[]={0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x6c,0x24,0x18,0x48,0x89,0x74,0x24,0x20};
    static const u8 input[]={0x40,0x56,0x57,0x48,0x83,0xec,0x38,0x4c,0x8b,0x15,0xba,0x1f,0x9a,0x03};
    static const u8 press[]={0x40,0x53,0x48,0x83,0xec,0x20,0x48,0x83,0xb9,0xf8,0,0,0,0};
    static const u8 force[]={0x40,0x53,0x48,0x83,0xec,0x20,0x48,0x8b,0xd9,0x48,0x8b,0x49,0x60,0x48,0x85,0xc9};
    P56Hook hooks[]={
        {"TPS.Game.UI","MenuBase","Update",0,update,16,(void*)p56_update_hook,(void**)&p56_originalUpdate},
        {"TPS.Game.UI","InspectorMenuRoom","RefreshButtons",0,refresh,16,(void*)p56_refresh_hook,(void**)&p56_originalRefresh},
        {"TPS.Game.UI","ItemColorPickerMenu","CloseMenu",1,close,15,(void*)p56_close_hook,(void**)&p56_originalClose},
        {"TPS.Game.UI","ItemColorPickerMenu","UpdateInput",0,input,14,(void*)p56_input_hook,(void**)&p56_originalInput,true},
        {"TPS.Core.UI","UINavigationFocus","PrimaryPress",0,press,14,(void*)p56_press_hook,(void**)&p56_originalPress},
        {"TPS.Core.UI","UINavigationFocus","ForcePrimaryPress",0,force,16,(void*)p56_force_hook,(void**)&p56_originalForce}
    };
    auto abandon=[&](){for(auto& h:hooks){if(h.trampoline)p78_free_trampoline(h.trampoline);*h.original=nullptr;}return false;};
    // Check EVERY target before modifying any code. Unsupported game builds fail closed.
    for(auto& h:hooks){auto c=find_class(h.ns,h.cls);auto m=c?find_method_name_params(c,h.method,h.argc):nullptr;h.target=m?(u8*)*(void* const*)m:nullptr;
        if(!h.target||!ptr_in_module(h.target,find_loaded_module("GameAssembly.dll"),32))return abandon();
        // Only the disp32 operand may vary; all instruction bytes remain exact.
        int checkedBytes=h.ripR10?10:h.size;
        for(int i=0;i<checkedBytes;++i)if(h.target[i]!=h.bytes[i]){char diag[300];psprintf(diag,"1.0.1-test UI: %s.%s prologue mismatch at +%d (got %02X expected %02X); no UI hooks installed.\r\n",h.cls,h.method,i,(unsigned)h.target[i],(unsigned)h.bytes[i]);log_raw(diag);return abandon();}
        if(h.ripR10){
            void* ga=find_loaded_module("GameAssembly.dll");uptr operand=0;
            if(!p91_input_operand(h.target,(uptr)ga,pe_image_size(ga),operand)){
                log_raw("1.0.1-test UI: UpdateInput RIP operand outside GameAssembly; no UI hooks installed.\r\n");return abandon();
            }
            using QueryFn=usize(*)(const void*,P91MemoryInfo*,usize);
            auto query=(QueryFn)resolve_export(find_loaded_module("kernel32.dll"),"VirtualQuery");
            P91MemoryInfo memory{};
            if(!query || query((void*)operand,&memory,sizeof(memory))!=sizeof(memory) || !p91_operand_readable(operand,(uptr)ga,memory)){
                log_raw("1.0.1-test UI: UpdateInput RIP operand is not readable GameAssembly image memory; no UI hooks installed.\r\n");return abandon();
            }
            char diag[300];psprintf(diag,"1.0.1-test UI: UpdateInput signature accepted; RIP bytes=%02X %02X %02X %02X; operand RVA=0x%X validated.\r\n",(unsigned)h.target[10],(unsigned)h.target[11],(unsigned)h.target[12],(unsigned)h.target[13],(unsigned)(operand-(uptr)ga));log_raw(diag);
        }
        memcpy(h.originalBytes,h.target,h.size);
        h.trampoline=(u8*)pVirtualAlloc(nullptr,64,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);if(!h.trampoline)return abandon();
        int n=h.size;
        if(h.ripR10){
            // Relocate mov r10,[rip+disp32] without a +/-2GB allocation assumption.
            memcpy(h.trampoline,h.target,7);h.trampoline[7]=0x49;h.trampoline[8]=0xba;
            uptr address=(uptr)(h.target+14)+(i64)*(i32*)(h.target+10);memcpy(h.trampoline+9,&address,8);
            h.trampoline[17]=0x4d;h.trampoline[18]=0x8b;h.trampoline[19]=0x12;n=20;
        }else memcpy(h.trampoline,h.target,n);
        p51_write_abs_jump(h.trampoline+n,h.target+h.size);if(!p78_seal_trampoline(h.trampoline,n+14))return abandon();
        *h.original=h.trampoline;
    }
    int patched=0;
    for(auto& h:hooks){DWORD old=0;if(!pVirtualProtect(h.target,h.size,PAGE_EXECUTE_READWRITE,&old))break;
        p51_write_abs_jump(h.target,h.replacement);for(int i=14;i<h.size;++i)h.target[i]=0x90;
        pFlushInstructionCache(pGetCurrentProcess(),h.target,h.size);DWORD ignored;pVirtualProtect(h.target,h.size,old,&ignored);++patched;
    }
    if(patched!=6){for(int i=0;i<patched;++i){auto& h=hooks[i];DWORD old=0;if(pVirtualProtect(h.target,h.size,PAGE_EXECUTE_READWRITE,&old)){memcpy(h.target,h.originalBytes,h.size);pFlushInstructionCache(pGetCurrentProcess(),h.target,h.size);DWORD ignored;pVirtualProtect(h.target,h.size,old,&ignored);}}return false;}
    p56_ready=true;log_raw("1.0.1-test UI: six guarded hooks installed on game thread; native aquarium button enabled.\r\n");return true;
}
