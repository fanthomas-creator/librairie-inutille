#ifndef HOTRELOAD_H
#define HOTRELOAD_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
  #ifdef HR_BUILD_DLL
    #define HR_API __declspec(dllexport)
  #else
    #define HR_API __declspec(dllimport)
  #endif
#else
  #define HR_API __attribute__((visibility("default")))
#endif

typedef enum {
    HR_LANG_AUTO = 0,
    HR_LANG_C,
    HR_LANG_CPP,
    HR_LANG_RUST,
    HR_LANG_ZIG,
    HR_LANG_GO
} hr_lang_t;

typedef enum {
    HR_OK = 0,
    HR_ERR_INIT,
    HR_ERR_COMPILE,
    HR_ERR_LOAD,
    HR_ERR_SYMBOL,
    HR_ERR_PATCH,
    HR_ERR_PLATFORM,
    HR_ERR_INVALID
} hr_result_t;

typedef enum {
    HR_LOG_NONE = 0,
    HR_LOG_ERROR,
    HR_LOG_WARN,
    HR_LOG_INFO,
    HR_LOG_DEBUG
} hr_log_level_t;

typedef struct {
    void*  data;
    size_t size;
} hr_state_t;

typedef void (*hr_save_state_fn)(hr_state_t* state);
typedef void (*hr_restore_state_fn)(hr_state_t* state);
typedef void (*hr_on_reload_fn)(const char* module_path, hr_result_t result);

typedef struct {
    hr_log_level_t      log_level;
    const char*         compiler_flags;
    const char*         build_dir;
    hr_save_state_fn    save_state;
    hr_restore_state_fn restore_state;
    hr_on_reload_fn     on_reload;
    int                 poll_interval_ms;
    int                 enable_patching;
} hr_config_t;

typedef struct hr_context hr_context_t;
typedef struct hr_module  hr_module_t;

HR_API hr_config_t    hr_default_config(void);
HR_API hr_context_t*  hr_init(const char* watch_dir, hr_lang_t lang, const hr_config_t* config);
HR_API void           hr_shutdown(hr_context_t* ctx);
HR_API hr_module_t*   hr_load(hr_context_t* ctx, const char* source_path);
HR_API void           hr_unload(hr_context_t* ctx, hr_module_t* mod);
HR_API hr_result_t    hr_poll(hr_context_t* ctx);
HR_API void*          hr_get_fn(hr_module_t* mod, const char* name);
HR_API hr_result_t    hr_reload_module(hr_context_t* ctx, hr_module_t* mod);
HR_API const char*    hr_result_str(hr_result_t result);
HR_API const char*    hr_version(void);

#ifdef __cplusplus
}
#endif

#endif
