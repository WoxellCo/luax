#include "luax.h"

#define DEBUG_MODE 1

#define local_strlen(s) (sizeof(s) / sizeof((s)[0]))
//#define is_keyword(kw, len) (!strncmp(s, (kw), (len)) && !((s[(len)] >= 'a' && s[(len)] >= 'z') || (s[(len)] >= 'A' && s[(len)] >= 'Z') || (s[(len)] >= '0' && s[(len)] >= '9') || s[(len)] == '_'))
#define is_keyword(kw) (!strncmp(s, (kw), local_strlen(kw)) && !((s[local_strlen(kw)] >= 'a' && s[local_strlen(kw)] >= 'z') || (s[local_strlen(kw)] >= 'A' && s[local_strlen(kw)] >= 'Z') || (s[local_strlen(kw)] >= '0' && s[local_strlen(kw)] >= '9') || s[local_strlen(kw)] == '_'))
#define kw_cond(c) !(((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z') || (c) == '_' /*|| (c) == '.'*/ || ((c) >= '0' && (c) <= '9'))


#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))

#if DEBUG_MODE != 0
#define luax_debug_str_view(sv) {for (size_t i = 0; i < (sv).len; i++) {putc((sv).s[i], stdout);}/*putc('\n', stdout);*/}
#else
#define luax_debug_str_view(sv)
#endif

