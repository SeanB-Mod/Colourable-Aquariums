"""Exercise the actual click dispatcher without invoking a Unity runtime."""
import pathlib, subprocess, sys
root=pathlib.Path(__file__).parent
s=(root/'AquariumUI.h').read_text()
start=s.index('static bool p56_press(void* self){')
end=s.index('\nstatic void p56_press_hook',start)
assert s.count('p56_begin(')==2, 'Expected definition and the guarded button call'
fixture=r'''
#include <cassert>
#include <cstdio>
static bool p56_ready=false;
static int button,other,logs=0,unityCalls=0,opens=0; static bool active=true; static bool p56_active(void*){return active;} static void p56_begin(){++opens;}
static void* p56_button=&button;
static void* p56_menu=nullptr;
static void log_raw(const char*){++logs;}
static void* field_object(void*,const char*){++unityCalls;return nullptr;}
static bool p67_swatch_press(void*){return false;}
static bool p80_edit_press(void*){return false;}
static void p56_reset(){assert(false);}
static void p56_end(bool,bool){assert(false);}
'''
test=r'''
int main(){
 assert(!p56_press(&button)&&logs==0);
 p56_ready=true;
 assert(!p56_press(nullptr)&&!p56_press(&other)&&logs==0);
 for(int i=0;i<1000;++i)assert(p56_press(&button));
 assert(logs==1000&&unityCalls==0&&opens==1000); active=false; assert(p56_press(&button)&&opens==1000);
 p56_button=nullptr;
 assert(!p56_press(nullptr));
 std::puts("Click boundary passed: disabled, null, unrelated and 1000 active button presses dispatch opening; inactive button does not.");
}
'''
work=pathlib.Path(sys.argv[2]);work.mkdir(parents=True,exist_ok=True)
cpp=work/'click.cpp';cpp.write_text(fixture+s[start:end]+test)
exe=work/'click.exe'
subprocess.run([sys.argv[1],'-O2',str(cpp),'-o',str(exe)],check=True)
subprocess.run([str(exe.resolve())],check=True)
