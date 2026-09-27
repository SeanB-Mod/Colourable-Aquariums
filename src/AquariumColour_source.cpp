// Colourable Aquariums 0.7.81.5 — UI-only release candidate.
// Unity work is dispatched to the game thread; save data lives under Mods/Saves.
// Optional Colour Picker bridges do not require that mod to be installed.
// See COMPATIBILITY.md for native layout and hook assumptions.

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;
using i32 = int;
using i64 = long long;
using uptr = unsigned long long;
using usize = unsigned long long;
using BOOL = int;
using DWORD = unsigned long;
using HANDLE = void*;
using HMODULE = void*;
using FARPROC = void*;
using LPVOID = void*;
using LPCSTR = const char*;
using LPSTR = char*;
using HWND = void*;

struct POINT_X { long x; long y; };
struct RECT_X { long left; long top; long right; long bottom; };

extern "C" int _fltused = 0;

extern "C" void* memcpy(void* dst, const void* src, usize n){ u8* d=(u8*)dst; const u8* q=(const u8*)src; for(usize i=0;i<n;++i)d[i]=q[i]; return dst; }
extern "C" void* memset(void* dst, int v, usize n){ u8* d=(u8*)dst; for(usize i=0;i<n;++i)d[i]=(u8)v; return dst; }

#define DLL_PROCESS_ATTACH 1
#define DLL_PROCESS_DETACH 0
#define VK_LBUTTON 0x01
#define GENERIC_WRITE 0x40000000u
#define GENERIC_READ  0x80000000u
#define FILE_SHARE_READ 0x00000001u
#define OPEN_ALWAYS 4u
#define OPEN_EXISTING 3u
#define FILE_ATTRIBUTE_NORMAL 0x00000080u
#define FILE_END 2u
#define INVALID_HANDLE_VALUE ((HANDLE)(uptr)-1)
#define MEM_COMMIT 0x00001000u
#define MEM_RESERVE 0x00002000u
#define PAGE_EXECUTE_READWRITE 0x40u

static usize slen(const char* s){ usize n=0; if(!s) return 0; while(s[n]) ++n; return n; }
static char lowerc(char c){ return (c>='A'&&c<='Z') ? (char)(c+('a'-'A')) : c; }
static bool streq(const char* a,const char* b){ if(!a||!b) return false; while(*a&&*b){ if(*a!=*b) return false; ++a;++b;} return *a==0&&*b==0; }
static bool contains_i(const char* h,const char* n){
    if(!h||!n||!*n) return false;
    for(usize i=0;h[i];++i){ usize j=0; while(n[j]&&h[i+j]&&lowerc(h[i+j])==lowerc(n[j])) ++j; if(!n[j]) return true; }
    return false;
}
static void scopy(char* d, usize cap, const char* s){ if(!d||cap==0) return; usize i=0; if(s){ for(;i+1<cap&&s[i];++i)d[i]=s[i]; } d[i]=0; }
static void sappend(char* d, usize cap, const char* s){ usize n=slen(d); if(n>=cap) return; usize i=0; if(s){ for(;n+i+1<cap&&s[i];++i)d[n+i]=s[i]; } d[n+i]=0; }

static void* get_peb(){ void* p; __asm__ __volatile__("movq %%gs:0x60, %0" : "=r"(p)); return p; }

struct LIST_ENTRY_X { void* Flink; void* Blink; };
struct UNICODE_STRING_X { u16 Length; u16 MaximumLength; wchar_t* Buffer; };

static bool wide_base_eq(const UNICODE_STRING_X* us, const char* ascii){
    if(!us||!us->Buffer||!ascii) return false;
    usize an=slen(ascii), wn=(usize)us->Length/2; if(an!=wn) return false;
    for(usize i=0;i<an;++i){ wchar_t wc=us->Buffer[i]; char ac=ascii[i]; wchar_t wl=(wc>='A'&&wc<='Z')?wc+32:wc; char al=lowerc(ac); if((wchar_t)al!=wl) return false; }
    return true;
}

static void* find_loaded_module(const char* name){
    u8* peb=(u8*)get_peb(); if(!peb) return nullptr;
    u8* ldr=*(u8**)(peb+0x18); if(!ldr) return nullptr;
    LIST_ENTRY_X* head=(LIST_ENTRY_X*)(ldr+0x20); LIST_ENTRY_X* cur=(LIST_ENTRY_X*)head->Flink;
    for(int guard=0;cur && cur!=head && guard<256; ++guard){
        u8* ent=(u8*)cur-0x10; void* base=*(void**)(ent+0x30); UNICODE_STRING_X* bn=(UNICODE_STRING_X*)(ent+0x58);
        if(base && wide_base_eq(bn,name)) return base;
        cur=(LIST_ENTRY_X*)cur->Flink;
    }
    return nullptr;
}

static void* resolve_export_raw(void* mod, const char* name, u32* outRva=nullptr, u32* outExpRva=nullptr, u32* outExpSize=nullptr){
    if(!mod||!name) return nullptr; u8* b=(u8*)mod;
    u32 pe=*(u32*)(b+0x3c); if(*(u32*)(b+pe)!=0x00004550u) return nullptr;
    u16 magic=*(u16*)(b+pe+24); if(magic!=0x20b) return nullptr;
    u32 expRva=*(u32*)(b+pe+24+0x70); u32 expSize=*(u32*)(b+pe+24+0x74); if(!expRva) return nullptr;
    u8* e=b+expRva; u32 nNames=*(u32*)(e+0x18); u32 funcsRva=*(u32*)(e+0x1c); u32 namesRva=*(u32*)(e+0x20); u32 ordsRva=*(u32*)(e+0x24);
    u32* funcs=(u32*)(b+funcsRva); u32* names=(u32*)(b+namesRva); u16* ords=(u16*)(b+ordsRva);
    for(u32 i=0;i<nNames;++i){ const char* nm=(const char*)(b+names[i]); if(streq(nm,name)){ u32 rva=funcs[ords[i]]; if(outRva)*outRva=rva; if(outExpRva)*outExpRva=expRva; if(outExpSize)*outExpSize=expSize; return b+rva; } }
    return nullptr;
}

static void split_forwarder(const char* fwd,char* mod,usize mcap,char* fn,usize fcap){
    mod[0]=0; fn[0]=0; if(!fwd)return; usize i=0; while(fwd[i]&&fwd[i]!='.'&&i+1<mcap){mod[i]=fwd[i];++i;} mod[i]=0; if(fwd[i]=='.')++i; usize j=0; while(fwd[i]&&j+1<fcap) fn[j++]=fwd[i++]; fn[j]=0; sappend(mod,mcap,".dll");
}

static void* resolve_export(void* mod,const char* name,int depth=0){
    if(depth>6) return nullptr; u32 r=0,er=0,es=0; u8* p=(u8*)resolve_export_raw(mod,name,&r,&er,&es); if(!p) return nullptr;
    if(r>=er && r<er+es){ char mn[80],fn[128]; split_forwarder((const char*)p,mn,sizeof(mn),fn,sizeof(fn)); void* m=find_loaded_module(mn); if(!m) return nullptr; return resolve_export(m,fn,depth+1); }
    return p;
}

typedef FARPROC (*GetProcAddress_t)(HMODULE,LPCSTR);
typedef HMODULE (*LoadLibraryA_t)(LPCSTR);
typedef HANDLE (*CreateThread_t)(void*,usize,DWORD(*)(void*),void*,DWORD,DWORD*);
typedef void (*Sleep_t)(DWORD);
typedef DWORD (*GetEnvironmentVariableA_t)(LPCSTR,LPSTR,DWORD);
typedef DWORD (*GetModuleFileNameA_t)(HMODULE,LPSTR,DWORD);
typedef HANDLE (*CreateFileA_t)(LPCSTR,DWORD,DWORD,void*,DWORD,DWORD,HANDLE);
typedef BOOL (*WriteFile_t)(HANDLE,const void*,DWORD,DWORD*,void*);
typedef BOOL (*ReadFile_t)(HANDLE,void*,DWORD,DWORD*,void*);
typedef DWORD (*SetFilePointer_t)(HANDLE,long,long*,DWORD);
typedef BOOL (*CloseHandle_t)(HANDLE);
typedef LPVOID (*VirtualAlloc_t)(LPVOID,usize,DWORD,DWORD);
typedef BOOL (*VirtualProtect_t)(LPVOID,usize,DWORD,DWORD*);
typedef BOOL (*FlushInstructionCache_t)(HANDLE,const void*,usize);
typedef HANDLE (*GetCurrentProcess_t)();
typedef short (*GetAsyncKeyState_t)(int);
typedef BOOL (*GetCursorPos_t)(POINT_X*);
typedef HWND (*GetForegroundWindow_t)();
typedef BOOL (*ScreenToClient_t)(HWND,POINT_X*);
typedef BOOL (*GetClientRect_t)(HWND,RECT_X*);
typedef int (*sprintf_t)(char*,const char*,...);
typedef double (*math1_t)(double);
typedef double (*math2_t)(double,double);

static GetProcAddress_t pGetProcAddress=nullptr; static LoadLibraryA_t pLoadLibraryA=nullptr; static CreateThread_t pCreateThread=nullptr; static Sleep_t pSleep=nullptr;
static GetEnvironmentVariableA_t pGetEnvironmentVariableA=nullptr; static GetModuleFileNameA_t pGetModuleFileNameA=nullptr; static CreateFileA_t pCreateFileA=nullptr; static WriteFile_t pWriteFile=nullptr; static ReadFile_t pReadFile=nullptr; static SetFilePointer_t pSetFilePointer=nullptr; static CloseHandle_t pCloseHandle=nullptr;
static VirtualAlloc_t pVirtualAlloc=nullptr; static VirtualProtect_t pVirtualProtect=nullptr; static FlushInstructionCache_t pFlushInstructionCache=nullptr; static GetCurrentProcess_t pGetCurrentProcess=nullptr;
static GetAsyncKeyState_t pGetAsyncKeyState=nullptr;
static DWORD (*p70_processId)()=nullptr;
static DWORD (*p70_windowProcess)(HWND,DWORD*)=nullptr;
static int (*p70_className)(HWND,LPSTR,int)=nullptr;
static int (*p70_metrics)(int)=nullptr;
static GetCursorPos_t pGetCursorPos=nullptr; static GetForegroundWindow_t pGetForegroundWindow=nullptr; static ScreenToClient_t pScreenToClient=nullptr; static GetClientRect_t pGetClientRect=nullptr;
static sprintf_t psprintf=nullptr;
static math1_t pSqrt=nullptr,pCos=nullptr,pSin=nullptr; static math2_t pAtan2=nullptr;

static bool init_winapi(){
    void* k=find_loaded_module("kernel32.dll"); if(!k) return false;
    pGetProcAddress=(GetProcAddress_t)resolve_export(k,"GetProcAddress"); pLoadLibraryA=(LoadLibraryA_t)resolve_export(k,"LoadLibraryA"); pCreateThread=(CreateThread_t)resolve_export(k,"CreateThread"); pSleep=(Sleep_t)resolve_export(k,"Sleep");
    pGetEnvironmentVariableA=(GetEnvironmentVariableA_t)resolve_export(k,"GetEnvironmentVariableA"); pGetModuleFileNameA=(GetModuleFileNameA_t)resolve_export(k,"GetModuleFileNameA"); pCreateFileA=(CreateFileA_t)resolve_export(k,"CreateFileA"); pWriteFile=(WriteFile_t)resolve_export(k,"WriteFile"); pReadFile=(ReadFile_t)resolve_export(k,"ReadFile"); pSetFilePointer=(SetFilePointer_t)resolve_export(k,"SetFilePointer"); pCloseHandle=(CloseHandle_t)resolve_export(k,"CloseHandle");
    pVirtualAlloc=(VirtualAlloc_t)resolve_export(k,"VirtualAlloc"); pVirtualProtect=(VirtualProtect_t)resolve_export(k,"VirtualProtect"); pFlushInstructionCache=(FlushInstructionCache_t)resolve_export(k,"FlushInstructionCache"); pGetCurrentProcess=(GetCurrentProcess_t)resolve_export(k,"GetCurrentProcess");
    if(!pLoadLibraryA||!pGetProcAddress||!pCreateThread||!pSleep||!pGetModuleFileNameA||!pCreateFileA||!pWriteFile||!pReadFile||!pSetFilePointer||!pCloseHandle||!pVirtualAlloc||!pVirtualProtect||!pFlushInstructionCache||!pGetCurrentProcess) return false;
    p70_processId=(decltype(p70_processId))resolve_export(k,"GetCurrentProcessId");
    HMODULE u=pLoadLibraryA("user32.dll"); if(u){
        p70_windowProcess=(decltype(p70_windowProcess))pGetProcAddress(u,"GetWindowThreadProcessId");
        p70_className=(decltype(p70_className))pGetProcAddress(u,"GetClassNameA");
        p70_metrics=(decltype(p70_metrics))pGetProcAddress(u,"GetSystemMetrics");
        pGetAsyncKeyState=(GetAsyncKeyState_t)pGetProcAddress(u,"GetAsyncKeyState");
        pGetCursorPos=(GetCursorPos_t)pGetProcAddress(u,"GetCursorPos");
        pGetForegroundWindow=(GetForegroundWindow_t)pGetProcAddress(u,"GetForegroundWindow");
        pScreenToClient=(ScreenToClient_t)pGetProcAddress(u,"ScreenToClient");
        pGetClientRect=(GetClientRect_t)pGetProcAddress(u,"GetClientRect");
    }
    HMODULE c=pLoadLibraryA("msvcrt.dll"); if(c){ psprintf=(sprintf_t)pGetProcAddress(c,"sprintf"); pSqrt=(math1_t)pGetProcAddress(c,"sqrt"); pCos=(math1_t)pGetProcAddress(c,"cos"); pSin=(math1_t)pGetProcAddress(c,"sin"); pAtan2=(math2_t)pGetProcAddress(c,"atan2"); }
    return pGetAsyncKeyState&&pGetCursorPos&&pGetForegroundWindow&&pScreenToClient&&pGetClientRect&&psprintf&&pSqrt&&pCos&&pSin&&pAtan2;
}

#include "RuntimeWork.h"
#include "InputSafety.h"
static HWND p70_game_foreground(){
    if(!pGetForegroundWindow||!p70_processId||!p70_windowProcess||!p70_className)return nullptr;
    HWND window=pGetForegroundWindow();DWORD owner=0;char name[80]{};
    if(!window||!p70_windowProcess(window,&owner)||owner!=p70_processId())return nullptr;
    if(!p70_className(window,name,sizeof(name))||!streq(name,"UnityWndClass"))return nullptr;
    return window;
}
static int p70_primary_button(){return p70_metrics&&p70_metrics(23)?0x02:VK_LBUTTON;}

static char g_logPath[600];
static char g_persistPath[600];
static void init_log_path(){
    g_logPath[0]=0;g_persistPath[0]=0;
    // All mod output remains below TPM.exe/Mods. Never fall back to CWD/AppData.
    char exe[520]{};
    DWORD count=pGetModuleFileNameA?pGetModuleFileNameA(nullptr,exe,sizeof(exe)):0;
    if(!count||count>=sizeof(exe)-80)return;
    usize n=slen(exe);while(n&&exe[n-1]!='\\'&&exe[n-1]!='/')--n;
    if(!n)return;exe[n]=0;
    scopy(g_logPath,sizeof(g_logPath),exe);
    sappend(g_logPath,sizeof(g_logPath),"Mods\\Logs\\AquariumColour.log");
    scopy(g_persistPath,sizeof(g_persistPath),exe);
    sappend(g_persistPath,sizeof(g_persistPath),"Mods\\Saves\\AquariumColour.dat");
}
#include "BoundedLog.h"

// ---------- IL2CPP ----------
struct Il2CppObject { void* klass; void* monitor; };
struct Il2CppArray { Il2CppObject obj; void* bounds; uptr max_length; void* vector[1]; };
using Il2CppDomain=void; using Il2CppAssembly=void; using Il2CppImage=void; using Il2CppClass=void; using MethodInfo=void; using Il2CppType=void; using Il2CppThread=void; using Il2CppString=void;

typedef Il2CppDomain* (*il2cpp_domain_get_t)();
typedef Il2CppThread* (*il2cpp_thread_attach_t)(Il2CppDomain*);
typedef void (*il2cpp_thread_detach_t)(Il2CppThread*);
typedef const Il2CppAssembly** (*il2cpp_domain_get_assemblies_t)(Il2CppDomain*, usize*);
typedef const Il2CppImage* (*il2cpp_assembly_get_image_t)(const Il2CppAssembly*);
typedef usize (*il2cpp_image_get_class_count_t)(const Il2CppImage*);
typedef Il2CppClass* (*il2cpp_image_get_class_t)(const Il2CppImage*,usize);
typedef const char* (*il2cpp_class_get_name_t)(Il2CppClass*);
typedef const char* (*il2cpp_class_get_namespace_t)(Il2CppClass*);
typedef const MethodInfo* (*il2cpp_class_get_method_from_name_t)(Il2CppClass*,const char*,int);
typedef const MethodInfo* (*il2cpp_class_get_methods_t)(Il2CppClass*,void**);
typedef const char* (*il2cpp_method_get_name_t)(const MethodInfo*);
typedef u32 (*il2cpp_method_get_param_count_t)(const MethodInfo*);
typedef const Il2CppType* (*il2cpp_method_get_param_t)(const MethodInfo*,u32);
typedef char* (*il2cpp_type_get_name_t)(const Il2CppType*);
typedef void (*il2cpp_free_t)(void*);
typedef Il2CppObject* (*il2cpp_runtime_invoke_t)(const MethodInfo*,void*,void**,Il2CppObject**);
typedef const Il2CppType* (*il2cpp_class_get_type_t)(Il2CppClass*);
typedef Il2CppObject* (*il2cpp_type_get_object_t)(const Il2CppType*);
typedef Il2CppString* (*il2cpp_string_new_t)(const char*);
typedef int (*il2cpp_string_length_t)(Il2CppString*);
typedef const u16* (*il2cpp_string_chars_t)(Il2CppString*);
typedef void* (*il2cpp_object_unbox_t)(Il2CppObject*);
typedef Il2CppObject* (*il2cpp_object_new_t)(Il2CppClass*);
typedef Il2CppObject* (*il2cpp_method_get_object_t)(const MethodInfo*, Il2CppClass*);
typedef Il2CppClass* (*il2cpp_object_get_class_t)(Il2CppObject*);
typedef Il2CppClass* (*il2cpp_class_get_parent_t)(Il2CppClass*);
typedef void* Il2CppFieldInfo; using Il2CppPropertyInfo=void;
typedef Il2CppFieldInfo* (*il2cpp_class_get_fields_t)(Il2CppClass*,void**);
typedef const char* (*il2cpp_field_get_name_t)(Il2CppFieldInfo*);
typedef const Il2CppType* (*il2cpp_field_get_type_t)(Il2CppFieldInfo*);
typedef Il2CppObject* (*il2cpp_field_get_value_object_t)(Il2CppFieldInfo*,void*);
typedef void (*il2cpp_field_static_get_value_t)(Il2CppFieldInfo*,void*);
typedef void (*il2cpp_field_set_value_t)(Il2CppObject*,Il2CppFieldInfo*,void*);
typedef const Il2CppPropertyInfo* (*il2cpp_class_get_properties_t)(Il2CppClass*,void**);
typedef const char* (*il2cpp_property_get_name_t)(const Il2CppPropertyInfo*);
typedef const MethodInfo* (*il2cpp_property_get_get_method_t)(const Il2CppPropertyInfo*);
typedef const Il2CppType* (*il2cpp_method_get_return_type_t)(const MethodInfo*);
using AquariumGCHandle=uptr;
static_assert(sizeof(AquariumGCHandle)==sizeof(void*),"GC handles must retain pointer width");
typedef AquariumGCHandle (*il2cpp_gchandle_new_t)(Il2CppObject*, bool);
typedef void (*il2cpp_gchandle_free_t)(AquariumGCHandle);

static il2cpp_domain_get_t il2cpp_domain_get=nullptr; static il2cpp_thread_attach_t il2cpp_thread_attach=nullptr; static il2cpp_thread_detach_t il2cpp_thread_detach=nullptr;
static il2cpp_domain_get_assemblies_t il2cpp_domain_get_assemblies=nullptr; static il2cpp_assembly_get_image_t il2cpp_assembly_get_image=nullptr; static il2cpp_image_get_class_count_t il2cpp_image_get_class_count=nullptr; static il2cpp_image_get_class_t il2cpp_image_get_class=nullptr;
static il2cpp_class_get_name_t il2cpp_class_get_name=nullptr; static il2cpp_class_get_namespace_t il2cpp_class_get_namespace=nullptr; static il2cpp_class_get_method_from_name_t il2cpp_class_get_method_from_name=nullptr; static il2cpp_class_get_methods_t il2cpp_class_get_methods=nullptr;
static il2cpp_method_get_name_t il2cpp_method_get_name=nullptr; static il2cpp_method_get_param_count_t il2cpp_method_get_param_count=nullptr; static il2cpp_method_get_param_t il2cpp_method_get_param=nullptr; static il2cpp_type_get_name_t il2cpp_type_get_name=nullptr; static il2cpp_free_t il2cpp_free=nullptr;
static il2cpp_runtime_invoke_t il2cpp_runtime_invoke=nullptr; static il2cpp_class_get_type_t il2cpp_class_get_type=nullptr; static il2cpp_type_get_object_t il2cpp_type_get_object=nullptr; static il2cpp_string_new_t il2cpp_string_new=nullptr; static il2cpp_string_length_t il2cpp_string_length=nullptr; static il2cpp_string_chars_t il2cpp_string_chars=nullptr; static il2cpp_object_unbox_t il2cpp_object_unbox=nullptr; static il2cpp_object_new_t il2cpp_object_new=nullptr;
static il2cpp_method_get_object_t il2cpp_method_get_object=nullptr;
static il2cpp_object_get_class_t il2cpp_object_get_class=nullptr; static il2cpp_class_get_parent_t il2cpp_class_get_parent=nullptr; static il2cpp_class_get_fields_t il2cpp_class_get_fields=nullptr; static il2cpp_field_get_name_t il2cpp_field_get_name=nullptr; static il2cpp_field_get_type_t il2cpp_field_get_type=nullptr; static il2cpp_field_get_value_object_t il2cpp_field_get_value_object=nullptr; static il2cpp_field_static_get_value_t il2cpp_field_static_get_value=nullptr; static il2cpp_field_set_value_t il2cpp_field_set_value=nullptr; static il2cpp_class_get_properties_t il2cpp_class_get_properties=nullptr; static il2cpp_property_get_name_t il2cpp_property_get_name=nullptr; static il2cpp_property_get_get_method_t il2cpp_property_get_get_method=nullptr; static il2cpp_method_get_return_type_t il2cpp_method_get_return_type=nullptr;
static il2cpp_gchandle_new_t il2cpp_gchandle_new=nullptr; static il2cpp_gchandle_free_t il2cpp_gchandle_free=nullptr;

static bool init_il2cpp(){
    HMODULE g=(HMODULE)find_loaded_module("GameAssembly.dll"); if(!g) return false;
    #define RESOLVE(N) N=(N##_t)resolve_export(g,#N); if(!N) return false
    RESOLVE(il2cpp_domain_get); RESOLVE(il2cpp_thread_attach); RESOLVE(il2cpp_thread_detach); RESOLVE(il2cpp_domain_get_assemblies); RESOLVE(il2cpp_assembly_get_image); RESOLVE(il2cpp_image_get_class_count); RESOLVE(il2cpp_image_get_class); RESOLVE(il2cpp_class_get_name); RESOLVE(il2cpp_class_get_namespace); RESOLVE(il2cpp_class_get_method_from_name); RESOLVE(il2cpp_class_get_methods); RESOLVE(il2cpp_method_get_name); RESOLVE(il2cpp_method_get_param_count); RESOLVE(il2cpp_method_get_param); RESOLVE(il2cpp_type_get_name); RESOLVE(il2cpp_free); RESOLVE(il2cpp_runtime_invoke); RESOLVE(il2cpp_class_get_type); RESOLVE(il2cpp_type_get_object); RESOLVE(il2cpp_string_new); RESOLVE(il2cpp_string_length); RESOLVE(il2cpp_string_chars); RESOLVE(il2cpp_object_unbox); RESOLVE(il2cpp_object_new); RESOLVE(il2cpp_object_get_class); RESOLVE(il2cpp_class_get_parent); RESOLVE(il2cpp_class_get_fields); RESOLVE(il2cpp_field_get_name); RESOLVE(il2cpp_field_get_type); RESOLVE(il2cpp_field_get_value_object); RESOLVE(il2cpp_field_static_get_value); RESOLVE(il2cpp_field_set_value); RESOLVE(il2cpp_class_get_properties); RESOLVE(il2cpp_property_get_name); RESOLVE(il2cpp_property_get_get_method); RESOLVE(il2cpp_method_get_return_type); RESOLVE(il2cpp_gchandle_new); RESOLVE(il2cpp_gchandle_free);
    #undef RESOLVE
    il2cpp_method_get_object=(il2cpp_method_get_object_t)resolve_export(g,"il2cpp_method_get_object");
    return true;
}

static Il2CppClass* find_class(const char* ns,const char* name){
    static P78MetadataCache<Il2CppClass*,128> cache;
    if(!ns)ns="";if(!name)return nullptr;if(auto c=cache.get(nullptr,ns,name))return c;
    Il2CppDomain* d=il2cpp_domain_get(); if(!d)return nullptr; usize ac=0; const Il2CppAssembly** as=il2cpp_domain_get_assemblies(d,&ac); if(!as)return nullptr;
    for(usize a=0;a<ac;++a){ const Il2CppImage* im=il2cpp_assembly_get_image(as[a]); if(!im)continue; usize cc=il2cpp_image_get_class_count(im); for(usize i=0;i<cc;++i){ Il2CppClass* c=il2cpp_image_get_class(im,i); if(!c)continue; const char* cn=il2cpp_class_get_name(c); const char* cs=il2cpp_class_get_namespace(c); if(streq(cn,name)&&streq(cs?cs:"",ns)){cache.put(nullptr,ns,name,c);return c;} } }
    return nullptr;
}