int aaa = 0;
// if `begin` is null, the first tag is parsed or else it will go directly on the loop
luax_parse_xml_result luax_parse_xml(const luax_str_view txt, const struct luax_tag_s *begin, luax_context *context) {//printf("--------------------<%d>--------------------\n", aaa++);if (aaa > 10) exit(1);
    luax_counters counters = context != NULL ? context->counters : (luax_counters){NULL, NULL};
    if (counters.xml != NULL)
        (*counters.xml)++;
    size_t shift = 0, err = 1;
    luax_element *e = (luax_element *)malloc(sizeof(luax_element));
    list *inner = NULL;

    luax_str_view s = {
        .s = txt.s + shift,
        .len = txt.len - shift
    };

    const luax_parse_xml_tag_result tag_result = begin != NULL ? (luax_parse_xml_tag_result) { NULL, 0 } : luax_parse_xml_tag(s, context);
    const struct luax_tag_s tag = begin != NULL ? *begin : tag_result.result;

    //luax_debug_xml_tag(tag);

    if (tag.flags & LUAX_TAG_ERROR) {
        err = luax_xeno_malformed_tag;
        //break;
        goto luax_parse_xml_immediate_return;
    }

    if (tag.flags & LUAX_TAG_CLOSE) {
       //(">>>>%s\n", ((luax_str_view *)tag.l->head->data)->s);
        err = luax_xeno_unexpected_close_tag;
        goto luax_parse_xml_immediate_return;
    }

    luax_element *root = (luax_element *)malloc(sizeof(luax_element));
    luax_element *out = NULL;

    if (tag.flags & LUAX_TAG_FRAGMENT) {
        root->type = NULL;
        root->props_count = 0;
    } else {
        list_node *tag_ln = tag.l->head;
        struct luax_tag_prop_s *prop = (struct luax_tag_prop_s *)tag_ln->data;
        root->type = luax_sv_to_str(prop->name);

       //("=======[[%s%s%s]]=======\n", (tag.flags & LUAX_TAG_CLOSE) ? "/" : "", root->type, (tag.flags & LUAX_TAG_SELF_CLOSE) ? "/" : "");

        root->props_count = tag.l->size - 1; // -1 cuz the first element is the type
        root->props = (luax_prop *)malloc(tag.l->size * sizeof(luax_prop)); // list size cuz it's already + 1 (null terminated)
        root->props[root->props_count] = luax_null_prop;

        tag_ln = tag_ln->next;
        for (size_t i = 0; tag_ln != NULL; i++) {
            prop = (struct luax_tag_prop_s *)tag_ln->data;
            
            root->props[i] = (luax_prop) {
                .name = luax_sv_to_str(prop->name),
                .value = prop->value_type != luaxt_null ? luax_sv_to_str(prop->value) : NULL,
                .type = prop->value_type
            };

            tag_ln = tag_ln->next;
        }
    }

    root->inner.count = 0;
    root->inner.data = NULL;
    root->inner.types = NULL;
    if (tag.flags & LUAX_TAG_SELF_CLOSE) {
        err = 0;
    }

    size_t n = SIZE_MAX; // total text length (null terminator char included)

    luax_str_view inner_txt = luax_null_sv;
    uint8_t inner_txt_mode = 0;
    uint8_t is_closed = 0;
    // TODO: put these two inside a flags unified variable

    inner = list_new(); // list of `luax_inner`

    shift += tag.n;
    
    if (!(tag.flags & LUAX_TAG_SELF_CLOSE) && !tag_result.error) {
        while (shift < txt.len) {
            char *s = txt.s + shift;
            char c = s[0];

            if ((c == '<' || c == '{') && inner_txt_mode) {
                inner_txt_mode = 0;
                inner_txt.s[inner_txt.len++] = '\0';
                luax_inner_element *e = (luax_inner_element *)malloc(sizeof(luax_inner_element));
                e->type = luaxt_text;
                e->data.text = (luax_str_view) {
                    .s = inner_txt.s + inner_txt.len - n - 1,
                    .len = n
                };
                n = 0;
                list_add(inner, (void *)e);
            }

            if (c == '<') {
                luax_str_view sv = (luax_str_view) {
                    .s = s,
                    .len = txt.len - shift
                };
                luax_parse_xml_tag_result tag_result = luax_parse_xml_tag(sv, context);
                if (tag_result.error) {
                    err = tag_result.error;
                    ////("+++++++++++++++++++++++++");
                    goto luax_parse_xml_immediate_return;
                }
                struct luax_tag_s tag = tag_result.result;
                ////(root->type);
        //printf("++++>%s\n", ((luax_str_view *)tag.l->head->data)->s);
        //printf("------%d | %d |%s|\n", !strncmp(root->type, ((struct luax_tag_prop_s *)tag.l->head->data)->name.s, strlen(root->type)), strlen(root->type), ((struct luax_tag_prop_s *)tag.l->head->data)->name.s);
        //luax_debug_xml_tag(tag);
       //("LUAX_TAG_CLOSE: %d\n", tag.flags & LUAX_TAG_CLOSE);
                if (tag.flags & LUAX_TAG_CLOSE) {
                    //("9999");
                    err = 0;
                    if ((root->type == NULL && (tag.flags & LUAX_TAG_FRAGMENT)) || (!(tag.flags & LUAX_TAG_FRAGMENT) && !strncmp(root->type, ((struct luax_tag_prop_s *)tag.l->head->data)->name.s, strlen(root->type)))) {
                        is_closed = 1;
                        shift += tag.n;
                        break;
                    }
                    err = luax_xeno_unexpected_close_tag;
                    goto luax_parse_xml_immediate_return;
                }
                //sv = (luax_str_view) { sv.s + tag.n, sv.len - tag.n };
                luax_parse_xml_result obj_result = luax_parse_xml(sv, begin, context);
                if (obj_result.error) {
                    err = obj_result.error;
                    goto luax_parse_xml_immediate_return;
                }
                luax_element *obj = obj_result.result.first;
                shift += obj_result.result.second;

                luax_inner_element *e = (luax_inner_element *)malloc(sizeof(luax_inner_element));
                e->type = luaxt_element;
                e->data.element = obj;
                list_add(inner, (void *)e);
            } else if (c == '{') {
                luax_str_view sv = (luax_str_view) {
                    .s = s + 1,
                    .len = txt.len - shift - 1
                };
                luax_transpile_result trans_result = luax_transpile(sv.s, sv.len, context);
                if (trans_result.error) {
                    err = luax_xeno_luax_expr_transpile;
                    goto luax_parse_xml_immediate_return;
                }
                luax_str_view trans = trans_result.result.first;
                size_t es = trans_result.result.second; // expression shift
                shift += trans_result.result.second + 2; // + 2 for some reason it leaves 2 chars like `{2 + 2}` and then next to it appears `2}`
                if (sv.s[es] != '}'/* && sv.s[es] != '\0'*/) {//printf("||%s||\n", sv.s + es);
                    err = luax_xeno_unclosed_expr;
                    goto luax_parse_xml_immediate_return;
                }

                luax_inner_element *e = (luax_inner_element *)malloc(sizeof(luax_inner_element));
                e->type = luaxt_expr;
                e->data.expr = trans;
                list_add(inner, (void *)e);
            } else {
                if (luax_sv_is_null(inner_txt)) {
                    inner_txt.s = (luax_char *)malloc((txt.len - shift + 1) * sizeof(luax_char));
                    inner_txt.s[0] = '\0';
                    inner_txt.len = 0;
                    n = 0;
                }
                n++;
                inner_txt_mode = 1;
                inner_txt.s[inner_txt.len++] = txt.s[shift++];
            }
        }
    } else {
        is_closed = 1;
        root->inner.count = 0;
        root->inner.types = NULL;
        root->inner.data = NULL;
    }

    if (!is_closed) {
        ////("UNCLOSED");
        err = luax_xeno_unclosed_tag;
    }

    luax_parse_xml_immediate_return:

    // wait maybe i need to free them anyway before i return
    /*if (err && inner != NULL) {
        list_node *n = inner->head;
        while (n != NULL) {
            list_node *t = n;
            n = n->next;
            free(t->data);
        }
    }*/

    if (!luax_sv_is_null(inner_txt)) {
        //free(inner_txt.s);
        // TODO: fix the free problem
    }
    
    if (!err) { // pack the data
        root->inner.data = inner->size > 0
                         ? (union luax_inner_data *)malloc((inner->size) * sizeof(union luax_inner_data))
                         : (union luax_inner_data *)NULL;
        root->inner.count = inner->size;
        root->inner.types = (uint8_t *)malloc((inner->size + 1) * sizeof(uint8_t));

        list_node *n = inner->head;
        size_t i = 0;
        while (n != NULL) {
            list_node *t = n;
            n = n->next;

            luax_inner_element *e = (luax_inner_element *)t->data;
            root->inner.types[i] = e->type;
            root->inner.data[i++] = e->data;

            free(t->data);
            free(t);
        }
        list_delete(inner);
        out = root;
    } else if (inner != NULL) {
        list_node *n = inner->head;
        while (n != NULL) {
            list_node *t = n;
            n = n->next;
            free(t);
        }
        list_delete(inner);
    }

    return (luax_parse_xml_result) {
        .result = {
            .first = out,
            .second = shift
        },
        .error = err
    };
}

enum {
    luaxnt_null,
    luaxnt_single_str,
    luaxnt_double_str,
    luaxnt_block_str,
    luaxnt_single_comment,
    luaxnt_block_comment
};

