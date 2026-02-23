#include "hr_differ.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static char* read_file(const char* path, size_t* out_size) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    char* buf = malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, (size_t)sz, f);
    buf[sz] = 0;
    fclose(f);
    if (out_size) *out_size = (size_t)sz;
    return buf;
}

static int contains_keyword(const char* src, const char* kw) {
    return strstr(src, kw) != NULL;
}

static int has_header_change(const char* diff_content) {
    return contains_keyword(diff_content, "struct ")  ||
           contains_keyword(diff_content, "typedef ") ||
           contains_keyword(diff_content, "#include ") ||
           contains_keyword(diff_content, "#define ");
}

static int has_signature_change(const char* old_src, const char* new_src) {
    const char* o = old_src;
    const char* n = new_src;
    while (*o && *n) {
        if (*o == '{' && *n == '{') break;
        if (*o != *n) return 1;
        o++; n++;
    }
    return 0;
}

hr_change_type_t hr_differ_analyze(const char* old_path, const char* new_path) {
    if (!old_path || !new_path) return HR_CHANGE_FULL;

    size_t old_sz, new_sz;
    char* old_src = read_file(old_path, &old_sz);
    char* new_src = read_file(new_path, &new_sz);

    if (!old_src || !new_src) {
        free(old_src); free(new_src);
        return HR_CHANGE_FULL;
    }

    if (old_sz == new_sz && memcmp(old_src, new_src, old_sz) == 0) {
        free(old_src); free(new_src);
        return HR_CHANGE_NONE;
    }

    hr_change_type_t result = HR_CHANGE_BODY_ONLY;

    if (has_header_change(new_src) && !has_header_change(old_src))
        result = HR_CHANGE_STRUCT;
    else if (has_signature_change(old_src, new_src))
        result = HR_CHANGE_SIGNATURE;

    free(old_src);
    free(new_src);
    return result;
}
