#include "hr_adapter.h"
#include "../platform/hr_platform.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static int go_detect(const char* path) {
    const char* ext = strrchr(path, '.');
    return ext && strcmp(ext, ".go") == 0;
}

static int go_compile(const char* src, const char* out, const char* flags) {
    char cmd[8192];
    snprintf(cmd, sizeof(cmd),
        "go build -buildmode=c-shared %s -o \"%s\" \"%s\" 2>&1",
        flags ? flags : "", out, src);
    char errbuf[4096] = {0};
    int ret = hr_platform_run_command(cmd, errbuf, sizeof(errbuf));
    if (ret != 0) fprintf(stderr, "[hr:go] compile error:\n%s\n", errbuf);
    return ret == 0;
}

static int go_list_symbols(const char* lib, char* out, size_t sz) {
    char cmd[4096];
    snprintf(cmd, sizeof(cmd), "nm -D --defined-only \"%s\" 2>&1", lib);
    return hr_platform_run_command(cmd, out, sz) == 0;
}

static char* go_demangle(const char* name) {
    return strdup(name);
}

hr_adapter_t hr_adapter_go = {
    .lang         = HR_LANG_GO,
    .name         = "Go",
    .source_ext   = ".go",
    .detect       = go_detect,
    .compile      = go_compile,
    .list_symbols = go_list_symbols,
    .demangle     = go_demangle,
};
