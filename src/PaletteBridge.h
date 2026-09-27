// Optional palette ownership handshake. No lookup/import of Colour Picker.
// 0 query, 1 acquire, 2 release; return 0 not owned, 1 handled, -1 unsafe.
extern "C" __declspec(dllexport) int TPM_Aquarium_PaletteBridgeV1(void* menu,unsigned operation){
    if(!menu||menu!=p56_menu)return 0;
    if(!p78_main()||operation>2)return -1;
    P78WorkScope work;
    if(operation==2){
        if(!p80_externalPalette)return 1;
        p80_externalPalette=false;p80_selected=-1;
        if(!p56_closing&&g_pickerOpen&&p56_alive(menu)&&p56_opened(menu)){
            p80_refresh_swatches();
            void* controls[]={p80_add,p80_delete};for(void* c:controls)if(p56_alive(c))p56_setActive(p56_go(c),true);
        }
        log_raw("PALETTE BRIDGE V1: standalone swatches restored.\r\n");return 1;
    }
    if(p56_closing||!g_pickerOpen||!p56_alive(menu)||!p56_opened(menu)||!p56_target_live())return -1;
    if(!p56_active(p56_inspector)||field_object(p56_inspector,"_room")!=p56_room)return -1;
    if(operation==0)return 1;
    if(!p56_alive(p67_swatchWidget)||!p56_alive(p80_add)||!p56_alive(p80_delete))return -1;
    p80_externalPalette=true;p80_selected=-1;
    void* controls[]={p67_swatchWidget,p80_add,p80_delete};
    for(void* c:controls)p56_setActive(p56_go(c),false);
    log_raw("PALETTE BRIDGE V1: external palette owns swatch controls.\r\n");return 1;
}