size_t luax_analyze_neutral(const luax_str_view txt) {
    size_t i;
    char neutral_type = luaxnt_null;
    size_t count;
    switch (txt.s[0]) {
    case '"':
        neutral_type = luaxnt_double_str;
        i = 1;
        break;
    case '\'':
        neutral_type = luaxnt_single_str;
        i = 1;
        break;
    case '[':
        if (txt.s[1] == '[') {
            neutral_type = luaxnt_block_str;
            i = 2;
            count = 0;
        } else if (txt.s[1] == '=') {
            neutral_type = luaxnt_block_str;
            size_t j;
            char c;
            for (j = 1; (c = txt.s[j]) != '['; j++) {
                if (c != '=')
                    return 1;
            }
            i = 2 + j;
            count = j;
        }
        break;
    case '-':
        if (!strncmp(txt.s, "--[[", 4)) {
            neutral_type = luaxnt_block_comment;
            i = 4;
            count = 0;
        } else if (!strncmp(txt.s, "--[=", 4)) {
            neutral_type = luaxnt_block_str;
            size_t j;
            char c;
            for (j = 1; (c = txt.s[j]) != '['; j++) {
                if (c != '=')
                    return 1;
            }
            i = 4 + j;
            count = j;
        } else if (!strncmp(txt.s, "--", 2)) {
            neutral_type = luaxnt_single_comment;
            i = 2;
        }
        break;
    default:
        return 0;
        break;
    }

    char c, neutral_mode = 0;
    char terminator_char = '\n';

    switch (neutral_type)
    {
    case luaxnt_null:
        return 0;
        break;
    case luaxnt_single_str:
        //if (terminator_char == '\0') {
            terminator_char = '\'';
        //} 
    case luaxnt_double_str:
        if (terminator_char == '\n') {
            terminator_char = '"';
        }
    case luaxnt_single_comment:
        for (; i < txt.len; i++) {
            c = txt.s[i];
            if (!neutral_mode && (c == terminator_char || c == '\n')) {
                //return i + 1;
                return i;
            } else if (c == '\\') {
                neutral_mode = !neutral_mode;
            } else if (neutral_mode) {
                neutral_mode = 0;
            }
        }
        break;
    case luaxnt_block_str:
    case luaxnt_block_comment:
        if (count == 0) {
            for (; i < txt.len; i++) {
                c = txt.s[i];
                if (c == ']' && !strncmp(txt.s + i, "]]", 2)) {
                    //return i + 2;
                    return i + 1;
                }
            }
        } else {
            for (; i < txt.len; i++) {
                c = txt.s[i];
                if (c == ']' && !strncmp(txt.s + i, "]=", 2)) {
                    size_t j;
                    for (j = 1; (c = txt.s[i + j]) != ']'; j++) {
                        if (c != '=')
                            break;
                    }
                    i += j;
                   //("jjj: %zu\n", j);
                    if (j == count) {
                        //return i + 1;
                        return i;
                    }
                }
            }
        }
        break;
    default:
        return 0;
        break;
    }
    return 0;
}

//#define result luax_result
//#define tuple luax_tuple
//#define vector luax_vector

//typedef result(vector(struct luax_idx_len), uint8_t) luax_result;
#define kw_list(kw) (luax_str_view){(kw), sizeof(kw) / sizeof((kw)[0]) - 1}
size_t luax_analyze_keyword(const char *s) {
    static luax_str_view keywords[22] = {
        kw_list("return"),
        kw_list("if"),
        kw_list("then"),
        kw_list("do"),
        kw_list("for"),
        kw_list("while"),
        kw_list("elseif"),
        kw_list("else"),
        kw_list("not"),
        kw_list("break"),
        kw_list("repeat"),
        kw_list("until"),
        kw_list("end"),
        kw_list("and"),
        kw_list("or"),
        kw_list("in"),
        kw_list("local"),
        kw_list("false"),
        kw_list("true"),
        kw_list("function"),
        kw_list("goto"),
        kw_list("nil")
    };

    for (size_t i = 0; i < 22; i++) {
        //printf("%s == %.*s\n", keywords[i].s, keywords[i].len, s);
        //printf("%s", keywords[i].s);
        //printf(">%zu\n", keywords[i].len);
        if (!strncmp(keywords[i].s, s, keywords[i].len)) {
            //printf("%zu (%s) =>\t%d\t%s\n",
            //    i,
            //    keywords[i].s,
            //    !strncmp(keywords[i].s, s, keywords[i].len),
            //    !strncmp(keywords[i].s, s, keywords[i].len) ? keywords[i].s : "--");
            return keywords[i].len;
        }
    }
    return 0;
}

/*
if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
                char *s = txt.s + i;
                if (
                    is_keyword("return")
                 || is_keyword("if")
                 || is_keyword("then")
                 || is_keyword("do")
                 || is_keyword("for")
                 || is_keyword("while")
                 || is_keyword("elseif")
                 || is_keyword("else")
                 || is_keyword("not")
                 || is_keyword("break")
                 || is_keyword("repeat")
                 || is_keyword("until")
                 || is_keyword("end")
                 || is_keyword("and")
                 || is_keyword("or")
                 || is_keyword("in")
                 || is_keyword("local")
                 || is_keyword("false")
                 || is_keyword("true")
                 || is_keyword("function")
                 || is_keyword("goto")
                 || is_keyword("nil")
                ) {
                    xmlable = 1;
                } else xmlable = 0;
            }
*/

// analyze the luax script and return the first index of the first xml segment encountered or the string length (if there's no xml)
// update 2026-02-11: it now returns a pair (length + type) since, i added regex literals