static const MethodInfo* find_method_param1(Il2CppClass* c,const char* name,const char* paramType){
    if(!c)return nullptr; void* it=nullptr; const MethodInfo* m=nullptr;
    while((m=il2cpp_class_get_methods(c,&it))!=nullptr){ const char* mn=il2cpp_method_get_name(m); if(!streq(mn,name))continue; if(il2cpp_method_get_param_count(m)!=1)continue; const Il2CppType* pt=il2cpp_method_get_param(m,0); char* tn=pt?il2cpp_type_get_name(pt):nullptr; bool ok=tn&&streq(tn,paramType); if(tn)il2cpp_free(tn); if(ok)return m; }
    return nullptr;
}

static const MethodInfo* find_method_param2(Il2CppClass* c,const char* name,const char* p0,const char* p1){
    if(!c)return nullptr; void* it=nullptr; const MethodInfo* m=nullptr;
    while((m=il2cpp_class_get_methods(c,&it))!=nullptr){
        const char* mn=il2cpp_method_get_name(m); if(!streq(mn,name))continue;
        if(il2cpp_method_get_param_count(m)!=2)continue;
        const Il2CppType* t0=il2cpp_method_get_param(m,0); const Il2CppType* t1=il2cpp_method_get_param(m,1);
        char* n0=t0?il2cpp_type_get_name(t0):nullptr; char* n1=t1?il2cpp_type_get_name(t1):nullptr;
        bool ok=n0&&n1&&streq(n0,p0)&&streq(n1,p1);
        if(n0)il2cpp_free(n0); if(n1)il2cpp_free(n1);
        if(ok)return m;
    }
    return nullptr;
}

static const MethodInfo* find_method_param3(Il2CppClass* c,const char* name,const char* p0,const char* p1,const char* p2){
    if(!c)return nullptr; void* it=nullptr; const MethodInfo* m=nullptr;
    while((m=il2cpp_class_get_methods(c,&it))!=nullptr){
        const char* mn=il2cpp_method_get_name(m); if(!streq(mn,name))continue;
        if(il2cpp_method_get_param_count(m)!=3)continue;
        const Il2CppType* t0=il2cpp_method_get_param(m,0); const Il2CppType* t1=il2cpp_method_get_param(m,1); const Il2CppType* t2=il2cpp_method_get_param(m,2);
        char* n0=t0?il2cpp_type_get_name(t0):nullptr; char* n1=t1?il2cpp_type_get_name(t1):nullptr; char* n2=t2?il2cpp_type_get_name(t2):nullptr;
        bool ok=n0&&n1&&n2&&streq(n0,p0)&&streq(n1,p1)&&streq(n2,p2);
        if(n0)il2cpp_free(n0); if(n1)il2cpp_free(n1); if(n2)il2cpp_free(n2);
        if(ok)return m;
    }
    return nullptr;
}

static const MethodInfo* find_method_paramcount(Il2CppClass* c,const char* name,int count){
    if(!c)return nullptr; void* it=nullptr; const MethodInfo* m=nullptr;
    while((m=il2cpp_class_get_methods(c,&it))!=nullptr){
        const char* mn=il2cpp_method_get_name(m); if(!streq(mn,name))continue;
        if((int)il2cpp_method_get_param_count(m)==count)return m;
    }
    return nullptr;
}

static const MethodInfo* find_method1_hierarchy(Il2CppClass* c,const char* name,const char* paramType){
    for(Il2CppClass* cur=c;cur;cur=il2cpp_class_get_parent(cur)){
        const MethodInfo* m=find_method_param1(cur,name,paramType);
        if(m)return m;
    }
    return nullptr;
}

static Il2CppObject* invoke(const MethodInfo* m,void* obj,void** args=nullptr){ if(!m)return nullptr; Il2CppObject* ex=nullptr; Il2CppObject* r=il2cpp_runtime_invoke(m,obj,args,&ex); if(ex)return nullptr; return r; }
static int boxed_i32(Il2CppObject* o,int def=0){ if(!o)return def; void* p=il2cpp_object_unbox(o); return p?*(int*)p:def; }
static bool boxed_bool(Il2CppObject* o,bool def=false){ if(!o)return def; void* p=il2cpp_object_unbox(o); return p?(*(u8*)p!=0):def; }
static float boxed_float(Il2CppObject* o,float def=0){ if(!o)return def; void* p=il2cpp_object_unbox(o); return p?*(float*)p:def; }
static void string_ascii(Il2CppString* s,char* out,usize cap){ if(!out||cap==0)return; out[0]=0; if(!s)return; int n=il2cpp_string_length(s); const u16* ch=il2cpp_string_chars(s); if(!ch)return; usize j=0; for(int i=0;i<n&&j+1<cap;++i){ u16 c=ch[i]; out[j++]=(c<128)?(char)c:'?'; } out[j]=0; }


struct F2{float x,y;}; struct F3{float x,y,z;}; struct F4{float x,y,z,w;}; struct Rect4{float x,y,width,height;};

static Il2CppClass *C_Object,*C_Component,*C_GameObject,*C_Transform,*C_RectTransform,*C_RectTransformUtility,*C_Input,*C_Resources,*C_ScriptableObject,*C_Material,*C_Shader,*C_Renderer,*C_Behaviour,*C_Graphic,*C_Slider;
static const MethodInfo *M_Object_get_name,*M_Object_GetInstanceID,*M_Object_Instantiate,*M_Resources_FindAll,*M_GameObject_GetComponents,*M_GameObject_SetActive,*M_GameObject_get_activeSelf,*M_GameObject_get_activeInHierarchy,*M_GameObject_get_transform,*M_Transform_get_parent,*M_Transform_get_position,*M_Transform_SetParent,*M_Transform_IsChildOf,*M_Transform_InverseTransformPoint,*M_Component_get_gameObject,*M_Renderer_get_sharedMaterial,*M_Renderer_set_sharedMaterial;
static const MethodInfo *M_RectTransform_get_rect,*M_RectTransform_get_anchoredPosition,*M_RectTransform_set_anchoredPosition,*M_RectTransform_get_anchorMin,*M_RectTransform_set_anchorMin,*M_RectTransform_get_anchorMax,*M_RectTransform_set_anchorMax,*M_RectTransform_get_sizeDelta,*M_RectTransform_set_sizeDelta,*M_RectTransform_get_pivot,*M_RectTransform_set_pivot,*M_RectTransformUtility_ScreenPointToLocalPointInRectangle,*M_Input_get_mousePosition;
static const MethodInfo *M_Behaviour_get_enabled,*M_Behaviour_set_enabled,*M_Graphic_get_raycastTarget,*M_Graphic_set_raycastTarget;
static const MethodInfo *M_Renderer_get_sharedMaterials,*M_Renderer_get_materials,*M_Renderer_get_bounds;
static const MethodInfo *M_Material_get_shader,*M_Material_GetColorStr,*M_Material_GetFloatStr,*M_Material_GetVectorStr,*M_Material_GetTextureStr,*M_Material_SetColorStr;
static const MethodInfo *M_Shader_GetPropertyCount,*M_Shader_GetPropertyName,*M_Shader_GetPropertyType;
static const MethodInfo *M_Shader_GetGlobalColorStr,*M_Shader_GetGlobalFloatStr,*M_Shader_GetGlobalVectorStr,*M_Shader_GetGlobalTextureStr;
static int g_snapshot=0;

static void log_resolve(const char* kind,const char* name,bool ok){ char b[512]; psprintf(b,"RESOLVE %-8s %-56s : %s\r\n",kind,name,ok?"OK":"MISSING"); log_raw(b); }

static bool init_unity_methods(){
    log_raw("-- Unity API resolution --\r\n");
    C_Object=find_class("UnityEngine","Object"); log_resolve("class","UnityEngine.Object",C_Object!=nullptr);
    C_Component=find_class("UnityEngine","Component"); log_resolve("class","UnityEngine.Component",C_Component!=nullptr);
    C_GameObject=find_class("UnityEngine","GameObject"); log_resolve("class","UnityEngine.GameObject",C_GameObject!=nullptr);
    C_Transform=find_class("UnityEngine","Transform"); log_resolve("class","UnityEngine.Transform",C_Transform!=nullptr);
    C_RectTransform=find_class("UnityEngine","RectTransform"); log_resolve("class","UnityEngine.RectTransform [manual picker]",C_RectTransform!=nullptr);
    C_RectTransformUtility=find_class("UnityEngine","RectTransformUtility"); log_resolve("class","UnityEngine.RectTransformUtility [manual picker]",C_RectTransformUtility!=nullptr);
    C_Input=find_class("UnityEngine","Input"); log_resolve("class","UnityEngine.Input [manual picker]",C_Input!=nullptr);
    C_Resources=find_class("UnityEngine","Resources"); log_resolve("class","UnityEngine.Resources",C_Resources!=nullptr);
    C_ScriptableObject=find_class("UnityEngine","ScriptableObject"); log_resolve("class","UnityEngine.ScriptableObject",C_ScriptableObject!=nullptr);
    C_Material=find_class("UnityEngine","Material"); log_resolve("class","UnityEngine.Material",C_Material!=nullptr);
    C_Shader=find_class("UnityEngine","Shader"); log_resolve("class","UnityEngine.Shader",C_Shader!=nullptr);
    C_Renderer=find_class("UnityEngine","Renderer"); log_resolve("class","UnityEngine.Renderer",C_Renderer!=nullptr);
    C_Behaviour=find_class("UnityEngine","Behaviour"); log_resolve("class","UnityEngine.Behaviour [picker safety]",C_Behaviour!=nullptr);
    C_Graphic=find_class("UnityEngine.UI","Graphic"); log_resolve("class","UnityEngine.UI.Graphic [picker safety]",C_Graphic!=nullptr);
    C_Slider=find_class("UnityEngine.UI","Slider"); log_resolve("class","UnityEngine.UI.Slider [picker safety]",C_Slider!=nullptr);
    if(!C_Object||!C_Component||!C_GameObject||!C_Transform||!C_Resources||!C_ScriptableObject||!C_Material||!C_Shader||!C_Renderer) return false;
    M_Object_get_name=il2cpp_class_get_method_from_name(C_Object,"get_name",0); log_resolve("method","Object.get_name()",M_Object_get_name!=nullptr);
    M_Object_GetInstanceID=il2cpp_class_get_method_from_name(C_Object,"GetInstanceID",0); log_resolve("method","Object.GetInstanceID() [optional]",M_Object_GetInstanceID!=nullptr);
    M_Object_Instantiate=find_method_param1(C_Object,"Instantiate","UnityEngine.Object"); log_resolve("method","Object.Instantiate(Object)",M_Object_Instantiate!=nullptr);
    M_Resources_FindAll=il2cpp_class_get_method_from_name(C_Resources,"FindObjectsOfTypeAll",1); log_resolve("method","Resources.FindObjectsOfTypeAll(Type)",M_Resources_FindAll!=nullptr);
    M_GameObject_GetComponents=find_method_param1(C_GameObject,"GetComponents","System.Type"); log_resolve("method","GameObject.GetComponents(Type)",M_GameObject_GetComponents!=nullptr);
    M_GameObject_SetActive=find_method_param1(C_GameObject,"SetActive","System.Boolean"); log_resolve("method","GameObject.SetActive(Boolean)",M_GameObject_SetActive!=nullptr);
    M_GameObject_get_activeSelf=il2cpp_class_get_method_from_name(C_GameObject,"get_activeSelf",0); log_resolve("method","GameObject.get_activeSelf()",M_GameObject_get_activeSelf!=nullptr);
    M_GameObject_get_activeInHierarchy=il2cpp_class_get_method_from_name(C_GameObject,"get_activeInHierarchy",0); log_resolve("method","GameObject.get_activeInHierarchy()",M_GameObject_get_activeInHierarchy!=nullptr);
    M_GameObject_get_transform=il2cpp_class_get_method_from_name(C_GameObject,"get_transform",0); log_resolve("method","GameObject.get_transform()",M_GameObject_get_transform!=nullptr);
    M_Transform_get_parent=il2cpp_class_get_method_from_name(C_Transform,"get_parent",0); log_resolve("method","Transform.get_parent()",M_Transform_get_parent!=nullptr);
    M_Transform_SetParent=find_method_param2(C_Transform,"SetParent","UnityEngine.Transform","System.Boolean"); log_resolve("method","Transform.SetParent(Transform, Boolean)",M_Transform_SetParent!=nullptr);
    M_Transform_IsChildOf=find_method_param1(C_Transform,"IsChildOf","UnityEngine.Transform"); log_resolve("method","Transform.IsChildOf(Transform)",M_Transform_IsChildOf!=nullptr);
    M_Transform_InverseTransformPoint=find_method_param1(C_Transform,"InverseTransformPoint","UnityEngine.Vector3"); log_resolve("method","Transform.InverseTransformPoint(Vector3) [manual picker]",M_Transform_InverseTransformPoint!=nullptr);
    if(C_RectTransform){
        M_RectTransform_get_rect=il2cpp_class_get_method_from_name(C_RectTransform,"get_rect",0);
        M_RectTransform_get_anchoredPosition=il2cpp_class_get_method_from_name(C_RectTransform,"get_anchoredPosition",0);
        M_RectTransform_set_anchoredPosition=find_method_param1(C_RectTransform,"set_anchoredPosition","UnityEngine.Vector2");
        M_RectTransform_get_anchorMin=il2cpp_class_get_method_from_name(C_RectTransform,"get_anchorMin",0);
        M_RectTransform_set_anchorMin=find_method_param1(C_RectTransform,"set_anchorMin","UnityEngine.Vector2");
        M_RectTransform_get_anchorMax=il2cpp_class_get_method_from_name(C_RectTransform,"get_anchorMax",0);
        M_RectTransform_set_anchorMax=find_method_param1(C_RectTransform,"set_anchorMax","UnityEngine.Vector2");
        M_RectTransform_get_sizeDelta=il2cpp_class_get_method_from_name(C_RectTransform,"get_sizeDelta",0);
        M_RectTransform_set_sizeDelta=find_method_param1(C_RectTransform,"set_sizeDelta","UnityEngine.Vector2");
        M_RectTransform_get_pivot=il2cpp_class_get_method_from_name(C_RectTransform,"get_pivot",0);
        M_RectTransform_set_pivot=find_method_param1(C_RectTransform,"set_pivot","UnityEngine.Vector2");
    }
    log_resolve("method","RectTransform.get_rect() [manual picker]",M_RectTransform_get_rect!=nullptr);
    log_resolve("method","RectTransform.get_anchoredPosition() [manual picker]",M_RectTransform_get_anchoredPosition!=nullptr);
    log_resolve("method","RectTransform.set_anchoredPosition(Vector2) [manual picker]",M_RectTransform_set_anchoredPosition!=nullptr);
    log_resolve("method","RectTransform anchor/size/pivot geometry accessors [picker repair]",M_RectTransform_get_anchorMin&&M_RectTransform_set_anchorMin&&M_RectTransform_get_anchorMax&&M_RectTransform_set_anchorMax&&M_RectTransform_get_sizeDelta&&M_RectTransform_set_sizeDelta&&M_RectTransform_get_pivot&&M_RectTransform_set_pivot);
    if(C_RectTransformUtility) M_RectTransformUtility_ScreenPointToLocalPointInRectangle=find_method_paramcount(C_RectTransformUtility,"ScreenPointToLocalPointInRectangle",4);
    log_resolve("method","RectTransformUtility.ScreenPointToLocalPointInRectangle(...) [manual picker]",M_RectTransformUtility_ScreenPointToLocalPointInRectangle!=nullptr);
    if(C_Input) M_Input_get_mousePosition=il2cpp_class_get_method_from_name(C_Input,"get_mousePosition",0);
    log_resolve("method","Input.get_mousePosition() [manual picker]",M_Input_get_mousePosition!=nullptr);
    M_Component_get_gameObject=il2cpp_class_get_method_from_name(C_Component,"get_gameObject",0); log_resolve("method","Component.get_gameObject()",M_Component_get_gameObject!=nullptr);
    if(C_Behaviour){ M_Behaviour_get_enabled=il2cpp_class_get_method_from_name(C_Behaviour,"get_enabled",0); M_Behaviour_set_enabled=find_method_param1(C_Behaviour,"set_enabled","System.Boolean"); }
    log_resolve("method","Behaviour.get_enabled() [picker safety]",M_Behaviour_get_enabled!=nullptr);
    log_resolve("method","Behaviour.set_enabled(Boolean) [picker safety]",M_Behaviour_set_enabled!=nullptr);
    if(C_Graphic){ M_Graphic_get_raycastTarget=il2cpp_class_get_method_from_name(C_Graphic,"get_raycastTarget",0); M_Graphic_set_raycastTarget=find_method_param1(C_Graphic,"set_raycastTarget","System.Boolean"); }
    log_resolve("method","Graphic.get_raycastTarget() [picker safety]",M_Graphic_get_raycastTarget!=nullptr);
    log_resolve("method","Graphic.set_raycastTarget(Boolean) [picker safety]",M_Graphic_set_raycastTarget!=nullptr);
    M_Renderer_get_sharedMaterial=il2cpp_class_get_method_from_name(C_Renderer,"get_sharedMaterial",0); log_resolve("method","Renderer.get_sharedMaterial()",M_Renderer_get_sharedMaterial!=nullptr);
    M_Renderer_set_sharedMaterial=find_method_param1(C_Renderer,"set_sharedMaterial","UnityEngine.Material"); log_resolve("method","Renderer.set_sharedMaterial(Material)",M_Renderer_set_sharedMaterial!=nullptr);
    M_Renderer_get_sharedMaterials=il2cpp_class_get_method_from_name(C_Renderer,"get_sharedMaterials",0); log_resolve("method","Renderer.get_sharedMaterials()",M_Renderer_get_sharedMaterials!=nullptr);
    M_Renderer_get_materials=il2cpp_class_get_method_from_name(C_Renderer,"get_materials",0); log_resolve("method","Renderer.get_materials()",M_Renderer_get_materials!=nullptr);
    M_Renderer_get_bounds=il2cpp_class_get_method_from_name(C_Renderer,"get_bounds",0); log_resolve("method","Renderer.get_bounds()",M_Renderer_get_bounds!=nullptr);

    M_Material_get_shader=il2cpp_class_get_method_from_name(C_Material,"get_shader",0); log_resolve("method","Material.get_shader()",M_Material_get_shader!=nullptr);
    M_Material_GetColorStr=find_method_param1(C_Material,"GetColor","System.String"); log_resolve("method","Material.GetColor(String)",M_Material_GetColorStr!=nullptr);
    M_Material_GetFloatStr=find_method_param1(C_Material,"GetFloat","System.String"); log_resolve("method","Material.GetFloat(String)",M_Material_GetFloatStr!=nullptr);
    M_Material_GetVectorStr=find_method_param1(C_Material,"GetVector","System.String"); log_resolve("method","Material.GetVector(String)",M_Material_GetVectorStr!=nullptr);
    M_Material_GetTextureStr=find_method_param1(C_Material,"GetTexture","System.String"); log_resolve("method","Material.GetTexture(String)",M_Material_GetTextureStr!=nullptr);
    M_Material_SetColorStr=find_method_param2(C_Material,"SetColor","System.String","UnityEngine.Color"); log_resolve("method","Material.SetColor(String, Color)",M_Material_SetColorStr!=nullptr);

    M_Shader_GetPropertyCount=il2cpp_class_get_method_from_name(C_Shader,"GetPropertyCount",0); log_resolve("method","Shader.GetPropertyCount()",M_Shader_GetPropertyCount!=nullptr);
    M_Shader_GetPropertyName=find_method_param1(C_Shader,"GetPropertyName","System.Int32"); log_resolve("method","Shader.GetPropertyName(Int32)",M_Shader_GetPropertyName!=nullptr);
    M_Shader_GetPropertyType=find_method_param1(C_Shader,"GetPropertyType","System.Int32"); log_resolve("method","Shader.GetPropertyType(Int32)",M_Shader_GetPropertyType!=nullptr);
    M_Shader_GetGlobalColorStr=find_method_param1(C_Shader,"GetGlobalColor","System.String"); log_resolve("method","Shader.GetGlobalColor(String)",M_Shader_GetGlobalColorStr!=nullptr);
    M_Shader_GetGlobalFloatStr=find_method_param1(C_Shader,"GetGlobalFloat","System.String"); log_resolve("method","Shader.GetGlobalFloat(String)",M_Shader_GetGlobalFloatStr!=nullptr);
    M_Shader_GetGlobalVectorStr=find_method_param1(C_Shader,"GetGlobalVector","System.String"); log_resolve("method","Shader.GetGlobalVector(String)",M_Shader_GetGlobalVectorStr!=nullptr);
    M_Shader_GetGlobalTextureStr=find_method_param1(C_Shader,"GetGlobalTexture","System.String"); log_resolve("method","Shader.GetGlobalTexture(String)",M_Shader_GetGlobalTextureStr!=nullptr);

    return M_Object_get_name&&M_Resources_FindAll&&M_GameObject_GetComponents&&M_Material_get_shader&&M_Shader_GetPropertyCount&&M_Shader_GetPropertyName&&M_Shader_GetPropertyType;
}

static void object_name(void* obj,char* out,usize cap){ if(!obj){if(out&&cap)out[0]=0;return;} string_ascii((Il2CppString*)invoke(M_Object_get_name,obj,nullptr),out,cap); }
static int object_id(void* obj){ return (obj&&M_Object_GetInstanceID)?boxed_i32(invoke(M_Object_GetInstanceID,obj,nullptr),0):0; }
static void class_full(Il2CppClass* c,char* out,usize cap){ if(!out||cap==0)return; out[0]=0; if(!c)return; const char* ns=il2cpp_class_get_namespace(c); const char* n=il2cpp_class_get_name(c); if(ns&&*ns){scopy(out,cap,ns);sappend(out,cap,".");sappend(out,cap,n?n:"");} else scopy(out,cap,n?n:""); }

static bool is_tps_class(Il2CppClass* c){ const char* ns=c?il2cpp_class_get_namespace(c):nullptr; return ns&&contains_i(ns,"TPS"); }
static bool key_name(const char* s){
    if(!s)return false;
    const char* k[]={"water","aquarium","colour","color","tint","environment","render","fog","absorp","effect","modifier","material","shader","visual","override","volume","postprocess","post_process","ambient"};
    for(unsigned i=0;i<sizeof(k)/sizeof(k[0]);++i) if(contains_i(s,k[i])) return true;
    return false;
}
static bool target_go_name(const char* s){
    if(!s)return false;
    const char* k[]={"aquarium","water surface","dredge","aberr","lantern","fish","angler","barreleye","mackerel","perch","voideye","cod","atlanti","gnashing","grotesque","bursting"};
    for(unsigned i=0;i<sizeof(k)/sizeof(k[0]);++i) if(contains_i(s,k[i])) return true;
    return false;
}
static bool target_component_class(Il2CppClass* c){ if(!c||!is_tps_class(c))return false; char f[256]; class_full(c,f,sizeof(f)); return key_name(f); }

static bool type_eq(const char* t,const char* n){ return t&&n&&streq(t,n); }
static void format_value(Il2CppObject* o,const char* declared,char* out,usize cap){
    if(!out||cap==0)return; out[0]=0; if(!o){scopy(out,cap,"<null>");return;}
    void* p=il2cpp_object_unbox(o);
    if(type_eq(declared,"System.String")){ char s[512]; string_ascii((Il2CppString*)o,s,sizeof(s)); psprintf(out,"\"%s\"",s); return; }
    if(type_eq(declared,"System.Boolean")&&p){psprintf(out,"%s",(*(u8*)p)?"true":"false");return;}
    if((type_eq(declared,"System.Int32")||type_eq(declared,"System.UInt32"))&&p){psprintf(out,"%d",*(i32*)p);return;}
    if((type_eq(declared,"System.Int16")||type_eq(declared,"System.UInt16"))&&p){psprintf(out,"%d",(int)*(short*)p);return;}
    if((type_eq(declared,"System.Byte")||type_eq(declared,"System.SByte"))&&p){psprintf(out,"%d",(int)*(u8*)p);return;}
    if(type_eq(declared,"System.Single")&&p){psprintf(out,"%.7g",(double)*(float*)p);return;}
    if(type_eq(declared,"System.Double")&&p){psprintf(out,"%.9g",*(double*)p);return;}
    if((type_eq(declared,"UnityEngine.Color")||type_eq(declared,"UnityEngine.Vector4"))&&p){F4 v=*(F4*)p;psprintf(out,"(%.6f, %.6f, %.6f, %.6f)",(double)v.x,(double)v.y,(double)v.z,(double)v.w);return;}
    if(type_eq(declared,"UnityEngine.Vector3")&&p){F3 v=*(F3*)p;psprintf(out,"(%.6f, %.6f, %.6f)",(double)v.x,(double)v.y,(double)v.z);return;}
    if(type_eq(declared,"UnityEngine.Vector2")&&p){F2 v=*(F2*)p;psprintf(out,"(%.6f, %.6f)",(double)v.x,(double)v.y);return;}
    Il2CppClass* oc=il2cpp_object_get_class(o); char cf[256]; class_full(oc,cf,sizeof(cf));
    // For obvious UnityEngine.Object references, include its object name/id.
    bool unityObj=false; for(Il2CppClass* c=oc;c;c=il2cpp_class_get_parent(c)){ char q[128]; class_full(c,q,sizeof(q)); if(streq(q,"UnityEngine.Object")){unityObj=true;break;} }
    if(unityObj){ char nm[256]; object_name(o,nm,sizeof(nm)); psprintf(out,"<%s name='%s' id=%d>",cf,nm,object_id(o)); return; }
    // Enums and other value types: print first 32 bits as a useful raw discriminator.
    if(p){ psprintf(out,"<%s raw32=%d>",cf,*(i32*)p); return; }
    psprintf(out,"<%s>",cf);
}

