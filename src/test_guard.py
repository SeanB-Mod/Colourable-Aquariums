import pathlib,subprocess,sys
root=pathlib.Path(__file__).parent
s=(root/'AquariumUI.h').read_text()
start=s.index('static bool p62_clear_click_guard(void* button){');end=s.index('\nstatic void p56_add_button',start)
fixture=r'''
#include <cassert>
#include <cstdio>
using Il2CppObject=void;
static bool setterExists=true,getterExists=true,setSucceeds=true,raiseError=false,stale=false;
static int sets=0,getCalls=0;
static void* il2cpp_object_get_class(void*){return (void*)1;}
static void* find_class(const char*,const char*){return (void*)1;}
static void* find_method1_hierarchy(void*,const char*,const char*){return setterExists?(void*)2:nullptr;}
static void* find_method_name_params(void*,const char*,int){return setterExists?(void*)2:nullptr;}
static void* find_method0_hierarchy(void*,const char*){return getterExists?(void*)3:nullptr;}
static bool p56_call(void*,void*,void** args){assert(args&&args[0]==nullptr);++sets;return setSucceeds;}
static void* il2cpp_runtime_invoke(void*,void*,void**,void** ex){++getCalls;*ex=raiseError?(void*)4:nullptr;return stale?(void*)5:nullptr;}
static void log_raw(const char*){}
static void p57_once(unsigned,const char*){}
'''
test=r'''
int main(){
 assert(p62_clear_click_guard((void*)1)&&sets==1&&getCalls==1);
 setterExists=false;assert(!p62_clear_click_guard((void*)1)&&sets==1);setterExists=true;
 getterExists=false;assert(!p62_clear_click_guard((void*)1)&&sets==1);getterExists=true;
 setSucceeds=false;assert(!p62_clear_click_guard((void*)1)&&getCalls==1);setSucceeds=true;
 raiseError=true;assert(!p62_clear_click_guard((void*)1));raiseError=false;
 stale=true;assert(!p62_clear_click_guard((void*)1));
 std::puts("Click guard passed: null argument, getter readback, missing API, setter failure, exception and stale callback rejection.");
}
'''
work=pathlib.Path(sys.argv[2]);work.mkdir(parents=True,exist_ok=True)
cpp=work/'guard.cpp';cpp.write_text(fixture+s[start:end]+test);exe=work/'guard.exe'
subprocess.run([sys.argv[1],'-O2',str(cpp),'-o',str(exe)],check=True)
subprocess.run([str(exe.resolve())],check=True)
