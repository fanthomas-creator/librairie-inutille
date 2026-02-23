#ifdef __linux__

#include "hr_platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <dirent.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

#define INOTIFY_BUF_SIZE (4096 * (sizeof(struct inotify_event) + 16))

struct hr_watcher_handle {
    int fd;
    int wd;
    hr_file_changed_cb cb;
    void* userdata;
    char watch_dir[4096];
};

hr_watcher_handle_t* hr_platform_watch_start(const char* dir, hr_file_changed_cb cb, void* userdata) {
    hr_watcher_handle_t* h = calloc(1, sizeof(hr_watcher_handle_t));
    if (!h) return NULL;
    h->fd = inotify_init1(IN_NONBLOCK);
    if (h->fd < 0) { free(h); return NULL; }
    h->wd = inotify_add_watch(h->fd, dir, IN_CLOSE_WRITE | IN_MOVED_TO);
    if (h->wd < 0) { close(h->fd); free(h); return NULL; }
    h->cb = cb;
    h->userdata = userdata;
    strncpy(h->watch_dir, dir, sizeof(h->watch_dir) - 1);
    return h;
}

void hr_platform_watch_stop(hr_watcher_handle_t* handle) {
    if (!handle) return;
    inotify_rm_watch(handle->fd, handle->wd);
    close(handle->fd);
    free(handle);
}

void hr_platform_watch_poll(hr_watcher_handle_t* handle) {
    if (!handle) return;
    char buf[INOTIFY_BUF_SIZE] __attribute__((aligned(__alignof__(struct inotify_event))));
    ssize_t len = read(handle->fd, buf, sizeof(buf));
    if (len <= 0) return;
    char* ptr = buf;
    while (ptr < buf + len) {
        struct inotify_event* ev = (struct inotify_event*)ptr;
        if ((ev->mask & IN_CLOSE_WRITE || ev->mask & IN_MOVED_TO) && ev->len > 0) {
            char full_path[8192];
            snprintf(full_path, sizeof(full_path), "%s/%s", handle->watch_dir, ev->name);
            handle->cb(full_path, handle->userdata);
        }
        ptr += sizeof(struct inotify_event) + ev->len;
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
    if (tmp[len-1] == '/') tmp[len-1] = 0;
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

void hr_platform_thread_pause_others(void) {
}

void hr_platform_thread_resume_others(void) {
}

void hr_platform_sleep_ms(int ms) {
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}

const char* hr_platform_lib_ext(void) { return ".so"; }
const char* hr_platform_name(void) { return "linux"; }

#endif
