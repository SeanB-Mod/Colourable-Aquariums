#include <initializer_list>
#include <cassert>
#include <cstdio>
#include <cstring>
using u8=unsigned char; using u32=unsigned int; using i64=long long;
using uptr=unsigned long long; using usize=unsigned long long;
#include "InputHookGuard.h"
static u8 image[0x4000000];
static void displacement(u8* code,u32 value){for(int i=0;i<4;++i)code[10+i]=(u8)(value>>(i*8));}
int main(){
 const u8 prefix[]={0x40,0x56,0x57,0x48,0x83,0xec,0x38,0x4c,0x8b,0x15};
 u8* code=image+128;std::memcpy(code,prefix,10);
 uptr base=(uptr)image,operand=0;
 for(u32 d:{0x039a1fbau,0x039a9be2u,0x039a1f02u}){
  displacement(code,d);assert(p91_input_operand(code,base,sizeof(image),operand));assert(operand==(uptr)code+14+d);
 }
 for(int i=0;i<10;++i){code[i]^=1;assert(!p91_input_operand(code,base,sizeof(image),operand));code[i]^=1;}
 displacement(code,0x80000000u);assert(!p91_input_operand(code,base,sizeof(image),operand));
 displacement(code,0x7fffffffu);assert(!p91_input_operand(code,base,sizeof(image),operand));
 displacement(code,(u32)(sizeof(image)-8-128-14));assert(p91_input_operand(code,base,sizeof(image),operand));assert(operand==base+sizeof(image)-8);
 displacement(code,(u32)(sizeof(image)-7-128-14));assert(!p91_input_operand(code,base,sizeof(image),operand));
 displacement(code,(u32)(-142));assert(p91_input_operand(code,base,sizeof(image),operand)&&operand==base);
 displacement(code,(u32)(-143));assert(!p91_input_operand(code,base,sizeof(image),operand));
 assert(!p91_input_operand(code,base,13,operand));
 assert(!p91_input_operand((u8*)(base-1),base,sizeof(image),operand));
 assert(!p91_input_operand(image+sizeof(image)-13,base,sizeof(image),operand));
 P91MemoryInfo m{};m.base=image;m.allocationBase=image;m.regionSize=sizeof(image);m.state=0x1000;m.type=0x1000000;
 for(unsigned long p:{2ul,4ul,8ul,0x20ul,0x40ul,0x80ul}){m.protect=p;assert(p91_operand_readable(base+128,base,m));}
 for(unsigned long p:{0ul,1ul,0x10ul,0x104ul}){m.protect=p;assert(!p91_operand_readable(base+128,base,m));}
 m.protect=4;m.state=0x2000;assert(!p91_operand_readable(base+128,base,m));m.state=0x1000;
 m.type=0x20000;assert(!p91_operand_readable(base+128,base,m));m.type=0x1000000;
 m.allocationBase=image+1;assert(!p91_operand_readable(base+128,base,m));m.allocationBase=image;
 assert(p91_operand_readable(base+sizeof(image)-8,base,m));assert(!p91_operand_readable(base+sizeof(image)-7,base,m));
 m.regionSize=7;assert(!p91_operand_readable(base,base,m));
 std::puts("Input hook guard: original/October/02 synthetic variants, all opcode mutations, signed displacement, image and readable-region boundaries passed.");
}

