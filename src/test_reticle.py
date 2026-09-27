import pathlib,subprocess,sys
p=pathlib.Path(__file__).parent;s=(p/'AquariumUI.h').read_text();a=s.index('static void p80_reticle(');b=s.index('static bool p62_clear_click_guard',a)
fixture=r"""
#include <cassert>
#include <cstdio>
#include <cstring>
struct F4{float x,y,z,w;};using Il2CppObject=void;
static void* p56_menu=(void*)1,*g_pickerWidget=(void*)2;static float p56_lastHue=-1,p56_lastSat=-1,g_manualHue=.2f,g_manualSat=.3f,g_manualVal=.4f;
static bool alive=true;static int paints=0,gradients=0,selections=0;static F4 imageColor{};static float valueField=-1;
static bool p56_alive(void*){return alive;}static void* field_object(void*,const char*){return (void*)3;}static void* il2cpp_object_get_class(void*o){return o;}
static void* find_method1_hierarchy(void*,const char*,const char*){return (void*)4;}static void* find_method0_hierarchy(void*,const char*){return (void*)5;}
static void invoke(void*m,void*,void**a=nullptr){if(m==(void*)4){imageColor=*(F4*)a[0];++paints;}else ++gradients;}
static F4 hsv_to_rgb(float h,float s,float v){return {h*v,s*v,v,0};}
static void set_field_float(void*,const char*n,float v){if(!strcmp(n,"_value"))valueField=v;}
static void p80_update_selection(){++selections;}
"""
test=r"""
int main(){imageColor={1,0,0,1};p56_gradient();assert(paints==1&&gradients==1&&imageColor.z==.4f&&imageColor.w==1&&valueField==.4f);
 p56_gradient();assert(paints==1);g_manualVal=.1f;p56_gradient();assert(paints==2&&gradients==1&&imageColor.z==.1f&&valueField==.1f);
 g_manualHue=.8f;p56_gradient();assert(paints==3&&gradients==2);p80_reticle(g_pickerWidget,{.7f,.8f,.9f,0});assert(imageColor.z==.9f&&imageColor.w==1);
 p56_menu=nullptr;g_manualVal=.5f;p56_gradient();assert(paints==4);puts("Reticle: stale base-item colour replaced on open, brightness-only edits repaint, unchanged values skip work, restore uses opaque colour, stock picker isolated.");}
"""
out=pathlib.Path(sys.argv[2]);cpp=out/'reticle.cpp';cpp.write_text(fixture+s[a:b]+test);exe=out/'reticle.exe';subprocess.run([sys.argv[1],'-O2','-static',str(cpp),'-o',str(exe)],check=True);subprocess.run([str(exe.resolve())],check=True)
