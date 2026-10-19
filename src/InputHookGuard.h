#pragma once
// x64 Windows MEMORY_BASIC_INFORMATION layout; no Windows SDK dependency.
struct P91MemoryInfo {
    void* base; void* allocationBase; unsigned long allocationProtect;
    unsigned short partitionId; usize regionSize;
    unsigned long state, protect, type;
};
static_assert(sizeof(P91MemoryInfo)==48,"x64 memory query layout");
static bool p91_input_operand(const u8* code, uptr module, usize imageSize, uptr& operand) {
    static const u8 prefix[]={0x40,0x56,0x57,0x48,0x83,0xec,0x38,0x4c,0x8b,0x15};
    operand=0;
    uptr pc=(uptr)code;
    if(pc<module || imageSize<14 || pc-module>imageSize-14)return false;
    for(int i=0;i<10;++i)if(code[i]!=prefix[i])return false;
    u32 raw=(u32)code[10]|((u32)code[11]<<8)|((u32)code[12]<<16)|((u32)code[13]<<24);
    i64 displacement=(raw&0x80000000u)?(i64)raw-0x100000000ll:(i64)raw;
    // Work in module-relative offsets to avoid unsigned address wraparound.
    i64 relative=(i64)(pc-module)+14+displacement;
    if(relative<0 || imageSize<8 || (uptr)relative>imageSize-8)return false;
    operand=module+(uptr)relative;
    return true;
}
static bool p91_operand_readable(uptr operand, uptr module, const P91MemoryInfo& m) {
    if((uptr)m.allocationBase!=module || m.state!=0x1000 || m.type!=0x1000000)return false;
    if(m.protect&0x100)return false; // PAGE_GUARD
    unsigned long access=m.protect&0xff;
    if(access!=0x02 && access!=0x04 && access!=0x08 && access!=0x20 && access!=0x40 && access!=0x80)return false;
    uptr base=(uptr)m.base;
    return operand>=base && m.regionSize>=8 && operand-base<=m.regionSize-8;
}
