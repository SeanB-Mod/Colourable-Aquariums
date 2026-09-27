import pathlib,subprocess,sys
root=pathlib.Path(__file__).parent
s=(root/'AquariumColour_source.cpp').read_text()
start=s.index('static bool p65_glowPending=');end=s.index('static void update_active_water_color(F4 color){',start)
fixture=r'''
#include <cassert>
#include <cstdio>
struct F4{float x,y,z,w;};
static int applied=0;static F4 actual{};
static int p49_sync_glow_for_current(F4 c,bool){actual=c;++applied;return 0;}
'''
test=r'''
int main(){
 p65_flush_glow();assert(applied==0);
 for(int i=0;i<100;++i){p65_pendingColor={(float)i,1,2,0};p65_glowPending=true;}
 p65_flush_glow();assert(applied==1&&actual.x==99&&!p65_glowPending);
 p65_flush_glow();assert(applied==1);
 p65_pendingColor={42,0,0,0};p65_glowPending=true;p65_glowPending=false;
 p65_flush_glow();assert(applied==1);
 std::puts("Glow queue passed: latest value wins, final flush once, empty and cancelled queue make no writes.");
}
'''
work=pathlib.Path(sys.argv[2]);work.mkdir(parents=True,exist_ok=True)
cpp=work/'glow_queue.cpp';cpp.write_text(fixture+s[start:end]+test);exe=work/'glow_queue.exe'
subprocess.run([sys.argv[1],'-O2',str(cpp),'-o',str(exe)],check=True);subprocess.run([str(exe.resolve())],check=True)
