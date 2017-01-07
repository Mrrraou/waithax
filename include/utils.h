#pragma once

#include <3ds/types.h>


// Totally not my code but cleaned up by TuxSH because I write messy instruction
// decoding code (this also wasn't made specifically for this, lenny)
static inline void* decodeARMBranch(const void *src)
{
    u32 instr = *(const u32 *)src;
    s32 off = (instr & 0xFFFFFF) << 2;
    off = (off << 6) >> 6; // sign extend

    return (void *)((const u8 *)src + 8 + off);
}


typedef u32(*backdoor_fn)(u32 arg0, u32 arg1);
u32 svc_7b(void* entry_fn, ...); // can pass up to two arguments to entry_fn(...)
u32 svc_30(void* entry_fn, ...); // can pass up to two arguments to entry_fn(...)

Result svcCreateSemaphoreKAddr(Handle *semaphore, s32 initialCount, s32 maxCount, u32 **kaddr);
void *convertVAToPA(const void *addr);
void flushEntireCaches(void);

void svcBackdoor_original(void(*)(void));
extern u32 svcBackdoor_original_size;
