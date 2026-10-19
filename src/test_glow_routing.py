import pathlib,subprocess,sys
root=pathlib.Path(__file__).parent
s=(root/'AquariumColour_source.cpp').read_text()
a=s.index('static P49GlowRoute p49_current_glow_route(bool verbose){');b=s.index('static int p49_sync_glow_for_current',a)
fixture=r"""
#include <cassert>
#include <cstdio>
enum P49GlowRoute{P49_ROUTE_UNKNOWN,P49_ROUTE_STANDARD,P49_ROUTE_DREDGE};
struct Water{void* renderer;};Water g_water[2]{{(void*)1},{(void*)2}};
int g_waterCount=2,g_waterTarget=0,searches=0;void* g_p48DredgeWater=(void*)1;bool standard=true;
void refresh_water_targets(){}void log_raw(const char*){}auto psprintf=std::sprintf;
bool p53_map_selected_standard(bool,void*& st,void*& floor){st=standard?(void*)3:nullptr;floor=standard?(void*)4:nullptr;return standard;}
int p48_cached_dredge_index(){++searches;return 0;}int p48_null_tint_state_count(){return standard?1:0;}
"""
test=r"""
int main(){
// Decorations identify this same water as Dredge; verified standard state wins.
assert(p49_current_glow_route(false)==P49_ROUTE_STANDARD&&searches==0);
standard=false;assert(p49_current_glow_route(false)==P49_ROUTE_UNKNOWN&&searches==0);
standard=true;g_waterTarget=1;assert(p49_current_glow_route(false)==P49_ROUTE_STANDARD&&searches==0);
g_waterTarget=2;assert(p49_current_glow_route(false)==P49_ROUTE_UNKNOWN);
g_waterCount=0;assert(p49_current_glow_route(false)==P49_ROUTE_UNKNOWN);
puts("Glow routing: standard-state priority over Dredge decorations, per-tank Dredge consumer, unknown and invalid selection passed.");
}
"""
# Standard mapping must consider waters even if scenery marked them as Dredge.
m=s[s.index('static int p53_build_standard_map'):s.index('static bool p53_map_selected_standard')]
assert 'water==g_p48DredgeWater' not in m
assert 'field_object(st,"_highestPriorityAquariumTintDefinition")' not in m
out=pathlib.Path(sys.argv[2]);out.mkdir(parents=True,exist_ok=True)
cpp=out/'glow_routing.cpp';cpp.write_text(fixture+s[a:b]+test);exe=out/'glow_routing.exe'
subprocess.run([sys.argv[1],'-O2',str(cpp),'-o',str(exe)],check=True)
subprocess.run([str(exe.resolve())],check=True)
