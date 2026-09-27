import pathlib,subprocess,sys
root=pathlib.Path(__file__).parent
s=(root/'AquariumColour_source.cpp').read_text()
a=s.index('static HWND p70_game_foreground(){');b=s.index('\nstatic char g_logPath',a)
fixture=r"""
#include <cassert>
#include <cstdio>
#include <cstring>
using HWND=void*;using DWORD=unsigned long;
#define VK_LBUTTON 1
static HWND window=(void*)1;static DWORD owner=7;static const char* className="UnityWndClass";static int swapped=0;
static HWND foreground(){return window;}static DWORD process(){return 7;}
static DWORD windowProcess(HWND,DWORD* p){*p=owner;return 1;}
static int name(HWND,char* p,int){std::strcpy(p,className);return std::strlen(p);}
static int metrics(int index){assert(index==23);return swapped;}
static auto pGetForegroundWindow=foreground;static auto p70_processId=process;static auto p70_windowProcess=windowProcess;static auto p70_className=name;static auto p70_metrics=metrics;
static bool streq(const char* a,const char* b){return !std::strcmp(a,b);}
"""
test=r"""
int main(){
 assert(p70_game_foreground()==window);owner=8;assert(!p70_game_foreground());owner=7;
 className="#32770";assert(!p70_game_foreground());className="UnityWndClass";
 window=nullptr;assert(!p70_game_foreground());window=(void*)1;
 p70_windowProcess=nullptr;assert(!p70_game_foreground());p70_windowProcess=windowProcess;
 assert(p70_primary_button()==1);swapped=1;assert(p70_primary_button()==2);
 puts("Input: other process, same-process dialog, no foreground, missing API rejected; swapped primary mouse respected.");
}
"""
work=pathlib.Path(sys.argv[2]).resolve();cpp=work/'input_focus.cpp';cpp.write_text(fixture+s[a:b]+test);exe=work/'input_focus.exe'
subprocess.run([sys.argv[1],'-O2','-static',str(cpp),'-o',str(exe)],check=True);subprocess.run([str(exe)],check=True,timeout=30)
