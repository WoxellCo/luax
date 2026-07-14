#include "luax_memory.h"

void htmw_init_pcre2_allocator() {
    // coming soon
}

compiled_regex_vector htmw_crv_new(size_t capacity) {
    // TODO: handle allocation error
    compiled_regex_vector v = {
        .capacity = capacity,
        .data = (compiled_regex_vector_data *)calloc(capacity, sizeof(compiled_regex_vector_data))
    };
    //printf("%p", v.data);
    return v;
}

void htmw_crv_reserve(compiled_regex_vector *v, size_t capacity) {
    // TODO: handle allocation error
    //printf("CAP: %zu\n", capacity);
    if (capacity <= v->capacity)
        return;

    v->data = (compiled_regex_vector_data *)realloc(v->data, capacity * sizeof(compiled_regex_vector_data));
    memset(v->data + v->capacity, 0, (capacity - v->capacity) * sizeof(compiled_regex_vector_data));

    v->capacity = capacity;
}

void htmw_crv_assign_uninit(compiled_regex_vector *v, size_t at, regex rex) {
    //printf("AT: %zu\n", at);
    //printf("V CORE: %p\n", v->data);
    if (at >= v->capacity)
        htmw_crv_reserve(v, v->capacity * 2);
    
    if (v->data[at].flags == 0) {
        //pcre2_jit_compile(rex->re, PCRE2_JIT_COMPLETE);
        pcre2_jit_compile(rex.re, PCRE2_JIT_COMPLETE);

        v->data[at].rex = rex.re;
        v->data[at].flags |= (rex.flags & REGEXF_G) ? (1 << 0) : 0;
        v->data[at].flags |= (1 << 7);
    }
}