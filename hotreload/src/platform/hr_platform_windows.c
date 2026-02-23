#ifdef _WIN32

#include "hr_platform.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

struct hr_watcher_handle {
    HANDLE dir_handle;
    HANDLE stop_event;
    hr_file_changed_cb cb;
    void* userdata;
    char watch_dir[4096];
    OVERLAPPED overlapped;
    char notify_buf[65536];
};

hr_watcher_handle_t* hr_platform_watch_start(const char* dir, hr_file_changed_cb cb, void* userdata) {
    hr_watcher_handle_t* h = calloc(1, sizeof(hr_watcher_handle_t));
    if (!h) return NULL;
    h->dir_handle = CreateFileA(dir, FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
    if (h->dir_handle == INVALID_HANDLE_VALUE) { free(h); return NULL; }
    h->stop_event = CreateEvent(NULL, TRUE, FALSE, NULL);
    h->cb = cb;
    h->userdata = userdata;
    strncpy(h->watch_dir, dir, sizeof(h->watch_dir) - 1);
    memset(&h->overlapped, 0, sizeof(h->overlapped));
    h->overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    ReadDirectoryChangesW(h->dir_handle, h->notify_buf, sizeof(h->notify_buf),
        FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME,
        NULL, &h->overlapped, NULL);
    return h;
}

void hr_platform_watch_stop(hr_watcher_handle_t* handle) {
    if (!handle) return;
    SetEvent(handle->stop_event);
    CancelIo(handle->dir_handle);
    CloseHandle(handle->overlapped.hEvent);
    CloseHandle(handle->stop_event);
    CloseHandle(handle->dir_handle);
    free(handle);
}

void hr_platform_watch_poll(hr_watcher_handle_t* handle) {
    if (!handle) return;
    DWORD bytes;
    if (!GetOverlappedResult(handle->dir_handle, &handle->overlapped, &bytes, FALSE)) return;
    if (bytes == 0) goto requeue;
    FILE_NOTIFY_INFORMATION* info = (FILE_NOTIFY_INFORMATION*)handle->notify_buf;
    while (1) {
        if (info->Action == FILE_ACTION_MODIFIED || info->Action == FILE_ACTION_RENAMED_NEW_NAME) {
            char filename[4096];
            int len = WideCharToMultiByte(CP_UTF8, 0, info->FileName,
                info->FileNameLength / sizeof(WCHAR), filename, sizeof(filename)-1, NULL, NULL);
            filename[len] = 0;
            char full_path[8192];
            snprintf(full_path, sizeof(full_path), "%s\\%s", handle->watch_dir, filename);
            handle->cb(full_path, handle->userdata);
        }
        if (info->NextEntryOffset == 0) break;
        info = (FILE_NOTIFY_INFORMATION*)((char*)info + info->NextEntryOffset);
    }
requeue:
    ResetEvent(handle->overlapped.hEvent);
    memset(handle->notify_buf, 0, sizeof(handle->notify_buf));
    ReadDirectoryChangesW(handle->dir_handle, handle->notify_buf, sizeof(handle->notify_buf),
        FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME,
        NULL, &handle->overlapped, NULL);
}

void* hr_platform_alloc_exec(size_t size) {
    return VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
}

void hr_platform_free_exec(void* addr, size_t size) {
    (void)size;
    VirtualFree(addr, 0, MEM_RELEASE);
}

int hr_platform_make_writable(void* addr, size_t size) {
    DWORD old;
    return VirtualProtect(addr, size, PAGE_READWRITE, &old) != 0;
}

int hr_platform_make_executable(void* addr, size_t size) {
    DWORD old;
    return VirtualProtect(addr, size, PAGE_EXECUTE_READ, &old) != 0;
}

void* hr_platform_lib_open(const char* path) {
    return (void*)LoadLibraryA(path);
}

void* hr_platform_lib_sym(void* handle, const char* name) {
    return (void*)GetProcAddress((HMODULE)handle, name);
}

int hr_platform_lib_close(void* handle) {
    return FreeLibrary((HMODULE)handle) != 0;
}

const char* hr_platform_lib_error(void) {
    static char buf[256];
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, buf, sizeof(buf), NULL);
    return buf;
}

int hr_platform_file_exists(const char* path) {
    return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

int64_t hr_platform_file_mtime(const char* path) {
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &data)) return -1;
    ULARGE_INTEGER ul;
    ul.LowPart = data.ftLastWriteTime.dwLowDateTime;
    ul.HighPart = data.ftLastWriteTime.dwHighDateTime;
    return (int64_t)ul.QuadPart;
}

int hr_platform_mkdir(const char* path) {
    char tmp[4096];
    strncpy(tmp, path, sizeof(tmp)-1);
    for (char* p = tmp+1; *p; p++) {
        if (*p == '/' || *p == '\\') {
            *p = 0;
            CreateDirectoryA(tmp, NULL);
            *p = '\\';
        }
    }
    return CreateDirectoryA(tmp, NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
}

int hr_platform_run_command(const char* cmd, char* output, size_t output_size) {
    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
    HANDLE read_pipe, write_pipe;
    if (!CreatePipe(&read_pipe, &write_pipe, &sa, 0)) return -1;
    SetHandleInformation(read_pipe, HANDLE_FLAG_INHERIT, 0);
    STARTUPINFOA si = { sizeof(si) };
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = write_pipe;
    si.hStdError  = write_pipe;
    si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
    PROCESS_INFORMATION pi;
    char cmd_buf[8192];
    snprintf(cmd_buf, sizeof(cmd_buf), "cmd /c %s", cmd);
    if (!CreateProcessA(NULL, cmd_buf, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(read_pipe); CloseHandle(write_pipe); return -1;
    }
    CloseHandle(write_pipe);
    if (output && output_size > 0) {
        DWORD total = 0, n;
        while (total < (DWORD)output_size - 1 &&
               ReadFile(read_pipe, output+total, (DWORD)(output_size-1-total), &n, NULL) && n > 0)
            total += n;
        output[total] = 0;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exit_code;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(read_pipe);
    return (int)exit_code;
}

void hr_platform_thread_pause_others(void) {}
void hr_platform_thread_resume_others(void) {}

void hr_platform_sleep_ms(int ms) {
    Sleep((DWORD)ms);
}

const char* hr_platform_lib_ext(void) { return ".dll"; }
const char* hr_platform_name(void) { return "windows"; }

#endif
