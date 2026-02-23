#include "hr_symbols.h"
#include <string.h>
#include <stdlib.h>

void hr_symbols_init(hr_symbol_table_t* table) {
    memset(table, 0, sizeof(*table));
}

void hr_symbols_clear(hr_symbol_table_t* table) {
    table->count = 0;
}

hr_symbol_t* hr_symbols_find(hr_symbol_table_t* table, const char* name) {
    for (int i = 0; i < table->count; i++) {
        if (strcmp(table->entries[i].name, name) == 0)
            return &table->entries[i];
    }
    return NULL;
}

hr_symbol_t* hr_symbols_add(hr_symbol_table_t* table, const char* name, void* addr) {
    if (table->count >= HR_MAX_SYMBOLS) return NULL;
    hr_symbol_t* sym = &table->entries[table->count++];
    strncpy(sym->name, name, HR_MAX_NAME - 1);
    sym->current_addr  = addr;
    sym->original_addr = addr;
    sym->checksum      = hr_symbols_checksum_fn(addr, 64);
    sym->patched       = 0;
    return sym;
}

void hr_symbols_update(hr_symbol_table_t* table, const char* name, void* new_addr) {
    hr_symbol_t* sym = hr_symbols_find(table, name);
    if (sym) {
        sym->current_addr = new_addr;
        sym->checksum     = hr_symbols_checksum_fn(new_addr, 64);
    } else {
        hr_symbols_add(table, name, new_addr);
    }
}

uint64_t hr_symbols_checksum_fn(void* addr, size_t len) {
    if (!addr) return 0;
    uint64_t hash = 14695981039346656037ULL;
    unsigned char* p = (unsigned char*)addr;
    for (size_t i = 0; i < len; i++) {
        hash ^= (uint64_t)p[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}
