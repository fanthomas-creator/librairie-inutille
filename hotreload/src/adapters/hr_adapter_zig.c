#include "hr_adapter.h"
#include "../platform/hr_platform.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static int zig_detect(const char* path) {
    const char* ext = strrchr(path, '.');
    return ext && strcmp(ext, ".zig") == 0;
}

static int zig_compile(const char* src, const char* out, const char* flags) {
    char cmd[8192];
    snprintf(cmd, sizeof(cmd),
        "zig build-lib -dynamic -O Debug %s \"%s\" --name hr_module 2>&1 && mv libhr_module%s \"%s\"",
        flags ? flags : "", src, hr_platform_lib_ext(), out);
    char errbuf[4096] = {0};
    int ret = hr_platform_run_command(cmd, errbuf, sizeof(errbuf));
    if (ret != 0) fprintf(stderr, "[hr:zig] compile error:\n%s\n", errbuf);
    return ret == 0;
}

static int zig_list_symbols(const char* lib, char* out, size_t sz) {
    char cmd[4096];
    snprintf(cmd, sizeof(cmd), "nm -D --defined-only \"%s\" 2>&1", lib);
    return hr_platform_run_command(cmd, out, sz) == 0;
}

static char* zig_demangle(const char* name) {
    return strdup(name);
}

hr_adapter_t hr_adapter_zig = {
    .lang         = HR_LANG_ZIG,
    .name         = "Zig",
    .source_ext   = ".zig",
    .detect       = zig_detect,
    .compile      = zig_compile,
    .list_symbols = zig_list_symbols,
    .demangle     = zig_demangle,
};
