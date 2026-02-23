#include "hr_watcher.h"
#include "../platform/hr_platform.h"
#include <stdlib.h>
#include <string.h>

struct hr_watcher {
    hr_watcher_handle_t* platform_handle;
    hr_watcher_cb        cb;
    void*                userdata;
};

hr_watcher_t* hr_watcher_create(const char* dir, hr_watcher_cb cb, void* userdata) {
    hr_watcher_t* w = calloc(1, sizeof(hr_watcher_t));
    if (!w) return NULL;
    w->cb       = cb;
    w->userdata = userdata;
    w->platform_handle = hr_platform_watch_start(dir, cb, userdata);
    if (!w->platform_handle) { free(w); return NULL; }
    return w;
}

void hr_watcher_destroy(hr_watcher_t* w) {
    if (!w) return;
    hr_platform_watch_stop(w->platform_handle);
    free(w);
}

void hr_watcher_poll(hr_watcher_t* w) {
    if (!w) return;
    hr_platform_watch_poll(w->platform_handle);
}
