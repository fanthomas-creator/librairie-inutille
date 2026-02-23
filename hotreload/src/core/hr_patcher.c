#include "hr_patcher.h"
#include "../platform/hr_platform.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#if defined(__x86_64__) || defined(_M_X64)
#define HR_ARCH_X64 1
#define TRAMPOLINE_SIZE 14
#elif defined(__aarch64__) || defined(_M_ARM64)
#define HR_ARCH_ARM64 1
#define TRAMPOLINE_SIZE 16
#else
#define HR_ARCH_UNSUPPORTED 1
#define TRAMPOLINE_SIZE 0
#endif

int hr_patcher_supported(void) {
#ifdef HR_ARCH_UNSUPPORTED
    return 0;
#else
    return 1;
#endif
}

#ifdef HR_ARCH_X64
static void write_trampoline(unsigned char* buf, void* target) {
    uint64_t addr = (uint64_t)(uintptr_t)target;
    buf[0]  = 0xFF;
    buf[1]  = 0x25;
    buf[2]  = 0x00;
    buf[3]  = 0x00;
    buf[4]  = 0x00;
    buf[5]  = 0x00;
    buf[6]  = (addr >>  0) & 0xFF;
    buf[7]  = (addr >>  8) & 0xFF;
    buf[8]  = (addr >> 16) & 0xFF;
    buf[9]  = (addr >> 24) & 0xFF;
    buf[10] = (addr >> 32) & 0xFF;
    buf[11] = (addr >> 40) & 0xFF;
    buf[12] = (addr >> 48) & 0xFF;
    buf[13] = (addr >> 56) & 0xFF;
}
#endif

#ifdef HR_ARCH_ARM64
static void write_trampoline(unsigned char* buf, void* target) {
    uint64_t addr = (uint64_t)(uintptr_t)target;
    uint32_t* u = (uint32_t*)buf;
    u[0] = 0x58000051;
    u[1] = 0xD61F0200;
    u[2] = (uint32_t)(addr & 0xFFFFFFFF);
    u[3] = (uint32_t)(addr >> 32);
}
#endif

int hr_patcher_apply(void* target_fn, void* new_fn, hr_patch_t* out_patch) {
#ifdef HR_ARCH_UNSUPPORTED
    (void)target_fn; (void)new_fn; (void)out_patch;
    fprintf(stderr, "[hr:patcher] unsupported architecture\n");
    return 0;
#else
    if (!target_fn || !new_fn || !out_patch) return 0;

    out_patch->target_addr = target_fn;
    out_patch->patched     = 0;
    memcpy(out_patch->original_bytes, target_fn, TRAMPOLINE_SIZE);

    if (!hr_platform_make_writable(target_fn, TRAMPOLINE_SIZE)) {
        fprintf(stderr, "[hr:patcher] cannot make memory writable\n");
        return 0;
    }

    unsigned char trampoline[TRAMPOLINE_SIZE];
    write_trampoline(trampoline, new_fn);
    memcpy(target_fn, trampoline, TRAMPOLINE_SIZE);

    if (!hr_platform_make_executable(target_fn, TRAMPOLINE_SIZE)) {
        memcpy(target_fn, out_patch->original_bytes, TRAMPOLINE_SIZE);
        return 0;
    }

    out_patch->patched = 1;
    return 1;
#endif
}

int hr_patcher_revert(hr_patch_t* patch) {
    if (!patch || !patch->patched) return 0;
    if (!hr_platform_make_writable(patch->target_addr, TRAMPOLINE_SIZE)) return 0;
    memcpy(patch->target_addr, patch->original_bytes, TRAMPOLINE_SIZE);
    hr_platform_make_executable(patch->target_addr, TRAMPOLINE_SIZE);
    patch->patched = 0;
    return 1;
}
