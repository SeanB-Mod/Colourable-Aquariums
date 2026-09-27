"""Exercise actual donor lookup with sibling inspectors and prefab exclusions."""
import pathlib,subprocess,sys
root=pathlib.Path(__file__).parent
s=root.joinpath('AquariumUI.h').read_text();start=s.index('static void* p56_item_inspector(){')
end=s.index('{',start)+1;depth=1
while depth:
    if s[end]=='{':depth+=1
    elif s[end]=='}':depth-=1
    end+=1
fixture=r'''
#include <cstdio>
#include <cstring>
#include <cstdlib>
#define psprintf std::sprintf
using uptr=unsigned long long;using Il2CppObject=void;
struct Node {bool alive=true,scene=false;void* root=nullptr;Node* button=nullptr;Node* menu=nullptr;const char* type="InspectorMenuEnhancerItem";};
struct Il2CppArray{uptr max_length;void* vector[3];};
Node own,enhancer,prefab,button,menu;int hud,assetRoot;
Il2CppArray objects{2,{&prefab,&enhancer}},empty{0,{}};
void* p56_inspector=&own;unsigned p57_diagnostics=0;void* M_Resources_FindAll=(void*)1;
void* find_class(const char*,const char* name){return std::strcmp(name,"InspectorMenu")==0?(void*)2:(void*)3;}
void* il2cpp_class_get_type(void* p){return p;}
void* il2cpp_type_get_object(void* p){return p;}
void* C_Transform=(void*)4;
void* find_method0_hierarchy(void*,const char*){return (void*)5;}
void* invoke(void* method,void* obj,void** args=nullptr){if(method==M_Resources_FindAll)return args[0]==(void*)2?&objects:&empty;return ((Node*)obj)->root;}
bool p56_alive(void* p){return p&&((Node*)p)->alive;}
void* p56_tr(void* p){return p;}
void* field_object(void* p,const char* field){return std::strcmp(field,"_buttonColorPicker")==0?((Node*)p)->button:((Node*)p)->menu;}
bool p57_scene_object(void* p){return ((Node*)p)->scene;}
void* il2cpp_object_get_class(void* p){return (void*)((Node*)p)->type;}
void class_full(void* p,char* out,unsigned long long cap){std::snprintf(out,cap,"%s",(const char*)p);}
void log_raw(const char*){}
void* p59_load_stock_inspector(){return nullptr;}
void check(bool ok){if(!ok)std::abort();}
'''
tests=r'''
int main(){
 own.root=&hud;enhancer.root=&hud;prefab.root=&assetRoot;
 prefab.button=enhancer.button=&button;prefab.menu=enhancer.menu=&menu;
 check(p56_item_inspector()==&enhancer); // no InspectorMenuItem instance exists
 enhancer.root=&assetRoot;check(p56_item_inspector()==nullptr); // prefab-only candidates
 enhancer.scene=true;check(p56_item_inspector()==&enhancer); // separate valid scene root
 button.alive=false;check(p56_item_inspector()==nullptr);
 button.alive=true;enhancer.menu=nullptr;check(p56_item_inspector()==nullptr);
 enhancer.menu=&menu;enhancer.alive=false;check(p56_item_inspector()==nullptr);
 std::puts("Donor regression passed: enhancer sibling, shared HUD, distinct scene, prefab exclusion, destroyed/missing references.");
}
'''
work=pathlib.Path(sys.argv[2]);work.mkdir(parents=True,exist_ok=True)
cpp=work/'donor.cpp';cpp.write_text(fixture+s[start:end]+tests);exe=work/'donor.exe'
subprocess.run([sys.argv[1],'-O2',str(cpp),'-o',str(exe)],check=True)
subprocess.run([str(exe.resolve())],check=True)
