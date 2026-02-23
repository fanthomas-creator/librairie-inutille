#ifndef HR_LOADER_H
#define HR_LOADER_H

#include "../../include/hotreload.h"
#include "hr_symbols.h"
#include "../adapters/hr_adapter.h"

typedef struct {
    void*            lib_handle;
    char             lib_path[4096];
    char             src_path[4096];
    hr_adapter_t*    adapter;
    hr_symbol_table_t symbols;
    int64_t          last_mtime;
} hr_loaded_module_t;

hr_loaded_module_t* hr_loader_open(const char* src_path, const char* build_dir,
                                   hr_adapter_t* adapter, const char* flags);
void                hr_loader_close(hr_loaded_module_t* mod);
hr_result_t         hr_loader_reload(hr_loaded_module_t* mod, const char* build_dir, const char* flags,
                                     hr_save_state_fn save_cb, hr_restore_state_fn restore_cb);
void*               hr_loader_get_sym(hr_loaded_module_t* mod, const char* name);

#endif
