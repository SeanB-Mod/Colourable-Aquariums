import pathlib,subprocess,sys
root=pathlib.Path(__file__).parent
s=(root/'DefaultAquariumLight.h').read_text();s=s[s.index('static P55RGB p82_default_baseline('):]
fixture=r"""
#include <cassert>
#include <cstdio>
#include <cstring>
#include "LightingMath.h"
struct F4{float x,y,z,w;};using Il2CppObject=void;using Il2CppClass=void;using Il2CppType=void;
int definitions,light;F4 color{2,3,4,1};bool wrongType=false,missing=false,failGamma=false;
void* getter(const int*){return &definitions;}void* (*p82_definitions)(const int*)=getter;
int p82_offset=1,p82_gamma=1;
void* il2cpp_object_get_class(void* obj){return obj;}
void* p48_field_at_offset(void* cls,int offset,int){if(missing)return nullptr;return cls==&definitions&&offset==0x2f8?(void*)1:cls==&light&&offset==0x44?(void*)2:nullptr;}
void* il2cpp_field_get_value_object(void* field,void*){return field==(void*)1?(void*)&light:(void*)&color;}
void* il2cpp_field_get_type(void*){return (void*)1;}
char* il2cpp_type_get_name(const void*){return (char*)(wrongType?"System.Single":"UnityEngine.Color");}
void il2cpp_free(void*){}bool streq(const char* a,const char* b){return !strcmp(a,b);}
F4 read_boxed_f4(void* obj){return *(F4*)obj;}
void* invoke(int,void*,void** args){static float value;if(failGamma)return nullptr;value=*(float*)args[0]*2;return &value;}
float boxed_float(void* obj,float){return *(float*)obj;}
"""
test=r"""
bool equal(P55RGB a,P55RGB b){return a.r==b.r&&a.g==b.g&&a.b==b.b;}
int main(){P55RGB retail{1.79348f,.18001f,.61478f};auto normal=p82_default_baseline(retail);assert(equal(normal,{4,6,8}));assert(color.x==2&&color.y==3&&color.z==4);
wrongType=true;assert(equal(p82_default_baseline(retail),retail));wrongType=false;
missing=true;assert(equal(p82_default_baseline(retail),retail));missing=false;
failGamma=true;assert(equal(p82_default_baseline(retail),retail));failGamma=false;
color.x=-1;assert(equal(p82_default_baseline(retail),retail));color.x=2;
p82_definitions=nullptr;assert(equal(p82_default_baseline(retail),retail));
puts("Dredge baseline: runtime default color, conversion, unchanged game data, metadata/type/negative/missing conversion fallback passed.");}
"""
out=pathlib.Path(sys.argv[2]);out.mkdir(parents=True,exist_ok=True);cpp=out/'dredge_baseline.cpp';cpp.write_text(fixture+s+test);exe=out/'dredge_baseline.exe'
subprocess.run([sys.argv[1],'-O2','-I'+str(root.resolve()),str(cpp),'-o',str(exe)],check=True);subprocess.run([str(exe.resolve())],check=True)
