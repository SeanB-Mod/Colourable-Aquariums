#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <cstdint>
#include "AquariumSelection.h"
static int failures=0;
static void check(bool ok,const char* what){if(!ok){++failures;std::printf("FAIL: %s\n",what);}}
int main(){
    P56Bounds b[]={{0,0,0,5,1,5},{12,0,0,5,1,5}};
    check(p56_unique_water(0,0,0,b,2)==0,"first room");
    check(p56_unique_water(12,0,0,b,2)==1,"second room");
    check(p56_unique_water(6,0,0,b,2)==-1,"gap is not a tank");
    check(p56_unique_water(100,0,0,b,2)==-1,"unmapped room");
    check(p56_unique_water(NAN,0,0,b,2)==-1,"NaN rejected");
    check(p56_unique_water(0,0,0,nullptr,2)==-1,"null bounds");
    check(p56_unique_water(0,0,0,b,33)==-1,"oversized list");
    P56Bounds overlap[]={{0,0,0,5,1,5},{1,0,0,5,1,5}};
    check(p56_unique_water(0,0,0,overlap,2)==-1,"overlap rejected");
    b[0].ex=0;check(p56_unique_water(0,0,0,b,2)==-1,"invalid geometry");b[0].ex=5;
    for(int i=0;i<10000;++i){P56Bounds reordered[]={b[1],b[0]};check(p56_unique_water(12,0,0,reordered,2)==0,"renderer order independent");}
    // Execute the actual replacement encoding for the one RIP-relative prologue.
    auto mem=(unsigned char*)VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);
    if(!mem)return 2;
    unsigned char head[]={0x40,0x56,0x57,0x48,0x83,0xec,0x38};
    std::memcpy(mem,head,7);mem[7]=0x49;mem[8]=0xba;
    auto address=(uintptr_t)(mem+256);std::memcpy(mem+9,&address,8);
    unsigned char tail[]={0x4d,0x8b,0x12,0x4c,0x89,0xd0,0x48,0x83,0xc4,0x38,0x5f,0x5e,0xc3};
    std::memcpy(mem+17,tail,sizeof(tail));FlushInstructionCache(GetCurrentProcess(),mem,64);
    auto f=(uint64_t(*)())mem;
    for(uint64_t i=0;i<10000;++i){uint64_t expected=0xf123456789abcdefULL^i;std::memcpy(mem+256,&expected,8);check(f()==expected,"relocated R10 load and stack restoration");}
    VirtualFree(mem,0,MEM_RELEASE);
    std::printf("UI tests: 10009 selection checks, 10000 relocated native calls; failures=%d\n",failures);
    return failures?1:0;
}
