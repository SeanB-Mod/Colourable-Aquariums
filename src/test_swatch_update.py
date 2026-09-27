import pathlib,subprocess,sys
p=pathlib.Path(__file__).parent;s=(p/'AquariumSwatches.h').read_text();a=s.index('static void p80_update_selection(){');b=s.index('static void p67_open_swatches(',a)
fixture=r"""
#include <cassert>
static bool p80_externalPalette=false;
#include <cmath>
#include <cstdio>
struct F4{float x,y,z,w;};using Il2CppObject=void;using uptr=unsigned long long;
struct Il2CppArray{uptr max_length;void* vector[2];};
static Il2CppArray buttons{2,{(void*)10,(void*)11}};
static void* p56_menu=(void*)1,*p80_palette=(void*)2,*p67_swatchWidget=(void*)3;
static int p80_selected=-1,p67_swatchCount=2,writes=0,marks=0,paints=0,refreshes=0,states=0;static bool live=true,matches=true,dirtyOK=true,writeOK=true;
static F4 p67_swatchColors[2]={{.1f,.2f,.3f,1},{.6f,.7f,.8f,1}},stored[2]={{.1f,.2f,.3f,1},{.6f,.7f,.8f,1}},g_currentWaterColor{.1f,.2f,.3f,0},painted{};
static bool p80_same_rgb(F4 a,F4 b){return fabs(a.x-b.x)<.00001&&fabs(a.y-b.y)<.00001&&fabs(a.z-b.z)<.00001;}
static bool p56_target_live(){return live;}static bool p80_palette_matches(){return matches;}static void p80_refresh_swatches(){++refreshes;}
static void p80_button_states(){++states;}static void log_raw(const char*){}
static void* p80_metagame(){return (void*)4;}static void* il2cpp_object_get_class(void*o){return o;}
static void* find_field_hierarchy(void*,const char*){return dirtyOK?(void*)5:nullptr;}
static void mark(void*,void*,void*a){assert(*(bool*)a);++marks;}static auto il2cpp_field_set_value=mark;
static void* find_method_name_params(void*,const char*,int){return (void*)6;}
static bool p56_call(void*,void*,void**a){if(!writeOK)return false;int i=*(int*)a[0];assert(i>=0&&i<2);stored[i]=*(F4*)a[1];++writes;return true;}
static void* field_object(void*o,const char*){return o==p67_swatchWidget?(void*)&buttons:(void*)12;}
static bool p56_alive(void*o){return o!=nullptr;}static void* find_method1_hierarchy(void*,const char*,const char*){return (void*)7;}
static void invoke(void*,void*,void**a){painted=*(F4*)a[0];++paints;}
"""
test=r"""
int main(){
 p80_externalPalette=true;p80_selected=1;p80_update_selection();assert(writes==0);p80_externalPalette=false;p80_selected=-1;
 p80_update_selection();assert(writes==0);p80_selected=1;g_currentWaterColor={.6f,.7f,.8f,0};p80_update_selection();assert(writes==0);
 for(int i=1;i<=1000;++i){float v=i/1001.f;g_currentWaterColor={v*.6f,v*.7f,v*.8f,0};p80_update_selection();assert(p80_selected==1&&p80_same_rgb(stored[1],g_currentWaterColor)&&p80_same_rgb(painted,g_currentWaterColor));}
 assert(writes==1000&&marks==1000&&paints==1000&&refreshes==0&&states==0&&stored[1].w==1&&stored[0].x==.1f);
 p80_update_selection();assert(writes==1000);matches=false;p80_update_selection();assert(p80_selected==-1&&refreshes==1&&writes==1000);matches=true;
 p80_selected=0;dirtyOK=false;p80_update_selection();assert(writes==1000&&p80_selected==-1);dirtyOK=true;
 p80_selected=0;writeOK=false;p80_update_selection();assert(writes==1000&&p80_selected==-1);writeOK=true;
 p80_selected=0;live=false;p80_update_selection();assert(writes==1000);live=true;p56_menu=nullptr;p80_update_selection();assert(writes==1000);
 puts("Selected swatch: 1000 live edits preserve selection, update only selected entry/tile, mark saves, retain opaque alpha, skip unchanged values; stale/failed/closed cases do not write.");
}
"""
out=pathlib.Path(sys.argv[2]);cpp=out/'swatch_update.cpp';cpp.write_text(fixture+s[a:b]+test);exe=out/'swatch_update.exe';subprocess.run([sys.argv[1],'-O2','-static',str(cpp),'-o',str(exe)],check=True);subprocess.run([str(exe.resolve())],check=True)