static void dump_fields_for_class(void* comp,Il2CppClass* c,bool allFields,const char* indent){
    char buf[1800];
    for(Il2CppClass* cur=c;cur;cur=il2cpp_class_get_parent(cur)){
        char cfull[256]; class_full(cur,cfull,sizeof(cfull));
        if(streq(cfull,"UnityEngine.MonoBehaviour")||streq(cfull,"UnityEngine.Behaviour")||streq(cfull,"UnityEngine.Component")||streq(cfull,"UnityEngine.Object")) break;
        void* it=nullptr; Il2CppFieldInfo* f=nullptr;
        while((f=il2cpp_class_get_fields(cur,&it))!=nullptr){
            const char* fn=il2cpp_field_get_name(f); if(!fn)continue; if(!allFields&&!key_name(fn))continue;
            const Il2CppType* ft=il2cpp_field_get_type(f); char* tn=ft?il2cpp_type_get_name(ft):nullptr;
            Il2CppObject* vo=il2cpp_field_get_value_object(f,comp); char val[700]; format_value(vo,tn?tn:"",val,sizeof(val));
            psprintf(buf,"%sFIELD %s.%s : %s = %s\r\n",indent,cfull,fn,tn?tn:"?",val); log_raw(buf); if(tn)il2cpp_free(tn);
        }
    }
}

static void dump_properties_for_class(void* comp,Il2CppClass* c,bool allProps,const char* indent){
    (void)comp; char buf[1400];
    for(Il2CppClass* cur=c;cur;cur=il2cpp_class_get_parent(cur)){
        char cfull[256]; class_full(cur,cfull,sizeof(cfull));
        if(streq(cfull,"UnityEngine.MonoBehaviour")||streq(cfull,"UnityEngine.Behaviour")||streq(cfull,"UnityEngine.Component")||streq(cfull,"UnityEngine.Object")) break;
        void* it=nullptr; const Il2CppPropertyInfo* pr=nullptr;
        while((pr=il2cpp_class_get_properties(cur,&it))!=nullptr){
            const char* pn=il2cpp_property_get_name(pr); if(!pn)continue; if(!allProps&&!key_name(pn))continue;
            const MethodInfo* gm=il2cpp_property_get_get_method(pr); const Il2CppType* rt=gm?il2cpp_method_get_return_type(gm):nullptr; char* tn=rt?il2cpp_type_get_name(rt):nullptr;
            psprintf(buf,"%sPROP  %s.%s : %s = <getter not invoked; safe read-only mode>\r\n",indent,cfull,pn,tn?tn:"?"); log_raw(buf); if(tn)il2cpp_free(tn);
        }
    }
}

static Il2CppArray* get_components(void* go){
    const Il2CppType* ct=il2cpp_class_get_type(C_Component); Il2CppObject* typeObj=il2cpp_type_get_object(ct); void* a[1]={typeObj}; return (Il2CppArray*)invoke(M_GameObject_GetComponents,go,a);
}



static bool target_asset_name(const char* s){
    if(!s)return false;
    const char* k[]={"dredge","aberr","bursting","gnashing","grotesque","voideye","threeheaded","three_headed","oceanicperch","oceanic_perch","lanternfish","lantern fish","barreleye","fishcharacteristic"};
    for(unsigned i=0;i<sizeof(k)/sizeof(k[0]);++i) if(contains_i(s,k[i])) return true;
    return false;
}
static bool target_meta_class(Il2CppClass* c){
    if(!c||!is_tps_class(c))return false;
    char f[256]; class_full(c,f,sizeof(f));
    return contains_i(f,"aquarium")||contains_i(f,"water");
}


static F4 read_boxed_f4(Il2CppObject* o){ F4 z{0,0,0,0}; if(!o)return z; void* p=il2cpp_object_unbox(o); if(p)z=*(F4*)p; return z; }
static float read_mat_float(void* m,Il2CppString* p){ if(!M_Material_GetFloatStr)return 0; void* a[1]={p}; return boxed_float(invoke(M_Material_GetFloatStr,m,a),0); }
static F4 read_mat_color(void* m,Il2CppString* p){ if(!M_Material_GetColorStr)return F4{0,0,0,0}; void* a[1]={p}; return read_boxed_f4(invoke(M_Material_GetColorStr,m,a)); }
static F4 read_mat_vector(void* m,Il2CppString* p){ if(!M_Material_GetVectorStr)return F4{0,0,0,0}; void* a[1]={p}; return read_boxed_f4(invoke(M_Material_GetVectorStr,m,a)); }
static void* read_mat_texture(void* m,Il2CppString* p){ if(!M_Material_GetTextureStr)return nullptr; void* a[1]={p}; return invoke(M_Material_GetTextureStr,m,a); }
static F4 read_global_color(Il2CppString* p){ if(!M_Shader_GetGlobalColorStr)return F4{0,0,0,0}; void* a[1]={p}; return read_boxed_f4(invoke(M_Shader_GetGlobalColorStr,nullptr,a)); }
static F4 read_global_vector(Il2CppString* p){ if(!M_Shader_GetGlobalVectorStr)return F4{0,0,0,0}; void* a[1]={p}; return read_boxed_f4(invoke(M_Shader_GetGlobalVectorStr,nullptr,a)); }
static float read_global_float(Il2CppString* p){ if(!M_Shader_GetGlobalFloatStr)return 0; void* a[1]={p}; return boxed_float(invoke(M_Shader_GetGlobalFloatStr,nullptr,a),0); }
static void* read_global_texture(Il2CppString* p){ if(!M_Shader_GetGlobalTextureStr)return nullptr; void* a[1]={p}; return invoke(M_Shader_GetGlobalTextureStr,nullptr,a); }

static bool aquarium_shader(void* sh){
    char sn[256]; object_name(sh,sn,sizeof(sn));
    return streq(sn,"Two Point/Aquarium Water Surface")||streq(sn,"Two Point/Aquarium Glass");
}


static Il2CppFieldInfo* find_field_hierarchy(Il2CppClass* c,const char* name){
    static P78MetadataCache<Il2CppFieldInfo*,256> cache;if(auto f=cache.get(c,"",name))return f;
    for(Il2CppClass* cur=c;cur;cur=il2cpp_class_get_parent(cur)){
        void* it=nullptr; Il2CppFieldInfo* f=nullptr;
        while((f=il2cpp_class_get_fields(cur,&it))!=nullptr){ const char* fn=il2cpp_field_get_name(f); if(fn&&streq(fn,name)){cache.put(c,"",name,f);return f;} }
    }
    return nullptr;
}
static const MethodInfo* find_method0_hierarchy(Il2CppClass* c,const char* name){
    for(Il2CppClass* cur=c;cur;cur=il2cpp_class_get_parent(cur)){
        void* it=nullptr; const MethodInfo* m=nullptr;
        while((m=il2cpp_class_get_methods(cur,&it))!=nullptr){ const char* mn=il2cpp_method_get_name(m); if(mn&&streq(mn,name)&&il2cpp_method_get_param_count(m)==0) return m; }
    }
    return nullptr;
}
static Il2CppObject* field_object(void* obj,const char* name){
    if(!obj)return nullptr; Il2CppClass* c=il2cpp_object_get_class((Il2CppObject*)obj); Il2CppFieldInfo* f=find_field_hierarchy(c,name); return f?il2cpp_field_get_value_object(f,obj):nullptr;
}
static int field_i32(void* obj,const char* name,int def=0){ return boxed_i32(field_object(obj,name),def); }
static i64 field_i64(void* obj,const char* name,i64 def=0){ Il2CppObject* o=field_object(obj,name); if(!o)return def; void* p=il2cpp_object_unbox(o); return p?*(i64*)p:def; }
static float field_float(void* obj,const char* name,float def=0){ return boxed_float(field_object(obj,name),def); }
static F4 field_f4(void* obj,const char* name){ F4 z{0,0,0,0}; Il2CppObject* o=field_object(obj,name); if(!o)return z; void* p=il2cpp_object_unbox(o); if(p)z=*(F4*)p; return z; }
static void* static_ref_field(Il2CppClass* c,const char* name){
    Il2CppFieldInfo* f=find_field_hierarchy(c,name); if(!f||!il2cpp_field_static_get_value)return nullptr; void* p=nullptr; il2cpp_field_static_get_value(f,&p); return p;
}
static int collection_count(void* obj){
    if(!obj)return -1; const MethodInfo* m=find_method0_hierarchy(il2cpp_object_get_class((Il2CppObject*)obj),"get_Count"); return m?boxed_i32(invoke(m,obj,nullptr),-1):-1;
}
static void log_room_identity(void* room){
    char buf[1800]; if(!room){log_raw("    RoomInstance: <null>\r\n");return;} Il2CppClass* rc=il2cpp_object_get_class((Il2CppObject*)room); char rf[256]; class_full(rc,rf,sizeof(rf));
    psprintf(buf,"    RoomInstance: <%s>\r\n",rf); log_raw(buf);
    int emitted=0;
    for(Il2CppClass* cur=rc;cur&&emitted<24;cur=il2cpp_class_get_parent(cur)){
        void* it=nullptr; Il2CppFieldInfo* f=nullptr;
        while(emitted<24&&(f=il2cpp_class_get_fields(cur,&it))!=nullptr){
            const char* fn=il2cpp_field_get_name(f); if(!fn)continue;
            if(!(contains_i(fn,"id")||contains_i(fn,"name")||contains_i(fn,"definition")))continue;
            const Il2CppType* ft=il2cpp_field_get_type(f); char* tn=ft?il2cpp_type_get_name(ft):nullptr; Il2CppObject* vo=il2cpp_field_get_value_object(f,room); char val[700]; format_value(vo,tn?tn:"",val,sizeof(val));
            psprintf(buf,"      %s : %s = %s\r\n",fn,tn?tn:"?",val); log_raw(buf); if(tn)il2cpp_free(tn); ++emitted;
        }
    }
}
static void log_tint_definition(void* tint,const char* prefix){
    char buf[1400]; if(!tint){ psprintf(buf,"%s<none>\r\n",prefix);log_raw(buf);return; }
    char n[256]; object_name(tint,n,sizeof(n)); float score=field_float(tint,"Score",0); F4 c=field_f4(tint,"TintColor"); i64 serialId=field_i64(tint,"_id",0);
    psprintf(buf,"%s'%s' instanceId=%d serialID=%lld  Score=%.6f  TintColor=(%.6f, %.6f, %.6f, %.6f)\r\n",prefix,n,object_id(tint),(long long)serialId,(double)score,(double)c.x,(double)c.y,(double)c.z,(double)c.w); log_raw(buf);
}

static void set_field_float(void* obj,const char* name,float v){
    if(!obj||!il2cpp_field_set_value)return; Il2CppFieldInfo* f=find_field_hierarchy(il2cpp_object_get_class((Il2CppObject*)obj),name); if(f)il2cpp_field_set_value((Il2CppObject*)obj,f,&v);
}
static void set_field_f4(void* obj,const char* name,F4 v){
    if(!obj||!il2cpp_field_set_value)return; Il2CppFieldInfo* f=find_field_hierarchy(il2cpp_object_get_class((Il2CppObject*)obj),name); if(f)il2cpp_field_set_value((Il2CppObject*)obj,f,&v);
}
static void set_field_ref(void* obj,const char* name,void* ref){
    if(!obj||!il2cpp_field_set_value)return; Il2CppFieldInfo* f=find_field_hierarchy(il2cpp_object_get_class((Il2CppObject*)obj),name); if(f){ void* r=ref; il2cpp_field_set_value((Il2CppObject*)obj,f,&r); }
}


static void* get_aquarium_manager(){
    Il2CppClass* mc=find_class("TPS.Game","AquariumManager"); if(!mc)return nullptr;
    void* manager=static_ref_field(mc,"<Instance>k__BackingField");
    if(!manager){ const MethodInfo* gi=find_method0_hierarchy(mc,"get_Instance"); if(gi)manager=invoke(gi,nullptr,nullptr); }
    return manager;
}

struct StateList{ void* states[32]; int count; };
static StateList get_states(){
    static StateList cached{};static void* lastDict=nullptr;static void* lastEntries=nullptr;static int lastVersion=-1;static u64 epoch=0;
    StateList out{}; void* manager=get_aquarium_manager(); if(!manager)return out;
    Il2CppObject* dict=field_object(manager,"_aquariumStates");
    if(!dict){ const MethodInfo* ga=find_method0_hierarchy(il2cpp_object_get_class((Il2CppObject*)manager),"get_AquariumStates"); if(ga)dict=invoke(ga,manager,nullptr); }
    if(!dict)return out;
    int dcnt=collection_count(dict); Il2CppObject* entries=field_object(dict,"_entries"); if(!entries)entries=field_object(dict,"entries"); if(!entries)return out;
    int version=field_i32(dict,"_version",-1);
    if(p78_workDepth&&version>=0&&epoch==p78_epoch&&lastDict==dict&&lastEntries==entries&&lastVersion==version){++p78_stateHits;return cached;}
    ++p78_stateScans;
    int rawCount=field_i32(dict,"_count",dcnt); if(rawCount<0)rawCount=dcnt; if(rawCount>256)rawCount=256;
    const MethodInfo* getValue=find_method1_hierarchy(il2cpp_object_get_class(entries),"GetValue","System.Int32"); if(!getValue)return out;
    for(int i=0;i<rawCount&&out.count<32;++i){ int idx=i; void* a[1]={&idx}; Il2CppObject* entry=invoke(getValue,entries,a); if(!entry)continue; Il2CppObject* state=field_object(entry,"value"); if(!state)state=field_object(entry,"_value"); if(state)out.states[out.count++]=state; }
    if(p78_workDepth&&version>=0){cached=out;epoch=p78_epoch;lastDict=dict;lastEntries=entries;lastVersion=version;}
    return out;
}

static int g_target=0;
struct SavedState{ void* state; void* originalTint; void* overrideTint; bool used; bool active; AquariumGCHandle gcHandle; };
static SavedState g_saved[32]{};

static SavedState* slot_for(void* state){
    for(int i=0;i<32;++i) if(g_saved[i].used&&g_saved[i].state==state)return &g_saved[i];
    for(int i=0;i<32;++i) if(!g_saved[i].used){ g_saved[i].used=true; g_saved[i].state=state; g_saved[i].originalTint=field_object(state,"_highestPriorityAquariumTintDefinition"); return &g_saved[i]; }
    return nullptr;
}


static void* first_loaded_tint(){
    Il2CppClass* tc=find_class("TPS.Game","AquariumTintDefinition"); if(!tc)return nullptr; const Il2CppType* tt=il2cpp_class_get_type(tc); Il2CppObject* to=il2cpp_type_get_object(tt); void* a[1]={to}; Il2CppArray* arr=(Il2CppArray*)invoke(M_Resources_FindAll,nullptr,a); if(!arr)return nullptr;
    for(uptr i=0;i<arr->max_length;++i){ void* o=arr->vector[i]; if(o)return o; } return nullptr;
}

static bool any_active(){ for(int i=0;i<32;++i) if(g_saved[i].used&&g_saved[i].active&&g_saved[i].state&&g_saved[i].overrideTint)return true; return false; }



static F4 g_currentWaterColor{0.167378f,0.272455f,0.292452f,0.0f};
static bool g_p55GlowEnabled=true;
static bool p55_glow_on(){return __atomic_load_n(&g_p55GlowEnabled,__ATOMIC_ACQUIRE);}

// 0.7.29 standard-tank proof state. This build does not call RecalculateHighestPriorityAquariumTint(),
// does not touch Fish/ECS data and does not create a synthetic tint. The retired probe borrowed only the already-live
// retail AquariumTintDefinition pointer from a tinted/Dredge tank. The assignment is one-shot and is never
// reasserted in the background. This isolates whether the cached retail definition pointer alone is enough
// to drive the standard aquarium's surrounding glow.
static bool g_standardRetailPointerProofActive=false;
static void* g_standardRetailPointerProofState=nullptr;
static void* g_standardRetailPointerProofOriginalTint=nullptr;
static void* g_standardRetailPointerProofTint=nullptr;

// 0.7.20 live glow sync:
// - never replaces AquariumState._highestPriorityAquariumTintDefinition;
// - mutates only TintColor on retail definitions the game has already selected;
// - captures the retail colour once so the reset control can restore it;
// - uses the retail colour's original max RGB channel as the HDR multiplier.
// AquariumTintDefinition is a shared ScriptableObject, so all live aquariums using the same
// retail tint definition move together in this POC. Standard/null-tint aquariums are untouched.
struct LiveTintSlot{ void* tint; F4 original; bool used; };
static LiveTintSlot g_liveTintSlots[16]{};

static LiveTintSlot* live_tint_slot(void* tint){
    if(!tint)return nullptr;
    for(int i=0;i<16;++i) if(g_liveTintSlots[i].used&&g_liveTintSlots[i].tint==tint)return &g_liveTintSlots[i];
    for(int i=0;i<16;++i) if(!g_liveTintSlots[i].used){
        g_liveTintSlots[i].used=true;
        g_liveTintSlots[i].tint=tint;
        g_liveTintSlots[i].original=field_f4(tint,"TintColor");
        return &g_liveTintSlots[i];
    }
    return nullptr;
}

static bool live_tint_seen(void** seen,int n,void* tint){
    for(int i=0;i<n;++i)if(seen[i]==tint)return true;
    return false;
}

static float max3f_local(float a,float b,float c){
    float m=a>b?a:b; return m>c?m:c;
}
static float clamp01_glow(float v){ return v<0.0f?0.0f:(v>1.0f?1.0f:v); }

static F4 picker_to_hdr_glow(F4 picker,F4 retailOriginal){
    float hdr=max3f_local(retailOriginal.x,retailOriginal.y,retailOriginal.z);
    if(hdr<0.05f)hdr=1.0f;
    F4 out{
        clamp01_glow(picker.x)*hdr,
        clamp01_glow(picker.y)*hdr,
        clamp01_glow(picker.z)*hdr,
        retailOriginal.w
    };
    return out;
}

static int sync_live_glow_to_picker(F4 picker,bool logIt){
    if(!p55_glow_on())return 0;
    StateList sl=get_states(); if(sl.count<=0)return 0;
    void* seen[16]{}; int seenCount=0,changed=0,skipped=0,shared=0;
    char b[1200];
    if(logIt)log_raw("\r\n-- 0.7.20 LIVE SPECIAL-TINT GLOW SYNC --\r\n");
    for(int i=0;i<sl.count;++i){
        void* state=sl.states[i]; if(!state)continue;
        void* tint=field_object(state,"_highestPriorityAquariumTintDefinition");
        if(!tint){
            ++skipped;
            if(logIt){ psprintf(b,"  state[%d]: no active AquariumTintDefinition (standard aquarium); untouched.\r\n",i+1);log_raw(b); }
            continue;
        }
        if(live_tint_seen(seen,seenCount,tint)){
            ++shared;
            if(logIt){ psprintf(b,"  state[%d]: shares already-synced retail tint=%p; no second write.\r\n",i+1,tint);log_raw(b); }
            continue;
        }
        if(seenCount<16)seen[seenCount++]=tint;
        LiveTintSlot* slot=live_tint_slot(tint);
        if(!slot){
            ++skipped;
            if(logIt){ psprintf(b,"  state[%d]: no live tint slot available; skipped.\r\n",i+1);log_raw(b); }
            continue;
        }
        F4 out=picker_to_hdr_glow(picker,slot->original);
        out.x*=0.45f;out.y*=0.45f;out.z*=0.45f;
        set_field_f4(tint,"TintColor",out);
        ++changed;
        if(logIt){
            char on[256]; object_name(tint,on,sizeof(on));
            float hdr=max3f_local(slot->original.x,slot->original.y,slot->original.z);
            psprintf(b,
                "  state[%d]: retail '%s' id=%d ptr=%p HDRmax=%.4f pickerRGB=(%.3f,%.3f,%.3f) -> TintColor=(%.4f,%.4f,%.4f,%.4f). State pointer unchanged.\r\n",
                i+1,on,object_id(tint),tint,(double)hdr,
                (double)picker.x,(double)picker.y,(double)picker.z,
                (double)out.x,(double)out.y,(double)out.z,(double)out.w);
            log_raw(b);
        }
    }
    if(logIt){
        psprintf(b,
            "GLOW SYNC: picker colour applied in place to %d distinct active special-tint definition(s); standard/null-tint states untouched=%d, shared states=%d. No pointer replacement or reassertion.\r\n",
            changed,skipped,shared);
        log_raw(b);
    }
    return changed;
}

static void restore_live_glow_tints(bool logIt){
    int restored=0; char b[700];
    for(int i=0;i<16;++i){
        LiveTintSlot& p=g_liveTintSlots[i];
        if(!p.used||!p.tint)continue;
        set_field_f4(p.tint,"TintColor",p.original);
        ++restored;
    }
    if(logIt){
        psprintf(b,"GLOW SYNC: restored original retail TintColor on %d captured AquariumTintDefinition asset(s).\r\n",restored);
        log_raw(b);
    }
}

// Forward declarations for the 0.7.49 standard FloorArea sink; implementation appears after
// the 0.7.47 native-analysis helpers, but glow-toggle code lives earlier in this file.
static bool p48_sync_standard_floor_sink_for_current(F4 color,bool verbose);
static void p48_reassert_standard_floor_sink(bool verbose);
static void p48_suspend_standard_floor_sink(bool verbose);
static void p48_clear_standard_floor_sink_if_selected(bool verbose);
static bool g_p55UpdateInstalled=false;
static bool p55_update_ready(){return __atomic_load_n(&g_p55UpdateInstalled,__ATOMIC_ACQUIRE);}
enum P49GlowRoute { P49_ROUTE_UNKNOWN=0, P49_ROUTE_DREDGE=1, P49_ROUTE_STANDARD=2 };
static P49GlowRoute p49_current_glow_route(bool verbose);
static int p49_sync_glow_for_current(F4 color,bool verbose);



// 0.7.29 standard-tank proof builds on the safe 0.7.27 findings.
// 0.7.25 proved all three Dredge fish carry ECAquariumTint while all three standard fish do not.
// 0.7.26 then CTD'd immediately after the first Dredge HasComponent=YES line when it attempted
// EntityManager.GetComponentDataRawRW. 0.7.27 removes that unsafe raw payload call completely.
// Instead it logs the active AquariumTintDefinition object's inherited serialized _id and the _id of every
// loaded AquariumTintDefinition. That gives us the Fish_Aberration definition ID without touching ECS payload memory.
static const MethodInfo* find_method_name_params(Il2CppClass* c,const char* name,u32 pc){
    for(Il2CppClass* cur=c;cur;cur=il2cpp_class_get_parent(cur)){
        void* it=nullptr; const MethodInfo* m=nullptr;
        while((m=il2cpp_class_get_methods(cur,&it))!=nullptr){
            const char* mn=il2cpp_method_get_name(m);
            if(mn&&streq(mn,name)&&il2cpp_method_get_param_count(m)==pc)return m;
        }
    }
    return nullptr;
}
static u32 pe_image_size(void* mod){
    if(!mod)return 0; u8* b=(u8*)mod; u32 pe=*(u32*)(b+0x3c); if(*(u32*)(b+pe)!=0x00004550u)return 0; if(*(u16*)(b+pe+24)!=0x20b)return 0; return *(u32*)(b+pe+24+0x38);
}
static bool ptr_in_module(void* p,void* mod,u32 bytes){
    if(!p||!mod)return false; uptr a=(uptr)p, b=(uptr)mod; u32 sz=pe_image_size(mod); if(!sz)return false; return a>=b && a+(uptr)bytes<=b+(uptr)sz;
}
static void log_method_signature(const MethodInfo* m,const char* prefix){
    if(!m){ char b[320]; psprintf(b,"%s<not found>\r\n",prefix?prefix:""); log_raw(b); return; }
    char b[1500]; const char* mn=il2cpp_method_get_name(m); u32 pc=il2cpp_method_get_param_count(m);
    psprintf(b,"%s%s(",prefix?prefix:"",mn?mn:"?"); log_raw(b);
    for(u32 i=0;i<pc;++i){ const Il2CppType* pt=il2cpp_method_get_param(m,i); char* tn=pt?il2cpp_type_get_name(pt):nullptr; if(i)log_raw(", "); log_raw(tn?tn:"?"); if(tn)il2cpp_free(tn); }
    log_raw(")\r\n");
}
static void dump_class_metadata_small(Il2CppClass* c,const char* title,int methodCap){
    char b[1300]; if(!c){ psprintf(b,"\r\n-- %s -- <class not found>\r\n",title);log_raw(b);return; }
    char cf[300],pf[300]; class_full(c,cf,sizeof(cf)); class_full(il2cpp_class_get_parent(c),pf,sizeof(pf));
    psprintf(b,"\r\n-- %s --\r\nCLASS %s parent=%s\r\n",title,cf,pf);log_raw(b);
    void* fit=nullptr; Il2CppFieldInfo* f=nullptr; int fc=0;
    while((f=il2cpp_class_get_fields(c,&fit))!=nullptr&&fc<64){ const char* fn=il2cpp_field_get_name(f); const Il2CppType* ft=il2cpp_field_get_type(f); char* tn=ft?il2cpp_type_get_name(ft):nullptr; psprintf(b,"  FIELD %s : %s\r\n",fn?fn:"?",tn?tn:"?");log_raw(b);if(tn)il2cpp_free(tn);++fc; }
    void* mit=nullptr; const MethodInfo* m=nullptr; int mc=0;
    while((m=il2cpp_class_get_methods(c,&mit))!=nullptr&&mc<methodCap){ log_method_signature(m,"  METHOD "); ++mc; }
}
struct EntityRaw0725 { i32 Index; i32 Version; };
struct HashSlotEntity0725 { i32 hashCode; i32 next; EntityRaw0725 value; };

