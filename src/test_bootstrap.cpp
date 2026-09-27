#include <windows.h>
#include <cassert>
#include <cstdio>
#include <cstring>
using u64=unsigned long long;using i64=long long;using uptr=unsigned long long;using usize=unsigned long long;
using CreateThread_t=HANDLE(*)(void*,usize,DWORD(*)(void*),void*,DWORD,DWORD*);using CloseHandle_t=BOOL(*)(HANDLE);
static bool streq(const char*a,const char*b){return !strcmp(a,b);}static usize slen(const char*a){return strlen(a);}static void scopy(char*d,usize n,const char*s){snprintf(d,n,"%s",s);}
#include "RuntimeWork.h"
static DWORD (*p70_processId)()=GetCurrentProcessId;
static DWORD (*p70_windowProcess)(HWND,DWORD*)=GetWindowThreadProcessId;
static int (*p70_className)(HWND,char*,int)=GetClassNameA;
static void* find_loaded_module(const char*n){return GetModuleHandleA(n);}static void* resolve_export(void*m,const char*n){return (void*)GetProcAddress((HMODULE)m,n);}
static bool timeoutTest=false;static DWORD gameThread=0;static int resolves=0,installed=0,loaded=0;
static bool init_winapi(){return true;}static void init_log_path(){}static void p78_init_logging(){}static void p61_install_exception_observer(){}
static void log_raw(const char*){}
static void pSleep(DWORD n){if(n==15000)return;Sleep(timeoutTest?1:n);}
static bool init_il2cpp(){assert(GetCurrentThreadId()==gameThread);return ++resolves>1&&!timeoutTest;}
static void* il2cpp_domain_get(){assert(GetCurrentThreadId()==gameThread);return (void*)1;}
static bool init_unity_methods(){assert(GetCurrentThreadId()==gameThread);return true;}
static void load_persistence(){assert(GetCurrentThreadId()==gameThread);++loaded;}
static bool p51_install_post_update_hook(){assert(GetCurrentThreadId()==gameThread);++installed;return true;}
#include "GameThreadBootstrap.h"
int main(int argc,char**){
 SetErrorMode(0x8003);timeoutTest=argc>1;gameThread=GetCurrentThreadId();
 WNDCLASSA wc{};wc.lpfnWndProc=DefWindowProcA;wc.hInstance=GetModuleHandleA(nullptr);wc.lpszClassName="UnityWndClass";assert(RegisterClassA(&wc));
 HWND window=CreateWindowA(wc.lpszClassName,"Hidden bootstrap fixture",WS_OVERLAPPED,0,0,1,1,nullptr,nullptr,wc.hInstance,nullptr);assert(window);
 HANDLE thread=CreateThread(nullptr,0,worker,nullptr,0,nullptr);assert(thread);auto start=GetTickCount64();
 while(WaitForSingleObject(thread,0)==WAIT_TIMEOUT){MSG msg;while(PeekMessageA(&msg,nullptr,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessageA(&msg);}Sleep(1);assert(GetTickCount64()-start<10000);}
 assert(installed==(timeoutTest?0:1));assert(loaded==installed);assert(!p78_bootReady);assert(p78_mainThread==gameThread);
 for(int i=0;i<10;++i){PostMessageA(window,WM_NULL,0,0);MSG msg;while(PeekMessageA(&msg,nullptr,0,0,PM_REMOVE))DispatchMessageA(&msg);}assert(installed==(timeoutTest?0:1));
 CloseHandle(thread);DestroyWindow(window);puts(timeoutTest?"Bootstrap timeout: no install, hook removed, worker exits.":"Bootstrap: real hidden window, main-thread-only setup, delayed readiness, exactly-once install, hook removal and worker exit passed.");
}