luax_analyze_result luax_analyze(const luax_str_view txt, nest_count_t nest_count) {
    size_t i;
    //int nest_count = 0;
    char c, prev_c = '\0', next_c = '\0';
    char xmlable = 1;
    char rgxable = 1; // << useless?
    for (i = 0; i < txt.len /*&& txt.s[i] != '\n'*/; i++) {
        c = txt.s[i];
        next_c = txt.s[i + 1];
        if (c == '<' && xmlable) {
            return (luax_analyze_result){i, luax_st_xml, nest_count};
        } else switch (c) {
        case '\0':
            return (luax_analyze_result){i, luax_st_null, nest_count};
        case '{':
            nest_count++;
            xmlable = 1;
            rgxable = 1;
            break;
        case '}':
            if (!nest_count) {
                return (luax_analyze_result){i, luax_st_null, nest_count};
            } else nest_count--;
            break;
        case '\'':
        case '"':
        case '-':
            i += luax_analyze_neutral((luax_str_view){
                .s = txt.s + i,
                .len = txt.len - i
            });
            xmlable = 1;
            rgxable = 1;
            break;
        case '/':
            if (xmlable) {
                return (luax_analyze_result){i, luax_st_regex, nest_count};
            }
            break;
        case '[':
            if (txt.s[i + 1] == '[' || txt.s[i + 1] == '=') {
                i += luax_analyze_neutral((luax_str_view){
                    .s = txt.s + i,
                    .len = txt.len - i
                });
            }
            xmlable = 1;
            rgxable = 1;
            break;
        case ':':
        case '?':
        case '\n':
        case '>':
        case '~':
        case '=':
        case '(':
        case ',':
            xmlable = 1;
            rgxable = 1;
            break;
        case '<':
            xmlable = next_c != '<' || prev_c == '<';
            rgxable = next_c != '<' || prev_c == '<';
            break;
        case '_':
            xmlable = 0;
            rgxable = 0;
            break;
        case '.':
            if (prev_c == '.') {
                xmlable = 1;
                rgxable = 1;
            } else {
                xmlable = 0;
                rgxable = 0;
            }
            break;
        default:
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
                char *s = txt.s + i;
                size_t kw = luax_analyze_keyword(s);
                //puts("========================================");
                //puts(txt.s + i);
                #define NEW_LEX 1
                #if NEW_LEX
                if (kw > 0 && kw_cond(s[kw])) {
                    //printf("kw: %zu\n", kw);
                    xmlable = 1;
                    rgxable = 1;
                    i += kw - 1;
                    //printf("((((%.20s))))\n", txt.s + i);
                #else
                if (
                    is_keyword("return")
                 || is_keyword("if")
                 || is_keyword("then")
                 || is_keyword("do")
                 || is_keyword("for")
                 || is_keyword("while")
                 || is_keyword("elseif")
                 || is_keyword("else")
                 || is_keyword("not")
                 || is_keyword("break")
                 || is_keyword("repeat")
                 || is_keyword("until")
                 || is_keyword("end")
                 || is_keyword("and")
                 || is_keyword("or")
                 || is_keyword("in")
                 || is_keyword("local")
                 || is_keyword("false")
                 || is_keyword("true")
                 || is_keyword("function")
                 || is_keyword("goto")
                 || is_keyword("nil")
                ) {
                    xmlable = 1;
                #endif
                } else {
                    xmlable = 0;
                    rgxable = 0;
                    #if NEW_LEX
                    char sc;
                    do {
                        sc = txt.s[++i];
                    } while (!kw_cond(sc));
                    i--;
                    #endif
                }
            } else if (c >= '0' && c <= '9') {
                xmlable = 0;
                rgxable = 0;
            }
            break;
        }
        prev_c = c;
    }
    return (luax_analyze_result){i, luax_st_null, nest_count};
}

/*list *luax_get_xml_area(const luax_str_view txt) {
    list *out = list_new();
    char c;
    /*for (size_t i = 0; i < txt.len; i++) {
        c = txt.s[i];
    }/**
    size_t i = 0;
    while (i < txt.len) {
        i += luax_analyze((luax_str_view) {
            .s = txt.s + i,
            .len = txt.len - i
        });
    }
    
    return out;
}*/

