import pathlib,subprocess,sys
p=pathlib.Path(__file__).parent;s=(p/'AquariumColour_source.cpp').read_text()
a=s.index('struct StateList{');b=s.index('static int g_target',a)
x=s.index('struct P53FloorSlot {');y=s.index('static bool g_p55ConsumerReady',x)
z=s.index('static bool p54_slot_write_begin');e=s.index('static u64 g_p54SnapshotBusySkips',z)
fixture=r"""
#include <cassert>
#include <cstring>
#include <cstdio>
using DWORD=unsigned long;using usize=unsigned long long;using u64=unsigned long long;using u32=unsigned;using Il2CppObject=void;using MethodInfo=void;
static bool streq(const char*a,const char*b){return !strcmp(a,b);}static usize slen(const char*a){return strlen(a);}static void scopy(char*d,usize n,const char*s){snprintf(d,n,"%s",s);}
#include "RuntimeWork.h"
static int version=1,reads=0;static void* dict=(void*)2;static void* entries=(void*)3;static void* state=(void*)100;
static void* get_aquarium_manager(){return (void*)1;}static int collection_count(void*){return 2;}
static void* il2cpp_object_get_class(void* o){return o;}
static void* find_method0_hierarchy(void*,const char*){return nullptr;}
static void* find_method1_hierarchy(void*,const char*,const char*){return (void*)5;}
static int field_i32(void*,const char* n,int def){return !strcmp(n,"_version")?version:2;}
static void* field_object(void* o,const char* name){if(!strcmp(name,"_aquariumStates"))return dict;if(!strcmp(name,"_entries"))return entries;if(!strcmp(name,"value"))return state;return nullptr;}
static void* invoke(const void*,void*,void** a){++reads;return (void*)10;}
struct P48OptionalColour{unsigned char present;float r,g,b,a;};
"""
test=r"""
int main(){
 {P78WorkScope scope;for(int i=0;i<1000;++i){auto s=get_states();assert(s.count==2&&s.states[0]==state);}assert(reads==2&&p78_stateHits==999);
  state=(void*)200;++version;assert(get_states().states[0]==state&&reads==4);
  dict=(void*)20;get_states();assert(reads==6);entries=(void*)30;get_states();assert(reads==8);
  version=-1;get_states();get_states();assert(reads==12);
 }
 version=5;{P78WorkScope scope;get_states();assert(reads==14);}get_states();get_states();assert(reads==18);
 P53FloorSlot& slot=g_p53FloorSlots[0];slot.active=true;slot.seq=1;assert(!p54_slot_write_begin(slot));p54_clear_slot(slot);assert(slot.active&&slot.seq==1);
 P54FloorSlotSnapshot snap{};assert(!p54_snapshot_slot(0,snap));slot.seq=2;assert(p54_snapshot_slot(0,snap)&&snap.active);p54_clear_slot(slot);assert(!slot.active&&slot.seq==4);
 assert(!p54_snapshot_slot(-1,snap)&&!p54_snapshot_slot(16,snap));puts("State cache: 1000 requests / one scan, version/dictionary/entries/batch invalidation, unknown version bypass; bounded slot contention passed.");
}
"""
out=pathlib.Path(sys.argv[2]);cpp=out/'state_cache.cpp';cpp.write_text(fixture+s[a:b]+s[x:y]+s[z:e]+test);exe=out/'state_cache.exe'
subprocess.run([sys.argv[1],'-O2','-static','-I',str(p),str(cpp),'-o',str(exe)],check=True);subprocess.run([str(exe.resolve())],check=True)
