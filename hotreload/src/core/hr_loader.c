#include "hr_loader.h"
#include "../platform/hr_platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void populate_symbols(hr_loaded_module_t* m) {
    char sym_buf[65536] = {0};
    if (!m->adapter->list_symbols(m->lib_path, sym_buf, sizeof(sym_buf))) return;

    char* line = strtok(sym_buf, "\n");
    while (line) {
        char addr_str[32], type_str[4], sym_name[HR_MAX_NAME];
        if (sscanf(line, "%31s %3s %255s", addr_str, type_str, sym_name) == 3) {
            if (type_str[0] == 'T' || type_str[0] == 't' || type_str[0] == 'W') {
                void* addr = hr_platform_lib_sym(m->lib_handle, sym_name);
                if (addr) hr_symbols_add(&m->symbols, sym_name, addr);
            }
        }
        line = strtok(NULL, "\n");
    }
}

static void make_lib_path(const char* src, const char* build_dir, char* out, size_t out_sz) {
    const char* base = strrchr(src, '/');
    if (!base) base = strrchr(src, '\\');
    base = base ? base + 1 : src;
    char name[256];
    strncpy(name, base, sizeof(name)-1);
    char* dot = strrchr(name, '.');
    if (dot) *dot = 0;
    snprintf(out, out_sz, "%s/hr_%s%s", build_dir, name, hr_platform_lib_ext());
}

hr_loaded_module_t* hr_loader_open(const char* src_path, const char* build_dir,
                                   hr_adapter_t* adapter, const char* flags) {
    hr_platform_mkdir(build_dir);

    hr_loaded_module_t* m = calloc(1, sizeof(hr_loaded_module_t));
    if (!m) return NULL;
    m->adapter = adapter;
    strncpy(m->src_path, src_path, sizeof(m->src_path)-1);
    make_lib_path(src_path, build_dir, m->lib_path, sizeof(m->lib_path));

    if (!adapter->compile(src_path, m->lib_path, flags)) {
        free(m); return NULL;
    }

    m->lib_handle = hr_platform_lib_open(m->lib_path);
    if (!m->lib_handle) {
        fprintf(stderr, "[hr:loader] dlopen failed: %s\n", hr_platform_lib_error());
        free(m); return NULL;
    }

    hr_symbols_init(&m->symbols);
    populate_symbols(m);
    m->last_mtime = hr_platform_file_mtime(src_path);
    return m;
}

void hr_loader_close(hr_loaded_module_t* mod) {
    if (!mod) return;
    if (mod->lib_handle) hr_platform_lib_close(mod->lib_handle);
    hr_symbols_clear(&mod->symbols);
    free(mod);
}

hr_result_t hr_loader_reload(hr_loaded_module_t* mod, const char* build_dir, const char* flags,
                              hr_save_state_fn save_cb, hr_restore_state_fn restore_cb) {
    hr_state_t state = {NULL, 0};
    if (save_cb) save_cb(&state);

    char new_lib[4096];
    make_lib_path(mod->src_path, build_dir, new_lib, sizeof(new_lib));

    char tmp_lib[4096];
    snprintf(tmp_lib, sizeof(tmp_lib), "%s.new%s", new_lib, "");

    if (!mod->adapter->compile(mod->src_path, tmp_lib, flags)) {
        free(state.data);
        return HR_ERR_COMPILE;
    }

    hr_platform_lib_close(mod->lib_handle);
    mod->lib_handle = NULL;

    rename(tmp_lib, new_lib);

    mod->lib_handle = hr_platform_lib_open(new_lib);
    if (!mod->lib_handle) {
        fprintf(stderr, "[hr:loader] reload dlopen failed: %s\n", hr_platform_lib_error());
        free(state.data);
        return HR_ERR_LOAD;
    }

    hr_symbols_clear(&mod->symbols);
    populate_symbols(mod);
    mod->last_mtime = hr_platform_file_mtime(mod->src_path);

    if (restore_cb && state.data) restore_cb(&state);
    free(state.data);
    return HR_OK;
}

void* hr_loader_get_sym(hr_loaded_module_t* mod, const char* name) {
    if (!mod) return NULL;
    hr_symbol_t* sym = hr_symbols_find(&mod->symbols, name);
    if (sym) return sym->current_addr;
    void* addr = hr_platform_lib_sym(mod->lib_handle, name);
    if (addr) hr_symbols_add(&mod->symbols, name, addr);
    return addr;
}