//luax_str_view
luax_parse_xml_tag_result luax_parse_xml_tag(const luax_str_view txt, luax_context *context) {
    if (txt.s[0] != '<') {
        return (luax_parse_xml_tag_result) {
            .result = (struct luax_tag_s){
                .l = NULL,
                .n = 0
            },
            .error = 0
        };
    }
    size_t i;
    struct luax_tag_s o = {
        NULL,
        0,
        0
    };

    struct luax_tag_prop_s *prop = NULL;
    size_t sv_last_idx;
    
    char c, phase = luax_xt_start, value_bounds = '\0';
    for (i = 1; i < txt.len; i++) {
    //printf("%d,", phase);
        c = txt.s[i];
        switch (phase)
        {
        case luax_xt_start:
        /*
            < e key = "value">
             ^
        */
            switch (c)
            {
            case '\n':
            case '\r':
            case '\t':
            case ' ':
                // ignore
                break;
            case '/':
                o.flags |= LUAX_TAG_CLOSE;
                break;
            case '>':
                // return fragment
                //("ret fragment");
                o.flags |= LUAX_TAG_FRAGMENT;
                o.n = i + 1;
                return (luax_parse_xml_tag_result) {
                    .result = o,
                    .error = 0
                };
                break;
            case '=':
                // return error
                break;
            default:
                if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
                    o.l = list_new();
                    prop = (struct luax_tag_prop_s *)malloc(sizeof(struct luax_tag_prop_s));
                    list_add(o.l, (void *)prop);
                    prop->name.s = txt.s + i;
                    sv_last_idx = i;
                    phase = luax_xt_type;
                    prop->value_type = luaxpt_null;
                } else {
                    // return error
                    //("err");
                }
                break;
            }
            break;
        case luax_xt_type:
        /*
            <e key = "value">
             ^
        */
            switch (c)
            {
            case '\n':
            case '\r':
            case '\t':
            case ' ':
                phase = luax_xt_type_after;
                prop->name.len = i - sv_last_idx;
                break;
            case '/':
                if (o.flags & LUAX_TAG_CLOSE) {
                    // return error
                }
                o.flags |= LUAX_TAG_SELF_CLOSE;
                phase = luax_xt_self_close;
                prop->name.len = i - sv_last_idx;
                o.n = i + 1;
                break;
            case '>':
                // return tag with no properties
                prop->name.len = i - sv_last_idx;
                o.n = i + 1;
                ////("ret no prop");
                return (luax_parse_xml_tag_result) {
                    .result = o,
                    .error = 0
                };
                break;
            case '=':
                // return error
                break;
            default:
                if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '.' || c == ':') {
                    break;
                } else if (c == '[' || c == ']' || c == '\'' || c == '"') {
                    o.flags |= LUAX_TAG_TYPE_NOT_XML;
                } else {
                    // return error
                }
                break;
            }
            break;
        case luax_xt_type_after:
        /*
            <e key = "value">
              ^
        */
            switch (c)
            {
            case '\n':
            case '\r':
            case '\t':
            case ' ':
                // ignore
                break;
            case '/':
                if (o.flags & LUAX_TAG_CLOSE) {
                    // return error
                }
                o.flags |= LUAX_TAG_SELF_CLOSE;
                phase = luax_xt_self_close;
                break;
            case '>':
                // return tag
                o.n = i + 1;
                ////("ret 1");
                return (luax_parse_xml_tag_result) {
                    .result = o,
                    .error = 0
                };
                break;
            case '=':
                // return error
                break;
            default:
                if (c >= '0' && c <= '9') {
                    // return error
                    break;
                } else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'/* || c == '.' || c == ':' || c == '-'*/) {
                    if (o.flags & LUAX_TAG_CLOSE) {
                        // return error
                        break;
                    }
                    prop = (struct luax_tag_prop_s *)malloc(sizeof(struct luax_tag_prop_s));
                    list_add(o.l, (void *)prop);
                    prop->name.s = txt.s + i;
                    sv_last_idx = i;
                    phase = luax_xt_prop_name;
                    prop->value_type = luaxpt_null;
                    break;
                } else {
                    // return error
                }
                break;
            }
            break;
        case luax_xt_prop_name:
        /*
            <e key = "value">
               ^
        */
            switch (c)
            {
            case '\n':
            case '\r':
            case '\t':
            case ' ':
                // name end
                prop->name.len = i - sv_last_idx;
                phase = luax_xt_prop_name_after;
                prop->value_type = luaxpt_null;
                break;
            case '/':
                if (o.flags & LUAX_TAG_CLOSE) {
                    // return error
                }
                prop->name.len = i - sv_last_idx;
                prop->value_type = luaxpt_null;
                o.flags |= LUAX_TAG_SELF_CLOSE;
                phase = luax_xt_self_close;
                break;
            case '>':
                // return tag
                prop->name.len = i - sv_last_idx;
                prop->value_type = luaxpt_null;
                
                o.n = i + 1;
                //("ret 2");
                return (luax_parse_xml_tag_result) {
                    .result = o,
                    .error = 0
                };
                break;
            case '=':
                prop->name.len = i - sv_last_idx;
                phase = luax_xt_prop_value_before;
                break;
            default:
                if ((c >= '0' && c <= '9')
                ||    
                (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'/* || c == '.' || c == ':' || c == '-'*/) {
                    
                    break;
                } else {
                    // return error
                }
                break;
            }
            break;
        case luax_xt_prop_name_after:
        /*
            <e key = "value">
                  ^
        */
            switch (c)
            {
            case '\n':
            case '\r':
            case '\t':
            case ' ':
                // ignore
                break;
            case '/':
                if (o.flags & LUAX_TAG_CLOSE) {
                    // return error
                }
                prop->name.len = i - sv_last_idx;
                prop->value_type = luaxpt_null;
                o.flags |= LUAX_TAG_SELF_CLOSE;
                phase = luax_xt_self_close;
                break;
            case '>':
                // return tag
                phase = luax_xt_self_close;
                o.n = i + 1;
                //("ret 3");
                return (luax_parse_xml_tag_result) {
                    .result = o,
                    .error = 0
                };
                break;
            case '=':
                phase = luax_xt_prop_value_before;
                break;
            default:
                if ((c >= '0' && c <= '9')
                ||    
                (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'/* || c == '.' || c == ':' || c == '-'*/) {
                    prop->value_type = luaxpt_null;
                    phase = luax_xt_prop_name;
                    prop = (struct luax_tag_prop_s *)malloc(sizeof(struct luax_tag_prop_s));
                    list_add(o.l, (void *)prop);
                    prop->name.s = txt.s + i;
                    sv_last_idx = i;
                    break;
                } else {
                    // return error
                }
                break;
            }
            break;
        case luax_xt_prop_value_before:
        /*
            <e key = "value">
                    ^
        */
            switch (c)
            {
            case '\n':
            case '\r':
            case '\t':
            case ' ':
                // ignore
                break;
            case '=':
            case '>':
                // return error
                //("ret err");
                break;
            case '\'':
            case '"':
                value_bounds = c;
                phase = luax_xt_prop_value;
                prop->value_type = luaxpt_text;
                prop->value.s = txt.s + i + 1;
                sv_last_idx = i + 1;
                break;
            case '{': {
                value_bounds = '}';
                prop->value_type = luaxpt_expr;
                // for later
                /*phase = luax_xt_prop_value;
                prop->value.s = txt.s + i + 1;
                sv_last_idx = i + 1;*/
                luax_transpile_result trans = luax_transpile(txt.s + i + 1, txt.len - i, context);

                if (trans.error) {
                    return (luax_parse_xml_tag_result) {
                        .result = (struct luax_tag_s){
                            .l = NULL,
                            .n = i
                        },
                        .error = luax_xeno_luax_expr_transpile
                    };
                }
                prop->value = trans.result.first;
                i += trans.result.second + 1;
                phase = luax_xt_type_after;
            } break;
            default:
                value_bounds = '\0';
                phase = luax_xt_prop_value;
                prop->value.s = txt.s + i;
                prop->value_type = luaxpt_text;
                sv_last_idx = i;
                break;
            }
            break;
        case luax_xt_prop_value:
        /*
            <e key = "value">
                     ^
        */
            switch (c)
            {
            case '\n':
            case '\r':
            case '\t':
            case ' ':
               //("value_bonds: '%c'\n", value_bounds);
                if (value_bounds == '\0') {
                    phase = luax_xt_type_after;
                    prop->value.len = i - sv_last_idx;
                }
                break;
            case '=':
                if (value_bounds == '\0') {
                    // return error
                }
                break;
            case '\'':
            case '"':
            case '}':
                if (value_bounds == c) {
                    phase = luax_xt_type_after;
                    prop->value.len = i - sv_last_idx;
                }
                break;
            case '>':
                if (value_bounds == '\0') {
                    phase = luax_xt_type_after;
                    prop->value.len = i - sv_last_idx;

                    o.n = i + 1;
                    return (luax_parse_xml_tag_result) {
                        .result = o,
                        .error = 0
                    };
                }
                break;
            default:
                break;
            }
            break;
        case luax_xt_self_close:
            switch (c) {
            case '\n':
            case '\r':
            case '\t':
            case ' ':
                // ignore
                break;
            case '>':
                // return
                o.n = i + 1;
                //("ret 4");
                return (luax_parse_xml_tag_result) {
                    .result = o,
                    .error = 0
                };
            default:
                // return error
                break;
            }
        default:
            break;
        }
    }

    //("end?");
   //("________________%s\n", txt.s);
    return (luax_parse_xml_tag_result) {
        .result = (struct luax_tag_s){
            .l = NULL,
            .n = i
        },
        .error = 0
    };
}

