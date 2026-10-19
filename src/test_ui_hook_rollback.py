"""Exercise the production original-byte capture and partial-install rollback."""
import pathlib,subprocess,sys,re
root=pathlib.Path(__file__).parent
s=(root/'AquariumUI.h').read_text()
struct=re.search(r'struct P56Hook\{.*?\};',s)[0]
rollback=next(line.strip() for line in s.splitlines() if line.strip().startswith('if(patched!=6)'))
capture=re.search(r'memcpy\(h.originalBytes,h.target,h.size\);',s)[0]
fixture=r'''
#include <cassert>
#include <cstdio>
#include <cstring>
using u8=unsigned char;using DWORD=unsigned long;
static int failIndex=-1,protectIndex=0,flushes=0;
static bool pVirtualProtect(void*,int,DWORD,DWORD* old){*old=0x20;return protectIndex++!=failIndex;}
static void* pGetCurrentProcess(){return nullptr;}
static void pFlushInstructionCache(void*,void*,int){++flushes;}
#define PAGE_EXECUTE_READWRITE 0x40
'''+struct+'\nstatic bool restore(int patched,P56Hook* hooks){'+rollback+'return true;}\n'+r'''
int main(){
 for(int count=1;count<6;++count){
  P56Hook hooks[6]{};u8 code[6][16],live[6][16],expected[16]{};
  for(int j=0;j<6;++j){auto& h=hooks[j];h.target=code[j];h.bytes=expected;h.size=j==3?14:16;
   for(int i=0;i<16;++i)code[j][i]=live[j][i]=(u8)(0x40+j+i);
   code[j][10]=live[j][10]=0x02;
'''+capture+r'''
   if(j<count)std::memset(code[j],0xcc,h.size);
  }
  protectIndex=0;failIndex=-1;flushes=0;assert(!restore(count,hooks));assert(flushes==count);
  for(int j=0;j<6;++j)assert(std::memcmp(code[j],live[j],16)==0);
 }
 std::puts("UI partial-install rollback: actual live bytes (including 02 operand) restored for failures after hooks 1 through 5; unpatched bytes preserved.");
}
'''
out=pathlib.Path(sys.argv[2]);out.mkdir(parents=True,exist_ok=True)
cpp=out/'ui_hook_rollback.cpp';cpp.write_text(fixture);exe=out/'ui_hook_rollback.exe'
subprocess.run([sys.argv[1],'-O2',str(cpp),'-o',str(exe)],check=True)
subprocess.run([str(exe.resolve())],check=True)
