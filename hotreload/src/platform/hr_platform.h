#ifndef HR_PLATFORM_H
#define HR_PLATFORM_H

#include <stddef.h>
#include <stdint.h>

typedef void (*hr_file_changed_cb)(const char* path, void* userdata);

typedef struct hr_watcher_handle hr_watcher_handle_t;

hr_watcher_handle_t* hr_platform_watch_start(const char* dir, hr_file_changed_cb cb, void* userdata);
void                 hr_platform_watch_stop(hr_watcher_handle_t* handle);
void                 hr_platform_watch_poll(hr_watcher_handle_t* handle);

void*  hr_platform_alloc_exec(size_t size);
void   hr_platform_free_exec(void* addr, size_t size);
int    hr_platform_make_writable(void* addr, size_t size);
int    hr_platform_make_executable(void* addr, size_t size);

void*  hr_platform_lib_open(const char* path);
void*  hr_platform_lib_sym(void* handle, const char* name);
int    hr_platform_lib_close(void* handle);
const char* hr_platform_lib_error(void);

int    hr_platform_file_exists(const char* path);
int64_t hr_platform_file_mtime(const char* path);
int    hr_platform_mkdir(const char* path);
int    hr_platform_run_command(const char* cmd, char* output, size_t output_size);

void   hr_platform_thread_pause_others(void);
void   hr_platform_thread_resume_others(void);
void   hr_platform_sleep_ms(int ms);

const char* hr_platform_lib_ext(void);
const char* hr_platform_name(void);

#endif
