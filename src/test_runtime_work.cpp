#include <cassert>
#include <cstring>
#include <cstdio>
using DWORD=unsigned long;using u64=unsigned long long;using usize=unsigned long long;
static bool streq(const char*a,const char*b){return !strcmp(a,b);}static usize slen(const char*a){return strlen(a);}static void scopy(char*d,usize n,const char*s){snprintf(d,n,"%s",s);}
#include "RuntimeWork.h"
static DWORD current=1;static DWORD thread(){return current;}
int main(){
 p78_threadId=thread;assert(!p78_main());p78_mainThread=1;assert(p78_main());current=2;assert(!p78_main());
 for(int i=0;i<1000;++i){{P78WorkScope a;assert(p78_workDepth==1&&!p78_waterFresh);auto epoch=p78_epoch;p78_waterFresh=true;{P78WorkScope b;assert(p78_epoch==epoch&&p78_waterFresh);}assert(p78_workDepth==1);}assert(!p78_workDepth);}
 P78MetadataCache<void*,2> c;assert(!c.get(nullptr,"N","C"));c.put(nullptr,"N","C",nullptr);assert(!c.get(nullptr,"N","C"));
 c.put(nullptr,"N","C",(void*)1);assert(c.get(nullptr,"N","C")== (void*)1);assert(!c.get(nullptr,"Other","C"));
 c.put((void*)2,"","field",(void*)3);assert(!c.get((void*)4,"","field"));assert(c.get((void*)2,"","field")== (void*)3);
 c.put(nullptr,"Third","C",(void*)5);assert(!c.get(nullptr,"N","C"));assert(c.get(nullptr,"Third","C")== (void*)5);
 puts("Runtime work: thread gate, 1000 nested/reset batch scopes, metadata identity, misses and bounded eviction passed.");
}
