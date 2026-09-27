// Temporarily host the borrowed window under an active HUD ancestor, never activate an item inspector.
static void* p68_windowParent=nullptr;
static RectState p68_windowRect{};
static bool p68_windowActive=false,p68_windowPrepared=false,p68_windowMoved=false;
static void p68_restore_window(void* menu){
    if(!p68_windowPrepared)return;
    if(p56_alive(menu)){
        if(p68_windowMoved&&p56_alive(p68_windowParent)){
            bool world=false;void* a[]={p68_windowParent,&world};
            invoke(M_Transform_SetParent,p56_tr(menu),a);
            restore_rect_state(p56_tr(menu),p68_windowRect);
        }
        p56_setActive(p56_go(menu),p68_windowActive);
    }
    p68_windowPrepared=false;p68_windowMoved=false;p68_windowParent=nullptr;
}
static bool p68_prepare_window(void* menu){
    void* tr=p56_tr(menu);void* go=p56_go(menu);
    if(!tr||!go)return false;
    p68_windowParent=invoke(M_Transform_get_parent,tr);
    if(!p56_alive(p68_windowParent)||!capture_rect_state(tr,p68_windowRect))return false;
    p68_windowActive=boxed_bool(invoke(M_GameObject_get_activeSelf,go));
    p68_windowPrepared=true;p68_windowMoved=false;p56_hold(p68_windowParent);
    if(!p56_active(p68_windowParent)){
        void* host=nearest_active_parent(tr);
        if(!p56_alive(host)){p68_restore_window(menu);return false;}
        bool world=false;void* a[]={host,&world};
        invoke(M_Transform_SetParent,tr,a);p68_windowMoved=true;
        restore_rect_state(tr,p68_windowRect);
    }
    p56_setActive(go,true);
    if(!p56_active(menu)){p68_restore_window(menu);return false;}
    log_raw("0.7.81 WINDOW HOST: menu is active; original parent and activation saved.\r\n");
    return true;
}
