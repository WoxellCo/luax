#ifndef HTMW_MEMORY_H
#define HTMW_MEMORY_H

#include <stddef.h>
#include <stdint.h>
#include "luax_regex.h"

#define HTMW_INIT_MEMV_SIZE 1024

typedef struct {
    uint8_t flags;
    pcre2_code *rex;
} compiled_regex_vector_data;

typedef struct {
    size_t capacity;
    compiled_regex_vector_data *data;
} compiled_regex_vector;

#define nullcrv ((compiled_regex_vector){ 0, NULL })

void htmw_init_pcre2_allocator();

compiled_regex_vector htmw_crv_new(size_t capacity);
void htmw_crv_reserve(compiled_regex_vector *v, size_t capacity);
void htmw_crv_assign_uninit(compiled_regex_vector *v, size_t at, regex rex);

#endif