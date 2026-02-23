#include "../../include/hotreload.h"
#include "hr_watcher.h"
#include "hr_differ.h"
#include "hr_loader.h"
#include "hr_patcher.h"
#include "hr_symbols.h"
#include "../adapters/hr_adapter.h"
#include "../platform/hr_platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HR_VERSION_STR "1.0.0"
#define HR_MAX_MODULES 64

struct hr_module {
    hr_loaded_module_t* loaded;
    hr_patch_t          patches[HR_MAX_SYMBOLS];
    int                 patch_count;
    char                cache_path[4096];
};

struct hr_context {
    hr_watcher_t*    watcher;
    hr_adapter_t*    adapter;
    hr_config_t      config;
    char             watch_dir[4096];
    char             build_dir[4096];
    hr_module_t*     modules[HR_MAX_MODULES];
    int              module_count;
    int              dirty;
    char             dirty_path[4096];
};

static hr_log_level_t g_log_level = HR_LOG_INFO;

static void hr_log(hr_log_level_t level, const char* fmt, ...) {
    if (level > g_log_level) return;
    const char* prefix[] = {"", "[ERROR]", "[WARN]", "[INFO]", "[DEBUG]"};
    fprintf(stderr, "hotreload %s ", prefix[level < 5 ? level : 4]);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
}

#include <stdarg.h>

static void on_file_changed(const char* path, void* userdata) {
    hr_context_t* ctx = (hr_context_t*)userdata;
    strncpy(ctx->dirty_path, path, sizeof(ctx->dirty_path)-1);
    ctx->dirty = 1;
    hr_log(HR_LOG_INFO, "file changed: %s", path);
}

hr_config_t hr_default_config(void) {
    hr_config_t cfg = {0};
    cfg.log_level       = HR_LOG_INFO;
    cfg.compiler_flags  = "";
    cfg.build_dir       = ".hotreload_build";
    cfg.poll_interval_ms = 50;
    cfg.enable_patching  = 1;
    return cfg;
}

hr_context_t* hr_init(const char* watch_dir, hr_lang_t lang, const hr_config_t* config) {
    hr_context_t* ctx = calloc(1, sizeof(hr_context_t));
    if (!ctx) return NULL;

    ctx->config = config ? *config : hr_default_config();
    g_log_level = ctx->config.log_level;

    strncpy(ctx->watch_dir, watch_dir, sizeof(ctx->watch_dir)-1);
    strncpy(ctx->build_dir,
            ctx->config.build_dir ? ctx->config.build_dir : ".hotreload_build",
            sizeof(ctx->build_dir)-1);

    hr_platform_mkdir(ctx->build_dir);

    if (lang == HR_LANG_AUTO) {
        ctx->adapter = NULL;
    } else {
        ctx->adapter = hr_adapter_get(lang);
    }

    ctx->watcher = hr_watcher_create(watch_dir, on_file_changed, ctx);
    if (!ctx->watcher) {
        hr_log(HR_LOG_ERROR, "failed to create watcher for: %s", watch_dir);
        free(ctx);
        return NULL;
    }

    hr_log(HR_LOG_INFO, "initialized | platform=%s | dir=%s", hr_platform_name(), watch_dir);
    return ctx;
}

void hr_shutdown(hr_context_t* ctx) {
    if (!ctx) return;
    for (int i = 0; i < ctx->module_count; i++) {
        hr_unload(ctx, ctx->modules[i]);
    }
    hr_watcher_destroy(ctx->watcher);
    free(ctx);
    hr_log(HR_LOG_INFO, "shutdown complete");
}

