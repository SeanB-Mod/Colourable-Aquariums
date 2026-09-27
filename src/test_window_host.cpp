#include <cassert>
#include <cstdio>
struct Node {Node* parent;bool active=true,alive=true;int rect=0;};
struct RectState {int value;};
enum {M_Transform_SetParent=1,M_Transform_get_parent,M_GameObject_get_activeSelf};
static bool captureWorks=true,blockActivation=false;
static int moves=0,holds=0;
static void* p56_tr(void* p){return p;}
static void* p56_go(void* p){return p;}
static bool p56_alive(void* p){return p&&((Node*)p)->alive;}
static bool p56_active(void* p){auto n=(Node*)p;return n&&n->active&&(!n->parent||p56_active(n->parent));}
static void p56_setActive(void* p,bool v){if(p&&!blockActivation)((Node*)p)->active=v;}
static void* invoke(int m,void* p,void** args=nullptr){
 auto n=(Node*)p;
 if(m==M_Transform_get_parent)return n->parent;
 if(m==M_GameObject_get_activeSelf)return n->active?(void*)1:nullptr;
 if(m==M_Transform_SetParent){n->parent=(Node*)args[0];n->rect=-1;++moves;}
 return nullptr;
}
static bool boxed_bool(void* p){return p!=nullptr;}
static bool capture_rect_state(void* p,RectState& r){r.value=((Node*)p)->rect;return captureWorks;}
static void restore_rect_state(void* p,RectState r){((Node*)p)->rect=r.value;}
static void p56_hold(void*){++holds;}
static void log_raw(const char*){}
static void* nearest_active_parent(void* p){for(auto n=((Node*)p)->parent;n;n=n->parent)if(p56_active(n))return n;return nullptr;}
#include "AquariumWindowHost.h"
int main(){
 Node root{nullptr},parent{&root},menu{&parent,false,true,42};
 assert(p68_prepare_window(&menu)&&p56_active(&menu)&&moves==0);
 p68_restore_window(&menu);assert(!menu.active&&menu.parent==&parent&&menu.rect==42);
 parent.active=false;
 assert(p68_prepare_window(&menu)&&menu.parent==&root&&menu.rect==42&&!parent.active);
 p68_restore_window(&menu);assert(menu.parent==&parent&&!menu.active&&menu.rect==42&&moves==2);
 root.active=false;assert(!p68_prepare_window(&menu)&&!p68_windowPrepared&&menu.parent==&parent);
 root.active=true;parent.active=true;blockActivation=true;
 assert(!p68_prepare_window(&menu)&&!p68_windowPrepared);
 blockActivation=false;captureWorks=false;assert(!p68_prepare_window(&menu)&&!p68_windowPrepared);
 captureWorks=true;assert(p68_prepare_window(&menu));menu.alive=false;
 p68_restore_window(&menu);assert(!p68_windowPrepared);
 puts("Window host: active parent, inactive parent, restoration, absent host, failed activation/capture and destroyed menu passed.");
}
