// (c) Woxell Co.
// this file contains some wrapper functions to analyze and process regex with PCRE2

#ifndef REGEX_H
#define REGEX_H

#define PCRE2_CODE_UNIT_WIDTH 8
#define ENABLE_PCRE2 1

#include <stdint.h>
#include <stddef.h>
#if ENABLE_PCRE2
#include <pcre2.h>
#endif
#include <string.h>

#define REGEXF_G        (1 << 0)
#if ENABLE_PCRE2
#define REGEXF_M        PCRE2_MULTILINE
#define REGEXF_I        PCRE2_CASELESS
#define REGEXF_S        PCRE2_DOTALL
#define REGEXF_X        PCRE2_EXTENDED
#define REGEXF_U        PCRE2_UTF
#else
#define REGEXF_M        (1 << 1)
#define REGEXF_I        (1 << 2)
#define REGEXF_S        (1 << 3)
#define REGEXF_X        (1 << 4)
#define REGEXF_U        (1 << 5)
#endif

#define REGEX_STRLEN    (1 << 0)
#define REGEX_NOCOMP    (1 << 1)

typedef struct {
    #if ENABLE_PCRE2
    pcre2_code *re;
    #endif
    char *pattern;
    size_t length;
    uint8_t g;
    uint32_t flags;
    size_t ref_id;
} regex;

struct regex_result_node {
    size_t idx;
    size_t len;
    struct regex_result_node *next;
};

typedef struct {
    struct regex_result_node *head;
    struct regex_result_node *tail;
    size_t size;
} regex_result_list;

struct regex_analyze_literal_s {
    regex result;  // the actual result
    size_t length; // how many characters did it take
};

// list of results in case global
//list *

struct regex_analyze_literal_s regex_analyze_literal(char *lit, uint8_t flags, size_t id);
#if ENABLE_PCRE2
regex_result_list regex_process(const regex *r, const char *s, size_t max_len, uint8_t flags);
#endif

#endif