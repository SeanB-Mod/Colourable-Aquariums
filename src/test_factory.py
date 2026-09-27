"""Exercise the actual factory adapter, including its bounded retry guard."""
import pathlib,subprocess,sys
root=pathlib.Path(__file__).parent
s=root.joinpath('AquariumUI.h').read_text();start=s.index('static void* p59_load_stock_inspector(){')
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
struct Node{bool alive=true,active=true;int id=1;};
Node manager,inspector,button,menu,asset,dict;
struct Il2CppArray{uptr max_length;void* vector[1];};Il2CppArray objects{1,{&manager}};
void* M_Resources_FindAll=(void*)1;bool ready=false,hasAsset=true,raiseError=false;int calls=0;
void* find_class(const char*,const char*){return (void*)2;}
void* find_method_name_params(void*,const char*,int){return (void*)3;}
void* il2cpp_class_get_type(void* p){return p;}
void* il2cpp_type_get_object(void* p){return p;}
void* invoke(void*,void*,void**){return &objects;}
bool p56_alive(void* p){return p&&((Node*)p)->alive;}
bool p56_active(void* p){return p&&((Node*)p)->active;}
void* field_object(void*,const char* field){
 if(std::strcmp(field,"_dynamicInspectors")==0)return ready?&dict:nullptr;
 if(std::strcmp(field,"_inspectorMenuEnhancerItemRef")==0)return hasAsset?&asset:nullptr;
 if(std::strcmp(field,"_buttonColorPicker")==0)return &button;
 return &menu;
}
int object_id(void* p){return ((Node*)p)->id;}
void* il2cpp_runtime_invoke(void*,void* obj,void** args,void** exception){
 if(obj!=&manager||args[0]!=&asset)std::abort();++calls;
 if(raiseError){*exception=&asset;return nullptr;}return &inspector;
}
void log_raw(const char*){}
void check(bool ok){if(!ok)std::abort();}
'''
tests=r'''
int main(){
 check(p59_load_stock_inspector()==nullptr&&calls==0); // wait for Awake
 ready=true;hasAsset=false;check(p59_load_stock_inspector()==nullptr&&calls==0);
 hasAsset=true;manager.active=false;check(p59_load_stock_inspector()==nullptr&&calls==0);
 manager.active=true;check(p59_load_stock_inspector()==&inspector&&calls==1);
 for(int i=0;i<1000;++i)check(p59_load_stock_inspector()==nullptr);check(calls==1);
 ++manager.id;raiseError=true;check(p59_load_stock_inspector()==nullptr&&calls==2);
 check(p59_load_stock_inspector()==nullptr&&calls==2); // no exception loop
 ++manager.id;raiseError=false;button.alive=false;check(p59_load_stock_inspector()==nullptr&&calls==3);
 std::puts("Factory regression passed: delayed initialization, absent asset, inactive manager, native asset argument, 1000 bounded retries, exception and invalid result.");
}
'''
work=pathlib.Path(sys.argv[2]);work.mkdir(parents=True,exist_ok=True)
cpp=work/'factory.cpp';cpp.write_text(fixture+s[start:end]+tests);exe=work/'factory.exe'
subprocess.run([sys.argv[1],'-O2',str(cpp),'-o',str(exe)],check=True)
subprocess.run([str(exe.resolve())],check=True)