static int read_fish_slots_direct(void* fish,EntityRaw0725* out,int cap,int stateIndex){
    char b[1000]; if(!fish||!out||cap<=0)return 0;
    Il2CppObject* slotsObj=field_object(fish,"_slots"); int last=field_i32(fish,"_lastIndex",-1), count=collection_count(fish);
    if(!slotsObj){ log_raw("    DIRECT SLOTS: _slots is null/unavailable.\r\n"); return 0; }
    Il2CppArray* slots=(Il2CppArray*)slotsObj; int arrLen=(int)slots->max_length; if(last<0||last>arrLen)last=arrLen;
    psprintf(b,"    DIRECT SLOTS: array=%p length=%d _lastIndex=%d expectedCount=%d assumed Slot<Entity> stride=%d bytes.\r\n",slots,arrLen,last,count,(int)sizeof(HashSlotEntity0725));log_raw(b);
    u8* data=(u8*)&slots->vector[0]; int emitted=0;
    for(int i=0;i<last&&i<arrLen;++i){
        HashSlotEntity0725* sl=(HashSlotEntity0725*)(data+(uptr)i*sizeof(HashSlotEntity0725));
        psprintf(b,"      slot[%d] hash=%d next=%d entity=(Index=%d,Version=%d)%s\r\n",i,sl->hashCode,sl->next,sl->value.Index,sl->value.Version,sl->hashCode>=0?" <LIVE-CANDIDATE>":"");log_raw(b);
        if(sl->hashCode<0)continue;
        bool dup=false; for(int j=0;j<emitted;++j)if(out[j].Index==sl->value.Index&&out[j].Version==sl->value.Version){dup=true;break;}
        if(!dup&&emitted<cap)out[emitted++]=sl->value;
    }
    psprintf(b,"    DIRECT SLOTS emitted %d unique Entity value(s); HashSet Count=%d.\r\n",emitted,count);log_raw(b);
    return emitted;
}
static int read_fish_enumerator_unboxed(void* fish,EntityRaw0725* out,int cap,int stateIndex){
    char b[1000]; if(!fish||!out||cap<=0)return 0; Il2CppClass* fc=il2cpp_object_get_class((Il2CppObject*)fish);
    const MethodInfo* getEnum=find_method_name_params(fc,"GetEnumerator",0); if(!getEnum){log_raw("    UNBOXED ENUM: GetEnumerator unavailable.\r\n");return 0;}
    Il2CppObject* enBox=invoke(getEnum,fish,nullptr); if(!enBox){log_raw("    UNBOXED ENUM: GetEnumerator failed.\r\n");return 0;}
    Il2CppClass* ec=il2cpp_object_get_class(enBox); if(stateIndex==0)dump_class_metadata_small(ec,"CONCRETE HashSet<Entity>.Enumerator runtime class",80);
    void* enThis=il2cpp_object_unbox(enBox); if(!enThis){log_raw("    UNBOXED ENUM: unbox failed.\r\n");return 0;}
    const MethodInfo* move=find_method_name_params(ec,"MoveNext",0); const MethodInfo* cur=find_method_name_params(ec,"get_Current",0); if(!move||!cur){log_raw("    UNBOXED ENUM: MoveNext/get_Current unavailable.\r\n");return 0;}
    int emitted=0;
    for(int guard=0;guard<64;++guard){
        Il2CppObject* mr=invoke(move,enThis,nullptr); if(!mr){log_raw("    UNBOXED ENUM: MoveNext invoke failed.\r\n");break;} if(!boxed_bool(mr,false))break;
        Il2CppObject* v=invoke(cur,enThis,nullptr); if(!v){log_raw("    UNBOXED ENUM: get_Current invoke failed.\r\n");break;} void* raw=il2cpp_object_unbox(v); if(!raw){log_raw("    UNBOXED ENUM: Entity unbox failed.\r\n");break;}
        EntityRaw0725 e=*(EntityRaw0725*)raw; psprintf(b,"      enumEntity[%d]=(Index=%d,Version=%d)\r\n",emitted,e.Index,e.Version);log_raw(b); if(emitted<cap)out[emitted]=e; ++emitted;
    }
    psprintf(b,"    UNBOXED ENUM emitted %d Entity value(s).\r\n",emitted);log_raw(b); return emitted<cap?emitted:cap;
}
static int collect_fish_entities_readonly(void* state,EntityRaw0725* out,int cap,int stateIndex){
    char b[800]; Il2CppObject* fish=field_object(state,"Fish"); int cnt=collection_count(fish); psprintf(b,"  state[%d] Fish set=%p Count=%d\r\n",stateIndex+1,fish,cnt);log_raw(b); if(!fish)return 0;
    if(stateIndex==0)dump_class_metadata_small(il2cpp_object_get_class(fish),"CONCRETE Fish HashSet<Entity> runtime class",80);
    EntityRaw0725 enumOut[32]{}; int en=read_fish_enumerator_unboxed(fish,enumOut,32,stateIndex); EntityRaw0725 slotOut[32]{}; int sn=read_fish_slots_direct(fish,slotOut,32,stateIndex);
    int n=0; const EntityRaw0725* src=sn>0?slotOut:enumOut; int srcN=sn>0?sn:en;
    for(int i=0;i<srcN&&n<cap;++i)out[n++]=src[i];
    psprintf(b,"    SELECTED READ PATH: %s, entities=%d.\r\n",sn>0?"direct _slots":"unboxed Enumerator fallback",n);log_raw(b); return n;
}

static void reset_target(){
    StateList sl=get_states(); if(sl.count<=0){log_raw("RESET: no aquarium states found.\r\n");return;} if(g_target>=sl.count)g_target=0; void* state=sl.states[g_target];
    const MethodInfo* rec=find_method0_hierarchy(il2cpp_object_get_class((Il2CppObject*)state),"RecalculateHighestPriorityAquariumTint");
    if(rec){invoke(rec,state,nullptr);log_raw("RESET: invoked retail RecalculateHighestPriorityAquariumTint(); 0.7.20 never replaces the AquariumState glow pointer.\r\n");}
    else log_raw("RESET: tint recalculation method unavailable.\r\n");
}

static bool colour_ui_type(Il2CppClass* c){
    if(!c)return false; char f[300]; class_full(c,f,sizeof(f));
    if(!(contains_i(f,"colour")||contains_i(f,"color")||contains_i(f,"customis")||contains_i(f,"customiz")||contains_i(f,"recolour")||contains_i(f,"recolor")))return false;
    return contains_i(f,"TPS.Game") || contains_i(f,"TPS.Core.UI");
}


static void action_cycle(){ StateList sl=get_states(); if(sl.count<=0){log_raw("CYCLE: no aquarium states.\r\n");return;} g_target=(g_target+1)%sl.count; char b[256];psprintf(b,"CYCLE: target aquarium is now %d of %d.\r\n",g_target+1,sl.count);log_raw(b); }


struct Bounds6 { F3 center; F3 extents; };
struct WaterTarget { void* renderer; void* originalMaterial; void* cloneMaterial; AquariumGCHandle cloneHandle,originalHandle,rendererHandle; bool active; F4 customColor; bool hasCustom; };
static WaterTarget g_water[32]{};
static int g_waterCount=0;
static int g_waterTarget=0;

// Owned water clones and roots are released only on the game thread.
static bool p72_alive(void* o){
    static const MethodInfo* m=nullptr;if(!m)m=find_method_param1(C_Object,"op_Implicit","UnityEngine.Object");
    void* a[]={o};return o&&m&&boxed_bool(invoke(m,nullptr,a));
}
static void p72_release(AquariumGCHandle& h){if(h&&il2cpp_gchandle_free)il2cpp_gchandle_free(h);h=0;}
static void p72_drop_water(WaterTarget& wt){
    // Renderer is rooted, so checking a destroyed Unity wrapper is safe.
    if(wt.cloneMaterial&&p72_alive(wt.renderer)&&invoke(M_Renderer_get_sharedMaterial,wt.renderer,nullptr)==wt.cloneMaterial){
        if(!p72_alive(wt.originalMaterial))return;
        void* a[]={wt.originalMaterial};invoke(M_Renderer_set_sharedMaterial,wt.renderer,a);
        if(invoke(M_Renderer_get_sharedMaterial,wt.renderer,nullptr)==wt.cloneMaterial)return;
    }
    if(wt.cloneMaterial&&p72_alive(wt.cloneMaterial)){
        void* a[]={wt.cloneMaterial};invoke(find_method_param1(C_Object,"Destroy","UnityEngine.Object"),nullptr,a);
    }
    p72_release(wt.cloneHandle);p72_release(wt.originalHandle);p72_release(wt.rendererHandle);wt=WaterTarget{};
}
static void p72_prune_glass(Il2CppArray* live);

// 0.7.4 detached stock WidgetColorPicker visuals with independent manual input.
static void* g_pickerWidget=nullptr;
static void* g_pickerWidgetGO=nullptr;
static void* g_pickerWidgetTransform=nullptr;
static void* g_pickerOriginalParent=nullptr;
static void* g_pickerHostParent=nullptr;
static void* g_pickerOriginalCallback=nullptr;
static void* g_pickerSafeCallback=nullptr;
static AquariumGCHandle g_pickerSafeCallbackHandle=0;
static bool g_pickerOriginalActiveSelf=true;
static bool g_pickerOriginalEnabled=true;
struct RectState { bool valid; F2 anchorMin,anchorMax,anchored,sizeDelta,pivot; };
struct PickerGraphicState { void* graphic; bool raycast; };
static PickerGraphicState g_pickerGraphics[96]{};
static int g_pickerGraphicCount=0;
struct PickerBehaviourState { void* behaviour; bool enabled; };
static PickerBehaviourState g_pickerBehaviours[16]{}; static int g_pickerBehaviourCount=0;
static bool g_pickerOpen=false; static bool g_pickerChanged=false;

static float g_lastHue=-10.0f,g_lastSat=-10.0f,g_lastVal=-10.0f;
static void* g_pickerWheelGO=nullptr; static void* g_pickerWheelTr=nullptr; static void* g_pickerReticleGO=nullptr; static void* g_pickerReticleTr=nullptr;
static void* g_pickerValueGO=nullptr; static void* g_pickerValueTr=nullptr; static void* g_pickerHandleGO=nullptr; static void* g_pickerHandleTr=nullptr;
static void* g_pickerGeometryWidget=nullptr; static void* g_pickerGeometryHost=nullptr; static bool g_pickerGeometryValid=false;
static RectState g_pickerRootState{},g_pickerWheelState{},g_pickerValueState{},g_pickerReticleState{},g_pickerHandleState{};
static F2 g_reticleOriginalAP{0,0},g_handleOriginalAP{0,0}; static bool g_haveReticleOriginal=false,g_haveHandleOriginal=false;
static float g_manualHue=0.0f,g_manualSat=0.0f,g_manualVal=0.5f; static int g_pickerDragMode=0; static bool g_manualMouseWasDown=false; static bool g_manualMissLogged=false;

static void* find_first_loaded_object(Il2CppClass* c){
    if(!c||!M_Resources_FindAll)return nullptr; const Il2CppType* t=il2cpp_class_get_type(c); if(!t)return nullptr; Il2CppObject* to=il2cpp_type_get_object(t); if(!to)return nullptr;
    void* a[1]={to}; Il2CppArray* arr=(Il2CppArray*)invoke(M_Resources_FindAll,nullptr,a); if(!arr||arr->max_length==0)return nullptr;
    void* fallback=nullptr; for(uptr i=0;i<arr->max_length;++i){void* o=arr->vector[i];if(!o)continue;if(!fallback)fallback=o;if(M_Object_GetInstanceID&&object_id(o)<0)return o;} return fallback;
}
static F4 hsv_to_rgb(float h,float s,float v){
    if(s<=0.00001f)return F4{v,v,v,0.0f};
    while(h<0)h+=1.0f; while(h>=1.0f)h-=1.0f; float hf=h*6.0f; int i=(int)hf; float f=hf-(float)i; float p=v*(1.0f-s); float q=v*(1.0f-s*f); float t=v*(1.0f-s*(1.0f-f));
    switch(i%6){case 0:return F4{v,t,p,0};case 1:return F4{q,v,p,0};case 2:return F4{p,v,t,0};case 3:return F4{p,q,v,0};case 4:return F4{t,p,v,0};default:return F4{v,p,q,0};}
}
static float fdiff(float a,float b){float d=a-b;return d<0?-d:d;}

static bool renderer_is_water_surface(void* renderer){
    if(!renderer||!M_Component_get_gameObject) return false;
    void* go=invoke(M_Component_get_gameObject,renderer,nullptr); if(!go)return false;
    char gn[256]; object_name(go,gn,sizeof(gn));
    return contains_i(gn,"Water Surface Mesh");
}

static void refresh_water_targets(){
    if(p78_workDepth&&p78_waterFresh){++p78_rendererHits;return;}
    void* oldRenderer=(g_waterCount>0&&g_waterTarget<g_waterCount)?g_water[g_waterTarget].renderer:nullptr;
    WaterTarget prev[32]{}; int prevCount=g_waterCount; for(int i=0;i<prevCount&&i<32;++i)prev[i]=g_water[i];
    if(!C_Renderer||!M_Resources_FindAll)return;
    const Il2CppType* rt=il2cpp_class_get_type(C_Renderer); Il2CppObject* ro=il2cpp_type_get_object(rt); void* a[1]={ro};
    Il2CppArray* arr=(Il2CppArray*)invoke(M_Resources_FindAll,nullptr,a); if(!arr)return;
    for(int i=0;i<32;++i)g_water[i]=WaterTarget{};g_waterCount=0;
    // Retain existing targets first. If restoring a retiring renderer fails, keep
    // its rooted record for retry instead of losing ownership when discovery changes.
    for(int p=0;p<prevCount;++p){
        bool found=false;for(uptr i=0;i<arr->max_length;++i)if(arr->vector[i]==prev[p].renderer){found=true;break;}
        if(!found)p72_drop_water(prev[p]);
        if(prev[p].renderer)g_water[g_waterCount++]=prev[p];
    }
    for(uptr i=0;i<arr->max_length&&g_waterCount<32;++i){void* r=arr->vector[i];
        bool tracked=false;for(int j=0;j<g_waterCount;++j)if(g_water[j].renderer==r){tracked=true;break;}
        if(tracked||!renderer_is_water_surface(r))continue;
        WaterTarget wt{};wt.renderer=r;
        if(M_Renderer_get_sharedMaterial)wt.originalMaterial=invoke(M_Renderer_get_sharedMaterial,r,nullptr);
        if(il2cpp_gchandle_new){wt.rendererHandle=il2cpp_gchandle_new((Il2CppObject*)r,false);
            if(wt.originalMaterial)wt.originalHandle=il2cpp_gchandle_new((Il2CppObject*)wt.originalMaterial,false);}
        g_water[g_waterCount++]=wt;
    }
    ++p78_rendererScans;p72_prune_glass(arr);
    if(p78_workDepth)p78_waterFresh=true;
    int found=-1; if(oldRenderer) for(int i=0;i<g_waterCount;++i) if(g_water[i].renderer==oldRenderer){found=i;break;}
    if(found>=0)g_waterTarget=found; else if(g_waterTarget>=g_waterCount)g_waterTarget=0;
}

static int g_glassCount=0;
static int g_causticCount=0;
static void apply_glass_sides_for_target();
static void apply_floor_caustics_for_target(bool verbose=false);
static void update_floor_caustics_for_owner(void* owner,F4 color);
static void restore_live_floor_caustics_for_owner_safe(void* owner);
static void log_floor_caustics_for_current();
static void update_active_water_color(F4 color);
static void close_stock_picker(bool restoreCallback=true);
static void restore_glass_targets();
static void persist_current_target(bool enabled);
static int apply_persisted_to_live_targets(bool force);

// 0.7.49 standard/null-tint FloorArea sink. Definitions live after the 0.7.47 static-analysis helpers.
static bool p48_sync_standard_floor_sink_for_current(F4 color,bool verbose);
static void p48_reassert_standard_floor_sink(bool verbose);
static void p48_suspend_standard_floor_sink(bool verbose);
static void p48_clear_standard_floor_sink_if_selected(bool verbose);


static u64 water_signature(){
    refresh_water_targets(); u64 h=1469598103934665603ull ^ (u64)g_waterCount;
    for(int i=0;i<g_waterCount;++i){ u64 v=(u64)(u32)object_id(g_water[i].renderer); h^=v; h*=1099511628211ull; }
    return h;
}

// Cheap save/load/rebuild detector from 0.7.7/0.7.8. It hashes AquariumState pointers only;
// a full Renderer scan happens only when that state set actually changes.
static u64 aquarium_state_signature(){
    StateList sl=get_states(); u64 h=1469598103934665603ull ^ (u64)sl.count;
    for(int i=0;i<sl.count;++i){ u64 v=(u64)(uptr)sl.states[i]; h^=v; h*=1099511628211ull; }
    return h;
}
static void log_water_status(const char* reason){
    refresh_water_targets(); char b[1000];
    psprintf(b,"\r\n-- WATER SURFACE TARGETS (%s) count=%d target=%d --\r\n",reason?reason:"",g_waterCount,g_waterCount?g_waterTarget+1:0);log_raw(b);
    psprintf(b,"  active side/glass tint materials=%d; legacy caustics tracker=%d (unused in 0.7.20)\r\n",g_glassCount,g_causticCount);log_raw(b);
    for(int i=0;i<g_waterCount;++i){
        void* r=g_water[i].renderer; void* go=M_Component_get_gameObject?invoke(M_Component_get_gameObject,r,nullptr):nullptr; char gn[200],mn[200]; object_name(go,gn,sizeof(gn)); object_name(g_water[i].originalMaterial,mn,sizeof(mn));
        psprintf(b,"  [%d]%s renderer=%p go='%s' originalMat=%p '%s' clone=%p active=%s\r\n",i+1,(i==g_waterTarget?" <TARGET>":""),r,gn,g_water[i].originalMaterial,mn,g_water[i].cloneMaterial,g_water[i].active?"yes":"no");log_raw(b);
    }
}

static void apply_water_green(){
    refresh_water_targets(); if(g_waterCount<=0){log_raw("APPLY WATER: no Water Surface Mesh renderers found.\r\n");return;} if(g_waterTarget>=g_waterCount)g_waterTarget=0;
    WaterTarget& wt=g_water[g_waterTarget]; char b[900];
    if(!wt.renderer||!wt.originalMaterial||!M_Object_Instantiate||!M_Renderer_set_sharedMaterial||!M_Material_SetColorStr){log_raw("APPLY WATER: required renderer/material method missing.\r\n");return;}
    if(!wt.cloneMaterial){ void* a[1]={wt.originalMaterial}; wt.cloneMaterial=invoke(M_Object_Instantiate,nullptr,a); if(!wt.cloneMaterial){log_raw("APPLY WATER: Material clone failed.\r\n");return;} if(il2cpp_gchandle_new)wt.cloneHandle=il2cpp_gchandle_new((Il2CppObject*)wt.cloneMaterial,false); }
    Il2CppString* prop=il2cpp_string_new("_Albedo"); void* ca[2]={prop,&g_currentWaterColor}; invoke(M_Material_SetColorStr,wt.cloneMaterial,ca);
    void* sa[1]={wt.cloneMaterial}; invoke(M_Renderer_set_sharedMaterial,wt.renderer,sa); wt.active=true;
    char mn[256]; object_name(wt.cloneMaterial,mn,sizeof(mn)); F4 verify=read_mat_color(wt.cloneMaterial,prop);
    psprintf(b,"APPLY WATER: current picker _Albedo applied ONLY to water renderer %d/%d. renderer=%p material=%p '%s' gchandle=%llu\r\n",g_waterTarget+1,g_waterCount,wt.renderer,wt.cloneMaterial,mn,wt.cloneHandle);log_raw(b);
    psprintf(b,"             verified clone _Albedo=(%.4f, %.4f, %.4f, %.4f)\r\n",(double)verify.x,(double)verify.y,(double)verify.z,(double)verify.w);log_raw(b);
    log_raw("APPLY WATER: stable 0.7.10 water/glass path. 0.7.20 adds only in-place AquariumTintDefinition TintColor sync during picker edits; no caustics/GlowLightComponents.\r\n");
    apply_glass_sides_for_target();
}


struct GlassTarget { void* renderer; void* material; F4 originalAlbedo; AquariumGCHandle materialHandle; void* ownerRenderer; bool active; };
static GlassTarget g_glass[64]{};

static bool p72_glass_bound(const GlassTarget& gt,Il2CppArray* live){
    bool found=false;for(uptr i=0;i<live->max_length;++i)if(live->vector[i]==gt.renderer){found=true;break;}
    if(!found)return false;
    auto mats=(Il2CppArray*)invoke(M_Renderer_get_sharedMaterials,gt.renderer,nullptr);
    if(mats)for(uptr i=0;i<mats->max_length;++i)if(mats->vector[i]==gt.material)return true;
    return false;
}
static void p72_prune_glass(Il2CppArray* live){
    int w=0;for(int i=0;i<g_glassCount;++i){auto gt=g_glass[i];bool owner=false;
        for(int j=0;j<g_waterCount;++j)if(g_water[j].renderer==gt.ownerRenderer)owner=true;
        bool bound=p72_glass_bound(gt,live);
        if(owner&&bound){g_glass[w++]=gt;continue;}
        if(bound){void* a[]={il2cpp_string_new("_Albedo"),&gt.originalAlbedo};invoke(M_Material_SetColorStr,gt.material,a);}
        p72_release(gt.materialHandle);
    }
    for(int i=w;i<g_glassCount;++i)g_glass[i]=GlassTarget{};g_glassCount=w;
}

static float fabsf_local(float v){ return v<0?-v:v; }
static bool get_renderer_bounds(void* renderer, Bounds6& out){
    out=Bounds6{}; if(!renderer||!M_Renderer_get_bounds) return false;
    Il2CppObject* box=invoke(M_Renderer_get_bounds,renderer,nullptr); if(!box)return false;
    void* p=il2cpp_object_unbox(box); if(!p)return false; memcpy(&out,p,sizeof(Bounds6)); return true;
}
static bool material_is_aquarium_glass(void* mat){
    if(!mat||!M_Material_get_shader)return false; void* sh=invoke(M_Material_get_shader,mat,nullptr); if(!sh)return false;
    char sn[256]; object_name(sh,sn,sizeof(sn)); return streq(sn,"Two Point/Aquarium Glass");
}
static bool state_is_current(void* state){
    if(!state)return false; StateList sl=get_states();
    for(int i=0;i<sl.count;++i)if(sl.states[i]==state)return true;
    return false;
}

static void clear_standard_retail_pointer_proof(bool logIt){
    if(!g_standardRetailPointerProofActive||!g_standardRetailPointerProofState){
        if(logIt)log_raw("STANDARD GLOW RETAIL-POINTER PROOF: no active proof to clear.\r\n");
        g_standardRetailPointerProofActive=false;
        g_standardRetailPointerProofState=nullptr;
        g_standardRetailPointerProofOriginalTint=nullptr;
        g_standardRetailPointerProofTint=nullptr;
        return;
    }
    void* st=g_standardRetailPointerProofState;
    if(!state_is_current(st)){
        if(logIt)log_raw("STANDARD GLOW RETAIL-POINTER PROOF: previous AquariumState is no longer live; bookkeeping cleared without dereferencing it.\r\n");
    }else{
        void* cur=field_object(st,"_highestPriorityAquariumTintDefinition");
        if(cur==g_standardRetailPointerProofTint){
            set_field_ref(st,"_highestPriorityAquariumTintDefinition",g_standardRetailPointerProofOriginalTint);
            if(logIt){
                log_raw("STANDARD GLOW RETAIL-POINTER PROOF: OFF. Restored the selected state cached tint pointer directly; no retail recalculation was called.\r\n");
                log_tint_definition(field_object(st,"_highestPriorityAquariumTintDefinition"),"  resulting cached tint = ");
            }
        }else if(logIt){
            log_raw("STANDARD GLOW RETAIL-POINTER PROOF: game already changed/cleared the cached tint pointer; no pointer write was needed while clearing.\r\n");
        }
    }
    g_standardRetailPointerProofActive=false;
    g_standardRetailPointerProofState=nullptr;
    g_standardRetailPointerProofOriginalTint=nullptr;
    g_standardRetailPointerProofTint=nullptr;
}