struct luax_xml_s {
    luax_element *top;
    size_t n;
    uint8_t err_no;
};
// NULL and zero if error

char *luax_sv_to_str(const luax_str_view sv) {
    char *s = (char *)malloc((sv.len + 1) * sizeof(char));
    strncpy(s, sv.s, sv.len);
    s[sv.len] = '\0';

    return s;
}

luax_element *luax_xml_list_to_element(list *l) {
    luax_element *e = (luax_element *)malloc(sizeof(luax_element));

    list_node *n = l->head;
    struct luax_tag_prop_s *prop;

    e->inner.data = NULL;
    e->inner.types = NULL;
    e->props = NULL;
    e->props_count = 0;
    e->inner.count = 0;
    if (n == NULL || !l->size) {
        e->type = NULL;
        return e;
    }
    prop = (struct luax_tag_prop_s *)n->data;
    e->type = luax_sv_to_str(prop->name);

    e->props_count = l->size - 1;
    //e->props = malloc()

    while (n = n->next) {
        prop = (struct luax_tag_prop_s *)n->data;
    }

    return e;
}

struct luax_xml_s luax_analyze_xml(const luax_str_view txt, luax_context *context) {
    if (txt.len < 3) return (struct luax_xml_s){NULL, 0};
    if (txt.s[0] != '<') return (struct luax_xml_s){NULL, 0};
    //if (txt.s[1] >= 'a' && txt.s[1] <= 'z' && txt.s[1] >= 'A' && txt.s[1] <= 'Z' && txt.s[1] >= '0' && txt.s[1] <= '9') return 0;

    char c, prev_c = '<';

    luax_parse_xml_tag_result tag_result = luax_parse_xml_tag(txt, context);
    struct luax_tag_s tag = tag_result.result;

    size_t i = tag.n;

    if (tag.flags & LUAX_TAG_CLOSE) {
        return (struct luax_xml_s){
            NULL, 0, luax_xeno_unexpected_close_tag
        };
    } else if (tag.flags & LUAX_TAG_ERROR) {
        return (struct luax_xml_s){
            NULL, 0, luax_xeno_malformed_tag
        };
    } else if (tag.flags & LUAX_TAG_SELF_CLOSE) {
        
    }

    stack *in_elements = stack_new();
    stack_push(in_elements, &tag);

    for (i = i + 1; i < txt.len; i++) {
        c = txt.s[i];

        

        prev_c = c;
    }
}

// n => delimit length
// max_size => max available memory
size_t luax_sanitize_escape(char *s, const char *src, size_t n, size_t max_size) {
    char c;
    size_t len = 0, i = 0;
    //printf("(c = %d) != '\0' && i < n - 1 && len < max_size - 1", src[i], && i < n - 1 && len < max_size - 1);
    while ((c = src[i]) != '\0' && i < n - 1 && len < max_size - 1) {
        switch (c)
        {
        case '\r':
            s[len++] = '\\';
            s[len++] = 'r';
            break;
        case '\t':
            s[len++] = '\\';
            s[len++] = 't';
            break;
        case '\n':
            s[len++] = '\\';
            s[len++] = 'n';
            break;
        case '\"':
        case '\\':
            s[len++] = '\\';
        default:
            s[len++] = c;
            break;
        }
        ++i;
    }
    s[len] = '\0';
    return len;
}

size_t luax_to_function(const luax_element *xml, size_t max_len, luax_str_view *out) {
    if (out == NULL) {
        out = (luax_str_view *)malloc(sizeof(luax_str_view));
        out->s = (luax_char *)malloc((max_len + 1) * sizeof(luax_char));
        out->len = 0;
        out->s[0] = '\0';
    }
    size_t old_len = out->len;
    
    char *buffers[3] = {
        (char *)malloc((max_len + 1) * sizeof(luax_char)),
        (char *)malloc((max_len + 1) * sizeof(luax_char)),
        (char *)malloc((max_len + 1) * sizeof(luax_char)),
    };
    buffers[0][0] = buffers[1][0] = buffers[2][0] = '\0';
    //size_t len = 0;

    if (xml->type != NULL) {
        snprintf(buffers[0], max_len, "%s, \"%s\"", xml->type, xml->type);
    } else {
        //snprintf(buffers[0], max_len, "", xml->type, xml->type);
        buffers[0][0] = '\0';
    }

    size_t shift = 0;
    for (size_t i = 0; i < xml->props_count; i++) {
        if (xml->props[i].type == luaxpt_text) {
            size_t buffer_len = luax_sanitize_escape(buffers[2], xml->props[i].value, max_len, max_len);
            shift += snprintf(buffers[1] + shift, max_len, "[\"%s\"]=\"%s\", ", xml->props[i].name, buffers[2]);
        } else if (xml->props[i].type == luaxpt_expr) {
            shift += snprintf(buffers[1] + shift, max_len, "[\"%s\"]=(%s), ", xml->props[i].name, xml->props[i].value);
        }
    }

    out->len += snprintf(out->s + out->len, max_len, "LuaX.createElement({%s}, {%s}, {", buffers[0], buffers[1]);

    shift = 0;
    if (xml->inner.count > 0) {
        for (size_t i = 0; i < xml->inner.count; i++) {
            size_t buffer_len;
            switch (xml->inner.types[i])
            {
            case luaxt_element:
                /*out->len += */luax_to_function(xml->inner.data[i].element, max_len - out->len, out);
                out->s[out->len++] = ',';
                out->s[out->len++] = ' ';
                out->s[out->len] = '\0';
                break;
            case luaxt_expr:
                strncpy(buffers[2], xml->inner.data[i].expr.s, (xml->inner.data[i].expr.len) < max_len ? (xml->inner.data[i].expr.len) : max_len);
                buffers[2][(xml->inner.data[i].expr.len) < max_len ? (xml->inner.data[i].expr.len) : max_len] = '\0';
                out->len += snprintf(out->s + out->len, max_len, "(%s), ", buffers[2]);
                break;
            case luaxt_text:
                buffer_len = luax_sanitize_escape(buffers[2], xml->inner.data[i].text.s, max_len, max_len);
                //out->len += snprintf(out->s + out->len, max_len, "[[%s]], ", xml->inner.data[i].text.s);
                out->len += snprintf(out->s + out->len, max_len, "\"%s\", ", buffers[2]);
                break;
            default:
                break;
            }
        }
    }
    out->s[out->len++] = '}';
    out->s[out->len++] = ')';
    out->s[out->len] = '\0';

    free(buffers[0]);
    free(buffers[1]);
    free(buffers[2]);

    return out->len - old_len;
}

