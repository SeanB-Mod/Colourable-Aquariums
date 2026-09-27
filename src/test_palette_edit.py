import pathlib,subprocess,sys
p=pathlib.Path(__file__).parent;s=(p/'AquariumSwatches.h').read_text();a=s.index('static bool p80_edit_press(');b=s.index('static bool p67_swatch_press(',a)
fixture=r"""
#include <cassert>
static bool p80_externalPalette=false;
#include <cstdio>
struct F4{float x,y,z,w;};using Il2CppObject=void;
static void* p56_menu=nullptr;static void* p80_add=(void*)1,*p80_delete=(void*)2,*p80_palette=(void*)3;
static int p80_selected=-1,p67_swatchCount=0,p80_capacity=3,count=0,mutations=0,marks=0,refreshes=0;static bool live=true,matches=true,dirtyAvailable=true,callWorks=true;
static F4 g_currentWaterColor{.1f,.2f,.3f,0};static F4 added{};static int removed=-1;
static bool p56_target_live(){return live;}static bool p80_palette_matches(){return matches;}
static void p80_refresh_swatches(){++refreshes;p67_swatchCount=count;}
static void* p80_metagame(){return (void*)4;}static void* il2cpp_object_get_class(void*o){return o;}
static void* find_field_hierarchy(void*,const char*){return dirtyAvailable?(void*)5:nullptr;}
static void mark(void*,void*,void* b){assert(*(bool*)b);++marks;}static auto il2cpp_field_set_value=mark;
static void* find_method_name_params(void*,const char*n,int){return n[0]=='A'?(void*)6:(void*)7;}
static bool p56_call(void*m,void*o,void**a){assert(o==p80_palette);if(!callWorks)return false;++mutations;if(m==(void*)6){added=*(F4*)a[0];++count;}else{removed=*(int*)a[0];--count;}return true;}
static int collection_count(void*){return count;}static void log_raw(const char*){}
"""
test=r"""
int main(){
 p80_externalPalette=true;p56_menu=(void*)99;assert(p80_edit_press(p80_add)&&mutations==0);p80_externalPalette=false;p56_menu=nullptr;
 assert(!p80_edit_press(p80_add));p56_menu=(void*)8;assert(!p80_edit_press((void*)99));
 assert(p80_edit_press(p80_delete)&&mutations==0);live=false;p80_edit_press(p80_add);assert(mutations==0);live=true;
 matches=false;p80_edit_press(p80_add);assert(mutations==0&&p80_selected==-1);matches=true;
 dirtyAvailable=false;p80_edit_press(p80_add);assert(mutations==0);dirtyAvailable=true;
 callWorks=false;p80_edit_press(p80_add);assert(mutations==0&&marks==0);callWorks=true;
 p80_edit_press(p80_add);assert(count==1&&mutations==1&&marks==1&&p80_selected==0);assert(added.x==.1f&&added.w==1);
 p80_edit_press(p80_add);p80_edit_press(p80_add);assert(count==3&&p80_selected==2);p80_edit_press(p80_add);assert(count==3&&mutations==3);
 p80_edit_press(p80_delete);assert(count==2&&removed==2&&p80_selected==-1&&marks==4);p80_edit_press(p80_delete);assert(mutations==4);
 p80_selected=0;p80_edit_press(p80_delete);assert(removed==0&&count==1);assert(g_currentWaterColor.x==.1f&&g_currentWaterColor.w==0);
 puts("Palette edit: empty/add/select/delete/full, disabled delete, stale palette, invalid target, unavailable save marker, failure and shared-save marking passed.");
}
"""
out=pathlib.Path(sys.argv[2]);cpp=out/'palette_edit.cpp';cpp.write_text(fixture+s[a:b]+test);exe=out/'palette_edit.exe';subprocess.run([sys.argv[1],'-O2','-static',str(cpp),'-o',str(exe)],check=True);subprocess.run([str(exe.resolve())],check=True)
