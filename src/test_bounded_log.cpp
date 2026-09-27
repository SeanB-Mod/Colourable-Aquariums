#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
using DWORD=unsigned long;using BOOL=int;using HANDLE=void*;using LPCSTR=const char*;using usize=unsigned long long;
#define INVALID_HANDLE_VALUE ((void*)-1)
#define GENERIC_READ 1
#define GENERIC_WRITE 2
#define FILE_SHARE_READ 1
#define OPEN_EXISTING 3
#define OPEN_ALWAYS 4
#define FILE_ATTRIBUTE_NORMAL 0
#define FILE_END 2
static char g_logPath[600]="fixture.log";
static bool streq(const char*a,const char*b){return !strcmp(a,b);}static usize slen(const char*a){return strlen(a);}static void scopy(char*d,usize n,const char*s){snprintf(d,n,"%s",s);}static void sappend(char*d,usize n,const char*s){strncat(d,s,n-strlen(d)-1);}static bool contains_i(const char*a,const char*b){return strstr(a,b)!=nullptr;}
static int writes=0,rotates=0;static bool moveFails=false;static DWORD partial=0;
static HANDLE create(LPCSTR,DWORD,DWORD,void*,DWORD,DWORD,HANDLE){return (void*)1;}static auto pCreateFileA=create;
static void pCloseHandle(HANDLE){}static void pSetFilePointer(HANDLE,int,void*,int){}
static BOOL pWriteFile(HANDLE,const void*,DWORD bytes,DWORD* wrote,void*){++writes;*wrote=partial?partial:bytes;return 1;}
static DWORD size(HANDLE,DWORD*){return 0;}static BOOL move(LPCSTR,LPCSTR,DWORD){++rotates;return !moveFails;}
static DWORD env(LPCSTR,char*out,DWORD){out[0]=0;return 0;}static auto pGetEnvironmentVariableA=env;
static void* find_loaded_module(const char*){return nullptr;}static void* resolve_export(void*,const char*n){return !strcmp(n,"GetFileSize")?(void*)size:(void*)move;}
#include "BoundedLog.h"
int main(){p78_init_logging();log_raw("UI OPEN 01");assert(writes==0);log_raw("ERROR");assert(writes==1);p78_logLock=1;log_raw("ERROR");assert(writes==1);p78_logLock=0;
 p78_logBytes=2*1024*1024;log_raw("ERROR");assert(rotates==1&&writes==2&&p78_logBytes==5);
 p78_logBytes=2*1024*1024;moveFails=true;log_raw("ERROR");assert(rotates==2&&writes==2&&!p78_logLock);
 moveFails=false;partial=2;log_raw("ERROR");assert(p78_logBytes==2);p78_debugLog=true;log_raw("UI OPEN 01");assert(writes==4);
 puts("Bounded log: debug filtering, important messages, reentrancy, rollover, failed rotation and partial-write accounting passed.");}
