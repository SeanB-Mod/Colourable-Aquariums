// Bounded per-process log with one previous file; logging never spins or holds a file open.
static bool p78_debugLog=false;
static unsigned p78_logLock=0;
static DWORD p78_logBytes=0;
static DWORD (*p78_fileSize)(HANDLE,DWORD*)=nullptr;
static BOOL (*p78_moveLog)(LPCSTR,LPCSTR,DWORD)=nullptr;
static void p78_init_logging(){
    void* k=find_loaded_module("kernel32.dll");
    p78_fileSize=(decltype(p78_fileSize))resolve_export(k,"GetFileSize");
    p78_moveLog=(decltype(p78_moveLog))resolve_export(k,"MoveFileExA");
    char debug[8]{};if(pGetEnvironmentVariableA)pGetEnvironmentVariableA("TPM_AQUARIUM_DEBUG",debug,sizeof(debug));p78_debugLog=streq(debug,"1");
    HANDLE h=pCreateFileA(g_logPath,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(h&&h!=INVALID_HANDLE_VALUE){if(p78_fileSize){DWORD size=p78_fileSize(h,nullptr);if(size!=0xffffffffu)p78_logBytes=size;}pCloseHandle(h);}
}
static void log_raw(const char* s){
    if(!s||!pCreateFileA)return;
    if(!p78_debugLog&&(contains_i(s,"UI OPEN 0")||contains_i(s,"LIGHT SAMPLE")||contains_i(s,"PICKER MANUAL:")))return;
    unsigned expected=0;if(!__atomic_compare_exchange_n(&p78_logLock,&expected,1,false,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED))return;
    DWORD bytes=(DWORD)slen(s);
    if(p78_logBytes+bytes>2u*1024u*1024u){
        char previous[640];scopy(previous,sizeof(previous),g_logPath);sappend(previous,sizeof(previous),".previous");
        if(!p78_moveLog||!p78_moveLog(g_logPath,previous,1)){__atomic_store_n(&p78_logLock,0,__ATOMIC_RELEASE);return;}
        p78_logBytes=0;
    }
    HANDLE h=pCreateFileA(g_logPath,GENERIC_WRITE,FILE_SHARE_READ,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(h&&h!=INVALID_HANDLE_VALUE){pSetFilePointer(h,0,nullptr,FILE_END);DWORD written=0;pWriteFile(h,s,bytes,&written,nullptr);p78_logBytes+=written;pCloseHandle(h);}
    __atomic_store_n(&p78_logLock,0,__ATOMIC_RELEASE);
}