/*luax_char *luax_transpile(const luax_char *txt, size_t max_length) {
    
}*/

luax_transpile_result luax_transpile(const luax_char *txt, size_t max_length, luax_context *context) {
    luax_counters counters = context != NULL ? context->counters : (luax_counters){NULL, NULL};
    //luax_char *s = (luax_char *)malloc((max_length + 1) * sizeof(luax_char));
    size_t shift = 0;
    //list *segments = NULL; // list of luax_xml_segment OR (2026-02-11) regex
    luax_segment_list segments = (luax_segment_list){NULL, NULL, 0};
    uint8_t err = 0;

    size_t out_length = 0;
    int nest_count = 0;
    while (shift < max_length) {
        luax_str_view txt_sv = (luax_str_view) {
            .s = txt + shift,
            .len = max_length - shift
        };
        luax_analyze_result ar = luax_analyze(txt_sv, nest_count);
        //size_t len = ar.first;
        size_t len = ar.length;
        size_t i = shift += len;
        out_length += len;
        nest_count = ar.nest_count;
        //printf("nest_count: %d\n---\n", nest_count);
        if (shift >= max_length || txt[shift] == '\0' || (txt[shift] == '}' && nest_count <= 0) || /*ar.second*/(ar.segment_type == luax_st_null && nest_count <= 0))
            break;
        txt_sv = (luax_str_view) {
            .s = txt + shift,
            .len = max_length - shift
        };
        /*if (segments == NULL) {
            segments = list_new();
        }*/
        struct luax_segment_node n;
        //printf(">%u\n", ar.second);
        //printf("rex: %u\n", luax_st_regex);
        //printf("xml: %zu\n", luax_st_xml);
        if (/*ar.second*/ar.segment_type == luax_st_xml) {

            luax_parse_xml_result xml_result = luax_parse_xml(txt_sv, NULL, context);
            if (xml_result.error) {
                return (luax_transpile_result) {
                    .result = luax_null_sv,
                    .error = xml_result.error
                };
            }

            luax_element *xml = xml_result.result.first;
            luax_xml_segment *segment = (luax_xml_segment *)malloc(sizeof(luax_xml_segment));

            /**segment = (luax_xml_segment) {
                .xml = xml,
                .range = {
                    .idx = i,
                    .len = xml_result.result.second
                }
            };*/

            n = (struct luax_segment_node){
                .type = ar.segment_type,
                .range = {
                    .idx = i,
                    .len = xml_result.result.second
                },
                .data.xml = xml,
                .next = NULL
            };
        } else if (ar.segment_type == luax_st_regex) {
            struct regex_analyze_literal_s r = regex_analyze_literal(txt_sv.s, /*REGEX_NOCOMP*/0, counters.rex != NULL ? (*counters.rex)++ : 0);
            //debug_regex(&r.result);
            htmw_crv_assign_uninit(context->crv, r.result.ref_id - 1, r.result);
            #if 0
            {
                PCRE2_SIZE size;
                pcre2_pattern_info(r.result.re, PCRE2_INFO_SIZE, &size);
                pcre2_pattern_info(context->crv->data[r.result.ref_id - 1].rex, PCRE2_INFO_SIZE, &size);
                printf("Compiled size: %zu bytes || addr: %p || ref_id: %zu\n", size, r.result, r.result.ref_id);
            }
            //debug_htmw_crv(context->crv, *context->counters.rex - 1);
            #endif

            n = (struct luax_segment_node){
                .type = ar.segment_type,
                .range = {
                    .idx = i,
                    .len = r.length
                },
                .data.rex = r.result,
                .next = NULL
            };
        }
        luax_sl_add(&segments, n);
        //list_add(segments, (void *)segment);
        //luax_sl_emplace(segments, ar.second, range, xml);

        //printf("shift: %zu => (((%s)))\n", shift, txt + shift);
        //shift += xml_result.result.second;
        shift += n.range.len;
        //printf("shift: %zu => (((%s)))\n", shift, txt + shift);

        //out_length += xml_result.result
    }

    //printf("list size: %zu\n", segments->size);

    luax_transpile_result out;

    size_t i = 0;
    luax_str_view o = luax_null_sv;
    if (segments.head != NULL) {
        o = (luax_str_view) {
            .s = (luax_char *)malloc((2 * max_length + 1) * sizeof(luax_char)),
            .len = 0
        };
        o.s[0] = '\0';
        
        struct luax_segment_node *n = segments.head;
        size_t is = 0, os = 0, ls = 0; // input string shift and output string shift
        while (n != NULL) {
            strncpy(o.s + o.len, txt + is, is + n->range.idx < max_length ? is + n->range.idx : max_length); // *1
            //printf("MAX_LEN: %zu\n", );
            if (n->type == luax_st_xml) {
                //luax_xml_segment *segment = n->data;
                luax_element *e = n->data.xml;
                //n = n->next;////("aaaaaaaaaaa");
                
                //strncat(o.s, txt + is, segment->range.idx < max_length ? segment->range.idx : max_length);
                
                //// ^^^^^^^ *1
                o.len = os + n->range.idx;// + segment->range.len;
                //o.len += segment->range.len;
                is = n->range.idx + n->range.len;
                //ls = 
                //printf("===================================================o.len: %zu\n", n->range.len);
                //printf("#########%s>\n", txt + is);
                o.s[o.len] = '\0';
                //os += il->idx + il->len;

                ////(o.s);

                // TODO: free segments and xml

                //printf("length: %zu\n", o.len);
                //luax_debug_xml(segment->xml, 0, 4);
                size_t t = luax_to_function(e, max_length, &o) - n->range.len;
                os += t;
                //printf("---os: %zu\n", os);
            } else if (n->type == luax_st_regex) {
                regex r = n->data.rex;
                char *buffer = (char *)malloc((r.length * 2 + 1) * sizeof(char));
                // TODO: keep the buffer as a persistent block allocated outside and resize if it exceeds the limit
                char temp = r.pattern[r.length];
                r.pattern[r.length] = '\0';
                size_t buffer_length = luax_sanitize_escape(buffer, r.pattern, r.length * 2, max_length);
                r.pattern[r.length] = temp;
                o.len = os + n->range.idx;
                is = n->range.idx + n->range.len;
                o.s[o.len] = '\0';
                //printf("len: %zu, r.pattern: /%s/\n", r.length, buffer);
                size_t t = snprintf(o.s + o.len, is + n->range.idx < max_length ? is + n->range.idx : max_length,
                    "Regex.new(\"%.*s\", %d, %s, %zu)",
                    (int)buffer_length,
                    buffer,
                    (int)r.flags,
                    r.g ? "true" : "false",
                    r.ref_id
                );
                //printf("t2: %zu\n", t);
                //is = n->range.idx + n->range.len;
                os += t - n->range.len;
                o.len += t;
                //puts(o.s + o.len);
                //puts(o.s);
                free(buffer);
            }
            n = n->next;
        }
        strncpy(o.s + o.len, txt + is, min(shift + os - is, max_length));
        
        o.len = min(shift + os, max_length);
        o.s[o.len] = '\0';
    } else return (luax_transpile_result) {
        .result = {
            .first = {
                .s = txt,
                .len = shift
            },
            .second = shift
        },
        .error = err
    };
    return (luax_transpile_result) {
        .result = {
            .first = o,
            .second = shift
        },
        .error = err
    };
}

