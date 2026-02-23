#ifndef HR_ADAPTER_H
#define HR_ADAPTER_H

#include "../../include/hotreload.h"
#include <stddef.h>

typedef struct {
    hr_lang_t lang;
    const char* name;
    const char* source_ext;
    int (*detect)(const char* source_path);
    int (*compile)(const char* source_path, const char* output_path, const char* extra_flags);
    int (*list_symbols)(const char* lib_path, char* out_buf, size_t buf_size);
    char* (*demangle)(const char* mangled);
} hr_adapter_t;

hr_adapter_t* hr_adapter_get(hr_lang_t lang);
hr_adapter_t* hr_adapter_detect(const char* source_path);

extern hr_adapter_t hr_adapter_c;
extern hr_adapter_t hr_adapter_cpp;
extern hr_adapter_t hr_adapter_rust;
extern hr_adapter_t hr_adapter_zig;
extern hr_adapter_t hr_adapter_go;

#endif