static bool renderer_has_shared_aquarium_glass(void* renderer){
    if(!renderer||!M_Renderer_get_sharedMaterials)return false;
    Il2CppArray* arr=(Il2CppArray*)invoke(M_Renderer_get_sharedMaterials,renderer,nullptr); if(!arr)return false;
    for(uptr i=0;i<arr->max_length;++i) if(material_is_aquarium_glass(arr->vector[i])) return true;
    return false;
}
static void restore_glass_for_owner(void* owner){
    if(!owner||g_glassCount<=0)return; Il2CppString* prop=il2cpp_string_new("_Albedo"); char b[256]; int n=0,w=0;
    for(int i=0;i<g_glassCount;++i){ GlassTarget gt=g_glass[i]; if(gt.ownerRenderer==owner){ if(gt.active&&gt.material&&M_Material_SetColorStr){ void* a[2]={prop,&gt.originalAlbedo}; invoke(M_Material_SetColorStr,gt.material,a); ++n; } p72_release(gt.materialHandle); } else g_glass[w++]=gt; }
    for(int i=w;i<g_glassCount;++i)g_glass[i]=GlassTarget{}; g_glassCount=w;
    psprintf(b,"RESET SIDES: restored _Albedo on %d aquarium-glass material instance(s) for selected aquarium.\r\n",n);log_raw(b);
}
static void restore_glass_targets(){
    if(g_glassCount<=0)return; Il2CppString* prop=il2cpp_string_new("_Albedo"); char b[256]; int n=0;
    for(int i=0;i<g_glassCount;++i){ GlassTarget& gt=g_glass[i]; if(gt.active&&gt.material&&M_Material_SetColorStr){ void* a[2]={prop,&gt.originalAlbedo}; invoke(M_Material_SetColorStr,gt.material,a); ++n; } }
    psprintf(b,"RESET SIDES: restored _Albedo on %d tracked aquarium-glass material instance(s).\r\n",n);log_raw(b);
    for(int i=0;i<64;++i){p72_release(g_glass[i].materialHandle);g_glass[i]=GlassTarget{};} g_glassCount=0;
}
static GlassTarget* find_glass_material(void* material){ for(int i=0;i<g_glassCount;++i) if(g_glass[i].material==material)return &g_glass[i]; return nullptr; }
static void apply_glass_sides_for_target(){
    if(g_waterCount<=0||g_waterTarget>=g_waterCount||!M_Resources_FindAll||!M_Renderer_get_materials||!M_Renderer_get_bounds){log_raw("APPLY SIDES: required state/API unavailable.\r\n");return;}
    void* owner=g_water[g_waterTarget].renderer;
    Bounds6 wb{}; if(!get_renderer_bounds(owner,wb)){log_raw("APPLY SIDES: could not read target water bounds.\r\n");return;}
    char b[900]; psprintf(b,"APPLY SIDES: water bounds center=(%.3f,%.3f,%.3f) extents=(%.3f,%.3f,%.3f)\r\n",(double)wb.center.x,(double)wb.center.y,(double)wb.center.z,(double)wb.extents.x,(double)wb.extents.y,(double)wb.extents.z);log_raw(b);
    const Il2CppType* rt=il2cpp_class_get_type(C_Renderer); Il2CppObject* ro=il2cpp_type_get_object(rt); void* a[1]={ro}; Il2CppArray* rs=(Il2CppArray*)invoke(M_Resources_FindAll,nullptr,a); if(!rs)return;
    Il2CppString* prop=il2cpp_string_new("_Albedo"); int touched=0;
    float padX=2.0f,padZ=2.0f,padY=8.0f;
    for(uptr i=0;i<rs->max_length&&g_glassCount<64;++i){
        void* r=rs->vector[i]; if(!r||r==owner)continue; if(!renderer_has_shared_aquarium_glass(r))continue;
        Bounds6 gb{}; if(!get_renderer_bounds(r,gb))continue;
        float dx=fabsf_local(gb.center.x-wb.center.x), dz=fabsf_local(gb.center.z-wb.center.z), dy=fabsf_local(gb.center.y-wb.center.y);
        if(dx>wb.extents.x+padX||dz>wb.extents.z+padZ||dy>padY)continue;
        Il2CppArray* mats=(Il2CppArray*)invoke(M_Renderer_get_materials,r,nullptr); if(!mats)continue;
        for(uptr mi=0;mi<mats->max_length;++mi){ void* m=mats->vector[mi]; if(!material_is_aquarium_glass(m))continue;
            GlassTarget* ex=find_glass_material(m);
            if(ex && ex->ownerRenderer!=owner) continue;
            if(!ex){ if(g_glassCount>=64)continue; GlassTarget gt{}; gt.renderer=r; gt.material=m; gt.originalAlbedo=read_mat_color(m,prop); gt.ownerRenderer=owner; if(il2cpp_gchandle_new)gt.materialHandle=il2cpp_gchandle_new((Il2CppObject*)m,false); gt.active=true; g_glass[g_glassCount++]=gt; ex=&g_glass[g_glassCount-1]; }
            void* ca[2]={prop,&g_currentWaterColor}; invoke(M_Material_SetColorStr,m,ca); ex->active=true; ++touched;
        }
    }
    psprintf(b,"APPLY SIDES: current picker _Albedo applied to %d Aquarium Glass material instance(s) for this aquarium; %d tracked overall.\r\n",touched,g_glassCount);log_raw(b);
}


struct CausticTarget { void* renderer; void* material; F4 originalColor; AquariumGCHandle materialHandle; void* ownerRenderer; bool active; };
static CausticTarget g_caustic[64]{};

static bool material_is_aquarium_caustics(void* mat){
    if(!mat||!M_Material_get_shader)return false; void* sh=invoke(M_Material_get_shader,mat,nullptr); if(!sh)return false;
    char sn[256]; object_name(sh,sn,sizeof(sn)); return streq(sn,"Shader Graphs/SH_Aquarium_Caustics");
}
static bool renderer_has_shared_aquarium_caustics(void* renderer){
    if(!renderer||!M_Renderer_get_sharedMaterials)return false; Il2CppArray* arr=(Il2CppArray*)invoke(M_Renderer_get_sharedMaterials,renderer,nullptr); if(!arr)return false;
    for(uptr i=0;i<arr->max_length;++i)if(material_is_aquarium_caustics(arr->vector[i]))return true; return false;
}
static CausticTarget* find_caustic_material(void* material){ for(int i=0;i<g_causticCount;++i)if(g_caustic[i].material==material)return &g_caustic[i]; return nullptr; }
static void forget_caustic_owner_without_touching(void* owner){
    int w=0; for(int i=0;i<g_causticCount;++i){ if(g_caustic[i].ownerRenderer!=owner)g_caustic[w++]=g_caustic[i];else p72_release(g_caustic[i].materialHandle); }
    for(int i=w;i<g_causticCount;++i)g_caustic[i]=CausticTarget{}; g_causticCount=w;
}
static F4 floor_caustic_colour(F4 water,F4 original){
    // SH_Aquarium_Caustics exposes _CausticsColor directly. Preserve the retail alpha/intensity while replacing hue/RGB.
    float a=original.w; if(a<=0.001f)a=0.788235f; return F4{water.x,water.y,water.z,a};
}
static void apply_floor_caustics_for_target(bool verbose){
    if(g_waterCount<=0||g_waterTarget>=g_waterCount||!M_Resources_FindAll||!M_Renderer_get_materials||!M_Renderer_get_bounds){ if(verbose)log_raw("FLOOR CAUSTICS: required API/target unavailable.\r\n"); return; }
    void* owner=g_water[g_waterTarget].renderer; Bounds6 wb{}; if(!get_renderer_bounds(owner,wb)){if(verbose)log_raw("FLOOR CAUSTICS: target water bounds unavailable.\r\n");return;}
    const Il2CppType* rt=il2cpp_class_get_type(C_Renderer); Il2CppObject* ro=rt?il2cpp_type_get_object(rt):nullptr; if(!ro)return; void* a[1]={ro}; Il2CppArray* rs=(Il2CppArray*)invoke(M_Resources_FindAll,nullptr,a); if(!rs)return;
    Il2CppString* prop=il2cpp_string_new("_CausticsColor"); int touched=0,candidates=0; char b[760];
    float padX=3.5f,padZ=3.5f,padY=10.0f;
    for(uptr i=0;i<rs->max_length&&g_causticCount<64;++i){
        void* r=rs->vector[i]; if(!r||!renderer_has_shared_aquarium_caustics(r))continue; ++candidates; Bounds6 cb{}; if(!get_renderer_bounds(r,cb))continue;
        float dx=fabsf_local(cb.center.x-wb.center.x),dz=fabsf_local(cb.center.z-wb.center.z),dy=fabsf_local(cb.center.y-wb.center.y);
        if(dx>wb.extents.x+padX||dz>wb.extents.z+padZ||dy>padY)continue;
        Il2CppArray* mats=(Il2CppArray*)invoke(M_Renderer_get_materials,r,nullptr); if(!mats)continue;
        for(uptr mi=0;mi<mats->max_length;++mi){ void* m=mats->vector[mi]; if(!material_is_aquarium_caustics(m))continue; CausticTarget* ex=find_caustic_material(m); if(ex&&ex->ownerRenderer!=owner)continue;
            if(!ex){ if(g_causticCount>=64)continue; CausticTarget ct{}; ct.renderer=r;ct.material=m;ct.originalColor=read_mat_color(m,prop);ct.ownerRenderer=owner;ct.active=true;if(il2cpp_gchandle_new)ct.materialHandle=il2cpp_gchandle_new((Il2CppObject*)m,false);g_caustic[g_causticCount++]=ct;ex=&g_caustic[g_causticCount-1]; }
            F4 cc=floor_caustic_colour(g_currentWaterColor,ex->originalColor); void* ca[2]={prop,&cc};invoke(M_Material_SetColorStr,m,ca);ex->active=true;++touched;
            if(verbose){void* go=M_Component_get_gameObject?invoke(M_Component_get_gameObject,r,nullptr):nullptr;char gn[220];object_name(go,gn,sizeof(gn));psprintf(b,"  MATCH renderer='%s' center=(%.2f,%.2f,%.2f) dXZ=(%.2f,%.2f) mat=%p _CausticsColor=(%.3f,%.3f,%.3f,%.3f)\r\n",gn,(double)cb.center.x,(double)cb.center.y,(double)cb.center.z,(double)dx,(double)dz,m,(double)cc.x,(double)cc.y,(double)cc.z,(double)cc.w);log_raw(b);}
        }
    }
    psprintf(b,"FLOOR CAUSTICS: scanned %d SH_Aquarium_Caustics renderer candidate(s); changed %d nearby material instance(s) for water[%d].\r\n",candidates,touched,g_waterTarget+1);log_raw(b);
}
static void update_floor_caustics_for_owner(void* owner,F4 color){
    if(!owner||!M_Material_SetColorStr)return; Il2CppString* prop=il2cpp_string_new("_CausticsColor");
    for(int i=0;i<g_causticCount;++i){CausticTarget& ct=g_caustic[i];if(ct.ownerRenderer!=owner||!ct.active||!ct.material)continue;F4 cc=floor_caustic_colour(color,ct.originalColor);void* a[2]={prop,&cc};invoke(M_Material_SetColorStr,ct.material,a);}
}
static void restore_live_floor_caustics_for_owner_safe(void* owner){
    if(!owner||!M_Resources_FindAll||!M_Renderer_get_materials)return; Bounds6 wb{};if(!get_renderer_bounds(owner,wb))return;
    // Use the captured pre-custom colour value when available; never dereference a stale tracked material pointer.
    F4 restore{0.219607f,0.321568f,0.321568f,0.788235f}; bool got=false; for(int i=0;i<g_causticCount;++i)if(g_caustic[i].ownerRenderer==owner){restore=g_caustic[i].originalColor;got=true;break;}
    const Il2CppType* rt=il2cpp_class_get_type(C_Renderer);Il2CppObject* ro=rt?il2cpp_type_get_object(rt):nullptr;if(!ro)return;void* a[1]={ro};Il2CppArray* rs=(Il2CppArray*)invoke(M_Resources_FindAll,nullptr,a);if(!rs)return;
    Il2CppString* prop=il2cpp_string_new("_CausticsColor");int touched=0;float padX=3.5f,padZ=3.5f,padY=10.0f;
    for(uptr i=0;i<rs->max_length;++i){void* r=rs->vector[i];if(!r||!renderer_has_shared_aquarium_caustics(r))continue;Bounds6 cb{};if(!get_renderer_bounds(r,cb))continue;float dx=fabsf_local(cb.center.x-wb.center.x),dz=fabsf_local(cb.center.z-wb.center.z),dy=fabsf_local(cb.center.y-wb.center.y);if(dx>wb.extents.x+padX||dz>wb.extents.z+padZ||dy>padY)continue;Il2CppArray* mats=(Il2CppArray*)invoke(M_Renderer_get_materials,r,nullptr);if(!mats)continue;for(uptr mi=0;mi<mats->max_length;++mi){void* m=mats->vector[mi];if(!material_is_aquarium_caustics(m))continue;void* ca[2]={prop,&restore};invoke(M_Material_SetColorStr,m,ca);++touched;}}
    forget_caustic_owner_without_touching(owner);char b[360];psprintf(b,"RESET FLOOR CAUSTICS SAFE: restored %d live material instance(s) using %s colour=(%.3f,%.3f,%.3f,%.3f).\r\n",touched,got?"captured retail":"known retail default",(double)restore.x,(double)restore.y,(double)restore.z,(double)restore.w);log_raw(b);
}
static void log_floor_caustics_for_current(){
    refresh_water_targets();if(g_waterCount<=0||g_waterTarget>=g_waterCount){log_raw("FLOOR CAUSTICS DIAG: no current water target.\r\n");return;}void* owner=g_water[g_waterTarget].renderer;Bounds6 wb{};if(!get_renderer_bounds(owner,wb))return;
    const Il2CppType* rt=il2cpp_class_get_type(C_Renderer);Il2CppObject* ro=rt?il2cpp_type_get_object(rt):nullptr;if(!ro)return;void* a[1]={ro};Il2CppArray* rs=(Il2CppArray*)invoke(M_Resources_FindAll,nullptr,a);if(!rs)return;char b[760];int n=0;psprintf(b,"FLOOR CAUSTICS DIAG: water[%d] center=(%.2f,%.2f,%.2f) extents=(%.2f,%.2f,%.2f).\r\n",g_waterTarget+1,(double)wb.center.x,(double)wb.center.y,(double)wb.center.z,(double)wb.extents.x,(double)wb.extents.y,(double)wb.extents.z);log_raw(b);
    for(uptr i=0;i<rs->max_length&&n<24;++i){void* r=rs->vector[i];if(!r||!renderer_has_shared_aquarium_caustics(r))continue;Bounds6 cb{};if(!get_renderer_bounds(r,cb))continue;void* go=M_Component_get_gameObject?invoke(M_Component_get_gameObject,r,nullptr):nullptr;char gn[220];object_name(go,gn,sizeof(gn));float dx=fabsf_local(cb.center.x-wb.center.x),dz=fabsf_local(cb.center.z-wb.center.z),dy=fabsf_local(cb.center.y-wb.center.y);psprintf(b,"  candidate[%d] go='%s' center=(%.2f,%.2f,%.2f) extents=(%.2f,%.2f,%.2f) d=(%.2f,%.2f,%.2f)%s\r\n",++n,gn,(double)cb.center.x,(double)cb.center.y,(double)cb.center.z,(double)cb.extents.x,(double)cb.extents.y,(double)cb.extents.z,(double)dx,(double)dy,(double)dz,(dx<=wb.extents.x+3.5f&&dz<=wb.extents.z+3.5f&&dy<=10.0f)?" <NEAR>":"");log_raw(b);}
    psprintf(b,"FLOOR CAUSTICS DIAG: listed %d renderer candidate(s), tracked custom materials=%d.\r\n",n,g_causticCount);log_raw(b);
}

static bool p65_glowPending=false;
static F4 p65_pendingColor{};
static void p65_flush_glow(){
    if(!p65_glowPending)return;
    p65_glowPending=false;p49_sync_glow_for_current(p65_pendingColor,false);
}
static void update_active_water_color(F4 color){
    g_currentWaterColor=color; g_currentWaterColor.w=0.0f;
    if(g_waterCount>0&&g_waterTarget<g_waterCount&&!g_water[g_waterTarget].active)apply_water_green();
    Il2CppString* prop=il2cpp_string_new("_Albedo"); void* owner=nullptr;
    if(g_waterCount>0&&g_waterTarget<g_waterCount){ WaterTarget& wt=g_water[g_waterTarget]; owner=wt.renderer; wt.customColor=g_currentWaterColor; wt.hasCustom=true; if(wt.active&&wt.cloneMaterial&&M_Material_SetColorStr){void* a[2]={prop,&g_currentWaterColor};invoke(M_Material_SetColorStr,wt.cloneMaterial,a);} }
    for(int i=0;i<g_glassCount;++i){GlassTarget& gt=g_glass[i];if(gt.ownerRenderer==owner&&gt.active&&gt.material&&M_Material_SetColorStr){void* a[2]={prop,&g_currentWaterColor};invoke(M_Material_SetColorStr,gt.material,a);}}
    // 0.7.49 strict physical routing: Dredge water -> retail AquariumTintDefinition only;
    // standard water -> FloorArea CeilingLightColorOverride only; ambiguous -> no glow write.
    // A failed standard sink must NEVER fall back to the Dredge tint path.
    p65_pendingColor=g_currentWaterColor;p65_glowPending=true;
}

#include "PersistenceRuntime.h"
static int apply_persisted_to_live_targets(bool force){
    refresh_water_targets(); if(g_waterCount<=0)return 0; int old=g_waterTarget; F4 oldc=g_currentWaterColor; int n=0;
    for(int i=0;i<g_waterCount;++i){ F4 c{}; bool migrated=false; if(!find_persist_for_renderer(g_water[i].renderer,c,migrated))continue; if(!force&&g_water[i].hasCustom)continue; g_waterTarget=i;g_currentWaterColor=c; apply_water_green(); g_water[i].customColor=c;g_water[i].hasCustom=true; p49_sync_glow_for_current(c,false); ++n;
        if(migrated){ Bounds6 wb{}; if(get_renderer_bounds(g_water[i].renderer,wb)){ append_persistence(wb.center,c,true); char mb[360]; psprintf(mb,"PERSIST: migrated nearby aquarium colour to current key=(%.2f,%.2f,%.2f).\r\n",(double)wb.center.x,(double)wb.center.y,(double)wb.center.z);log_raw(mb); } }
    }
    if(old<g_waterCount)g_waterTarget=old; else g_waterTarget=0; g_currentWaterColor=oldc; if(n){char b[240];psprintf(b,"PERSIST: reapplied saved colours to %d live aquarium(s).\r\n",n);log_raw(b);} return n;
}

static void* find_hidden_callback_sink(){
    // 0.6.9 deliberately keeps the delegate target OUTSIDE the WidgetColorPicker hierarchy.
    // 0.6.8 bound Action<Color> to one of the picker's own Images; the widget then immediately
    // recoloured that UI element during enable/data refresh, leaving only the centre dot visible.
    Il2CppClass* ic=find_class("UnityEngine.UI","Image");
    if(!ic||!M_Resources_FindAll||!M_Component_get_gameObject||!M_GameObject_get_transform||!M_GameObject_get_activeInHierarchy||!M_Transform_IsChildOf)return nullptr;
    const Il2CppType* it=il2cpp_class_get_type(ic); if(!it)return nullptr; Il2CppObject* ito=il2cpp_type_get_object(it); if(!ito)return nullptr;
    void* aa[1]={ito}; Il2CppArray* arr=(Il2CppArray*)invoke(M_Resources_FindAll,nullptr,aa); if(!arr)return nullptr;
    for(uptr i=0;i<arr->max_length;++i){
        void* img=arr->vector[i]; if(!img)continue;
        void* go=invoke(M_Component_get_gameObject,img,nullptr); if(!go||go==g_pickerWidgetGO)continue;
        void* tr=invoke(M_GameObject_get_transform,go,nullptr); if(!tr)continue;
        void* ca[1]={g_pickerWidgetTransform}; bool isChild=boxed_bool(invoke(M_Transform_IsChildOf,tr,ca),false); if(isChild)continue;
        bool active=boxed_bool(invoke(M_GameObject_get_activeInHierarchy,go,nullptr),false); if(active)continue;
        return img;
    }
    return nullptr;
}

static bool install_safe_picker_callback(){
    if(!g_pickerWidget||!il2cpp_method_get_object){log_raw("PICKER CALLBACK: method reflection API unavailable.\r\n");return false;}
    Il2CppClass* wc=il2cpp_object_get_class((Il2CppObject*)g_pickerWidget); Il2CppFieldInfo* of=find_field_hierarchy(wc,"OnColorPicked");
    if(!of){log_raw("PICKER CALLBACK: OnColorPicked field not found.\r\n");return false;}
    const Il2CppType* ot=il2cpp_field_get_type(of); Il2CppObject* actionType=ot?il2cpp_type_get_object(ot):nullptr;
    if(!actionType){log_raw("PICKER CALLBACK: Action<Color> System.Type unavailable.\r\n");return false;}
    void* target=find_hidden_callback_sink();
    if(!target){log_raw("PICKER CALLBACK: no inactive Image outside picker hierarchy found; aborting rather than touching picker visuals.\r\n");return false;}
    void* targetGO=M_Component_get_gameObject?invoke(M_Component_get_gameObject,target,nullptr):nullptr; char tgn[220]; object_name(targetGO,tgn,sizeof(tgn));
    Il2CppClass* tc=il2cpp_object_get_class((Il2CppObject*)target); const MethodInfo* setColor=find_method1_hierarchy(tc,"set_color","UnityEngine.Color");
    if(!setColor){log_raw("PICKER CALLBACK: hidden Image.set_color(Color) not found.\r\n");return false;}
    Il2CppObject* methodObj=il2cpp_method_get_object(setColor,tc); if(!methodObj){log_raw("PICKER CALLBACK: reflection MethodInfo creation failed.\r\n");return false;}
    Il2CppClass* dc=find_class("System","Delegate"); if(!dc){log_raw("PICKER CALLBACK: System.Delegate class not found.\r\n");return false;}
    const MethodInfo* create=find_method_param3(dc,"CreateDelegate","System.Type","System.Object","System.Reflection.MethodInfo");
    if(!create){log_raw("PICKER CALLBACK: Delegate.CreateDelegate(Type,Object,MethodInfo) not found.\r\n");return false;}
    void* a[3]={actionType,target,methodObj}; Il2CppObject* del=invoke(create,nullptr,a); if(!del){log_raw("PICKER CALLBACK: Delegate.CreateDelegate invocation failed.\r\n");return false;}
    g_pickerSafeCallback=del; if(il2cpp_gchandle_new)g_pickerSafeCallbackHandle=il2cpp_gchandle_new(del,false);
    set_field_ref(g_pickerWidget,"OnColorPicked",del);
    char b[420]; psprintf(b,"PICKER CALLBACK: isolated safe Action<Color> installed. delegate=%p gchandle=%llu hiddenTarget=%p go='%s'.\r\n",del,g_pickerSafeCallbackHandle,target,tgn);log_raw(b);
    return true;
}


static bool is_picker_descendant_go(void* go){
    if(!go||!g_pickerWidgetTransform||!M_GameObject_get_transform||!M_Transform_IsChildOf)return false;
    if(go==g_pickerWidgetGO)return true;
    void* tr=invoke(M_GameObject_get_transform,go,nullptr); if(!tr)return false;
    void* a[1]={g_pickerWidgetTransform}; return boxed_bool(invoke(M_Transform_IsChildOf,tr,a),false);
}

static float clamp01(float v){ return v<0.0f?0.0f:(v>1.0f?1.0f:v); }

static void rgb_to_hsv(F4 c,float& h,float& s,float& v){
    float mx=c.x; if(c.y>mx)mx=c.y; if(c.z>mx)mx=c.z;
    float mn=c.x; if(c.y<mn)mn=c.y; if(c.z<mn)mn=c.z;
    v=mx; float d=mx-mn; s=(mx<=0.000001f)?0.0f:(d/mx);
    if(d<=0.000001f){h=0.0f;return;}
    if(mx==c.x){ h=(c.y-c.z)/d; if(h<0.0f)h+=6.0f; }
    else if(mx==c.y) h=((c.z-c.x)/d)+2.0f;
    else h=((c.x-c.y)/d)+4.0f;
    h/=6.0f; while(h<0.0f)h+=1.0f; while(h>=1.0f)h-=1.0f;
}

static void* find_picker_go_exact(const char* wanted){
    if(!wanted||!C_GameObject||!M_Resources_FindAll||!g_pickerWidgetTransform)return nullptr;
    const Il2CppType* got=il2cpp_class_get_type(C_GameObject); Il2CppObject* gotoo=got?il2cpp_type_get_object(got):nullptr; if(!gotoo)return nullptr;
    void* a0[1]={gotoo}; Il2CppArray* arr=(Il2CppArray*)invoke(M_Resources_FindAll,nullptr,a0); if(!arr)return nullptr;
    for(uptr i=0;i<arr->max_length;++i){ void* go=arr->vector[i]; if(!go||!is_picker_descendant_go(go))continue; char gn[220];object_name(go,gn,sizeof(gn)); if(streq(gn,wanted))return go; }
    return nullptr;
}

static bool get_rect4(void* rectTr,Rect4& r){
    r=Rect4{}; if(!rectTr||!M_RectTransform_get_rect)return false; Il2CppObject* box=invoke(M_RectTransform_get_rect,rectTr,nullptr); if(!box)return false; void* u=il2cpp_object_unbox(box); if(!u)return false; memcpy(&r,u,sizeof(r)); return true;
}
static bool get_anchored(void* rectTr,F2& v){
    v=F2{}; if(!rectTr||!M_RectTransform_get_anchoredPosition)return false; Il2CppObject* box=invoke(M_RectTransform_get_anchoredPosition,rectTr,nullptr); if(!box)return false; void* u=il2cpp_object_unbox(box); if(!u)return false; memcpy(&v,u,sizeof(v)); return true;
}
static void set_anchored(void* rectTr,F2 v){
    if(!rectTr||!M_RectTransform_set_anchoredPosition)return; void* a[1]={&v}; invoke(M_RectTransform_set_anchoredPosition,rectTr,a);
}

static bool get_vec2_prop(void* tr,const MethodInfo* m,F2& out){ out=F2{}; if(!tr||!m)return false; Il2CppObject* box=invoke(m,tr,nullptr); if(!box)return false; void* u=il2cpp_object_unbox(box); if(!u)return false; memcpy(&out,u,sizeof(out)); return true; }
static void set_vec2_prop(void* tr,const MethodInfo* m,F2 v){ if(!tr||!m)return; void* a[1]={&v}; invoke(m,tr,a); }
static bool capture_rect_state(void* tr,RectState& st){
    st=RectState{}; if(!tr)return false;
    if(!get_vec2_prop(tr,M_RectTransform_get_anchorMin,st.anchorMin))return false;
    if(!get_vec2_prop(tr,M_RectTransform_get_anchorMax,st.anchorMax))return false;
    if(!get_vec2_prop(tr,M_RectTransform_get_anchoredPosition,st.anchored))return false;
    if(!get_vec2_prop(tr,M_RectTransform_get_sizeDelta,st.sizeDelta))return false;
    if(!get_vec2_prop(tr,M_RectTransform_get_pivot,st.pivot))return false;
    st.valid=true; return true;
}
static void restore_rect_state(void* tr,const RectState& st){
    if(!tr||!st.valid)return;
    set_vec2_prop(tr,M_RectTransform_set_anchorMin,st.anchorMin);
    set_vec2_prop(tr,M_RectTransform_set_anchorMax,st.anchorMax);
    set_vec2_prop(tr,M_RectTransform_set_pivot,st.pivot);
    set_vec2_prop(tr,M_RectTransform_set_sizeDelta,st.sizeDelta);
    set_vec2_prop(tr,M_RectTransform_set_anchoredPosition,st.anchored);
}
static bool mouse_position(F3& p){
    p=F3{};
    // 0.7.2 still relied on UnityEngine.Input.mousePosition from this background
    // worker thread. In retail that call can return no boxed value, which silently
    // consumed every click before hit-testing. Read the physical cursor through
    // user32 instead, convert it to the foreground game window's client area, and
    // flip Y to Unity's bottom-left screen origin. Keep Unity Input as fallback.
    if(pGetCursorPos&&pGetForegroundWindow&&pScreenToClient&&pGetClientRect){
        POINT_X pt{}; HWND hwnd=p70_game_foreground();
        if(!hwnd)return false;
        RECT_X rc{};
        if(hwnd&&pGetCursorPos(&pt)&&pScreenToClient(hwnd,&pt)&&pGetClientRect(hwnd,&rc)){
            long w=rc.right-rc.left, h=rc.bottom-rc.top;
            if(w>0&&h>0){
                p.x=(float)pt.x;
                p.y=(float)(h-1-pt.y);
                p.z=0.0f;
                return true;
            }
        }
    }
    if(!M_Input_get_mousePosition)return false;
    Il2CppObject* box=invoke(M_Input_get_mousePosition,nullptr,nullptr); if(!box)return false;
    void* u=il2cpp_object_unbox(box); if(!u)return false; memcpy(&p,u,sizeof(p)); return true;
}
static bool inverse_point(void* tr,F3 world,F3& local){
    local=F3{}; if(!tr||!M_Transform_InverseTransformPoint)return false; void* a[1]={&world}; Il2CppObject* box=invoke(M_Transform_InverseTransformPoint,tr,a); if(!box)return false; void* u=il2cpp_object_unbox(box); if(!u)return false; memcpy(&local,u,sizeof(local)); return true;
}

