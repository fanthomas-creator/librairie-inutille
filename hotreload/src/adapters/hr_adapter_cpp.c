#include "hr_adapter.h"
#include "../platform/hr_platform.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static int cpp_detect(const char* path) {
    const char* ext = strrchr(path, '.');
    return ext && (strcmp(ext, ".cpp") == 0 || strcmp(ext, ".cc") == 0 || strcmp(ext, ".cxx") == 0);
}

static int cpp_compile(const char* src, const char* out, const char* flags) {
    char cmd[8192];
    snprintf(cmd, sizeof(cmd),
        "g++ -shared -fPIC -O0 -fno-inline -g %s \"%s\" -o \"%s\" 2>&1",
        flags ? flags : "", src, out);
    char errbuf[4096] = {0};
    int ret = hr_platform_run_command(cmd, errbuf, sizeof(errbuf));
    if (ret != 0) fprintf(stderr, "[hr:cpp] compile error:\n%s\n", errbuf);
    return ret == 0;
}

static int cpp_list_symbols(const char* lib, char* out, size_t sz) {
    char cmd[4096];
    snprintf(cmd, sizeof(cmd), "nm -D --defined-only \"%s\" 2>&1", lib);
    return hr_platform_run_command(cmd, out, sz) == 0;
}

static char* cpp_demangle(const char* name) {
    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "echo '%s' | c++filt 2>/dev/null", name);
    char out[1024] = {0};
    hr_platform_run_command(cmd, out, sizeof(out));
    size_t len = strlen(out);
    while (len > 0 && (out[len-1] == '\n' || out[len-1] == '\r')) out[--len] = 0;
    return strdup(len > 0 ? out : name);
}

hr_adapter_t hr_adapter_cpp = {
    .lang         = HR_LANG_CPP,
    .name         = "C++",
    .source_ext   = ".cpp",
    .detect       = cpp_detect,
    .compile      = cpp_compile,
    .list_symbols = cpp_list_symbols,
    .demangle     = cpp_demangle,
};
