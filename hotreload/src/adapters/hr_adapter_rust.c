#include "hr_adapter.h"
#include "../platform/hr_platform.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static int rust_detect(const char* path) {
    const char* ext = strrchr(path, '.');
    return ext && strcmp(ext, ".rs") == 0;
}

static int rust_compile(const char* src, const char* out, const char* flags) {
    char cmd[8192];
    snprintf(cmd, sizeof(cmd),
        "rustc --crate-type=cdylib -C opt-level=0 -C debuginfo=2 %s \"%s\" -o \"%s\" 2>&1",
        flags ? flags : "", src, out);
    char errbuf[4096] = {0};
    int ret = hr_platform_run_command(cmd, errbuf, sizeof(errbuf));
    if (ret != 0) fprintf(stderr, "[hr:rust] compile error:\n%s\n", errbuf);
    return ret == 0;
}

static int rust_list_symbols(const char* lib, char* out, size_t sz) {
    char cmd[4096];
    snprintf(cmd, sizeof(cmd), "nm -D --defined-only \"%s\" 2>&1", lib);
    return hr_platform_run_command(cmd, out, sz) == 0;
}

static char* rust_demangle(const char* name) {
    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "echo '%s' | rustfilt 2>/dev/null", name);
    char out[1024] = {0};
    int ret = hr_platform_run_command(cmd, out, sizeof(out));
    if (ret != 0) return strdup(name);
    size_t len = strlen(out);
    while (len > 0 && (out[len-1] == '\n' || out[len-1] == '\r')) out[--len] = 0;
    return strdup(len > 0 ? out : name);
}

hr_adapter_t hr_adapter_rust = {
    .lang         = HR_LANG_RUST,
    .name         = "Rust",
    .source_ext   = ".rs",
    .detect       = rust_detect,
    .compile      = rust_compile,
    .list_symbols = rust_list_symbols,
    .demangle     = rust_demangle,
};
