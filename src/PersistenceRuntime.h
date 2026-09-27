// Main-thread-only persistence adapter. Never imports the unscoped legacy file.
static void* p50_level_state_instance();
static void* p50_level_from_state(void*);
static BOOL (*p70_virtualFree)(void*,usize,DWORD)=nullptr;
static DWORD (*p70_fileSize)(HANDLE,DWORD*)=nullptr;
static BOOL (*p70_flushFile)(HANDLE)=nullptr;
static BOOL (*p70_moveFile)(LPCSTR,LPCSTR,DWORD)=nullptr;
static BOOL (*p70_copyFile)(LPCSTR,LPCSTR,BOOL)=nullptr;
static DWORD (*p70_lastError)()=nullptr;
static void* p70_alloc(unsigned long long bytes){return pVirtualAlloc(nullptr,bytes,MEM_COMMIT|MEM_RESERVE,4);}
static void p70_free(void* p){if(p)p70_virtualFree(p,0,0x8000);}
#include "PersistenceCore.h"
static P70Store p70_store{};
static P70Scope p70_scope{};
static u32 p70_scopeGeneration=0;
static void* p70_levelObject=nullptr;
static bool p70_haveScope=false,p70_dirty=false,p70_primaryHealthy=false,p70_writable=false;
static char p70_path[800]{},p70_backup[800]{},p70_temp[800]{};
static bool p70_io_ready(){
    if(p70_virtualFree&&p70_fileSize&&p70_flushFile&&p70_moveFile&&p70_copyFile&&p70_lastError)return true;void* k=find_loaded_module("kernel32.dll");
    p70_virtualFree=(decltype(p70_virtualFree))resolve_export(k,"VirtualFree");
    p70_fileSize=(decltype(p70_fileSize))resolve_export(k,"GetFileSize");
    p70_flushFile=(decltype(p70_flushFile))resolve_export(k,"FlushFileBuffers");
    p70_moveFile=(decltype(p70_moveFile))resolve_export(k,"MoveFileExA");
    p70_copyFile=(decltype(p70_copyFile))resolve_export(k,"CopyFileA");
    p70_lastError=(decltype(p70_lastError))resolve_export(k,"GetLastError");
    return p70_virtualFree&&p70_fileSize&&p70_flushFile&&p70_moveFile&&p70_copyFile&&p70_lastError;
}
static bool p70_current_scope(P70Scope& out,void*& level){
    level=p50_level_from_state(p50_level_state_instance());if(!level)return false;
    if(!boxed_bool(field_object(level,"<LoadHUDComplete>k__BackingField")))return false;
    void* history=field_object(level,"<LevelPlaythroughHistory>k__BackingField");if(!history)return false;
    static Il2CppClass* appClass=nullptr;if(!appClass)appClass=find_class("TPS.Core","App");
    void* app=appClass?static_ref_field(appClass,"<Instance>k__BackingField"):nullptr;
    if(!app||boxed_bool(field_object(app,"<IsQuitting>k__BackingField"))||boxed_bool(field_object(app,"<IsLoading>k__BackingField")))return false;
    void* saves=field_object(app,"_saveSystem");void* context=field_object(saves,"_context");if(!saves||!context)return false;
    const char* kind=il2cpp_class_get_name(il2cpp_object_get_class((Il2CppObject*)context));
    int mode=streq(kind,"CareerSaveSystemContext")?1:streq(kind,"SandboxSaveSystemContext")?2:0;
    int slot=field_i32(saves,"_currentSaveSlot",-1);
    auto career=field_object(history,"CareerPlaythroughIDPlaythroughStarted");
    auto museum=field_object(history,"LevelPlaythroughID");
    if(!mode||slot<0||!career||!museum)return false;
    out={(u32)boxed_i32(career),(u32)boxed_i32(museum),(u32)slot,(u32)mode};
    return out.level!=0; // Unknown/uninitialised identities never share a default file.
}
static int p70_read_snapshot(const char* path,P70Scope scope){
    HANDLE file=pCreateFileA(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE){DWORD error=p70_lastError();return error==2||error==3?0:-1;}
    DWORD high=0,size=p70_fileSize(file,&high);bool ok=false;
    if(!high&&size>=sizeof(P70Header)&&size<=sizeof(P70Header)+P70_MAX_RECORDS*sizeof(P70Record)){
        void* bytes=p70_alloc(size);if(bytes){DWORD read=0;
            ok=pReadFile(file,bytes,size,&read,nullptr)&&read==size&&p70_decode(p70_store,scope,bytes,size);
            p70_free(bytes);
        }
    }
    pCloseHandle(file);return ok?1:-1;
}
static bool p70_commit(){
    if(!p70_dirty)return true;if(!p70_haveScope||!p70_writable)return false;
    u32 size=p70_size(p70_store);void* bytes=p70_alloc(size);if(!bytes)return false;
    bool ok=p70_encode(p70_store,p70_scope,bytes,size);
    HANDLE file=ok?pCreateFileA(p70_temp,GENERIC_WRITE,0,nullptr,2,FILE_ATTRIBUTE_NORMAL,nullptr):INVALID_HANDLE_VALUE;
    if(file==INVALID_HANDLE_VALUE)ok=false;
    else {DWORD written=0;ok=pWriteFile(file,bytes,size,&written,nullptr)&&written==size&&p70_flushFile(file);pCloseHandle(file);}
    p70_free(bytes);
    // Preserve only a verified primary; never overwrite a good backup with corrupt input.
    if(ok&&p70_primaryHealthy)ok=p70_copyFile(p70_path,p70_backup,0)!=0;
    if(ok)ok=p70_moveFile(p70_temp,p70_path,9)!=0; // Replace existing + write through.
    if(ok){p70_dirty=false;p70_primaryHealthy=true;return true;}
    log_raw("0.7.81 PERSIST ERROR: snapshot commit failed; previous file retained, pending edits kept for retry.\r\n");return false;
}
static bool p70_sync_scope(){
    if(!p70_io_ready()||!g_persistPath[0])return false;
    P70Scope next{};void* level=nullptr;if(!p70_current_scope(next,level))return false;
    if(p70_haveScope&&p70_equal(next,p70_scope)&&level==p70_levelObject)return p70_writable;
    if(p70_dirty&&!p70_commit())return false;
    p70_clear(p70_store);p70_scope=next;p70_levelObject=level;p70_haveScope=true;++p70_scopeGeneration;
    psprintf(p70_path,"%s.v2-%u-%u-%u-%u",g_persistPath,next.mode,next.slot,next.career,next.level);
    psprintf(p70_backup,"%s.bak",p70_path);psprintf(p70_temp,"%s.tmp",p70_path);
    int current=p70_read_snapshot(p70_path,next);p70_primaryHealthy=current==1;
    int backup=current==1?0:p70_read_snapshot(p70_backup,next);
    p70_writable=current==1||backup==1||(current==0&&backup==0);
    if(backup==1)log_raw("0.7.81 PERSIST: recovered last-good backup; latest snapshot was missing or invalid.\r\n");
    if(!p70_writable)log_raw("0.7.81 PERSIST ERROR: no valid snapshot; editing blocked to preserve damaged files.\r\n");
    char message[1024];psprintf(message,"0.7.81 PERSIST SCOPE: mode=%u slot=%u career=%u museum=%u records=%u writable=%d file=%s. Legacy file untouched.\r\n",next.mode,next.slot,next.career,next.level,p70_store.count,(int)p70_writable,p70_path);log_raw(message);
    return p70_writable;
}
static void load_persistence(){log_raw("0.7.81 PERSIST: fresh scoped storage enabled; legacy colour file preserved without import. Waiting for loaded museum identity.\r\n");}
static bool append_persistence(F3 key,F4 color,bool enabled){
    P70Scope current{};void* level=nullptr;
    // A close callback for the previous museum must not save its renderer into the next museum.
    if(!p70_haveScope||!p70_current_scope(current,level)||!p70_equal(current,p70_scope)||level!=p70_levelObject){
        log_raw("0.7.81 PERSIST ERROR: save context changed; refusing a write from the previous museum.\r\n");return false;
    }
    if(!p70_sync_scope()){log_raw("0.7.81 PERSIST ERROR: museum identity/storage unavailable; colour was not saved.\r\n");return false;}
    color.w=0;P70Record record{key,color,enabled?1u:0u};
    if(!p70_put(p70_store,record)){log_raw("0.7.81 PERSIST ERROR: invalid record, allocation failure or capacity exceeded; colour was not saved.\r\n");return false;}
    p70_dirty=true;return p70_commit();
}
static void persist_current_target(bool enabled){
    if(g_waterCount<=0||g_waterTarget<0||g_waterTarget>=g_waterCount)return;
    Bounds6 b{};if(!get_renderer_bounds(g_water[g_waterTarget].renderer,b))return;
    F4 c=g_water[g_waterTarget].hasCustom?g_water[g_waterTarget].customColor:g_currentWaterColor;
    if(append_persistence(b.center,c,enabled))log_raw(enabled?"0.7.81 PERSIST: colour snapshot saved.\r\n":"0.7.81 PERSIST: reset snapshot saved.\r\n");
}
static bool find_persist_for_renderer(void* renderer,F4& color,bool& migrated){
    migrated=false;if(!p70_sync_scope())return false;
    Bounds6 bounds{};return get_renderer_bounds(renderer,bounds)&&p70_find(p70_store,bounds.center,color);
}
