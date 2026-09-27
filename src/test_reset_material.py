import pathlib,subprocess,sys
root=pathlib.Path(__file__).parent
s=(root/'AquariumColour_source.cpp').read_text();start=s.index('static void* find_canonical_aquarium_material(');end=s.index('\nstatic void forget_glass_owner',start)
fixture=r'''
#include <cassert>
#include <cstring>
#include <cstdio>
using uptr=unsigned long long;using Il2CppType=void;using Il2CppObject=void;
struct Material{const char* name;const char* shader;};
struct Il2CppArray{uptr max_length;void* vector[4];};
static int C_Material=1,M_Resources_FindAll=1,M_Material_get_shader=2;
static Il2CppArray materials;
static const Il2CppType* il2cpp_class_get_type(int){return (void*)1;}
static void* il2cpp_type_get_object(const void*){return (void*)1;}
static void* invoke(int method,void* self,void**){return method==1?(void*)&materials:(void*)((Material*)self)->shader;}
static bool streq(const char* a,const char* b){return !strcmp(a,b);}
static bool contains_i(const char* a,const char* b){return strstr(a,b)!=nullptr;}
static void object_name(void* p,char* out,int size){
 for(uptr i=0;i<materials.max_length;++i)if(p==materials.vector[i]){snprintf(out,size,"%s",((Material*)p)->name);return;}
 snprintf(out,size,"%s",(const char*)p);
}
'''
test=r'''
int main(){
 Material clone{"Water(Clone)(Clone)","WaterShader"},instance{"Water(Instance)","WaterShader"},other{"Other","GlassShader"},retail{"Water","WaterShader"};
 materials={4,{&clone,&instance,&other,&retail}};
 assert(find_canonical_aquarium_material("WaterShader")==&retail);
 materials.max_length=3;assert(!find_canonical_aquarium_material("WaterShader"));
 assert(find_canonical_aquarium_material("GlassShader")==&other);
 puts("Reset material selection: tinted clones and instances excluded; absent retail material fails closed.");
}
'''
work=pathlib.Path(sys.argv[2]);cpp=work/'reset_material.cpp';cpp.write_text(fixture+s[start:end]+test);exe=work/'reset_material.exe'
subprocess.run([sys.argv[1],'-O2',str(cpp),'-o',str(exe)],check=True);subprocess.run([str(exe.resolve())],check=True)
