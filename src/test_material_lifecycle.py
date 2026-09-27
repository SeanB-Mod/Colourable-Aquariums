import pathlib,subprocess,sys
p=pathlib.Path(__file__).parent;s=(p/'AquariumColour_source.cpp').read_text()
a=s.index('static bool p72_alive(');b=s.index('static void p72_prune_glass',a)
x=s.index('static P48OptionalColour p48_from_picker(');y=s.index('static Il2CppFieldInfo*',x)
c=s.index('static bool p72_glass_bound(');d=s.index('static void p72_prune_glass(Il2CppArray* live){',c)
fixture=r'''#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdio>
#include "LightingMath.h"
using uptr=unsigned long long;using AquariumGCHandle=unsigned long long;using MethodInfo=int;
struct F4{float x,y,z,w;};struct P48OptionalColour{int present;float r,g,b,a;};
struct WaterTarget {void* renderer;void* originalMaterial;void* cloneMaterial;AquariumGCHandle cloneHandle,originalHandle,rendererHandle;bool active;F4 customColor;bool hasCustom;};
struct Il2CppArray{uptr max_length;void* vector[2];};
struct GlassTarget{void* renderer;void* material;F4 originalAlbedo;AquariumGCHandle materialHandle;void* ownerRenderer;bool active;};
static Il2CppArray materials;static const MethodInfo shared=5;static auto M_Renderer_get_sharedMaterials=&shared;
static int C_Object=0;static const MethodInfo alive=1,get=2,set=3,destroy=4;
static auto M_Renderer_get_sharedMaterial=&get;static auto M_Renderer_set_sharedMaterial=&set;
static void* bound;static bool rendererAlive=true,originalAlive=true,failSet=false;static int destroyed=0,freed=0;
static const MethodInfo* find_method_param1(int,const char* name,const char*){return !strcmp(name,"Destroy")?&destroy:&alive;}
static void* invoke(const MethodInfo* m,void* self,void** a){
 if(m==&alive)return ((a[0]==(void*)1&&!rendererAlive)||(a[0]==(void*)2&&!originalAlive))?nullptr:a[0];
 if(m==&shared)return &materials;
 if(m==&get)return bound;if(m==&set){if(!failSet)bound=a[0];return nullptr;}
 if(m==&destroy){assert(a[0]==(void*)3);assert(!rendererAlive||bound!=(void*)3);++destroyed;}return nullptr;
}
static bool boxed_bool(void* p){return p!=nullptr;}
static void release(AquariumGCHandle h){assert(h>0xffffffffull);++freed;}
static auto il2cpp_gchandle_free=&release;
static float clamp01_glow(float x){return x<0?0:x>1?1:x;}
'''
test=r'''
int main(){
 Il2CppArray live{1,{(void*)1}};GlassTarget gt{};gt.renderer=(void*)1;gt.material=(void*)3;materials={1,{(void*)3}};assert(p72_glass_bound(gt,&live));materials.vector[0]=(void*)4;assert(!p72_glass_bound(gt,&live));live.max_length=0;materials.vector[0]=(void*)3;assert(!p72_glass_bound(gt,&live));
 for(int i=0;i<1000;++i){WaterTarget wt{(void*)1,(void*)2,(void*)3,0x100000001ull,0x100000002ull,0x100000003ull};bound=(void*)3;p72_drop_water(wt);assert(!wt.renderer&&!wt.cloneHandle&&bound==(void*)2);p72_drop_water(wt);}
 assert(destroyed==1000&&freed==3000);
 WaterTarget wt{(void*)1,(void*)2,(void*)3,0x100000001ull,0x100000002ull,0x100000003ull};bound=(void*)3;failSet=true;p72_drop_water(wt);assert(wt.cloneHandle&&destroyed==1000&&freed==3000);
 failSet=false;rendererAlive=false;p72_drop_water(wt);assert(destroyed==1001&&freed==3003);
 P55RGB retail{5.91f,10.35f,18.89f};
 for(int r=0;r<=10;++r)for(int g=0;g<=10;++g)for(int b=0;b<=10;++b){
  auto full=p48_from_picker({r/10.f,g/10.f,b/10.f,0});auto half=p48_from_picker({r/20.f,g/20.f,b/20.f,0});
  auto c=p67_soft_glow(retail,{full.r,full.g,full.b});auto h=p67_soft_glow(retail,{half.r,half.g,half.b});
  float v=std::fmax(full.r,std::fmax(full.g,full.b));float ratio=std::fmin(1.f,.5f+v)/std::fmin(1.f,.5f+2.f*v);
  assert(std::fabs(p55_luminance(c)-p55_luminance(retail)*std::fmin(1.f,.5f+2.f*v))<.0001f);
  assert(std::fabs(h.r-c.r*ratio)<.0001f&&std::fabs(h.g-c.g*ratio)<.0001f&&std::fabs(h.b-c.b*ratio)<.0001f);
 }
 auto black=p48_from_picker({0,0,0,0});auto z=p67_soft_glow(retail,{black.r,black.g,black.b});assert(z.r>0&&z.g>0&&z.b>0);
 puts("Step 2: 1000 ownership cycles, failed rebind retention, destroyed renderer cleanup, full slot-to-glow brightness pipeline passed.");
}
'''
out=pathlib.Path(sys.argv[2]);cpp=out/'material_lifecycle.cpp';cpp.write_text(fixture+s[a:b]+s[c:d]+s[x:y]+test);exe=out/'material_lifecycle.exe'
subprocess.run([sys.argv[1],'-O2','-static','-I',str(p),str(cpp),'-o',str(exe)],check=True);subprocess.run([str(exe.resolve())],check=True)
