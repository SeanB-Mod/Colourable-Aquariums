// Isolated stock controls editing the game's shared Metagame.SwatchColors palette.
static void* p67_swatchWidget=nullptr;
static void* p67_swatchButtons[32]{};
static F4 p67_swatchColors[32]{};
static int p67_swatchCount=0;
static void* p80_add=nullptr,*p80_delete=nullptr,*p80_palette=nullptr;
static int p80_selected=-1,p80_capacity=0;
static bool p80_paletteReady=false;
static bool p80_externalPalette=false;
static bool p80_same_rgb(F4 a,F4 b){return fdiff(a.x,b.x)<0.00001f&&fdiff(a.y,b.y)<0.00001f&&fdiff(a.z,b.z)<0.00001f;}
static void* p80_metagame(){auto c=find_class("TPS.Game","Metagame");return c?static_ref_field(c,"<Instance>k__BackingField"):nullptr;}
static void p80_lock_button(void* button,bool locked){
    if(!p56_alive(button))return;bool selected=false;void* a[]={&locked,&selected};
    p56_call(find_method_name_params(il2cpp_object_get_class((Il2CppObject*)button),"SetButtonStatus",2),button,a);
}
static void p80_button_states(){
    p80_lock_button(p80_add,!p80_paletteReady||p67_swatchCount>=p80_capacity);
    p80_lock_button(p80_delete,!p80_paletteReady||p80_selected<0||p80_selected>=p67_swatchCount);
    if(p56_alive(p67_swatchWidget)){void* a[]={&p80_selected};p56_call(find_method_name_params(il2cpp_object_get_class((Il2CppObject*)p67_swatchWidget),"RefreshButtonStates",1),p67_swatchWidget,a);}
}
static void* p80_clone_button(void* donor){
    if(!p56_alive(donor))return nullptr;RectState rect{};void* tr=p56_tr(donor);void* parent=invoke(M_Transform_get_parent,tr);
    if(!parent||!capture_rect_state(tr,rect))return nullptr;
    int index=boxed_i32(invoke(find_method0_hierarchy(il2cpp_object_get_class((Il2CppObject*)tr),"GetSiblingIndex"),tr),-1);
    void* a[]={donor};void* button=invoke(M_Object_Instantiate,nullptr,a);if(!p56_alive(button))return nullptr;p56_hold(button);
    const char* events[]={"onClick","onSecondaryClick","onClickWithContext","onSecondaryClickWithContext"};for(auto name:events)set_field_ref(button,name,nullptr);
    bool no=false;void* pa[]={parent,&no};invoke(M_Transform_SetParent,p56_tr(button),pa);restore_rect_state(p56_tr(button),rect);
    if(index>=0){++index;void* ia[]={&index};invoke(find_method_name_params(il2cpp_object_get_class((Il2CppObject*)tr),"SetSiblingIndex",1),p56_tr(button),ia);}
    if(!p62_clear_click_guard(button)){p56_setActive(p56_go(button),false);void* da[]={p56_go(button)};invoke(find_method_param1(C_Object,"Destroy","UnityEngine.Object"),nullptr,da);return nullptr;}
    p56_setActive(p56_go(button),true);return button;
}
static void p80_refresh_swatches(){
    if(p80_externalPalette)return;
    p67_swatchCount=0;p80_paletteReady=false;
    void* meta=p80_metagame();void* colors=meta?field_object(meta,"SwatchColors"):nullptr;
    if(!colors||!p56_alive(p67_swatchWidget)){p80_selected=-1;p80_button_states();return;}
    if(p80_palette!=colors){p80_palette=colors;p80_selected=-1;}
    int count=collection_count(colors);if(count<0){p80_button_states();return;}
    auto cls=il2cpp_object_get_class((Il2CppObject*)p67_swatchWidget);void* setup[]={colors,&p80_selected};
    if(!p56_call(find_method_name_params(cls,"Set",2),p67_swatchWidget,setup)){p80_button_states();return;}
    auto buttons=(Il2CppArray*)field_object(p67_swatchWidget,"_buttons");
    auto item=find_method_name_params(il2cpp_object_get_class((Il2CppObject*)colors),"get_Item",1);
    if(!buttons||!item){p80_button_states();return;}
    p80_capacity=buttons->max_length<32?(int)buttons->max_length:32;
    bool complete=count<=p80_capacity;
    for(uptr i=0;i<buttons->max_length;++i){
        void* wrapper=buttons->vector[i];if(!p56_alive(wrapper)){if(i<(uptr)count)complete=false;continue;}
        p56_setActive(p56_go(wrapper),true);
        if(i>=(uptr)count||i>=32){p56_setActive(p56_go(field_object(wrapper,"_rootContent")),false);continue;}
        int index=(int)i;void* a[]={&index};auto boxed=invoke(item,colors,a);void* raw=boxed?il2cpp_object_unbox(boxed):nullptr;
        void* button=field_object(wrapper,"_button");
        if(!raw||!p56_alive(button)||!p62_clear_click_guard(button)){complete=false;p56_setActive(p56_go(wrapper),false);continue;}
        p67_swatchColors[i]=*(F4*)raw;p67_swatchButtons[i]=button;++p67_swatchCount;
    }
    if(!complete||p67_swatchCount!=count){p67_swatchCount=0;p80_selected=-1;}
    else p80_paletteReady=true;
    if(p80_selected>=p67_swatchCount)p80_selected=-1;
    p56_setActive(p56_go(p67_swatchWidget),true);p80_button_states();
}
static bool p80_palette_matches(){
    void* meta=p80_metagame();void* colors=meta?field_object(meta,"SwatchColors"):nullptr;
    if(!p80_paletteReady||colors!=p80_palette||collection_count(colors)!=p67_swatchCount)return false;
    auto item=find_method_name_params(il2cpp_object_get_class((Il2CppObject*)colors),"get_Item",1);if(!item)return false;
    for(int i=0;i<p67_swatchCount;++i){void* a[]={&i};auto box=invoke(item,colors,a);void* raw=box?il2cpp_object_unbox(box):nullptr;if(!raw)return false;
        F4 c=*(F4*)raw;if(!p80_same_rgb(c,p67_swatchColors[i])||fdiff(c.w,p67_swatchColors[i].w)>0.00001f)return false;
    }return true;
}
// Like retail OnColorPicked: editing while a swatch is selected edits that swatch.
static void p80_update_selection(){
    if(p80_externalPalette)return;
    if(!p56_menu||p80_selected<0)return;
    if(!p56_target_live())return;
    if(p80_selected>=p67_swatchCount||!p80_palette_matches()){p80_selected=-1;p80_refresh_swatches();return;}
    if(p80_same_rgb(g_currentWaterColor,p67_swatchColors[p80_selected]))return;
    void* meta=p80_metagame();auto dirty=meta?find_field_hierarchy(il2cpp_object_get_class((Il2CppObject*)meta),"_hasUnsavedChangesLowImportance"):nullptr;
    auto setter=find_method_name_params(il2cpp_object_get_class((Il2CppObject*)p80_palette),"set_Item",2);
    F4 color=g_currentWaterColor;color.w=1.0f;int index=p80_selected;void* a[]={&index,&color};
    if(!dirty||!il2cpp_field_set_value||!p56_call(setter,p80_palette,a)){
        p80_selected=-1;p80_button_states();log_raw("SWATCH EDIT: selected swatch update failed; selection cleared.\r\n");return;
    }
    bool yes=true;il2cpp_field_set_value((Il2CppObject*)meta,dirty,&yes);
    p67_swatchColors[index]=color;
    // Repaint just the selected tile, avoiding a full palette rebuild on every drag tick.
    auto buttons=(Il2CppArray*)field_object(p67_swatchWidget,"_buttons");
    if(buttons&&(uptr)index<buttons->max_length){void* wrapper=buttons->vector[index];void* image=p56_alive(wrapper)?field_object(wrapper,"_imageColor"):nullptr;
        if(p56_alive(image)){void* ca[]={&color};invoke(find_method1_hierarchy(il2cpp_object_get_class((Il2CppObject*)image),"set_color","UnityEngine.Color"),image,ca);}
    }
}
static void p67_open_swatches(void* menu){
    p80_externalPalette=false;
    p67_swatchCount=0;p80_selected=-1;p80_palette=nullptr;p80_paletteReady=false;p80_capacity=0;
    void* donor=field_object(menu,"_widgetColorSwatch");if(!p56_alive(donor))return;
    RectState rect{};void* parent=invoke(M_Transform_get_parent,p56_tr(donor));if(!parent||!capture_rect_state(p56_tr(donor),rect))return;
    void* args[]={donor};p67_swatchWidget=invoke(M_Object_Instantiate,nullptr,args);if(!p56_alive(p67_swatchWidget)){p67_swatchWidget=nullptr;return;}p56_hold(p67_swatchWidget);
    bool no=false;void* pa[]={parent,&no};invoke(M_Transform_SetParent,p56_tr(p67_swatchWidget),pa);restore_rect_state(p56_tr(p67_swatchWidget),rect);
    p80_add=p80_clone_button(field_object(menu,"_buttonAdd"));p80_delete=p80_clone_button(field_object(menu,"_buttonDelete"));
    p80_refresh_swatches();
}
static bool p80_edit_press(void* button){
    if(!p56_menu||!button||(button!=p80_add&&button!=p80_delete))return false;
    if(p80_externalPalette)return true;
    if(!p56_target_live())return true;
    if(!p80_palette_matches()){p80_selected=-1;p80_refresh_swatches();return true;}
    bool add=button==p80_add;int count=p67_swatchCount;
    if((add&&count>=p80_capacity)||(!add&&(p80_selected<0||p80_selected>=count)))return true;
    void* meta=p80_metagame();auto dirty=meta?find_field_hierarchy(il2cpp_object_get_class((Il2CppObject*)meta),"_hasUnsavedChangesLowImportance"):nullptr;
    if(!dirty||!il2cpp_field_set_value){log_raw("SWATCH EDIT: save marker unavailable; palette unchanged.\r\n");return true;}
    auto cls=il2cpp_object_get_class((Il2CppObject*)p80_palette);
    F4 color=g_currentWaterColor;color.w=1.0f;int index=p80_selected;void* a[]={add?(void*)&color:(void*)&index};
    auto method=find_method_name_params(cls,add?"Add":"RemoveAt",1);
    if(!p56_call(method,p80_palette,a)||collection_count(p80_palette)!=count+(add?1:-1)){log_raw("SWATCH EDIT: palette update failed.\r\n");p80_selected=-1;p80_refresh_swatches();return true;}
    bool yes=true;il2cpp_field_set_value((Il2CppObject*)meta,dirty,&yes);
    p80_selected=add?count:-1;p80_refresh_swatches();
    log_raw(add?"SWATCH EDIT: aquarium colour added to shared game palette.\r\n":"SWATCH EDIT: selected shared swatch deleted; aquarium colour retained.\r\n");return true;
}
static bool p67_swatch_press(void* button){
    if(!p56_menu)return false;
    for(int i=0;i<p67_swatchCount;++i)if(button==p67_swatchButtons[i]){
        if(p80_externalPalette)return true;
        if(!p56_target_live())return true;
        if(!p80_palette_matches()){p80_selected=-1;p80_refresh_swatches();return true;}
        p80_selected=i;
        rgb_to_hsv(p67_swatchColors[i],g_manualHue,g_manualSat,g_manualVal);
        apply_manual_hsv(true);p56_gradient();p65_flush_glow();persist_current_target(true);p80_button_states();
        log_raw("SWATCHES: stored colour applied to selected aquarium only.\r\n");return true;
    }return false;
}
static void p67_close_swatches(){
    p80_externalPalette=false;
    p67_swatchCount=0;p80_selected=-1;p80_paletteReady=false;p80_palette=nullptr;p80_capacity=0;
    void* owned[]={p67_swatchWidget,p80_add,p80_delete};
    for(void* component:owned)if(p56_alive(component)){p56_setActive(p56_go(component),false);void* a[]={p56_go(component)};invoke(find_method_param1(C_Object,"Destroy","UnityEngine.Object"),nullptr,a);}
    p67_swatchWidget=nullptr;p80_add=nullptr;p80_delete=nullptr;
}
