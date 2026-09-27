import pathlib,subprocess,sys
root=pathlib.Path(__file__).parent
s=(root/'AquariumSwatches.h').read_text();start=s.index('static bool p67_swatch_press(');end=s.index('\nstatic void p67_close_swatches',start)
fixture=r'''
#include <cassert>
static bool p80_externalPalette=false;
#include <cstdio>
struct F4{float x,y,z,w;};
static void* p56_menu=nullptr;static void* p67_swatchButtons[2]={(void*)1,(void*)2};
static F4 p67_swatchColors[2]={{.2f,.3f,.4f,1},{.6f,.7f,.8f,1}};
static int p80_selected=-1;static bool p80_palette_matches(){return true;}static void p80_refresh_swatches(){}static void p80_button_states(){}
static int p67_swatchCount=2,edits=0,saves=0,flushes=0;static bool live=true;
static float g_manualHue,g_manualSat,g_manualVal;
static bool p56_target_live(){return live;}
static void rgb_to_hsv(F4 c,float& h,float& s,float& v){h=c.x;s=c.y;v=c.z;}
static void apply_manual_hsv(bool){++edits;}
static void p56_gradient(){}
static void p65_flush_glow(){++flushes;}
static void persist_current_target(bool){++saves;}
static void log_raw(const char*){}
'''
test=r'''
int main(){
 p80_externalPalette=true;p56_menu=(void*)3;assert(p67_swatch_press((void*)1)&&edits==0);p80_externalPalette=false;p56_menu=nullptr;
 assert(!p67_swatch_press((void*)1)&&edits==0);
 p56_menu=(void*)3;assert(!p67_swatch_press((void*)9)&&edits==0);
 live=false;assert(p67_swatch_press((void*)1)&&edits==0);
 live=true;assert(p67_swatch_press((void*)2)&&edits==1&&saves==1&&flushes==1&&g_manualHue==.6f);
 std::puts("Swatch dispatch passed: outside editor and unrelated buttons untouched; invalid target rejected; owned colour applied and saved once.");
}
'''
work=pathlib.Path(sys.argv[2]);work.mkdir(parents=True,exist_ok=True)
cpp=work/'swatches.cpp';cpp.write_text(fixture+s[start:end]+test);exe=work/'swatches.exe'
subprocess.run([sys.argv[1],'-O2',str(cpp),'-o',str(exe)],check=True);subprocess.run([str(exe.resolve())],check=True)
