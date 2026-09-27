import pathlib,subprocess,sys
root=pathlib.Path(__file__).parent
s=(root/'PersistenceRuntime.h').read_text()
def extract(marker):
 a=s.index(marker);b=s.index('{',a)+1;depth=1
 while depth:depth+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a:b]
fixture=r"""
#include <windows.h>
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <vector>
using u32=unsigned int;struct F3{float x,y,z;};struct F4{float x,y,z,w;};
static void* p70_alloc(unsigned long long n){return std::malloc(n);}static void p70_free(void* p){std::free(p);}
#include "PersistenceCore.h"
static int fault=0;
static BOOL writeFile(HANDLE f,const void* p,DWORD n,DWORD* out,LPOVERLAPPED v){return WriteFile(f,p,fault==1?n/2:n,out,v);}
static BOOL flushFile(HANDLE h){return fault==2?FALSE:FlushFileBuffers(h);}
static BOOL copyFile(LPCSTR a,LPCSTR b,BOOL fail){return fault==3?FALSE:CopyFileA(a,b,fail);}
static BOOL moveFile(LPCSTR a,LPCSTR b,DWORD flags){return fault==4?FALSE:MoveFileExA(a,b,flags);}
static auto pCreateFileA=CreateFileA;static auto pWriteFile=writeFile;static auto pReadFile=ReadFile;static auto pCloseHandle=CloseHandle;
static auto p70_fileSize=GetFileSize;static auto p70_lastError=GetLastError;static auto p70_flushFile=flushFile;static auto p70_copyFile=copyFile;static auto p70_moveFile=moveFile;
static P70Store p70_store{};static P70Scope p70_scope{1,2,3,1};
static bool p70_dirty=false,p70_haveScope=true,p70_writable=true,p70_primaryHealthy=false;
static const char* p70_path="io-primary.dat";static const char* p70_backup="io-backup.dat";static const char* p70_temp="io-temp.dat";
static void log_raw(const char*){}
static void* p70_levelObject=(void*)1;static P70Scope currentScope=p70_scope;static void* currentLevel=p70_levelObject;
static bool p70_current_scope(P70Scope& s,void*& l){s=currentScope;l=currentLevel;return true;}
static bool p70_sync_scope(){return true;}
static std::vector<char> read(const char* path){std::ifstream f(path,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
"""
test=r"""
int main(){
 DeleteFileA(p70_path);DeleteFileA(p70_backup);DeleteFileA(p70_temp);
 assert(append_persistence({0,0,0},{.2f,.3f,.4f,0},true));auto original=read(p70_path);assert(!original.empty());
 for(fault=1;fault<=4;++fault){assert(!append_persistence({0,0,0},{.9f,.3f,.4f,0},true));assert(p70_dirty&&read(p70_path)==original);}
 fault=0;assert(p70_commit());assert(!p70_dirty&&read(p70_backup)==original&&read(p70_path)!=original);
 currentScope.slot++;auto good=read(p70_path);assert(!append_persistence({0,0,0},{.1f,0,0,0},true));assert(read(p70_path)==good);currentScope=p70_scope;
 currentLevel=(void*)2;assert(!append_persistence({0,0,0},{.1f,0,0,0},true));currentLevel=p70_levelObject;
 {std::ofstream f(p70_path,std::ios::binary|std::ios::trunc);f<<"torn";}
 assert(p70_read_snapshot(p70_path,p70_scope)==-1);assert(p70_read_snapshot(p70_backup,p70_scope)==1);
 assert(p70_store.count==1&&p70_store.records[0].color.x==.2f);
 p70_primaryHealthy=false;assert(append_persistence({0,0,0},{.5f,0,0,0},true));assert(read(p70_backup)==original);
 p70_clear(p70_store);DeleteFileA(p70_path);DeleteFileA(p70_backup);DeleteFileA(p70_temp);
 puts("Snapshot IO: partial write, flush/copy/replace failures retain primary; retry commits; backup recovery; stale-scope writes rejected.");
}
"""
work=pathlib.Path(sys.argv[2]).resolve();work.mkdir(parents=True,exist_ok=True)
cpp=work/'snapshot_io.cpp';cpp.write_text(fixture+extract('static int p70_read_snapshot(')+extract('static bool p70_commit(')+extract('static bool append_persistence(')+test)
exe=work/'snapshot_io.exe';subprocess.run([sys.argv[1],'-O2','-static','-I',str(root.resolve()),str(cpp),'-o',str(exe)],check=True);subprocess.run([str(exe)],cwd=work,check=True,timeout=30)
