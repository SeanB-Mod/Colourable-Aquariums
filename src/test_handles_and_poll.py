import pathlib,re,subprocess,sys
root=pathlib.Path(__file__).parent
s=(root/'AquariumUI.h').read_text();main=(root/'AquariumColour_source.cpp').read_text()
def extract(marker):
 start=s.index(marker);end=s.index('{',start)+1;depth=1
 while depth:
  depth+=(s[end]=='{')-(s[end]=='}');end+=1
 return s[start:end]
types=main[main.index('using AquariumGCHandle='):main.index('typedef void (*il2cpp_gchandle_free_t)')]
types+=re.search(r'typedef void \(\*il2cpp_gchandle_free_t\).*?;',main)[0]
fixture=r'''
#include <cassert>
#include <cstdio>
using uptr=unsigned long long;using u64=unsigned long long;using Il2CppObject=void;
'''+types+r'''
static AquariumGCHandle nextHandle=0x0000028407af2e50ull,freed[64];static int freeCount=0;
static AquariumGCHandle mockNew(void*,bool){return nextHandle++;}
static void mockFree(AquariumGCHandle h){freed[freeCount++]=h;}
static il2cpp_gchandle_new_t il2cpp_gchandle_new=mockNew;
static il2cpp_gchandle_free_t il2cpp_gchandle_free=mockFree;
static AquariumGCHandle p56_holds[32]{};static int p56_holdCount=0;
static void log_raw(const char*){}
static void* p56_state=(void*)1,*p56_inspector=(void*)2,*p56_room=(void*)3,*p56_water=(void*)4;
static bool alive=true,selection=true,live=true,opened=true,active=true;
static bool p56_opened(void*){return opened;}
static bool p56_active(void*){return active;}static int scans=0;
static bool p56_alive(void*){return alive;}
static void* field_object(void*,const char*){return selection?p56_room:nullptr;}
static bool p56_target_live(){++scans;return live;}
'''
tests=r'''
int main(){
 for(int i=0;i<32;++i)p56_hold((void*)1);
 p56_release();assert(freeCount==32&&p56_holdCount==0);
 for(int i=0;i<32;++i)assert(freed[i]==0x0000028407af2e50ull+i);
 p56_release();assert(freeCount==32);
 assert(p64_tick_target_live(1000)&&scans==1);
 for(int i=1001;i<1250;++i)assert(p64_tick_target_live(i));assert(scans==1);
 opened=false;assert(!p64_tick_target_live(1098));opened=true;
 active=false;assert(!p64_tick_target_live(1099));active=true;
 selection=false;assert(!p64_tick_target_live(1100));selection=true;
 alive=false;assert(!p64_tick_target_live(1101));alive=true;
 live=false;assert(!p64_tick_target_live(1250)&&scans==2);
 p56_state=(void*)5;live=true;assert(p64_tick_target_live(1251)&&scans==3);
 std::puts("Passed: 32 handles above 4GB retained and freed exactly; selection/destruction checks immediate, state scan bounded to 250ms.");
}
'''
work=pathlib.Path(sys.argv[2]);work.mkdir(parents=True,exist_ok=True)
cpp=work/'handles_poll.cpp';cpp.write_text(fixture+extract('static void p56_hold(')+extract('static void p56_release(')+extract('static bool p64_tick_target_live(')+tests)
exe=work/'handles_poll.exe';subprocess.run([sys.argv[1],'-O2',str(cpp),'-o',str(exe)],check=True);subprocess.run([str(exe.resolve())],check=True)
