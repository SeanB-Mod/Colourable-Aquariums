#include <cstdio>
#include <cassert>
#include <cstddef>
#include <cstring>
#define psprintf std::sprintf
static int logs=0;
static char message[512];
static void log_raw(const char* text){++logs;std::snprintf(message,sizeof(message),"%s",text);}
static void* find_loaded_module(const char*){return (void*)0x180000000ull;}
static void* resolve_export(void*,const char*){return nullptr;}
#include "CrashDiagnostic.h"
int main(){
 static_assert(offsetof(P61ExceptionRecord,address)==16);
 assert(p61_exception(nullptr)==0&&logs==0);
 P61ExceptionRecord r{0xe0434352u,0,nullptr,(void*)0x1820de380ull};
 P61ExceptionPointers p{&r,nullptr};
 assert(p61_exception(&p)==0&&logs==0);
 r.code=0xc0000005u;
 assert(p61_exception(&p)==0&&logs==1);
 assert(std::strstr(message,"C0000005")&&std::strstr(message,"first chance"));
 p61_reporting=1;assert(p61_exception(&p)==0&&logs==1);p61_reporting=0;
 for(int i=0;i<20;++i)assert(p61_exception(&p)==0);
 assert(logs==8&&p61_reporting==0);
 std::puts("Exception observer passed: x64 layout, filter, always continue-search, recursion guard and eight-record bound.");
}
