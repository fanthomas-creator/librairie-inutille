#ifdef __APPLE__

#include "hr_platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/event.h>
#include <sys/time.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>

#define MAX_WATCH_FILES 256

struct hr_watcher_handle {
    int kq;
    int fds[MAX_WATCH_FILES];
    char paths[MAX_WATCH_FILES][4096];
    int count;
    hr_file_changed_cb cb;
    void* userdata;
    char watch_dir[4096];
};

static void scan_dir_fds(hr_watcher_handle_t* h, const char* dir) {
    DIR* d = opendir(dir);
    if (!d) return;
    struct dirent* ent;
    while ((ent = readdir(d)) != NULL && h->count < MAX_WATCH_FILES) {
        if (ent->d_name[0] == '.') continue;
        char path[4096];
        snprintf(path, sizeof(path), "%s/%s", dir, ent->d_name);
        int fd = open(path, O_RDONLY);
        if (fd < 0) continue;
        struct kevent ke;
        EV_SET(&ke, fd, EVFILT_VNODE, EV_ADD | EV_CLEAR,
               NOTE_WRITE | NOTE_RENAME | NOTE_DELETE, 0, NULL);
        kevent(h->kq, &ke, 1, NULL, 0, NULL);
        h->fds[h->count] = fd;
        strncpy(h->paths[h->count], path, sizeof(h->paths[0]) - 1);
        h->count++;
    }
    closedir(d);
}

hr_watcher_handle_t* hr_platform_watch_start(const char* dir, hr_file_changed_cb cb, void* userdata) {
    hr_watcher_handle_t* h = calloc(1, sizeof(hr_watcher_handle_t));
    if (!h) return NULL;
    h->kq = kqueue();
    if (h->kq < 0) { free(h); return NULL; }
    h->cb = cb;
    h->userdata = userdata;
    strncpy(h->watch_dir, dir, sizeof(h->watch_dir) - 1);
    scan_dir_fds(h, dir);
    return h;
}

void hr_platform_watch_stop(hr_watcher_handle_t* handle) {
    if (!handle) return;
    for (int i = 0; i < handle->count; i++) close(handle->fds[i]);
    close(handle->kq);
    free(handle);
}

void hr_platform_watch_poll(hr_watcher_handle_t* handle) {
    if (!handle) return;
    struct timespec timeout = {0, 0};
    struct kevent events[32];
    int n = kevent(handle->kq, NULL, 0, events, 32, &timeout);
    for (int i = 0; i < n; i++) {
        int fd = (int)events[i].ident;
        for (int j = 0; j < handle->count; j++) {
            if (handle->fds[j] == fd) {
                handle->cb(handle->paths[j], handle->userdata);
                break;
            }
        }
    }
}

void* hr_platform_alloc_exec(size_t size) {
    void* p = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return (p == MAP_FAILED) ? NULL : p;
}

void hr_platform_free_exec(void* addr, size_t size) {
    munmap(addr, size);
}

int hr_platform_make_writable(void* addr, size_t size) {
    return mprotect(addr, size, PROT_READ | PROT_WRITE) == 0;
}

int hr_platform_make_executable(void* addr, size_t size) {
    return mprotect(addr, size, PROT_READ | PROT_EXEC) == 0;
}

void* hr_platform_lib_open(const char* path) {
    return dlopen(path, RTLD_NOW | RTLD_LOCAL);
}

void* hr_platform_lib_sym(void* handle, const char* name) {
    return dlsym(handle, name);
}

int hr_platform_lib_close(void* handle) {
    return dlclose(handle) == 0;
}

const char* hr_platform_lib_error(void) {
    return dlerror();
}

int hr_platform_file_exists(const char* path) {
    return access(path, F_OK) == 0;
}

int64_t hr_platform_file_mtime(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) return -1;
    return (int64_t)st.st_mtime;
}

int hr_platform_mkdir(const char* path) {
    char tmp[4096];
    snprintf(tmp, sizeof(tmp), "%s", path);
    size_t len = strlen(tmp);
    if (len > 0 && tmp[len-1] == '/') tmp[len-1] = 0;
    for (char* p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    return mkdir(tmp, 0755) == 0 || errno == EEXIST;
}

int hr_platform_run_command(const char* cmd, char* output, size_t output_size) {
    FILE* fp = popen(cmd, "r");
    if (!fp) return -1;
    size_t total = 0;
    if (output && output_size > 0) {
        while (total < output_size - 1) {
            size_t n = fread(output + total, 1, output_size - 1 - total, fp);
            if (n == 0) break;
            total += n;
        }
        output[total] = 0;
    }
    return pclose(fp);
}

void hr_platform_thread_pause_others(void) {}
void hr_platform_thread_resume_others(void) {}

void hr_platform_sleep_ms(int ms) {
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}

const char* hr_platform_lib_ext(void) { return ".dylib"; }
const char* hr_platform_name(void) { return "macos"; }

#endif