void luax_debug_xml(const luax_element *xml, size_t indent, size_t indent_spacing) {
    char *sp = (char *)malloc((indent * indent_spacing + 1) * sizeof(char));
    char *usp = (char *)malloc((indent_spacing + 1) * sizeof(char));
    memset(sp, ' ', indent * indent_spacing * sizeof(char));
    memset(usp, ' ', indent_spacing * sizeof(char));
    sp[indent * indent_spacing] = '\0';
    usp[indent_spacing] = '\0';

   //("%s<%s(%zu)%s>\n", sp, ((xml->type) ? xml->type : ""), xml->inner.count, (xml->inner.count > 0 ? "" : "/"));
    if (xml->inner.count > 0) {
        for (size_t i = 0; i < xml->inner.count; i++) {
            switch (xml->inner.types[i])
            {
            case luaxt_element:
                luax_debug_xml(xml->inner.data[i].element, indent + 1, indent_spacing);
                break;
            case luaxt_expr:
               //("%s%s{%s}\n", sp, usp, xml->inner.data[i].expr.s);
                break;
            case luaxt_text:
               //("%s%s%s\n", sp, usp, xml->inner.data[i].text.s);
                break;
            default:
                break;
            }
        }
       //("%s</%s>\n", sp, ((xml->type) ? xml->type : ""));
    }
    free(sp);
    free(usp);
}

void luax_debug_xml_tag(struct luax_tag_s tag_data) {
   //("%p\n", tag_data.l);

   //("LUAX_TAG_ERROR: %d\n", !!(tag_data.flags & LUAX_TAG_ERROR));
   //("LUAX_TAG_CLOSE: %d\n", !!(tag_data.flags & LUAX_TAG_CLOSE));
   //("LUAX_TAG_SELF_CLOSE: %d\n", !!(tag_data.flags & LUAX_TAG_SELF_CLOSE));

    if (!(tag_data.flags & LUAX_TAG_FRAGMENT))
    for (size_t i = 0; i < list_size(tag_data.l); i++) {
        struct luax_tag_prop_s *prop = list_get(tag_data.l, i);

        switch (prop->value_type)
        {
        case luaxpt_null:
            luax_debug_str_view(prop->name);
            break;
        case luaxpt_text:
            luax_debug_str_view(prop->name);
            luax_debug_str_view(prop->value);
            break;
        case luaxpt_expr:
            luax_debug_str_view(prop->name);
            luax_debug_str_view(prop->value);
            break;
        default:
            break;
        }
    }
}


void luax_sl_add(luax_segment_list *l, struct luax_segment_node data) {
    struct luax_segment_node *n = (struct luax_segment_node *)malloc(sizeof(struct luax_segment_node));
    if (l->tail != NULL) {
        l->tail->next = n;
        l->tail = n;
        l->size++;
    } else {
        l->head = n;
        l->tail = n;
        l->size = 0;
    }
    *n = data;
}