#ifndef HR_SYMBOLS_H
#define HR_SYMBOLS_H

#include <stddef.h>
#include <stdint.h>

#define HR_MAX_SYMBOLS 512
#define HR_MAX_NAME    256

typedef struct {
    char     name[HR_MAX_NAME];
    void*    current_addr;
    void*    original_addr;
    uint64_t checksum;
    int      patched;
} hr_symbol_t;

typedef struct {
    hr_symbol_t entries[HR_MAX_SYMBOLS];
    int         count;
} hr_symbol_table_t;

void         hr_symbols_init(hr_symbol_table_t* table);
void         hr_symbols_clear(hr_symbol_table_t* table);
hr_symbol_t* hr_symbols_find(hr_symbol_table_t* table, const char* name);
hr_symbol_t* hr_symbols_add(hr_symbol_table_t* table, const char* name, void* addr);
void         hr_symbols_update(hr_symbol_table_t* table, const char* name, void* new_addr);
uint64_t     hr_symbols_checksum_fn(void* addr, size_t len);

#endif