static bool screen_to_local(void* rectTr,F3 screen,F3& local){
    local=F3{};
    // Cursor position supplied here is in Unity-style SCREEN client pixels. 0.7.1 incorrectly fed those
    // pixels to Transform.InverseTransformPoint(), which expects WORLD coordinates.
    // The picker therefore rendered correctly but every hit test missed. Use Unity's
    // own screen->RectTransform conversion instead. The game's HUD canvas is screen-space,
    // so a null camera is the correct first choice here.
    if(rectTr&&M_RectTransformUtility_ScreenPointToLocalPointInRectangle){
        F2 sp{screen.x,screen.y}; F2 lp{0,0};
        void* camera=nullptr; void* a[4]={rectTr,&sp,camera,&lp};
        Il2CppObject* box=invoke(M_RectTransformUtility_ScreenPointToLocalPointInRectangle,nullptr,a);
        if(box&&boxed_bool(box,false)){ local.x=lp.x; local.y=lp.y; local.z=0.0f; return true; }
    }
    // Diagnostic fallback only. This is not expected to be geometrically correct for screen pixels,
    // but preserves behaviour if a future build removes RectTransformUtility metadata.
    return inverse_point(rectTr,screen,local);
}

static bool resolve_picker_manual_visuals(){
    g_pickerWheelGO=find_picker_go_exact("imageWheel");
    g_pickerReticleGO=find_picker_go_exact("imageReticle");
    g_pickerValueGO=find_picker_go_exact("imageValue");
    g_pickerHandleGO=find_picker_go_exact("Handle");
    g_pickerWheelTr=g_pickerWheelGO&&M_GameObject_get_transform?invoke(M_GameObject_get_transform,g_pickerWheelGO,nullptr):nullptr;
    g_pickerReticleTr=g_pickerReticleGO&&M_GameObject_get_transform?invoke(M_GameObject_get_transform,g_pickerReticleGO,nullptr):nullptr;
    g_pickerValueTr=g_pickerValueGO&&M_GameObject_get_transform?invoke(M_GameObject_get_transform,g_pickerValueGO,nullptr):nullptr;
    g_pickerHandleTr=g_pickerHandleGO&&M_GameObject_get_transform?invoke(M_GameObject_get_transform,g_pickerHandleGO,nullptr):nullptr;
    g_haveReticleOriginal=get_anchored(g_pickerReticleTr,g_reticleOriginalAP);
    g_haveHandleOriginal=get_anchored(g_pickerHandleTr,g_handleOriginalAP);
    char b[520]; psprintf(b,"PICKER MANUAL: visuals wheel=%p reticle=%p value=%p handle=%p originals(reticle=%s handle=%s).\r\n",g_pickerWheelTr,g_pickerReticleTr,g_pickerValueTr,g_pickerHandleTr,g_haveReticleOriginal?"yes":"no",g_haveHandleOriginal?"yes":"no");log_raw(b);
    return g_pickerWheelTr&&g_pickerReticleTr&&g_pickerValueTr&&g_pickerHandleTr;
}

static bool picker_geometry_sane(){ Rect4 wr{},vr{}; return get_rect4(g_pickerWheelTr,wr)&&get_rect4(g_pickerValueTr,vr)&&wr.width>150.0f&&wr.height>150.0f&&vr.width>15.0f&&vr.height>120.0f; }
static void cache_or_restore_picker_geometry(){
    char b[520];
    bool same=(g_pickerGeometryValid&&g_pickerGeometryWidget==g_pickerWidget&&g_pickerGeometryHost==g_pickerHostParent);
    if(!same){
        g_pickerGeometryValid=false; g_pickerGeometryWidget=g_pickerWidget; g_pickerGeometryHost=g_pickerHostParent;
        bool ok=capture_rect_state(g_pickerWidgetTransform,g_pickerRootState)&&capture_rect_state(g_pickerWheelTr,g_pickerWheelState)&&capture_rect_state(g_pickerValueTr,g_pickerValueState);
        if(g_pickerReticleTr)capture_rect_state(g_pickerReticleTr,g_pickerReticleState);
        if(g_pickerHandleTr)capture_rect_state(g_pickerHandleTr,g_pickerHandleState);
        if(ok&&picker_geometry_sane()){ g_pickerGeometryValid=true; Rect4 wr{},vr{};get_rect4(g_pickerWheelTr,wr);get_rect4(g_pickerValueTr,vr); psprintf(b,"PICKER GEOMETRY: cached first known-good stock layout wheel=(%.1fx%.1f) value=(%.1fx%.1f).\r\n",(double)wr.width,(double)wr.height,(double)vr.width,(double)vr.height);log_raw(b); }
        else log_raw("PICKER GEOMETRY: first layout was not sane enough to cache; leaving stock geometry untouched.\r\n");
        return;
    }
    restore_rect_state(g_pickerWidgetTransform,g_pickerRootState);
    restore_rect_state(g_pickerWheelTr,g_pickerWheelState);
    restore_rect_state(g_pickerValueTr,g_pickerValueState);
    restore_rect_state(g_pickerReticleTr,g_pickerReticleState);
    restore_rect_state(g_pickerHandleTr,g_pickerHandleState);
    Rect4 wr{},vr{};get_rect4(g_pickerWheelTr,wr);get_rect4(g_pickerValueTr,vr);
    psprintf(b,"PICKER GEOMETRY: restored cached 0.7.4 layout after reparent wheel=(%.1fx%.1f) value=(%.1fx%.1f).\r\n",(double)wr.width,(double)wr.height,(double)vr.width,(double)vr.height);log_raw(b);
}

static void update_manual_picker_visuals(){
    Rect4 wr{}; if(g_pickerWheelTr&&g_pickerReticleTr&&get_rect4(g_pickerWheelTr,wr)){
        float cx=wr.x+wr.width*0.5f, cy=wr.y+wr.height*0.5f;
        float radius=(wr.width<wr.height?wr.width:wr.height)*0.5f;
        const double twopi=6.2831853071795864769;
        double ang=(double)g_manualHue*twopi;
        F2 ap{cx+(float)(pCos(ang)*(double)(g_manualSat*radius)),cy+(float)(pSin(ang)*(double)(g_manualSat*radius))};
        set_anchored(g_pickerReticleTr,ap);
    }
    Rect4 vr{}; if(g_pickerValueTr&&g_pickerHandleTr&&get_rect4(g_pickerValueTr,vr)){
        float y=vr.y+clamp01(g_manualVal)*vr.height;
        F2 ap=g_haveHandleOriginal?g_handleOriginalAP:F2{0,0}; ap.y=y; set_anchored(g_pickerHandleTr,ap);
    }
}

static void apply_manual_hsv(bool logIt){
    g_manualHue=clamp01(g_manualHue); g_manualSat=clamp01(g_manualSat); g_manualVal=clamp01(g_manualVal);
    F4 c=hsv_to_rgb(g_manualHue,g_manualSat,g_manualVal); c.w=0.0f;
    if(!logIt&&c.x==g_currentWaterColor.x&&c.y==g_currentWaterColor.y&&c.z==g_currentWaterColor.z)return;
    update_active_water_color(c); g_pickerChanged=true; update_manual_picker_visuals();
    if(logIt){ char b[360]; psprintf(b,"PICKER MANUAL: HSV=(%.3f,%.3f,%.3f) RGB=(%.3f,%.3f,%.3f) applied to selected aquarium.\r\n",(double)g_manualHue,(double)g_manualSat,(double)g_manualVal,(double)c.x,(double)c.y,(double)c.z);log_raw(b); }
}

static bool point_in_value(F3 mouse,float& outV){
    if(!g_pickerValueTr)return false; Rect4 r{};F3 l{};if(!get_rect4(g_pickerValueTr,r)||!screen_to_local(g_pickerValueTr,mouse,l))return false;
    if(l.x<r.x||l.x>r.x+r.width||l.y<r.y||l.y>r.y+r.height)return false;
    outV=clamp01((l.y-r.y)/(r.height>0.0001f?r.height:1.0f)); return true;
}
static bool point_in_wheel(F3 mouse,float& outH,float& outS){
    if(!g_pickerWheelTr)return false; Rect4 r{};F3 l{};if(!get_rect4(g_pickerWheelTr,r)||!screen_to_local(g_pickerWheelTr,mouse,l))return false;
    float cx=r.x+r.width*0.5f,cy=r.y+r.height*0.5f; float dx=l.x-cx,dy=l.y-cy; float rad=(r.width<r.height?r.width:r.height)*0.5f; if(rad<=0.0001f)return false;
    double dist=pSqrt((double)dx*(double)dx+(double)dy*(double)dy); if(dist>(double)rad*1.08)return false;
    outS=clamp01((float)(dist/(double)rad)); double a=pAtan2((double)dy,(double)dx); if(a<0)a+=6.2831853071795864769; outH=clamp01((float)(a/6.2831853071795864769)); return true;
}


static void neutralize_picker_interaction(){
    g_pickerGraphicCount=0; g_pickerBehaviourCount=0;
    if(g_pickerWidget&&M_Behaviour_get_enabled&&M_Behaviour_set_enabled){
        g_pickerOriginalEnabled=boxed_bool(invoke(M_Behaviour_get_enabled,g_pickerWidget,nullptr),true);
        bool off=false; void* a[1]={&off}; invoke(M_Behaviour_set_enabled,g_pickerWidget,a);
        log_raw("PICKER SAFETY: WidgetColorPicker behaviour disabled. Retail pointer/drag handlers will not run.\r\n");
    }
    // Disable the child Slider behaviour as well, but deliberately leave Graphic raycast targets intact.
    // That makes the borrowed picker count as UI so clicks do not select museum items behind it, while
    // our Win32 mouse polling remains the only code that changes HSV.
    if(C_Slider&&M_Resources_FindAll&&M_Behaviour_get_enabled&&M_Behaviour_set_enabled){
        const Il2CppType* st=il2cpp_class_get_type(C_Slider); Il2CppObject* sto=st?il2cpp_type_get_object(st):nullptr; if(sto){ void* aa[1]={sto}; Il2CppArray* arr=(Il2CppArray*)invoke(M_Resources_FindAll,nullptr,aa); if(arr){ for(uptr i=0;i<arr->max_length&&g_pickerBehaviourCount<16;++i){ void* sl=arr->vector[i]; if(!sl)continue; void* go=M_Component_get_gameObject?invoke(M_Component_get_gameObject,sl,nullptr):nullptr; if(!is_picker_descendant_go(go))continue; bool en=boxed_bool(invoke(M_Behaviour_get_enabled,sl,nullptr),true); g_pickerBehaviours[g_pickerBehaviourCount++]={sl,en}; if(en){bool off=false;void* x[1]={&off};invoke(M_Behaviour_set_enabled,sl,x);} } } }
    }
    char b[260]; psprintf(b,"PICKER SAFETY: disabled %d child interactive Behaviour(s); picker Graphic raycasts left intact to block click-through.\r\n",g_pickerBehaviourCount);log_raw(b);
}

static void restore_picker_interaction_state(){
    if(M_Behaviour_set_enabled){ for(int i=0;i<g_pickerBehaviourCount;++i){ if(!g_pickerBehaviours[i].behaviour)continue; bool v=g_pickerBehaviours[i].enabled;void* a[1]={&v};invoke(M_Behaviour_set_enabled,g_pickerBehaviours[i].behaviour,a);} }
    g_pickerBehaviourCount=0;
    if(g_pickerWidget&&M_Behaviour_set_enabled){ bool v=g_pickerOriginalEnabled; void* a[1]={&v}; invoke(M_Behaviour_set_enabled,g_pickerWidget,a); }
    g_pickerGraphicCount=0;
}


static void close_stock_picker(bool restoreCallback){
    (void)restoreCallback;
    log_raw("PICKER CLOSE: begin.\r\n");
    p65_flush_glow();
    if(g_pickerChanged && g_waterCount>0 && g_waterTarget<g_waterCount){ persist_current_target(true); }
    if(g_haveReticleOriginal&&g_pickerReticleTr)set_anchored(g_pickerReticleTr,g_reticleOriginalAP);
    if(g_haveHandleOriginal&&g_pickerHandleTr)set_anchored(g_pickerHandleTr,g_handleOriginalAP);
    g_pickerDragMode=0; g_manualMouseWasDown=false; g_manualMissLogged=false; g_pickerChanged=false;
    restore_picker_interaction_state();
    if(g_pickerWidgetTransform&&g_pickerOriginalParent&&M_Transform_SetParent){
        bool keepWorld=true; void* a[2]={g_pickerOriginalParent,&keepWorld}; invoke(M_Transform_SetParent,g_pickerWidgetTransform,a); log_raw("PICKER CLOSE: widget returned to original parent.\r\n");
    }
    if(g_pickerWidgetGO&&M_GameObject_SetActive){ bool astate=g_pickerOriginalActiveSelf; void* a[1]={&astate}; invoke(M_GameObject_SetActive,g_pickerWidgetGO,a); log_raw("PICKER CLOSE: original widget activeSelf restored.\r\n"); }
    g_pickerOpen=false; g_pickerWidget=nullptr; g_pickerWidgetGO=nullptr; g_pickerWidgetTransform=nullptr; g_pickerOriginalParent=nullptr; g_pickerHostParent=nullptr; g_pickerOriginalCallback=nullptr; g_pickerSafeCallback=nullptr;
    g_pickerWheelGO=nullptr;g_pickerWheelTr=nullptr;g_pickerReticleGO=nullptr;g_pickerReticleTr=nullptr;g_pickerValueGO=nullptr;g_pickerValueTr=nullptr;g_pickerHandleGO=nullptr;g_pickerHandleTr=nullptr;g_haveReticleOriginal=false;g_haveHandleOriginal=false;
    log_raw("PICKER: detached stock colour picker closed. Current aquarium colour retained.\r\n");
}

static void* nearest_active_parent(void* tr){
    if(!tr||!M_Transform_get_parent||!M_Component_get_gameObject||!M_GameObject_get_activeInHierarchy)return nullptr;
    void* p=invoke(M_Transform_get_parent,tr,nullptr);
    for(int guard=0;p&&guard<32;++guard){
        void* go=invoke(M_Component_get_gameObject,p,nullptr);
        if(go&&boxed_bool(invoke(M_GameObject_get_activeInHierarchy,go,nullptr),false)) return p;
        p=invoke(M_Transform_get_parent,p,nullptr);
    }
    return nullptr;
}


static void* find_relaunchable_picker_widget(Il2CppClass* wc){
    if(!wc||!M_Resources_FindAll||!M_Component_get_gameObject||!M_GameObject_get_transform)return nullptr;
    const Il2CppType* t=il2cpp_class_get_type(wc); Il2CppObject* to=t?il2cpp_type_get_object(t):nullptr; if(!to)return nullptr; void* a[1]={to}; Il2CppArray* arr=(Il2CppArray*)invoke(M_Resources_FindAll,nullptr,a); if(!arr)return nullptr;
    void* fallback=nullptr;
    for(uptr i=0;i<arr->max_length;++i){ void* w=arr->vector[i]; if(!w)continue; if(!fallback)fallback=w; void* go=invoke(M_Component_get_gameObject,w,nullptr); void* tr=go?invoke(M_GameObject_get_transform,go,nullptr):nullptr; if(!tr)continue; void* p=invoke(M_Transform_get_parent,tr,nullptr); void* host=nearest_active_parent(tr); if(p&&host)return w; }
    return fallback;
}

static bool open_stock_picker(){
    if(g_pickerOpen){close_stock_picker(true);return true;}
    log_raw("PICKER OPEN: UI request received; starting visual-shell-only path.\r\n");
    refresh_water_targets(); if(g_waterCount<=0){log_raw("PICKER OPEN: no water target available.\r\n");return false;} if(g_waterTarget>=g_waterCount)g_waterTarget=0;
    log_raw("PICKER OPEN: water target ready.\r\n");

    Il2CppClass* wc=find_class("TPS.Game.UI","WidgetColorPicker"); if(!wc){log_raw("PICKER OPEN: WidgetColorPicker class not found.\r\n");return false;}
    log_raw("PICKER OPEN: WidgetColorPicker class resolved.\r\n");
    g_pickerWidget=find_relaunchable_picker_widget(wc); if(!g_pickerWidget){log_raw("PICKER OPEN: loaded WidgetColorPicker instance not found.\r\n");return false;}
    char wn[220]; object_name(g_pickerWidget,wn,sizeof(wn)); char b[520]; psprintf(b,"PICKER OPEN: loaded widget=%p name='%s'.\r\n",g_pickerWidget,wn); log_raw(b);

    g_pickerWidgetGO=M_Component_get_gameObject?invoke(M_Component_get_gameObject,g_pickerWidget,nullptr):nullptr;
    if(!g_pickerWidgetGO){log_raw("PICKER OPEN: widget GameObject unavailable.\r\n");g_pickerWidget=nullptr;return false;}
    char gon[220]; object_name(g_pickerWidgetGO,gon,sizeof(gon)); psprintf(b,"PICKER OPEN: widget GameObject=%p name='%s'.\r\n",g_pickerWidgetGO,gon);log_raw(b);

    g_pickerOriginalActiveSelf=M_GameObject_get_activeSelf?boxed_bool(invoke(M_GameObject_get_activeSelf,g_pickerWidgetGO,nullptr),true):true;
    g_pickerWidgetTransform=M_GameObject_get_transform?invoke(M_GameObject_get_transform,g_pickerWidgetGO,nullptr):nullptr;
    if(!g_pickerWidgetTransform){log_raw("PICKER OPEN: widget Transform unavailable.\r\n");return false;}
    g_pickerOriginalParent=M_Transform_get_parent?invoke(M_Transform_get_parent,g_pickerWidgetTransform,nullptr):nullptr;
    g_pickerHostParent=nearest_active_parent(g_pickerWidgetTransform);
    psprintf(b,"PICKER OPEN: transform=%p originalParent=%p nearestActiveParent=%p originalActiveSelf=%s.\r\n",g_pickerWidgetTransform,g_pickerOriginalParent,g_pickerHostParent,g_pickerOriginalActiveSelf?"true":"false");log_raw(b);
    if(!g_pickerOriginalParent||!g_pickerHostParent){log_raw("PICKER OPEN: could not determine a safe active UI host parent.\r\n");return false;}

    // 0.7.0 does NOT alter OnColorPicked and does NOT run the retail picker logic.
    // 0.6.7-0.6.9 proved that first pointer interaction can CTD even with a valid callback.
    // We therefore borrow the visuals only and neutralise all retail interaction before exposing them.
    g_pickerOriginalCallback=field_object(g_pickerWidget,"OnColorPicked");
    neutralize_picker_interaction();

    // IMPORTANT: do NOT activate ItemColorPickerMenu. 0.6.5 CTD'd because that menu expects a valid station/item.
    // Borrow only the already-initialised WidgetColorPicker and temporarily detach it from the inactive menu hierarchy.
    if(!M_Transform_SetParent){log_raw("PICKER OPEN: Transform.SetParent unavailable.\r\n");return false;}
    log_raw("PICKER OPEN: detaching WidgetColorPicker to active UI parent (menu itself remains inactive).\r\n");
    bool keepWorld=true; void* pa[2]={g_pickerHostParent,&keepWorld}; invoke(M_Transform_SetParent,g_pickerWidgetTransform,pa);
    void* verifyParent=M_Transform_get_parent?invoke(M_Transform_get_parent,g_pickerWidgetTransform,nullptr):nullptr;
    psprintf(b,"PICKER OPEN: SetParent returned; currentParent=%p expectedHost=%p.\r\n",verifyParent,g_pickerHostParent);log_raw(b);
    if(verifyParent!=g_pickerHostParent){log_raw("PICKER OPEN: parent move did not take; restoring picker safety state and aborting.\r\n"); restore_picker_interaction_state(); return false;}

    log_raw("PICKER OPEN: enabling WidgetColorPicker GameObject only.\r\n");
    bool on=true; void* aa[1]={&on}; if(M_GameObject_SetActive)invoke(M_GameObject_SetActive,g_pickerWidgetGO,aa);
    bool activeNow=M_GameObject_get_activeInHierarchy?boxed_bool(invoke(M_GameObject_get_activeInHierarchy,g_pickerWidgetGO,nullptr),false):false;
    psprintf(b,"PICKER OPEN: widget activeInHierarchy=%s.\r\n",activeNow?"true":"false");log_raw(b);

    // 0.7.0: DO NOT call WidgetColorPicker.SetColor. 0.6.6 proved that method CTDs
    // when the widget is borrowed outside its normal ItemColorPickerMenu context.
    // We preserve the widget's current HSV only for diagnostics. Retail interaction is disabled.
    log_raw("PICKER OPEN: skipping WidgetColorPicker.SetColor (unsafe outside normal item menu).\r\n");
    g_lastHue=field_float(g_pickerWidget,"_hue",-10);g_lastSat=field_float(g_pickerWidget,"_saturation",-10);g_lastVal=field_float(g_pickerWidget,"_value",-10);
    { char hb[260]; psprintf(hb,"PICKER OPEN: existing widget HSV=(%.3f, %.3f, %.3f).\r\n",(double)g_lastHue,(double)g_lastSat,(double)g_lastVal); log_raw(hb); }
    resolve_picker_manual_visuals();
    cache_or_restore_picker_geometry();
    { F3 mp{}; char mb[320]; if(mouse_position(mp)){ psprintf(mb,"PICKER MANUAL: cursor source ready at open, Unity-style screen=(%.1f,%.1f).\r\n",(double)mp.x,(double)mp.y); log_raw(mb); } else log_raw("PICKER MANUAL: WARNING cursor position unavailable at picker open.\r\n"); }
    { WaterTarget& wt=g_water[g_waterTarget]; if(wt.hasCustom) g_currentWaterColor=wt.customColor; else { Il2CppString* p=il2cpp_string_new("_Albedo"); g_currentWaterColor=wt.originalMaterial?read_mat_color(wt.originalMaterial,p):F4{0.167378f,0.272455f,0.292452f,0.0f}; } }
    rgb_to_hsv(g_currentWaterColor,g_manualHue,g_manualSat,g_manualVal);
    // Opening is read-only for water/glass too; material clones are created on first edit.
    // 0.7.81: route by the proven physical Dredge locator. Standard selection never falls through
    // to the Dredge AquariumTintDefinition path if its FloorArea sink cannot bind.
    log_raw("0.7.81 PICKER OPEN: no lighting change; glow updates only on an edit, saved-colour restore, or glow enable.\r\n");
    // Existing custom materials are retained; opening alone is not an edit.
    if(g_waterCount>0&&g_waterTarget<g_waterCount&&!g_water[g_waterTarget].hasCustom){}
    g_pickerChanged=false;
    g_pickerOpen=true; update_manual_picker_visuals();
    psprintf(b,"PICKER: stock WidgetColorPicker VISUALS opened for water target %d/%d. source widget HSV=(%.3f, %.3f, %.3f)\r\n",g_waterTarget+1,g_waterCount,(double)g_lastHue,(double)g_lastSat,(double)g_lastVal);log_raw(b);
    log_raw("PICKER: 0.7.10 MANUAL INPUT ACTIVE (0.7.4 path + geometry repair). Retail pointer/drag logic remains disabled. Drag the wheel or brightness strip; 0.7.4 reads the physical cursor with Win32 and converts client pixels into the picker RectTransforms with RectTransformUtility, then moves the markers and recolours only this aquarium.\r\n");
    return true;
}

