#include "hr_adapter.h"
#include "../platform/hr_platform.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static int c_detect(const char* path) {
    const char* ext = strrchr(path, '.');
    return ext && strcmp(ext, ".c") == 0;
}

static int c_compile(const char* src, const char* out, const char* flags) {
    char cmd[8192];
    const char* compiler = "gcc";
    snprintf(cmd, sizeof(cmd),
        "%s -shared -fPIC -O0 -fno-inline -g %s \"%s\" -o \"%s\" 2>&1",
        compiler, flags ? flags : "", src, out);
    char errbuf[4096] = {0};
    int ret = hr_platform_run_command(cmd, errbuf, sizeof(errbuf));
    if (ret != 0) {
        fprintf(stderr, "[hr:c] compile error:\n%s\n", errbuf);
    }
    return ret == 0;
}

static int c_list_symbols(const char* lib, char* out, size_t sz) {
    char cmd[4096];
    snprintf(cmd, sizeof(cmd), "nm -D --defined-only \"%s\" 2>&1", lib);
    return hr_platform_run_command(cmd, out, sz) == 0;
}

static char* c_demangle(const char* name) {
    return strdup(name);
}

hr_adapter_t hr_adapter_c = {
    .lang         = HR_LANG_C,
    .name         = "C",
    .source_ext   = ".c",
    .detect       = c_detect,
    .compile      = c_compile,
    .list_symbols = c_list_symbols,
    .demangle     = c_demangle,
};
