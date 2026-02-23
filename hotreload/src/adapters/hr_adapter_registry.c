#include "hr_adapter.h"
#include <string.h>

static hr_adapter_t* adapters[] = {
    &hr_adapter_c,
    &hr_adapter_cpp,
    &hr_adapter_rust,
    &hr_adapter_zig,
    &hr_adapter_go,
    NULL
};

hr_adapter_t* hr_adapter_get(hr_lang_t lang) {
    for (int i = 0; adapters[i]; i++) {
        if (adapters[i]->lang == lang) return adapters[i];
    }
    return NULL;
}

hr_adapter_t* hr_adapter_detect(const char* source_path) {
    for (int i = 0; adapters[i]; i++) {
        if (adapters[i]->detect(source_path)) return adapters[i];
    }
    return NULL;
}
