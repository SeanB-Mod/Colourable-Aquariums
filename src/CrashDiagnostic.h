// Windows x64 EXCEPTION_RECORD prefix. Observe only; never handle or resume a fault.
struct P61ExceptionRecord { unsigned code,flags;void* nested;void* address; };
struct P61ExceptionPointers { P61ExceptionRecord* record;void* context; };
static_assert(sizeof(P61ExceptionRecord)==24,"Windows x64 exception prefix layout");
static volatile long p61_reporting=0;
static unsigned p61_reportCount=0;
static long p61_exception(P61ExceptionPointers* pointers){
    if(!pointers||!pointers->record)return 0;
    auto record=pointers->record;
    if(record->code!=0xc0000005u&&record->code!=0xc000001du&&record->code!=0xc0000096u)return 0;
    if(__atomic_exchange_n(&p61_reporting,1,__ATOMIC_SEQ_CST))return 0;
    if(p61_reportCount<8){
        ++p61_reportCount;
        char b[512];
        psprintf(b,"0.7.81 NATIVE EXCEPTION (first chance, not necessarily fatal): code=0x%08X address=%p GameAssembly=%p UnityPlayer=%p Aquarium=%p. Exception continues to normal handlers.\r\n",
            record->code,record->address,find_loaded_module("GameAssembly.dll"),find_loaded_module("UnityPlayer.dll"),find_loaded_module("TPM-AquaColour.dll"));
        log_raw(b);
    }
    __atomic_store_n(&p61_reporting,0,__ATOMIC_SEQ_CST);
    return 0; // EXCEPTION_CONTINUE_SEARCH
}
static void p61_install_exception_observer(){
    using AddHandler=void*(*)(unsigned,long(*)(P61ExceptionPointers*));
    auto add=(AddHandler)resolve_export(find_loaded_module("kernel32.dll"),"AddVectoredExceptionHandler");
    bool installed=add&&add(1,p61_exception);
    log_raw(installed?"0.7.81 DIAGNOSTIC: native exception observer installed; native picker test enabled.\r\n":"0.7.81 DIAGNOSTIC: native exception observer unavailable.\r\n");
}
