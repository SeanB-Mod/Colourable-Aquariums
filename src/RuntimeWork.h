// Main-thread ownership is captured from the process-owned Unity window.
static DWORD (*p78_threadId)()=nullptr;
static u64 (*p78_clock)()=nullptr;
static DWORD p78_mainThread=0;
static bool p78_main(){return p78_threadId&&p78_mainThread&&p78_threadId()==p78_mainThread;}
static unsigned p78_workDepth=0;
static bool p78_waterFresh=false;
static u64 p78_epoch=0;
struct P78WorkScope {
    P78WorkScope(){if(p78_workDepth++==0){++p78_epoch;p78_waterFresh=false;}}
    ~P78WorkScope(){--p78_workDepth;}
};
// Only stable successful metadata resolutions are cached, never live game objects.
template<class T,int N> struct P78MetadataCache {
    struct Entry {void* owner;char space[96],name[128];T value;};
    Entry entries[N]{};unsigned next=0;
    T get(void* owner,const char* space,const char* name){
        for(auto& e:entries)if(e.value&&e.owner==owner&&streq(e.space,space)&&streq(e.name,name))return e.value;
        return nullptr;
    }
    void put(void* owner,const char* space,const char* name,T value){
        if(!value||slen(space)>=96||slen(name)>=128)return;
        auto& e=entries[next++%N];e.owner=owner;scopy(e.space,sizeof(e.space),space);scopy(e.name,sizeof(e.name),name);e.value=value;
    }
};

static u64 p78_stateScans=0,p78_stateHits=0,p78_rendererScans=0,p78_rendererHits=0;
