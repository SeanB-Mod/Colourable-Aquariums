import pathlib,subprocess,sys
root=pathlib.Path(__file__).parent
s=(root/'LightingConsumer.h').read_text();a=s.index('static bool p55_live_pair(');b=s.index('static F3* p55_linear_hook(',a)
fixture=r"""
#include <cassert>
#include <cstdio>
using u64=unsigned long long;
struct State{void* floor;void* tint;};State states[2]{{(void*)1,nullptr},{(void*)2,(void*)7}};
struct StateList{int count;void* states[2];};StateList live{2,{&states[0],&states[1]}};
struct P54FloorSlotSnapshot{bool active;void* state;void* floor;int desired;void* water=nullptr;};
P54FloorSlotSnapshot slots[16]{},g_p55Notified[16]{};void* g_p80NotifiedFishTint[16]{};
u64 g_p55Refreshes=0,g_p55RefreshFailures=0;bool enabled=true,fail=false;int dirty[2]{};
bool p54_state_in_list(const StateList& sl,void* st){for(int i=0;i<sl.count;++i)if(sl.states[i]==st)return true;return false;}
bool p48_floor_from_state(void* st,void*& room,void*& floor){assert(p54_state_in_list(live,st));room=st;floor=((State*)st)->floor;return true;}
void* field_object(void* st,const char*){assert(p54_state_in_list(live,st));return ((State*)st)->tint;}
bool p48_payload_equal(int a,int b){return a==b;}
bool p48_mark_dirty(void* floor,bool){++dirty[floor==(void*)1?0:1];return !fail;}
void p81_probe_water(void*,void*,void*){}
bool p55_consumer_ready(){return true;}bool p55_glow_on(){return enabled;}
StateList get_states(){return live;}
bool p54_snapshot_slot(int i,P54FloorSlotSnapshot& out){out=slots[i];return true;}
"""
test=r"""
int main(){
slots[0]={true,&states[0],(void*)1,42};slots[1]={true,&states[1],(void*)2,99};
p55_dispatch_refreshes();assert(dirty[0]==1&&dirty[1]==1);
for(int i=0;i<1000;++i)p55_dispatch_refreshes();assert(dirty[0]==1&&dirty[1]==1);
// Add Dredge fish without replacing AquariumState: selected colour survives.
states[0].tint=(void*)9;p55_dispatch_refreshes();assert(dirty[0]==2&&dirty[1]==1&&g_p55Notified[0].desired==42);
assert(p55_live_pair(live,slots[0],true));
// Remove fish: same custom hue, one refresh; neighbour is untouched.
states[0].tint=nullptr;p55_dispatch_refreshes();assert(dirty[0]==3&&dirty[1]==1&&g_p55Notified[0].desired==42);
for(int i=0;i<1000;++i)p55_dispatch_refreshes();assert(dirty[0]==3);
// Failed notification retains old marker and retries, then settles.
states[0].tint=(void*)10;fail=true;p55_dispatch_refreshes();assert(g_p80NotifiedFishTint[0]==nullptr);
fail=false;p55_dispatch_refreshes();assert(g_p80NotifiedFishTint[0]==(void*)10);
// Destroyed state: do not inspect cached pointer.
live.states[0]=nullptr;p55_dispatch_refreshes();assert(!g_p55Notified[0].active);
puts("Glow transitions: standard/Dredge add/remove, retained custom hue, isolated neighbour, idle no-op, retry and stale state passed.");
}
"""
out=pathlib.Path(sys.argv[2]);out.mkdir(parents=True,exist_ok=True);cpp=out/'glow_transition.cpp';cpp.write_text(fixture+s[a:b]+test);exe=out/'glow_transition.exe'
subprocess.run([sys.argv[1],'-O2',str(cpp),'-o',str(exe)],check=True);subprocess.run([str(exe.resolve())],check=True)
