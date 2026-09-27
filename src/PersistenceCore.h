// Portable snapshot codec. The host supplies p70_alloc/p70_free and F3/F4/u32.
struct P70Scope { u32 career,level,slot,mode; };
static bool p70_equal(P70Scope a,P70Scope b){return a.career==b.career&&a.level==b.level&&a.slot==b.slot&&a.mode==b.mode;}
struct P70Record { F3 key; F4 color; u32 enabled; };
struct P70Store { P70Record* records; u32 count,capacity; };
struct P70Header { u32 magic,version,count; P70Scope scope; u32 checksum; };
static_assert(sizeof(P70Record)==32&&sizeof(P70Header)==32,"Snapshot layout changed");
static constexpr u32 P70_MAX_RECORDS=262144;
static bool p70_coord(float v){return v>=-10000000.0f&&v<=10000000.0f;}
static bool p70_color(float v){return v>=0.0f&&v<=1.0f;}
static bool p70_valid(P70Record r){return p70_coord(r.key.x)&&p70_coord(r.key.y)&&p70_coord(r.key.z)&&p70_color(r.color.x)&&p70_color(r.color.y)&&p70_color(r.color.z)&&r.color.w==0&&r.enabled<=1;}
static float p70_abs(float v){return v<0?-v:v;}
static bool p70_same_key(F3 a,F3 b){return p70_abs(a.x-b.x)<.20f&&p70_abs(a.y-b.y)<.20f&&p70_abs(a.z-b.z)<.20f;}
static void p70_clear(P70Store& s){if(s.records)p70_free(s.records);s=P70Store{};}
static bool p70_reserve(P70Store& s,u32 required){
    if(required<=s.capacity)return true;
    if(required>P70_MAX_RECORDS)return false;
    u32 cap=s.capacity?s.capacity:128;while(cap<required)cap*=2;
    auto p=(P70Record*)p70_alloc((unsigned long long)cap*sizeof(P70Record));if(!p)return false;
    for(u32 i=0;i<s.count;++i)p[i]=s.records[i];
    if(s.records)p70_free(s.records);s.records=p;s.capacity=cap;return true;
}
static bool p70_put(P70Store& s,P70Record r){
    if(!p70_valid(r))return false;
    for(u32 i=0;i<s.count;++i)if(p70_same_key(s.records[i].key,r.key)){s.records[i]=r;return true;}
    if(!p70_reserve(s,s.count+1))return false;s.records[s.count++]=r;return true;
}
static bool p70_find(const P70Store& s,F3 key,F4& color){
    for(u32 i=0;i<s.count;++i)if(p70_same_key(s.records[i].key,key)){
        if(!s.records[i].enabled)return false;color=s.records[i].color;return true;
    }
    return false; // Never borrow a nearby tank's colour.
}
static u32 p70_hash(const unsigned char* data,u32 bytes){
    u32 h=2166136261u;for(u32 i=0;i<bytes;++i){unsigned char b=(i>=28&&i<32)?0:data[i];h=(h^b)*16777619u;}return h;
}
static u32 p70_size(const P70Store& s){return sizeof(P70Header)+s.count*sizeof(P70Record);}
static bool p70_encode(const P70Store& s,P70Scope scope,void* output,u32 size){
    if(size!=p70_size(s)||s.count>P70_MAX_RECORDS)return false;
    auto h=(P70Header*)output;*h={0x32435141u,2,s.count,scope,0};
    auto r=(P70Record*)(h+1);for(u32 i=0;i<s.count;++i){if(!p70_valid(s.records[i]))return false;r[i]=s.records[i];}
    h->checksum=p70_hash((unsigned char*)output,size);return true;
}
static bool p70_decode(P70Store& output,P70Scope scope,const void* data,u32 size){
    if(size<sizeof(P70Header))return false;auto h=(const P70Header*)data;
    if(h->magic!=0x32435141u||h->version!=2||h->count>P70_MAX_RECORDS||!p70_equal(h->scope,scope))return false;
    if(size!=sizeof(P70Header)+h->count*sizeof(P70Record)||h->checksum!=p70_hash((const unsigned char*)data,size))return false;
    auto r=(const P70Record*)(h+1);for(u32 i=0;i<h->count;++i)if(!p70_valid(r[i]))return false;
    P70Store next{};if(!p70_reserve(next,h->count))return false;
    for(u32 i=0;i<h->count;++i)next.records[i]=r[i];next.count=h->count;
    p70_clear(output);output=next;return true;
}