static bool g_pickerSampling=false;
static void poll_stock_picker(){
    if(g_pickerSampling){g_pickerDragMode=0;g_manualMouseWasDown=true;return;}
    if(!g_pickerOpen||!g_pickerWheelTr||!g_pickerValueTr)return;
    if(!p70_game_foreground()){g_pickerDragMode=0;g_manualMouseWasDown=true;return;}
    short ms=pGetAsyncKeyState(p70_primary_button());
    bool down=(ms&0x8000)!=0;
    bool pressed=p70_mouse_edge(down,g_manualMouseWasDown);
    F3 mp{};
    if(!mouse_position(mp)){
        if(pressed&&!g_manualMissLogged){ log_raw("PICKER MANUAL: left click detected but cursor position could not be resolved.\r\n"); g_manualMissLogged=true; }
        g_manualMouseWasDown=down; return;
    }
    if(pressed){
        float h=0,s=0,v=0;
        if(point_in_wheel(mp,h,s)){g_pickerDragMode=1;g_manualHue=h;g_manualSat=s;apply_manual_hsv(true);log_raw("PICKER MANUAL: wheel drag started.\r\n");}
        else if(point_in_value(mp,v)){g_pickerDragMode=2;g_manualVal=v;apply_manual_hsv(true);log_raw("PICKER MANUAL: value drag started.\r\n");}
        else {
            g_pickerDragMode=0;
            if(!g_manualMissLogged){
                Rect4 wr{},vr{}; F3 wl{},vl{}; bool wok=get_rect4(g_pickerWheelTr,wr)&&screen_to_local(g_pickerWheelTr,mp,wl); bool vok=get_rect4(g_pickerValueTr,vr)&&screen_to_local(g_pickerValueTr,mp,vl);
                char mb[520]; psprintf(mb,"PICKER MANUAL: click missed controls. screen=(%.1f,%.1f) wheelLocal=(%.1f,%.1f) wheelRect=(%.1f,%.1f,%.1f,%.1f) valueLocal=(%.1f,%.1f) valueRect=(%.1f,%.1f,%.1f,%.1f) convert=(%s/%s).\r\n",(double)mp.x,(double)mp.y,(double)wl.x,(double)wl.y,(double)wr.x,(double)wr.y,(double)wr.width,(double)wr.height,(double)vl.x,(double)vl.y,(double)vr.x,(double)vr.y,(double)vr.width,(double)vr.height,wok?"ok":"fail",vok?"ok":"fail"); log_raw(mb); g_manualMissLogged=true;
            }
        }
    }else if(down&&g_pickerDragMode==1){
        Rect4 r{};F3 l{}; if(get_rect4(g_pickerWheelTr,r)&&screen_to_local(g_pickerWheelTr,mp,l)){ float cx=r.x+r.width*0.5f,cy=r.y+r.height*0.5f,dx=l.x-cx,dy=l.y-cy,rad=(r.width<r.height?r.width:r.height)*0.5f; if(rad>0.0001f){double dist=pSqrt((double)dx*dx+(double)dy*dy);g_manualSat=clamp01((float)(dist/(double)rad));double a=pAtan2((double)dy,(double)dx);if(a<0)a+=6.2831853071795864769;g_manualHue=clamp01((float)(a/6.2831853071795864769));apply_manual_hsv(false);} }
    }else if(down&&g_pickerDragMode==2){
        Rect4 r{};F3 l{}; if(get_rect4(g_pickerValueTr,r)&&screen_to_local(g_pickerValueTr,mp,l)){g_manualVal=clamp01((l.y-r.y)/(r.height>0.0001f?r.height:1.0f));apply_manual_hsv(false);}
    }
    if(!down&&g_manualMouseWasDown&&g_pickerDragMode!=0){apply_manual_hsv(true);persist_current_target(true);log_raw("PICKER MANUAL: drag ended; colour retained and saved.\r\n");g_pickerDragMode=0;}
    g_manualMouseWasDown=down;
}

static void* find_canonical_aquarium_material(const char* shaderName){
    if(!C_Material||!M_Resources_FindAll||!M_Material_get_shader)return nullptr;
    const Il2CppType* mt=il2cpp_class_get_type(C_Material); Il2CppObject* to=mt?il2cpp_type_get_object(mt):nullptr; if(!to)return nullptr;
    void* a[1]={to}; Il2CppArray* arr=(Il2CppArray*)invoke(M_Resources_FindAll,nullptr,a); if(!arr)return nullptr; void* fallback=nullptr;
    for(uptr i=0;i<arr->max_length;++i){ void* m=arr->vector[i]; if(!m)continue; void* sh=invoke(M_Material_get_shader,m,nullptr); if(!sh)continue; char sn[180];object_name(sh,sn,sizeof(sn)); if(!streq(sn,shaderName))continue; char mn[220];object_name(m,mn,sizeof(mn)); if(!contains_i(mn,"(Instance)")&&!contains_i(mn,"(Clone)"))return m; }
    return fallback;
}
static void forget_glass_owner_without_touching(void* owner){
    int w=0; for(int i=0;i<g_glassCount;++i){ if(g_glass[i].ownerRenderer!=owner)g_glass[w++]=g_glass[i];else p72_release(g_glass[i].materialHandle); }
    for(int i=w;i<g_glassCount;++i)g_glass[i]=GlassTarget{}; g_glassCount=w;
}
static void restore_live_glass_for_owner_safe(void* owner){
    // Only materials still bound to a live renderer are touched. Never guess a neighbour by distance.
    const Il2CppType* rt=il2cpp_class_get_type(C_Renderer);void* a[]={il2cpp_type_get_object(rt)};
    auto live=(Il2CppArray*)invoke(M_Resources_FindAll,nullptr,a);if(!live)return;
    for(int i=0;i<g_glassCount;++i){auto& gt=g_glass[i];if(gt.ownerRenderer!=owner)continue;
        if(p72_glass_bound(gt,live)){void* ca[]={il2cpp_string_new("_Albedo"),&gt.originalAlbedo};invoke(M_Material_SetColorStr,gt.material,ca);}
    }
    forget_glass_owner_without_touching(owner);
}

static void reset_water_target(){
    if(g_pickerOpen)close_stock_picker(true);
    refresh_water_targets(); if(g_waterCount<=0){log_raw("RESET WATER: no water targets.\r\n");return;} if(g_waterTarget>=g_waterCount)g_waterTarget=0; WaterTarget& wt=g_water[g_waterTarget];
    g_pickerChanged=false;g_pickerDragMode=0;p65_glowPending=false;
    void* owner=wt.renderer;
    // The rooted pre-edit material is authoritative; never substitute another tank's asset.
    void* baseWater=wt.originalMaterial;
    if(!p72_alive(baseWater)||!M_Renderer_set_sharedMaterial){log_raw("RESET: original water unavailable; override retained.\r\n");return;}
    void* a[]={baseWater};invoke(M_Renderer_set_sharedMaterial,owner,a);
    if(invoke(M_Renderer_get_sharedMaterial,owner,nullptr)!=baseWater){log_raw("RESET: water rebind failed; override retained.\r\n");return;}
    restore_live_glass_for_owner_safe(owner);
    g_currentWaterColor=read_mat_color(baseWater,il2cpp_string_new("_Albedo"));
    persist_current_target(false);
    p72_drop_water(wt);
    p78_waterFresh=false;refresh_water_targets();
    log_raw("0.7.81 RESET: original water and glass restored; owned clone destroyed and roots released.\r\n");
}


static void log_live_water_keys_for_persist(){
    refresh_water_targets(); char b[420]; psprintf(b,"PERSIST DIAG: live water targets=%d.\r\n",g_waterCount);log_raw(b);
    for(int i=0;i<g_waterCount;++i){ Bounds6 wb{}; if(get_renderer_bounds(g_water[i].renderer,wb)){ F4 c{}; bool mig=false; bool hit=find_persist_for_renderer(g_water[i].renderer,c,mig); psprintf(b,"  water[%d] key=(%.2f,%.2f,%.2f) savedMatch=%s%s.\r\n",i+1,(double)wb.center.x,(double)wb.center.y,(double)wb.center.z,hit?"yes":"no",(hit&&mig)?" (nearby)":"");log_raw(b); } }
}


// ---------- 0.7.34 physical Dredge water locator ----------
// AquariumState order comes from AquariumManager's dictionary while water-target order comes
// from Resources.FindObjectsOfTypeAll(Renderer). Those orders are unrelated. 0.7.34 therefore
// never pairs state[i] with water[i]. Instead it finds active scene renderers whose names are
// characteristic of the Dredge/aberration fish set, assigns each marker to the nearby water
// surface, and chooses the unambiguous highest-scoring water target.
static bool p34_dredge_marker_name(const char* s){
    if(!s)return false;
    // Keep this as direct tests rather than a static pointer table. The aquarium DLL is intentionally
    // import-free and previous stable builds also avoided a PE base-relocation table.
    return contains_i(s,"threeheaded") || contains_i(s,"three_headed") ||
           contains_i(s,"voideye") || contains_i(s,"gnashing") ||
           contains_i(s,"grotesque") || contains_i(s,"bursting") ||
           contains_i(s,"dredge") || contains_i(s,"aberr") ||
           contains_i(s,"oceanicperch") || contains_i(s,"oceanic_perch") ||
           contains_i(s,"lanternfish") || contains_i(s,"lantern fish") ||
           contains_i(s,"barreleye");
}
static bool p34_inside_water_zone(const Bounds6& rb,const Bounds6& wb){
    float dx=fabsf_local(rb.center.x-wb.center.x),dy=fabsf_local(rb.center.y-wb.center.y),dz=fabsf_local(rb.center.z-wb.center.z);
    return dx<=wb.extents.x+3.0f && dz<=wb.extents.z+3.0f && dy<=8.0f;
}
struct P34WaterScore{ int score; int markers; char examples[360]; };
static int p34_score_water(int wi,P34WaterScore& out){
    out=P34WaterScore{};
    if(wi<0||wi>=g_waterCount||!g_water[wi].renderer||!C_Renderer||!M_Resources_FindAll)return 0;
    Bounds6 wb{};if(!get_renderer_bounds(g_water[wi].renderer,wb))return 0;
    const Il2CppType* rt=il2cpp_class_get_type(C_Renderer);Il2CppObject* ro=rt?il2cpp_type_get_object(rt):nullptr;if(!ro)return 0;
    void* a[1]={ro};Il2CppArray* rs=(Il2CppArray*)invoke(M_Resources_FindAll,nullptr,a);if(!rs)return 0;
    for(uptr ri=0;ri<rs->max_length;++ri){
        void* r=rs->vector[ri];if(!r||r==g_water[wi].renderer)continue;
        void* go=M_Component_get_gameObject?invoke(M_Component_get_gameObject,r,nullptr):nullptr;
        if(go&&M_GameObject_get_activeInHierarchy&&!boxed_bool(invoke(M_GameObject_get_activeInHierarchy,go,nullptr),false))continue;
        Bounds6 rb{};if(!get_renderer_bounds(r,rb)||!p34_inside_water_zone(rb,wb))continue;
        char rn[120];object_name(r,rn,sizeof(rn));int local=0;
        if(p34_dredge_marker_name(rn))local+=10;
        if(M_Renderer_get_sharedMaterials){
            Il2CppArray* mats=(Il2CppArray*)invoke(M_Renderer_get_sharedMaterials,r,nullptr);
            if(mats)for(uptr mi=0;mi<mats->max_length;++mi){void* m=mats->vector[mi];if(!m)continue;char mn[120];object_name(m,mn,sizeof(mn));if(p34_dredge_marker_name(mn)){local+=3;break;}}
        }
        if(local>0){
            out.score+=local;++out.markers;
            if(out.examples[0])sappend(out.examples,sizeof(out.examples),", ");
            if(slen(out.examples)<300)sappend(out.examples,sizeof(out.examples),rn);
        }
    }
    return out.score;
}
static int p34_find_physical_dredge_water(){
    char b[900];if(g_waterCount<=0)return -1;int best=-1,bestScore=0,secondScore=0;static P34WaterScore scores[32];memset(scores,0,sizeof(scores));
    log_raw("\r\n-- 0.7.34 PHYSICAL DREDGE WATER SCORES (state index is NOT used) --\r\n");
    for(int i=0;i<g_waterCount;++i){
        int sc=p34_score_water(i,scores[i]);Bounds6 wb{};get_renderer_bounds(g_water[i].renderer,wb);
        psprintf(b,"  water[%d] center=(%.2f,%.2f,%.2f) DredgeScore=%d markers=%d examples=[%s]\r\n",i+1,(double)wb.center.x,(double)wb.center.y,(double)wb.center.z,sc,scores[i].markers,scores[i].examples[0]?scores[i].examples:"<none>");log_raw(b);
        if(sc>bestScore){secondScore=bestScore;bestScore=sc;best=i;}else if(sc>secondScore)secondScore=sc;
    }
    if(best<0||bestScore<=0){log_raw("PHYSICAL DREDGE LOCATOR: no active Dredge-specific renderer markers were found near any aquarium water surface. Probe NOT armed.\r\n");return -1;}
    if(secondScore==bestScore){psprintf(b,"PHYSICAL DREDGE LOCATOR: ambiguous tie at score=%d. Probe NOT armed rather than guessing.\r\n",bestScore);log_raw(b);return -1;}
    psprintf(b,"PHYSICAL DREDGE LOCATOR: chose water[%d] from world-space renderer evidence (score=%d, runner-up=%d). AquariumState list order was ignored.\r\n",best+1,bestScore,secondScore);log_raw(b);
    return best;
}



// ---------- 0.7.40 READ-ONLY aquarium floor / caustics component-system probe ----------
// 0.7.38/0.7.39 showed that copying MaterialPropertyBlocks across aquarium floor renderers is
// unsafe in this IL2CPP runtime. 0.7.40 therefore performs NO MPB resolution, GetPropertyBlock,
// SetPropertyBlock, AquariumState/Fish/ECS writes, or native recalculation. The retired probe compared the
// live Dredge and standard aquarium Flooring renderers, shared shader/material values, and the
// attached GameObject/parent component hierarchy.
static bool p40_floor_renderer(void* r){
    if(!r)return false; char rn[128],mn[160],sn[160]; object_name(r,rn,sizeof(rn));
    if(!M_Renderer_get_sharedMaterial)return false; void* m=invoke(M_Renderer_get_sharedMaterial,r,nullptr); if(!m)return false;
    object_name(m,mn,sizeof(mn)); void* sh=M_Material_get_shader?invoke(M_Material_get_shader,m,nullptr):nullptr; if(sh)object_name(sh,sn,sizeof(sn)); else sn[0]=0;
    return streq(rn,"Flooring") && contains_i(mn,"Aquarium_Floor") && contains_i(sn,"Aquarium_Caustics");
}
struct P40Floor{ void* r; Bounds6 b; char material[160]; char shader[160]; };
static bool p40_interesting_class(Il2CppClass*c){
    if(!c)return false; if(is_tps_class(c))return true; char f[256]; class_full(c,f,sizeof(f));
    const char*k[]={"aquarium","tint","glow","light","caustic","room","floor","water","visual","render","material","shader","volume","ambient"};
    for(unsigned i=0;i<sizeof(k)/sizeof(k[0]);++i)if(contains_i(f,k[i]))return true; return false;
}
static void p40_dump_fields(void*comp,Il2CppClass*c,const char*indent){
    char b[1800]; int emitted=0;
    for(Il2CppClass*cur=c;cur&&emitted<48;cur=il2cpp_class_get_parent(cur)){ char cf[256]; class_full(cur,cf,sizeof(cf));
        if(streq(cf,"UnityEngine.MonoBehaviour")||streq(cf,"UnityEngine.Behaviour")||streq(cf,"UnityEngine.Component")||streq(cf,"UnityEngine.Object")||streq(cf,"System.Object"))break;
        void*it=nullptr; Il2CppFieldInfo*f=nullptr; while(emitted<48&&(f=il2cpp_class_get_fields(cur,&it))!=nullptr){ const char*fn=il2cpp_field_get_name(f); if(!fn)continue; const Il2CppType*ft=il2cpp_field_get_type(f); char*tn=ft?il2cpp_type_get_name(ft):nullptr;
            Il2CppObject*vo=il2cpp_field_get_value_object(f,comp); char val[700]; format_value(vo,tn?tn:"",val,sizeof(val)); psprintf(b,"%sFIELD %s.%s : %s = %s\r\n",indent,cf,fn,tn?tn:"?",val); log_raw(b); if(tn)il2cpp_free(tn); ++emitted; }
    }
    if(emitted>=48)log_raw("        ... field dump capped at 48 ...\r\n");
}

// Runtime field-offset query used by the verified layout checks.
using P47FieldOffsetFn = usize(*)(Il2CppFieldInfo*);

// ---------- 0.7.81 standard/null-tint FloorArea optional-colour sink ----------
// 0.7.47 proved the retail AquariumManager::OnUpdate path:
//   AquariumState +0x18 -> RoomInstance
//   RoomInstance +0x68 -> FloorAreaInstance
//   optional colour payload at FloorAreaInstance +0xB0..+0xC0
//   CeilingOccludersManager.MarkFloorAreaAndParentsAsDirty(floorArea, 0)
//
// The native stores show a 20-byte optional-colour layout:
//   +0xB0 dword present flag (retail tint branch writes 1)
//   +0xB4 float R
//   +0xB8 float G
//   +0xBC float B
//   +0xC0 float A
//
// Standard aquariums have no active AquariumTintDefinition, so 0.7.81 does not replace the
// AquariumState tint pointer. Instead it mirrors the selected picker colour into this already-retail
// FloorArea sink, then asks the same retail CeilingOccludersManager to dirty the floor area.
// 0.7.81 removes the old single-null-state restriction. Null-tint AquariumStates are physically paired
// one-to-one to water surfaces from each RoomInstance camera-focus position, while Dredge retains its retail tint path.
struct P48OptionalColour { u32 present; float r,g,b,a; };
static volatile bool g_p48FloorActive=false;
static void* g_p48BoundWater=nullptr;
static void* g_p48State=nullptr;
static void* g_p48Floor=nullptr;
static void* g_p48DredgeWater=nullptr;
static P48OptionalColour g_p48Original{};
static P48OptionalColour g_p48Desired{};
// 0.7.81 keeps an independent FloorArea hold for each customised standard aquarium.
// The old g_p48* pointers remain as a selected/diagnostic mirror only.
struct P53FloorSlot { volatile u32 seq; bool active; void* water; void* state; void* floor; P48OptionalColour original; P48OptionalColour desired; };
struct P54FloorSlotSnapshot { bool active; void* water; void* state; void* floor; P48OptionalColour original; P48OptionalColour desired; };
static P53FloorSlot g_p53FloorSlots[16]{};
static bool g_p55ConsumerReady=false;
static bool p55_consumer_ready(){return __atomic_load_n(&g_p55ConsumerReady,__ATOMIC_ACQUIRE);}
static bool p55_install_consumer_hook();
static void p78_free_trampoline(void*);
static bool p78_seal_trampoline(void*,usize);
static void p55_status();
// Slot publication uses a bounded exclusive lock. No unbounded game-thread spin.
static bool p54_slot_write_begin(P53FloorSlot& slot){
    for(int tries=0;tries<3;++tries){u32 q=__atomic_load_n(&slot.seq,__ATOMIC_RELAXED);
        if(!(q&1u)&&__atomic_compare_exchange_n(&slot.seq,&q,q+1u,false,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED))return true;}
    return false;
}
static void p54_slot_write_end(P53FloorSlot& slot){__atomic_fetch_add(&slot.seq,1u,__ATOMIC_RELEASE);}
static bool p54_snapshot_slot(int index,P54FloorSlotSnapshot& out){
    out=P54FloorSlotSnapshot{};if(index<0||index>=16)return false;P53FloorSlot& slot=g_p53FloorSlots[index];
    for(int tries=0;tries<3;++tries){u32 q=__atomic_load_n(&slot.seq,__ATOMIC_RELAXED);
        if(q&1u)continue;
        if(!__atomic_compare_exchange_n(&slot.seq,&q,q+1u,false,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED))continue;
        out.active=slot.active;out.water=slot.water;out.state=slot.state;out.floor=slot.floor;
        out.original=slot.original;out.desired=slot.desired;
        __atomic_store_n(&slot.seq,q,__ATOMIC_RELEASE);return true;
    }return false;
}
static void p54_clear_slot(P53FloorSlot& s){
    if(!p54_slot_write_begin(s))return; s.active=false; s.water=nullptr; s.state=nullptr; s.floor=nullptr;
    s.original=P48OptionalColour{}; s.desired=P48OptionalColour{}; p54_slot_write_end(s);
}
static u64 g_p54SnapshotBusySkips=0;
static u64 g_p54StaleStateSkips=0;
static u64 g_p54FloorChangedSkips=0;
static u64 g_p54StateChangeInvalidations=0;
static bool g_p48LayoutResolved=false;
static bool g_p48LayoutOk=false;
static bool g_p48LayoutLogged=false;
static Il2CppFieldInfo* g_p48OptionalColourField=nullptr;
static Il2CppClass* g_p48CeilingClass=nullptr;
static const MethodInfo* g_p48DirtyMethod=nullptr;

static bool p48_payload_equal(const P48OptionalColour& a,const P48OptionalColour& b){
    return a.present==b.present &&
           fabsf_local(a.r-b.r)<0.00005f &&
           fabsf_local(a.g-b.g)<0.00005f &&
           fabsf_local(a.b-b.b)<0.00005f &&
           fabsf_local(a.a-b.a)<0.00005f;
}
static P48OptionalColour p48_read_payload(void* floor){
    P48OptionalColour v{}; if(floor)memcpy(&v,(u8*)floor+0xB0,sizeof(v)); return v;
}

static float p48_hdr_scale(){
    float hdr=1.0f;
    for(int i=0;i<16;++i){
        if(!g_liveTintSlots[i].used)continue;
        F4 o=g_liveTintSlots[i].original; float m=max3f_local(o.x,o.y,o.z);
        if(m>hdr&&m<8.0f)hdr=m;
    }
    // When the standard tank is selected first, no special tint may have been synced yet.
    // Capture (but do not modify) one already-live retail tint definition solely to inherit its
    // proven aquarium-lighting HDR range.
    StateList sl=get_states();
    for(int i=0;i<sl.count;++i){
        void* tint=field_object(sl.states[i],"_highestPriorityAquariumTintDefinition"); if(!tint)continue;
        LiveTintSlot* slot=live_tint_slot(tint); if(!slot)continue;
        float m=max3f_local(slot->original.x,slot->original.y,slot->original.z);
        if(m>hdr&&m<8.0f)hdr=m;
    }
    return hdr;
}
// 0.7.81: FloorArea colour is a lighting override, not a water-albedo copy. Feeding
// the picker's V/brightness directly into this field darkened the whole aquarium FloorArea.
static P48OptionalColour p48_from_picker(F4 c){
    P48OptionalColour v{};v.present=1;v.r=clamp01_glow(c.x);v.g=clamp01_glow(c.y);v.b=clamp01_glow(c.z);v.a=1.0f;return v;
}

static Il2CppFieldInfo* p48_field_at_offset(Il2CppClass* c,usize wanted,P47FieldOffsetFn getOff){
    if(!c||!getOff)return nullptr;
    for(Il2CppClass* cur=c;cur;cur=il2cpp_class_get_parent?il2cpp_class_get_parent(cur):nullptr){
        void* it=nullptr; Il2CppFieldInfo* f=nullptr;
        while((f=il2cpp_class_get_fields(cur,&it))!=nullptr) if(getOff(f)==wanted)return f;
    }
    return nullptr;
}
static bool p48_resolve_layout(bool verbose){
    if(g_p48LayoutResolved&&g_p48LayoutOk)return true;
    // Failed/too-early attempts are retryable. 0.7.48 cached a first quiet failure forever.
    g_p48LayoutResolved=true; g_p48LayoutOk=false;
    char b[1200]; void* ga=find_loaded_module("GameAssembly.dll");
    P47FieldOffsetFn getOff=ga?(P47FieldOffsetFn)resolve_export(ga,"il2cpp_field_get_offset"):nullptr;
    Il2CppClass* floorClass=find_class("TPS.Game","FloorAreaInstance");
    if(!getOff||!floorClass){
        if(verbose)log_raw("0.7.81 FLOOR SINK: FloorAreaInstance metadata/field-offset API unavailable; standard sink disabled.\r\n");
        return false;
    }
    g_p48OptionalColourField=p48_field_at_offset(floorClass,0xB0,getOff);
    if(!g_p48OptionalColourField){
        if(verbose)log_raw("0.7.81 FLOOR SINK: no FloorAreaInstance field starts at proven retail offset +0xB0; refusing raw write.\r\n");
        return false;
    }
    const char* fn=il2cpp_field_get_name(g_p48OptionalColourField);
    const Il2CppType* ft=il2cpp_field_get_type(g_p48OptionalColourField);
    char* tn=ft?il2cpp_type_get_name(ft):nullptr;
    if(verbose||!g_p48LayoutLogged){
        psprintf(b,"0.7.81 FLOOR SINK LAYOUT: FloorAreaInstance +0xB0 field='%s' type='%s'; retail native span +0xB0..+0xC0 accepted.\r\n",
                 fn?fn:"?",tn?tn:"?"); log_raw(b); g_p48LayoutLogged=true;
    }
    if(tn)il2cpp_free(tn);

    // 0.7.81 reuses the already-proven darkening-project route. The live
    // CeilingLightPlacementManager is owned by Level._systems and its
    // MarkFloorAreaAndParentsAsDirty(FloorAreaInstance) call was previously
    // validated successfully against every live floor area.
    g_p48CeilingClass=find_class("TPS.Game","CeilingLightPlacementManager");
    g_p48DirtyMethod=g_p48CeilingClass?find_method_name_params(g_p48CeilingClass,"MarkFloorAreaAndParentsAsDirty",1):nullptr;
    if(!g_p48CeilingClass||!g_p48DirtyMethod){
        if(verbose)log_raw("0.7.81 FLOOR SINK: CeilingLightPlacementManager.MarkFloorAreaAndParentsAsDirty(1 managed param) not resolved; will retry on next use.\r\n");
        return false;
    }
    if(verbose)log_method_signature(g_p48DirtyMethod,"0.7.81 FLOOR SINK DIRTY METHOD: ");
    g_p48LayoutOk=true;
    return true;
}
static void* p50_level_state_instance(){
    Il2CppClass* c=find_class("TPS.Game","LevelState"); if(!c)return nullptr;
    void* s=static_ref_field(c,"<Instance>k__BackingField");
    if(!s){const MethodInfo* gi=find_method0_hierarchy(c,"get_Instance");if(gi)s=invoke(gi,nullptr,nullptr);}
    return s;
}
static void* p50_level_from_state(void* state){
    if(!state)return nullptr;
    void* level=field_object(state,"Level");
    if(!level)level=field_object(state,"<Level>k__BackingField");
    if(!level){const MethodInfo* gm=find_method0_hierarchy(il2cpp_object_get_class((Il2CppObject*)state),"get_Level");if(gm)level=invoke(gm,state,nullptr);}
    return level;
}
static void* p50_ceiling_light_placement_manager(bool verbose){
    void* ls=p50_level_state_instance();
    if(!ls){if(verbose)log_raw("0.7.81 FLOOR SINK: LevelState.Instance unavailable; dirty route not ready yet.\r\n");return nullptr;}
    void* level=p50_level_from_state(ls);
    if(!level){if(verbose)log_raw("0.7.81 FLOOR SINK: LevelState.Level unavailable; dirty route not ready yet.\r\n");return nullptr;}
    void* systems=field_object(level,"_systems");
    if(!systems){if(verbose)log_raw("0.7.81 FLOOR SINK: Level._systems unavailable; dirty route not ready yet.\r\n");return nullptr;}
    int count=collection_count(systems);
    const MethodInfo* getItem=find_method1_hierarchy(il2cpp_object_get_class((Il2CppObject*)systems),"get_Item","System.Int32");
    if(count<0||!getItem){if(verbose)log_raw("0.7.81 FLOOR SINK: Level._systems list access unavailable.\r\n");return nullptr;}
    if(count>512)count=512;
    for(int i=0;i<count;++i){
        int idx=i;void* a[1]={&idx};Il2CppObject* o=invoke(getItem,systems,a);if(!o)continue;
        Il2CppClass* oc=il2cpp_object_get_class(o);char full[260];class_full(oc,full,sizeof(full));
        if(streq(full,"TPS.Game.CeilingLightPlacementManager")){
            if(verbose){char b[320];psprintf(b,"0.7.81 FLOOR SINK: found CeilingLightPlacementManager at Level._systems[%d] = %p.\r\n",i,o);log_raw(b);}
            return o;
        }
    }
    if(verbose)log_raw("0.7.81 FLOOR SINK: CeilingLightPlacementManager not found in Level._systems.\r\n");
    return nullptr;
}
__declspec(noinline) static bool p48_mark_dirty(void* floor,bool verbose){
    if(!floor)return false;
    void* ls=p50_level_state_instance();void* level=ls?p50_level_from_state(ls):nullptr;
    void* systems=level?field_object(level,"_systems"):nullptr;if(!systems)return false;
    const MethodInfo* getItem=find_method1_hierarchy(il2cpp_object_get_class((Il2CppObject*)systems),"get_Item","System.Int32");
    int count=collection_count(systems);if(!getItem||count<0||count>512)return false;
    unsigned done=0;
    for(int i=0;i<count;++i){int index=i;void* a[1]={&index};Il2CppObject* obj=invoke(getItem,systems,a);if(!obj)continue;
        Il2CppClass* cls=il2cpp_object_get_class(obj);char name[180];class_full(cls,name,sizeof(name));
        unsigned bit=streq(name,"TPS.Game.CeilingLightPlacementManager")?1u:streq(name,"TPS.Game.CeilingOccludersManager")?2u:0u;
        if(!bit||(done&bit))continue;
        const MethodInfo* method=find_method_name_params(cls,"MarkFloorAreaAndParentsAsDirty",1);if(!method)continue;
        void* args[1]={floor};Il2CppObject* exception=nullptr;il2cpp_runtime_invoke(method,obj,args,&exception);
        if(!exception)done|=bit;
    }
    if(verbose)log_raw(done==3?"0.7.81 REFRESH: both ceiling managers notified.\r\n":"0.7.81 REFRESH: manager unresolved/exception; retry pending.\r\n");
    return done==3;
}
static int p48_null_tint_state_count(){
    int n=0; StateList sl=get_states();
    for(int i=0;i<sl.count;++i) if(sl.states[i]&&!field_object(sl.states[i],"_highestPriorityAquariumTintDefinition")) ++n;
    return n;
}
static bool p48_floor_from_state(void* st,void*& room,void*& floor){
    room=nullptr;floor=nullptr;if(!st)return false;
    room=field_object(st,"<AquariumRoomInstance>k__BackingField"); if(!room)return false;
    floor=field_object(room,"_floorArea"); if(!floor)return false;
    Il2CppClass* fc=il2cpp_object_get_class((Il2CppObject*)floor);char full[260];class_full(fc,full,sizeof(full));
    return streq(full,"TPS.Game.FloorAreaInstance");
}
static int p48_cached_dredge_index(){
    refresh_water_targets();
    if(g_p48DredgeWater){
        for(int i=0;i<g_waterCount;++i)if(g_water[i].renderer==g_p48DredgeWater)return i;
        g_p48DredgeWater=nullptr;
    }
    int di=p34_find_physical_dredge_water();
    if(di>=0&&di<g_waterCount)g_p48DredgeWater=g_water[di].renderer;
    return di;
}

