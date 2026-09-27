#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <limits>
using u32=unsigned int;
struct F3{float x,y,z;};struct F4{float x,y,z,w;};
static bool failAllocation=false;
static void* p70_alloc(unsigned long long n){return failAllocation?nullptr:std::malloc(n);}
static void p70_free(void* p){std::free(p);}
#include "PersistenceCore.h"
#include "InputSafety.h"
int main(){
 P70Store s{};P70Scope scope{42,97,1,1};F4 color{};
 for(int i=0;i<1000;++i)assert(p70_put(s,{{float(i),0,0},{.2f,.4f,.6f,0},1}));
 assert(s.count==1000&&p70_find(s,{999,0,0},color));
 for(int i=0;i<1000;++i)assert(p70_put(s,{{1,0,0},{.5f,.4f,.6f,0},1}));
 assert(s.count==1000); // Repeated edits are compacted; they do not grow the snapshot.
 assert(!p70_find(s,{1001,0,0},color)); // No nearby fallback.
 assert(p70_put(s,{{1,0,0},{0,0,0,0},0}));assert(!p70_find(s,{1,0,0},color));
 auto invalid=P70Record{{0,0,0},{std::numeric_limits<float>::quiet_NaN(),0,0,0},1};assert(!p70_put(s,invalid));
 invalid.color={0,0,0,0};invalid.key.x=std::numeric_limits<float>::infinity();assert(!p70_put(s,invalid));
 invalid.key.x=0;invalid.enabled=2;assert(!p70_put(s,invalid));
 std::vector<unsigned char> bytes(p70_size(s));assert(p70_encode(s,scope,bytes.data(),bytes.size()));
 P70Store loaded{};assert(p70_decode(loaded,scope,bytes.data(),bytes.size()));assert(loaded.count==1000&&!p70_find(loaded,{1,0,0},color));
 for(int field=0;field<4;++field){auto other=scope;(&other.career)[field]++;assert(!p70_decode(loaded,other,bytes.data(),bytes.size()));}
 for(unsigned int i=0;i<bytes.size();i+=17){auto damaged=bytes;damaged[i]^=1;assert(!p70_decode(loaded,scope,damaged.data(),damaged.size()));}
 for(unsigned int i=0;i<bytes.size();i+=19)assert(!p70_decode(loaded,scope,bytes.data(),i));
 auto damaged=bytes;auto records=(P70Record*)(damaged.data()+sizeof(P70Header));records[0].color.x=std::numeric_limits<float>::infinity();
 ((P70Header*)damaged.data())->checksum=p70_hash(damaged.data(),damaged.size());assert(!p70_decode(loaded,scope,damaged.data(),damaged.size()));
 failAllocation=true;assert(!p70_decode(loaded,scope,bytes.data(),bytes.size()));assert(loaded.count==1000);failAllocation=false;
 P70Store empty{};std::vector<unsigned char> blank(p70_size(empty));assert(p70_encode(empty,scope,blank.data(),blank.size()));assert(p70_decode(loaded,scope,blank.data(),blank.size())&&loaded.count==0);
 P70FocusGate gate;assert(!gate.accept(false));assert(!gate.accept(true));assert(gate.accept(true));assert(!gate.accept(false));assert(!gate.accept(true));
 assert(!p70_mouse_edge(true,true)&&p70_mouse_edge(true,false)&&!p70_mouse_edge(false,true));
 p70_clear(s);p70_clear(loaded);puts("Persistence/input: 1000 records, compaction, scope isolation, reset, corruption/truncation, NaN/Inf, allocation failures, focus transitions and button edges passed.");
}
