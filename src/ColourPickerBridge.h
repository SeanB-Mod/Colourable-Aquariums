// Optional ABI: 0 not owned, 1 handled, -1 owned but unsafe. No dependency on ColourPicker.
extern "C" __declspec(dllexport) int TPM_Aquarium_ColourBridgeV1(void* menu,unsigned operation,float* rgba){
    if(!menu||menu!=p56_menu)return 0;
    if(!p78_main()||p56_closing||!g_pickerOpen||!rgba||operation>1)return -1;
    P78WorkScope work;
    if(!p56_alive(menu)||!p56_opened(menu)||!p56_alive(g_pickerWidget)||!p56_target_live())return -1;
    if(!p56_active(p56_inspector)||field_object(p56_inspector,"_room")!=p56_room)return -1;
    if(operation==0){rgba[0]=g_currentWaterColor.x;rgba[1]=g_currentWaterColor.y;rgba[2]=g_currentWaterColor.z;rgba[3]=1;return 1;}
    for(int i=0;i<3;++i)if(!(rgba[i]>=0.f&&rgba[i]<=1.f))return -1;
    F4 color{rgba[0],rgba[1],rgba[2],0};
    rgb_to_hsv(color,g_manualHue,g_manualSat,g_manualVal);
    update_active_water_color(color);g_pickerChanged=true;
    update_manual_picker_visuals();p56_gradient();p65_flush_glow();persist_current_target(true);p80_button_states();
    log_raw("COLOUR BRIDGE V1: RGB/HEX edit applied through aquarium path.\r\n");
    return 1;
}
