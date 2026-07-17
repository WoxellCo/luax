#ifndef LUAX_H
#define LUAX_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "nx/list.h"
#include "nx/stack.h"

#include "luax_regex.h"

#include "luax_memory.h"

#define luax_result(r, e) \
struct { \
    r result; \
    e error; \
}

// luax_##r##_##e##_result
//luax_ptr_len

#define luax_vector(t) \
struct { \
    t *v; \
    size_t len; \
}

#define luax_pair(t1, t2) \
struct { \
    t1 first; \
    t2 second; \
}

#define LUAX_TAG_SELF_CLOSE         (1 << 0)
#define LUAX_TAG_FRAGMENT           (1 << 1)
#define LUAX_TAG_CLOSE              (1 << 2)
#define LUAX_TAG_ERROR              (1 << 3)
#define LUAX_TAG_TYPE_NOT_XML       (1 << 3)

//#define LUAX_TRANS_BRACES_DELIM     (1 << 0)

/*#define luax_sl_add(l, data) ;{\
    struct luax_segment_node *n = (struct luax_segment_node *)malloc(sizeof(struct luax_segment_node));\
    if ((l).tail != NULL) {\
        (l).tail->next = n;\
        (l).tail = n;\
    } else {\
        (l).head = n;\
        (l).tail = n;\
    }\
    *n = (struct luax_segment_node)(data);\
}*/

/*#define luax_sl_emplace(l, type, range, data) {\
    struct luax_segment_node n;\
    n.t##ype = (type);\
    n.r##ange = (range);\
    n.d##ata = (data);\
    n.n##ext = NULL;\
    #if ((type) == luax_st_xml)\
        n.da##ta.xml = (data);\
    #elif ((type) == luax_st_regex)\
        n.da##ta.rex = (data);\
    #endif\
    luax_sl_add((l), n);\
}*/

int test();

typedef char luax_char;

enum {
    luaxt_null,
    luaxt_text,
    luaxt_expr,
    luaxt_element
};

enum {
    luaxpt_null,
    luaxpt_text,
    luaxpt_expr
};

typedef struct {
    char *name;
    char *value;
    uint8_t /*(luaxpt)*/ type;
} luax_prop;

#define luax_null_prop ((luax_prop) {(char *)NULL, (char *)NULL, luaxpt_null })
#define luax_prop_is_null(prop) ((prop).name == (char *)NULL && (prop).type == luaxpt_null)

typedef struct {
    luax_char *s;
    size_t len;
} luax_str_view;

#define luax_null_sv ((luax_str_view) {(luax_char *)NULL, 0})
#define luax_sv_is_null(sv) ((sv).s == NULL)

struct luax_tag_s {
    list *l;
    size_t n;
    uint8_t flags;
};

typedef struct {
    uint8_t /*(luaxt)*/ *types; // null terminated
    union luax_inner_data *data; // array of data
    size_t count;
} luax_inner;

typedef struct luax_element_s {
    char *type; // NULL => fragment
    luax_inner inner;
    luax_prop *props; // null terminated
    size_t props_count;
} luax_element;

union luax_inner_data { // i know `text` and `expr` are technically the same here, but i still separated them to generate less confustion
    luax_str_view text;
    luax_str_view expr;
    luax_element *element;
};

typedef struct {
    uint8_t type;
    union luax_inner_data data;
} luax_inner_element;

enum {
    luax_xt_start,
    luax_xt_type,
    luax_xt_type_after,
    luax_xt_prop_name,
    luax_xt_prop_name_after,
    luax_xt_prop_value_before,
    luax_xt_prop_value,
    luax_xt_self_close
};

enum {
    luax_xeno_null,
    luax_xeno_malformed_tag,
    luax_xeno_unexpected_close_tag,
    luax_xeno_unclosed_tag,
    luax_xeno_luax_expr_transpile,
    luax_xeno_unclosed_expr
};

// segment type
enum luax_st {
    luax_st_null,
    luax_st_xml,
    luax_st_regex
};

struct luax_tag_prop_s {
    luax_str_view name;
    luax_str_view value;
    uint8_t value_type;
};

struct luax_idx_len {
    size_t idx;
    size_t len;
};

typedef struct {
    struct luax_idx_len range;
    luax_element *xml;
} luax_xml_segment;

struct luax_segment_node {
    uint8_t type; // luax_st
    struct luax_idx_len range;
    union {
        luax_element *xml;
        regex rex;
    } data;
    struct luax_segment_node *next;
};

typedef struct {
    struct luax_segment_node *head;
    struct luax_segment_node *tail;
    size_t size;
} luax_segment_list;

// <string, processed input length>
typedef luax_result(luax_pair(luax_element *, size_t), size_t) luax_parse_xml_result;
// <<string, output length>, processed input length>
typedef luax_result(luax_pair(luax_str_view, size_t), size_t) luax_transpile_result;
typedef luax_result(struct luax_tag_s, size_t) luax_parse_xml_tag_result;
//typedef luax_pair(size_t, enum luax_st) luax_analyze_result;

typedef int nest_count_t;

typedef struct {
    size_t length;
    enum luax_st segment_type;
    int nest_count;
} luax_analyze_result;

typedef struct {
    size_t *xml;
    size_t *rex;
} luax_counters;

typedef struct {
    luax_counters counters;
    compiled_regex_vector *crv;
} luax_context;

#define luax_nullctx ((luax_context){ { NULL, NULL }, NULL })


struct luax_xml_s luax_analyze_xml(const luax_str_view txt, luax_context *context);
luax_parse_xml_result luax_parse_xml(const luax_str_view txt, const struct luax_tag_s *begin, luax_context *context);
luax_parse_xml_tag_result luax_parse_xml_tag(const luax_str_view txt, luax_context *context);
luax_transpile_result luax_transpile(const luax_char *txt, size_t max_length, luax_context *context);
char *luax_sv_to_str(const luax_str_view sv);
void luax_debug_xml_tag(struct luax_tag_s tag_data);

void luax_sl_add(luax_segment_list *l, struct luax_segment_node data);

#ifdef __cplusplus
}
#endif
#endif