// 0.7.81 physical standard mapping. RoomInstance::_cameraFocus is a live Transform anchored
// inside that aquarium room. Pair each null-tint AquariumState to the nearest water surface whose
// X/Z bounds contain that room focus (with a small expansion), then enforce a one-to-one assignment.
// This removes the old "exactly one standard aquarium" shortcut without relying on state/list order.
struct P53StandardMapEntry { void* water; void* state; void* floor; F3 waterCenter; F3 focus; float score; int waterIndex; int stateIndex; };
static bool p53_transform_position(void* tr,F3& out){
    out=F3{}; if(!tr)return false;
    if(!M_Transform_get_position&&C_Transform) M_Transform_get_position=il2cpp_class_get_method_from_name(C_Transform,"get_position",0);
    if(!M_Transform_get_position)return false;
    Il2CppObject* box=invoke(M_Transform_get_position,tr,nullptr); if(!box)return false;
    void* p=il2cpp_object_unbox(box); if(!p)return false; memcpy(&out,p,sizeof(out)); return true;
}
static bool p53_state_focus(void* st,F3& focus,void*& floor){
    void* room=nullptr; floor=nullptr; if(!p48_floor_from_state(st,room,floor))return false;
    void* tr=field_object(room,"_cameraFocus"); if(!tr)return false;
    return p53_transform_position(tr,focus);
}
static int p53_build_standard_map(P53StandardMapEntry* out,int cap,bool verbose){
    if(!out||cap<=0)return 0; refresh_water_targets(); p48_cached_dredge_index();
    StateList sl=get_states();
    struct SRec{void* st;void* floor;F3 focus;int index;}; SRec states[16]{}; int ns=0;
    for(int i=0;i<sl.count&&ns<16;++i){
        void* st=sl.states[i]; if(!st||field_object(st,"_highestPriorityAquariumTintDefinition"))continue;
        F3 fp{};void* floor=nullptr;if(!p53_state_focus(st,fp,floor))continue;
        states[ns++]={st,floor,fp,i};
    }
    struct WRec{void* water;Bounds6 b;int index;}; WRec waters[16]{}; int nw=0;
    for(int i=0;i<g_waterCount&&nw<16;++i){
        void* water=g_water[i].renderer;if(!water||water==g_p48DredgeWater)continue;Bounds6 b{};if(!get_renderer_bounds(water,b))continue;
        waters[nw++]={water,b,i};
    }
    bool usedS[16]{}; bool usedW[16]{}; int mapped=0;
    for(;;){
        int bestS=-1,bestW=-1;float best=1.0e30f;
        for(int si=0;si<ns;++si){if(usedS[si])continue;for(int wi=0;wi<nw;++wi){if(usedW[wi])continue;
            float dx=fabsf_local(states[si].focus.x-waters[wi].b.center.x), dz=fabsf_local(states[si].focus.z-waters[wi].b.center.z);
            float lx=waters[wi].b.extents.x+4.0f,lz=waters[wi].b.extents.z+4.0f;
            if(dx>lx||dz>lz)continue;
            float nx=dx/(lx>0.01f?lx:1.0f),nz=dz/(lz>0.01f?lz:1.0f),score=nx*nx+nz*nz;
            if(score<best){best=score;bestS=si;bestW=wi;}
        }}
        if(bestS<0||bestW<0||mapped>=cap)break;
        usedS[bestS]=true;usedW[bestW]=true;
        P53StandardMapEntry e{};e.water=waters[bestW].water;e.state=states[bestS].st;e.floor=states[bestS].floor;e.waterCenter=waters[bestW].b.center;e.focus=states[bestS].focus;e.score=best;e.waterIndex=waters[bestW].index;e.stateIndex=states[bestS].index;out[mapped++]=e;
    }
    if(verbose){
        char b[900];psprintf(b,"0.7.81 STANDARD MAP: waters=%d candidateStandardWaters=%d nullTintStates=%d mapped=%d physicalDredge=%p.\r\n",g_waterCount,nw,ns,mapped,g_p48DredgeWater);log_raw(b);
        for(int i=0;i<mapped;++i){P53StandardMapEntry& e=out[i];psprintf(b,"  MAP[%d] water[%d]=%p center=(%.2f,%.2f,%.2f) -> state[%d]=%p focus=(%.2f,%.2f,%.2f) floor=%p score=%.4f\r\n",i+1,e.waterIndex+1,e.water,(double)e.waterCenter.x,(double)e.waterCenter.y,(double)e.waterCenter.z,e.stateIndex+1,e.state,(double)e.focus.x,(double)e.focus.y,(double)e.focus.z,e.floor,(double)e.score);log_raw(b);}
    }
    return mapped;
}
static bool p53_map_selected_standard(bool verbose,void*& st,void*& floor){
    st=nullptr;floor=nullptr;refresh_water_targets();if(g_waterCount<=0||g_waterTarget<0||g_waterTarget>=g_waterCount)return false;
    void* current=g_water[g_waterTarget].renderer;if(!current)return false;
    P53StandardMapEntry map[16]{};int n=p53_build_standard_map(map,16,verbose);
    for(int i=0;i<n;++i)if(map[i].water==current){st=map[i].state;floor=map[i].floor;return true;}
    return false;
}
static P53FloorSlot* p53_slot_for_water(void* water){for(int i=0;i<16;++i)if(g_p53FloorSlots[i].active&&g_p53FloorSlots[i].water==water)return &g_p53FloorSlots[i];return nullptr;}
static P53FloorSlot* p53_slot_for_floor(void* floor){for(int i=0;i<16;++i)if(g_p53FloorSlots[i].active&&g_p53FloorSlots[i].floor==floor)return &g_p53FloorSlots[i];return nullptr;}
static P53FloorSlot* p54_slot_for_state_floor(void* state,void* floor){for(int i=0;i<16;++i)if(g_p53FloorSlots[i].active&&g_p53FloorSlots[i].state==state&&g_p53FloorSlots[i].floor==floor)return &g_p53FloorSlots[i];return nullptr;}
static int p53_active_floor_slot_count(){int n=0;for(int i=0;i<16;++i){P54FloorSlotSnapshot snap{};if(p54_snapshot_slot(i,snap)&&snap.active)++n;}return n;}
static P53FloorSlot* p53_alloc_slot(void* water,void* state,void* floor){
    P53FloorSlot* s=p53_slot_for_water(water);if(s)return s;
    // Renderer objects can be rebuilt while AquariumState/FloorArea survive. Reuse that logical slot
    // instead of leaving two active slots fighting over the same FloorArea with different desired colours.
    s=p54_slot_for_state_floor(state,floor);if(s)return s;
    for(int i=0;i<16;++i)if(!g_p53FloorSlots[i].active)return &g_p53FloorSlots[i];
    return nullptr;
}
static void p54_clear_duplicate_bindings(P53FloorSlot* keep,void* water,void* state,void* floor){
    for(int i=0;i<16;++i){P53FloorSlot& s=g_p53FloorSlots[i];if(&s==keep||!s.active)continue;
        if((water&&s.water==water)||(state&&s.state==state)||(floor&&s.floor==floor))p54_clear_slot(s);
    }
}
static void p53_mirror_slot(P53FloorSlot* s){
    if(!s||!s->active){g_p48FloorActive=p53_active_floor_slot_count()>0;return;}
    g_p48FloorActive=true;g_p48BoundWater=s->water;g_p48State=s->state;g_p48Floor=s->floor;g_p48Original=s->original;g_p48Desired=s->desired;
}
static bool p48_selected_can_bind_standard(bool verbose,void*& st,void*& floor){
    if(p53_map_selected_standard(verbose,st,floor))return true;
    if(verbose){char b[420];psprintf(b,"0.7.81 FLOOR SINK: selected water could not be physically paired to a null-tint AquariumState (waters=%d nullTintStates=%d). Glow write suppressed rather than guessing.\r\n",g_waterCount,p48_null_tint_state_count());log_raw(b);}
    return false;
}
static bool p48_sync_standard_floor_sink_for_current(F4 color,bool verbose){
    if(!p55_glow_on()||!p55_update_ready()||!p55_consumer_ready()){
        if(verbose)log_raw("0.7.81 STANDARD: both consumer and update hooks are required; no fallback writes.\r\n");return false;
    }
    void* st=nullptr;void* floor=nullptr;if(!p48_selected_can_bind_standard(verbose,st,floor))return false;
    refresh_water_targets();void* water=(g_waterCount>0&&g_waterTarget<g_waterCount)?g_water[g_waterTarget].renderer:nullptr;if(!water)return false;
    P53FloorSlot* slot=p53_alloc_slot(water,st,floor);if(!slot)return false;
    P48OptionalColour desired=p48_from_picker(color);
    if(!p54_slot_write_begin(*slot))return false;
    slot->water=water;slot->state=st;slot->floor=floor;slot->original=P48OptionalColour{};slot->desired=desired;slot->active=true;
    p54_slot_write_end(*slot);p54_clear_duplicate_bindings(slot,water,st,floor);p53_mirror_slot(slot);
    if(verbose){char b[460];psprintf(b,"0.7.81 STANDARD: published consumer tint for water=%p state=%p floor=%p direction=(%.4f,%.4f,%.4f). No FloorArea payload write.\r\n",water,st,floor,(double)desired.r,(double)desired.g,(double)desired.b);log_raw(b);}
    return true;
}

static P49GlowRoute p49_current_glow_route(bool verbose){
    refresh_water_targets();
    if(g_waterCount<=0||g_waterTarget<0||g_waterTarget>=g_waterCount){if(verbose)log_raw("0.7.81 ROUTE: no valid selected water; glow write suppressed.\r\n");return P49_ROUTE_UNKNOWN;}
    p48_cached_dredge_index();void* current=g_water[g_waterTarget].renderer;
    if(g_p48DredgeWater&&current==g_p48DredgeWater){if(verbose){char b[300];psprintf(b,"0.7.81 ROUTE: water %d/%d is PHYSICAL DREDGE -> retail AquariumTintDefinition path only.\r\n",g_waterTarget+1,g_waterCount);log_raw(b);}return P49_ROUTE_DREDGE;}
    void* st=nullptr;void* floor=nullptr;if(p53_map_selected_standard(verbose,st,floor)){if(verbose){char b[420];psprintf(b,"0.7.81 ROUTE: water %d/%d physically maps to null-tint state=%p floor=%p -> STANDARD FloorArea path.\r\n",g_waterTarget+1,g_waterCount,st,floor);log_raw(b);}return P49_ROUTE_STANDARD;}
    if(verbose){char b[400];psprintf(b,"0.7.81 ROUTE: selected water has no safe physical standard mapping (waters=%d nullTintStates=%d); glow write suppressed.\r\n",g_waterCount,p48_null_tint_state_count());log_raw(b);}return P49_ROUTE_UNKNOWN;
}

static int p49_sync_glow_for_current(F4 color,bool verbose){
    if(!p55_glow_on())return 0;
    P49GlowRoute route=p49_current_glow_route(verbose);
    if(route==P49_ROUTE_DREDGE){
        return sync_live_glow_to_picker(color,verbose);
    }
    if(route==P49_ROUTE_STANDARD){
        bool ok=p48_sync_standard_floor_sink_for_current(color,verbose);
        if(!ok&&verbose)log_raw("0.7.81 ROUTE: STANDARD FloorArea sink failed; Dredge tint was NOT touched.\r\n");
        return 0;
    }
    if(verbose)log_raw("0.7.81 ROUTE: UNKNOWN; water/glass updated, glow left unchanged.\r\n");
    return 0;
}

static bool g_p51PostHookActive=false;
static u64 g_p51HookCalls=0;
static u64 g_p51PostOverwriteDetections=0;
static u64 g_p52PostWriteOnly=0;
static u64 g_p51PostDirtyOk=0;   // fallback-only dirty calls when the hook cannot install
static u64 g_p51PostDirtyFail=0; // fallback-only dirty calls when the hook cannot install

static bool p54_state_in_list(const StateList& sl,void* state){
    if(!state)return false;for(int i=0;i<sl.count;++i)if(sl.states[i]==state)return true;return false;
}
#include "LightingConsumer.h"

static void p48_reassert_standard_floor_sink(bool verbose){
    (void)verbose;
    // Called only by the game-thread OnUpdate hook, including when sync is OFF.
    p55_dispatch_refreshes();
}
static void p54_invalidate_all_standard_slots_on_state_change(bool verbose){
    int dropped=0;
    for(int i=0;i<16;++i){P54FloorSlotSnapshot snap{};if(p54_snapshot_slot(i,snap)&&snap.active){p54_clear_slot(g_p53FloorSlots[i]);++dropped;}}
    if(dropped>0)++g_p54StateChangeInvalidations;
    g_p48FloorActive=false;g_p48BoundWater=nullptr;g_p48State=nullptr;g_p48Floor=nullptr;g_p48Original=P48OptionalColour{};g_p48Desired=P48OptionalColour{};
    if(verbose&&dropped>0){char b[360];psprintf(b,"0.7.81 FLOOR SINK SAFETY: AquariumState set changed; invalidated %d standard hold slot(s) without dereferencing cached state/FloorArea pointers. Fresh physical mapping is required before any further standard glow write.\r\n",dropped);log_raw(b);}
}
static void p48_suspend_standard_floor_sink(bool verbose){
    // No game payload was overwritten; the next game update refreshes retired tints.
    if(verbose)log_raw("0.7.81 STANDARD: retail lighting restoration queued; no raw floor writes.\r\n");
}
static void p48_clear_standard_floor_sink_if_selected(bool verbose){
    refresh_water_targets();void* current=(g_waterCount>0&&g_waterTarget<g_waterCount)?g_water[g_waterTarget].renderer:nullptr;if(!current)return;
    P53FloorSlot* slot=p53_slot_for_water(current);if(!slot)return;
    p54_clear_slot(*slot);g_p48FloorActive=p53_active_floor_slot_count()>0;
    if(g_p48BoundWater==current){g_p48BoundWater=nullptr;g_p48State=nullptr;g_p48Floor=nullptr;g_p48Desired=P48OptionalColour{};}
    if(verbose)log_raw("0.7.81 STANDARD: selected tint removed; game-thread refresh queued.\r\n");
}

// Post-retail game-thread dispatch. No raw FloorArea writes or worker fallback.
using P51AquariumOnUpdateFn = void(*)(void*, const MethodInfo*);
static P51AquariumOnUpdateFn g_p51OriginalOnUpdate=nullptr;
static void* g_p51OnUpdateTarget=nullptr;
static bool g_p51HookReentrant=false;
static u8 g_p51OriginalPrologue[14]{};

static void p51_write_abs_jump(u8* at,void* dst){
    at[0]=0xFF;at[1]=0x25;at[2]=0;at[3]=0;at[4]=0;at[5]=0;
    uptr p=(uptr)dst;memcpy(at+6,&p,8);
}
static void p78_free_trampoline(void* memory){
    auto release=(BOOL(*)(void*,usize,DWORD))resolve_export(find_loaded_module("kernel32.dll"),"VirtualFree");
    if(memory&&release)release(memory,0,0x8000);
}
static bool p78_seal_trampoline(void* memory,usize bytes){
    DWORD old=0;return pFlushInstructionCache(pGetCurrentProcess(),memory,bytes)&&pVirtualProtect(memory,64,0x20,&old);
}
static bool p56_install();
static void p56_tick();
static void p51_aquarium_onupdate_hook(void* self,const MethodInfo* method){
    P51AquariumOnUpdateFn original=g_p51OriginalOnUpdate;
    if(!p78_main()){if(original)original(self,method);return;}P78WorkScope work;
    if(original)original(self,method);
    p78_waterFresh=false; // Retail may have rebuilt renderers during this update.
    ++g_p51HookCalls;
    if(g_p51HookReentrant)return;
    g_p51HookReentrant=true;
    static bool consumerAttempted=false;
    if(!consumerAttempted&&p55_update_ready()){consumerAttempted=true;p55_install_consumer_hook();}
    static bool uiAttempted=false;
    if(!uiAttempted&&p55_update_ready()){uiAttempted=true;if(!p56_install())log_raw("0.7.81 UI: installation unavailable; see guard diagnostics.\r\n");}
    p56_tick();
    g_p51PostHookActive=true;
    p48_reassert_standard_floor_sink(false);
    g_p51PostHookActive=false;
    g_p51HookReentrant=false;
}
static bool p51_install_post_update_hook(){
    if(!p78_main())return false;
    if(p55_update_ready())return true;
    if(!pVirtualAlloc||!pVirtualProtect||!pFlushInstructionCache||!pGetCurrentProcess){log_raw("0.7.81 POST-UPDATE HOLD: required VirtualAlloc/VirtualProtect/FlushInstructionCache APIs unavailable; hook not installed.\r\n");return false;}
    Il2CppClass* c=find_class("TPS.Game","AquariumManager");
    const MethodInfo* m=c?find_method_name_params(c,"OnUpdate",0):nullptr;
    u8* target=m?(u8*)*(void* const*)m:nullptr; void* ga=find_loaded_module("GameAssembly.dll");
    if(!target||!ptr_in_module(target,ga,32)){log_raw("0.7.81 POST-UPDATE HOLD: AquariumManager::OnUpdate native pointer unavailable/outside GameAssembly; hook not installed.\r\n");return false;}
    static const u8 expected[14]={0x40,0x55,0x53,0x56,0x57,0x41,0x55,0x41,0x56,0x48,0x8D,0x6C,0x24,0xC8};
    for(int i=0;i<14;++i)if(target[i]!=expected[i]){char b[360];psprintf(b,"0.7.81 POST-UPDATE HOLD: OnUpdate prologue mismatch at +0x%X (got=%02X expected=%02X); refusing unsafe detour.\r\n",i,(unsigned)target[i],(unsigned)expected[i]);log_raw(b);return false;}
    u8* trampoline=(u8*)pVirtualAlloc(nullptr,64,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);
    if(!trampoline){log_raw("0.7.81 POST-UPDATE HOLD: executable trampoline allocation failed; hook not installed.\r\n");return false;}
    memcpy(g_p51OriginalPrologue,target,14); memcpy(trampoline,target,14); p51_write_abs_jump(trampoline+14,target+14);
    if(!p78_seal_trampoline(trampoline,28)){p78_free_trampoline(trampoline);return false;}
    g_p51OriginalOnUpdate=(P51AquariumOnUpdateFn)trampoline; g_p51OnUpdateTarget=target;
    DWORD oldProtect=0; if(!pVirtualProtect(target,14,PAGE_EXECUTE_READWRITE,&oldProtect)){g_p51OriginalOnUpdate=nullptr;g_p51OnUpdateTarget=nullptr;p78_free_trampoline(trampoline);log_raw("0.7.81 POST-UPDATE HOLD: VirtualProtect failed; hook not installed.\r\n");return false;}
    p51_write_abs_jump(target,(void*)&p51_aquarium_onupdate_hook);
    pFlushInstructionCache(pGetCurrentProcess(),target,14);
    DWORD ignored=0;pVirtualProtect(target,14,oldProtect,&ignored);
    __atomic_store_n(&g_p55UpdateInstalled,true,__ATOMIC_RELEASE);
    log_raw("0.7.81 UPDATE DISPATCH: installed; retail runs first, then pending lighting changes are refreshed on the game thread.\r\n");
    return true;
}

#include "AquariumUI.h"
#include "ColourPickerBridge.h"
#include "PaletteBridge.h"
#include "SamplingBridge.h"

static void p56_tick(){
    if(!p78_main())return;
    if(!p56_clock||p56_busy||p56_closing)return;
    u64 now=p56_clock();static u64 last=0,lastPersistPoll=0;
    if(now-last<15)return;last=now;p56_busy=true;
    static bool escapeWasDown=false;
    static P70FocusGate focusGate{};
    bool inputAccepted=focusGate.accept(p70_game_foreground()!=nullptr);
    if(g_pickerSampling)inputAccepted=false;
    if(!inputAccepted){g_pickerDragMode=0;g_manualMouseWasDown=true;}
    static u64 lastStateSig=0;static u32 lastScopeGeneration=0;static int reapplyDelayTicks=0,reapplyAttempts=0;
    bool escape=(pGetAsyncKeyState(0x1b)&0x8000)!=0;
    if(p56_menu){
        if(!p64_tick_target_live(now)){log_raw("0.7.81 UI CLOSE reason: selection closed/changed or target invalid.\r\n");p56_end(false,true);}
        else if(!p56_opened(p56_menu)){log_raw("0.7.81 UI CLOSE reason: native menu reports closed.\r\n");p56_end(false,true);}
        else if(inputAccepted&&(escape&&!escapeWasDown)){log_raw("0.7.81 UI CLOSE reason: Escape.\r\n");p56_end(true,true);}
    }
    if(inputAccepted)poll_stock_picker();
    p56_gradient();
    static u64 lastGlow=0;
    if(p65_glowPending&&(g_pickerDragMode==0||now-lastGlow>=100)){
        lastGlow=now;p65_flush_glow();
    }
        // Cheap background detector only. Full Renderer scans are delayed and limited to load/rebuild events.
        if(now-lastPersistPoll>=500){
            lastPersistPoll=now;
            bool scopeReady=p70_sync_scope();
            if(scopeReady&&p70_dirty)p70_commit();
            p57_reconcile();
            u64 sig=aquarium_state_signature();
            if(sig!=lastStateSig||lastScopeGeneration!=p70_scopeGeneration){ lastStateSig=sig;lastScopeGeneration=p70_scopeGeneration; p54_invalidate_all_standard_slots_on_state_change(true); reapplyDelayTicks=12; reapplyAttempts=4; log_raw("PERSIST: AquariumState or save scope changed; waiting ~6 seconds for water renderers before reapply.\r\n"); }
            else if(scopeReady&&reapplyDelayTicks>0&&!g_pickerOpen){
                --reapplyDelayTicks;
                if(reapplyDelayTicks==0&&reapplyAttempts>0){ int applied=apply_persisted_to_live_targets(true); --reapplyAttempts;
                    if(applied>0){ reapplyAttempts=0; log_raw("PERSIST: delayed load/rebuild reapply completed.\r\n"); }
                    else if(reapplyAttempts>0){ log_live_water_keys_for_persist(); reapplyDelayTicks=8; log_raw("PERSIST: no matching live water renderer yet; retrying in ~4 seconds.\r\n"); }
                    else { log_live_water_keys_for_persist(); log_raw("PERSIST: delayed reapply exhausted; no saved live aquarium match.\r\n"); }
                }
            }
        }

    escapeWasDown=escape;p56_busy=false;
}

#include "CrashDiagnostic.h"

#include "GameThreadBootstrap.h"
