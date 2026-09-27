// Optional, main-thread-only input suspension during a one-shot screen sample.
// operation 0 queries support, 1 suspends, 2 resumes. No dependency on a caller DLL.
extern "C" __declspec(dllexport) int TPM_Aquarium_SamplingBridgeV1(void* menu,unsigned operation){
    if(!menu||menu!=p56_menu)return 0;
    if(!p78_main()||operation>2)return -1;
    P78WorkScope work;
    if(operation==2){g_pickerSampling=false;g_pickerDragMode=0;g_manualMouseWasDown=true;return 1;}
    if(p56_closing||!g_pickerOpen||!p56_alive(menu)||!p56_opened(menu)||!p56_target_live())return -1;
    if(!p56_active(p56_inspector)||field_object(p56_inspector,"_room")!=p56_room)return -1;
    if(operation==1){g_pickerSampling=true;g_pickerDragMode=0;g_manualMouseWasDown=true;}
    return 1;
}