hr_module_t* hr_load(hr_context_t* ctx, const char* source_path) {
    if (!ctx || !source_path) return NULL;
    if (ctx->module_count >= HR_MAX_MODULES) {
        hr_log(HR_LOG_ERROR, "max modules reached");
        return NULL;
    }

    hr_adapter_t* adapter = ctx->adapter;
    if (!adapter) {
        adapter = hr_adapter_detect(source_path);
        if (!adapter) {
            hr_log(HR_LOG_ERROR, "cannot detect language for: %s", source_path);
            return NULL;
        }
    }

    hr_log(HR_LOG_INFO, "loading %s [%s]", source_path, adapter->name);

    hr_loaded_module_t* loaded = hr_loader_open(source_path, ctx->build_dir,
                                                adapter, ctx->config.compiler_flags);
    if (!loaded) {
        hr_log(HR_LOG_ERROR, "failed to load: %s", source_path);
        return NULL;
    }

    hr_module_t* mod = calloc(1, sizeof(hr_module_t));
    if (!mod) { hr_loader_close(loaded); return NULL; }
    mod->loaded = loaded;

    ctx->modules[ctx->module_count++] = mod;
    hr_log(HR_LOG_INFO, "loaded OK | symbols=%d", loaded->symbols.count);
    return mod;
}

void hr_unload(hr_context_t* ctx, hr_module_t* mod) {
    if (!ctx || !mod) return;
    for (int i = 0; i < mod->patch_count; i++)
        hr_patcher_revert(&mod->patches[i]);
    hr_loader_close(mod->loaded);
    for (int i = 0; i < ctx->module_count; i++) {
        if (ctx->modules[i] == mod) {
            ctx->modules[i] = ctx->modules[--ctx->module_count];
            break;
        }
    }
    free(mod);
}

hr_result_t hr_reload_module(hr_context_t* ctx, hr_module_t* mod) {
    if (!ctx || !mod) return HR_ERR_INVALID;
    hr_log(HR_LOG_INFO, "reloading: %s", mod->loaded->src_path);

    for (int i = 0; i < mod->patch_count; i++)
        hr_patcher_revert(&mod->patches[i]);
    mod->patch_count = 0;

    hr_result_t res = hr_loader_reload(mod->loaded, ctx->build_dir,
                                       ctx->config.compiler_flags,
                                       ctx->config.save_state,
                                       ctx->config.restore_state);
    if (ctx->config.on_reload)
        ctx->config.on_reload(mod->loaded->src_path, res);
    if (res == HR_OK)
        hr_log(HR_LOG_INFO, "reload OK | symbols=%d", mod->loaded->symbols.count);
    else
        hr_log(HR_LOG_ERROR, "reload failed: %s", hr_result_str(res));
    return res;
}

hr_result_t hr_poll(hr_context_t* ctx) {
    if (!ctx) return HR_ERR_INVALID;
    hr_watcher_poll(ctx->watcher);

    if (!ctx->dirty) return HR_OK;
    ctx->dirty = 0;

    hr_result_t result = HR_OK;
    for (int i = 0; i < ctx->module_count; i++) {
        hr_module_t* mod = ctx->modules[i];
        if (strstr(ctx->dirty_path, mod->loaded->src_path) ||
            strstr(mod->loaded->src_path, ctx->dirty_path)) {
            result = hr_reload_module(ctx, mod);
        }
    }

    if (result == HR_OK && ctx->module_count > 0) {
        hr_result_t any = HR_OK;
        for (int i = 0; i < ctx->module_count; i++) {
            int64_t mtime = hr_platform_file_mtime(ctx->modules[i]->loaded->src_path);
            if (mtime > ctx->modules[i]->loaded->last_mtime) {
                any = hr_reload_module(ctx, ctx->modules[i]);
                if (any != HR_OK) result = any;
            }
        }
    }

    return result;
}

void* hr_get_fn(hr_module_t* mod, const char* name) {
    if (!mod || !name) return NULL;
    return hr_loader_get_sym(mod->loaded, name);
}

const char* hr_result_str(hr_result_t result) {
    switch (result) {
        case HR_OK:           return "OK";
        case HR_ERR_INIT:     return "ERR_INIT";
        case HR_ERR_COMPILE:  return "ERR_COMPILE";
        case HR_ERR_LOAD:     return "ERR_LOAD";
        case HR_ERR_SYMBOL:   return "ERR_SYMBOL";
        case HR_ERR_PATCH:    return "ERR_PATCH";
        case HR_ERR_PLATFORM: return "ERR_PLATFORM";
        case HR_ERR_INVALID:  return "ERR_INVALID";
        default:              return "UNKNOWN";
    }
}

const char* hr_version(void) {
    return HR_VERSION_STR;
}
