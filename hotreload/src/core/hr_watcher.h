#ifndef HR_WATCHER_H
#define HR_WATCHER_H

typedef void (*hr_watcher_cb)(const char* path, void* userdata);

typedef struct hr_watcher hr_watcher_t;

hr_watcher_t* hr_watcher_create(const char* dir, hr_watcher_cb cb, void* userdata);
void          hr_watcher_destroy(hr_watcher_t* w);
void          hr_watcher_poll(hr_watcher_t* w);

#endif
