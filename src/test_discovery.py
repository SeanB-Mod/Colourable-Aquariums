"""Compile and exercise the actual periodic reconciliation function with a delayed UI fixture."""
import pathlib,subprocess,sys
root=pathlib.Path(__file__).parent
source=root.joinpath('AquariumUI.h').read_text()
start=source.index('static void p57_reconcile(){')
pos=source.index('{',start);depth=1;end=pos+1
while depth:
    if source[end]=='{':depth+=1
    elif source[end]=='}':depth-=1
    end+=1
work=pathlib.Path(sys.argv[2]);work.mkdir(parents=True,exist_ok=True)
fixture=r'''
#include <cstdio>
#include <cstdlib>
using uptr=unsigned long long;
struct Node {bool alive=true,active=false,open=false,aquarium=false;};
struct Il2CppArray{uptr max_length;void* vector[2];};
Node first,second,button;Il2CppArray objects{2,{&first,&second}};
bool p56_ready=false,donorReady=false;void *p56_menu=nullptr,*p56_inspector=nullptr,*p56_button=nullptr;
void* M_Resources_FindAll=(void*)1;int refreshes=0,creates=0,visibleCount=0;
void* find_class(const char*,const char*){return (void*)1;}
void* il2cpp_class_get_type(void* p){return p;}
void* il2cpp_type_get_object(void* p){return p;}
void* invoke(void*,void*,void**){return &objects;}
bool p56_alive(void* p){return p&&((Node*)p)->alive;}
bool p56_active(void* p){return p&&((Node*)p)->active;}
bool p56_opened(void* p){return p&&((Node*)p)->open;}
void p57_once(unsigned,const char*){}
void* field_object(void* p,const char*){return p;}
void* p56_live_state(void* p){return p&&((Node*)p)->aquarium?p:nullptr;}
void p56_originalRefresh(void*,void*){++refreshes;visibleCount=0;}
void p56_add_button(void* p){if(!donorReady)return;p56_inspector=p;p56_button=&button;button.active=true;++creates;++visibleCount;}
void check(bool x){if(!x)std::abort();}
'''
tests=r'''
int main(){
 p57_reconcile();check(refreshes==0);
 p56_ready=true;p57_reconcile();check(refreshes==0);
 first.active=first.open=true;p57_reconcile();check(refreshes==0);
 first.aquarium=true;p57_reconcile();check(refreshes==1&&creates==0);
 donorReady=true;p57_reconcile();check(creates==1&&visibleCount==1);
 for(int i=0;i<1000;++i)p57_reconcile();check(creates==1&&visibleCount==1);
 button.active=false;p57_reconcile();check(creates==2&&visibleCount==1);
 p56_menu=&first;button.active=false;p57_reconcile();check(creates==2);
 p56_menu=nullptr;first.open=false;second.active=second.open=second.aquarium=true;
 p57_reconcile();check(p56_inspector==&second&&visibleCount==1&&creates==3);
 std::puts("Discovery regression passed: delayed room/donor, 1000 duplicate-free retries, hidden-button recovery, editor exclusion, inspector replacement.");
}
'''
cpp=work/'discovery.cpp';cpp.write_text(fixture+source[start:end]+tests)
exe=work/'discovery.exe'
subprocess.run([sys.argv[1],'-O2',str(cpp),'-o',str(exe)],check=True)
subprocess.run([str(exe.resolve())],check=True)
