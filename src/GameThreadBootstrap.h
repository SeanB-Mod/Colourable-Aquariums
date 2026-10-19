// A thread-specific message hook marshals one-time setup to Unity's window thread.
// The worker never invokes Unity, attaches to IL2CPP, patches code, or loads other mods.
static HMODULE p78_module=nullptr;
static void* p78_messageHook=nullptr;
static i64 (*p78_nextHook)(void*,int,uptr,i64)=nullptr;
static bool p78_bootReady=false,p78_bootBusy=false,p78_bootDone=false;
static i64 p78_on_message(int code,uptr w,i64 l){
    i64 result=p78_nextHook?p78_nextHook(nullptr,code,w,l):0;
    if(code<0||!__atomic_load_n(&p78_bootReady,__ATOMIC_ACQUIRE)||!p78_main()||
       __atomic_load_n(&p78_bootDone,__ATOMIC_ACQUIRE))return result;
    bool expected=false;
    if(!__atomic_compare_exchange_n(&p78_bootBusy,&expected,true,false,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED))return result;
    static u64 last=0;u64 now=p78_clock();
    if(now-last<1000){__atomic_store_n(&p78_bootBusy,false,__ATOMIC_RELEASE);return result;}last=now;
    if(init_il2cpp()&&il2cpp_domain_get()){
        if(init_unity_methods()){
            P78WorkScope work;load_persistence();
            bool ok=p51_install_post_update_hook();
            log_raw(ok?"0.7.81 BOOT: update hook installed on Unity window thread.\r\n":"0.7.81 BOOT: guarded update hook refused; aquarium mod inactive.\r\n");
            __atomic_store_n(&p78_bootDone,true,__ATOMIC_RELEASE);
        }
    }
    __atomic_store_n(&p78_bootBusy,false,__ATOMIC_RELEASE);return result;
}
static HWND p78_window=nullptr;
static BOOL p78_find_window(HWND window,i64){
    DWORD process=0;char name[80]{};DWORD thread=p70_windowProcess(window,&process);
    if(thread&&process==p70_processId()&&p70_className(window,name,sizeof(name))&&streq(name,"UnityWndClass")){
        p78_window=window;p78_mainThread=thread;return 0;
    }return 1;
}
static DWORD worker(void*){
    if(!init_winapi())return 0;
    init_log_path();p78_init_logging();
    void* k=find_loaded_module("kernel32.dll");void* u=find_loaded_module("user32.dll");
    p78_threadId=(decltype(p78_threadId))resolve_export(k,"GetCurrentThreadId");
    p78_clock=(decltype(p78_clock))resolve_export(k,"GetTickCount64");
    auto enumerate=(BOOL(*)(BOOL(*)(HWND,i64),i64))resolve_export(u,"EnumWindows");
    auto hook=(void*(*)(int,i64(*)(int,uptr,i64),HMODULE,DWORD))resolve_export(u,"SetWindowsHookExA");
    auto unhook=(BOOL(*)(void*))resolve_export(u,"UnhookWindowsHookEx");
    auto post=(BOOL(*)(HWND,unsigned,uptr,i64))resolve_export(u,"PostMessageA");
    p78_nextHook=(decltype(p78_nextHook))resolve_export(u,"CallNextHookEx");
    if(!p78_threadId||!p78_clock||!enumerate||!hook||!unhook||!post||!p78_nextHook||!p70_windowProcess||!p70_processId||!p70_className){log_raw("0.7.81 BOOT ERROR: message-dispatch API missing.\r\n");return 0;}
    log_raw("TPM Colourable Aquariums Steam Workshop v1.0: game-thread bootstrap; Dredge glow at 25% of corrected baseline; standard retail-luminance glow, 50% at black / 100% from quarter value.\r\n");
    pSleep(15000);
    for(int i=0;i<120&&!p78_window;++i){enumerate(p78_find_window,0);if(!p78_window)pSleep(1000);}
    if(!p78_window){log_raw("0.7.81 BOOT ERROR: Unity window not found within timeout.\r\n");return 0;}
    p78_messageHook=hook(3,p78_on_message,nullptr,p78_mainThread); // WH_GETMESSAGE
    if(!p78_messageHook){log_raw("0.7.81 BOOT ERROR: thread message hook failed.\r\n");return 0;}
    __atomic_store_n(&p78_bootReady,true,__ATOMIC_RELEASE);
    for(int i=0;i<600&&!__atomic_load_n(&p78_bootDone,__ATOMIC_ACQUIRE);++i){post(p78_window,0,0,0);pSleep(100);}
    __atomic_store_n(&p78_bootReady,false,__ATOMIC_RELEASE);
    unhook(p78_messageHook);
    log_raw(__atomic_load_n(&p78_bootDone,__ATOMIC_ACQUIRE)?"0.7.81 BOOT: bootstrap worker exited.\r\n":"0.7.81 BOOT ERROR: setup timeout; message hook removed.\r\n");
    return 0;
}
extern "C" BOOL DllMain(HMODULE module,DWORD reason,LPVOID){
    if(reason==DLL_PROCESS_ATTACH){p78_module=module;void* k=find_loaded_module("kernel32.dll");
        auto create=k?(CreateThread_t)resolve_export(k,"CreateThread"):nullptr;
        auto close=k?(CloseHandle_t)resolve_export(k,"CloseHandle"):nullptr;
        if(create&&close){DWORD tid=0;HANDLE h=create(nullptr,0,worker,nullptr,0,&tid);if(h)close(h);}
    }return 1;
}
