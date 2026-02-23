#ifndef HR_PATCHER_H
#define HR_PATCHER_H

#include <stddef.h>

typedef struct {
    void*  target_addr;
    unsigned char original_bytes[16];
    int    patched;
} hr_patch_t;

int  hr_patcher_apply(void* target_fn, void* new_fn, hr_patch_t* out_patch);
int  hr_patcher_revert(hr_patch_t* patch);
int  hr_patcher_supported(void);

#endif
