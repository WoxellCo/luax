#include "luax_regex.h"

#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))

// non-const char ptr because the function borrows `lit`
struct regex_analyze_literal_s regex_analyze_literal(char *lit, uint8_t flags, size_t ref_id) {
    regex o = {
        #if ENABLE_PCRE2
        .re = NULL,
        #endif
        .pattern = NULL,
        .g = 0,
        .flags = 0,
        .length = 0,
        .ref_id = ref_id
    };
    size_t idx = 0;
    if (lit[0] != '/') {
        return (struct regex_analyze_literal_s){
            .result = o,
            .length = 0
        };
    }
    //uint8_t valid = 0;
    size_t square_nest = 0;
    uint8_t escaping = 0;
    char c;
    while ((c = lit[++idx]) != '\0') { // analyze pattern + boundary `/`
        if (!escaping) {
            if (c == '/' && !square_nest) {
                o.length = idx - 1;
                o.pattern = lit + 1;
                break;
            } else if (c == '[') {
                square_nest++;
            } else if (c == ']') {
                if (square_nest > 0) {
                    square_nest--;
                } // => `/ xyz ] /`
            } else if (c == '\\') {
                escaping = 1;
            }
        } else {
            escaping = 0;
        }
    }
    if (o.length == 0) { // never reached the end delimiter
        return (struct regex_analyze_literal_s){
            .result = o,
            .length = 0
        };
    }

    // is it really necessary?
    char *new_str = (char *)malloc((o.length + 1) * sizeof(char));
    strncpy(new_str, o.pattern, o.length);
    new_str[o.length] = '\0';
    o.pattern = new_str;
    //

    while ((c = lit[++idx]) != '\0') { // analyze flags
        switch (c)
        {
        case 'g': if (!(o.g & REGEXF_G)) {
            o.g |= REGEXF_G;
        } else {
            goto regex_analyze_out;
        } break;
        case 'i': if (!(o.flags & REGEXF_I)) {
            o.flags |= REGEXF_I;
        } else {
            goto regex_analyze_out;
        } break;
        case 'm': if (!(o.flags & REGEXF_M)) {
            o.flags |= REGEXF_M;
        } else {
            goto regex_analyze_out;
        } break;
        case 'u': if (!(o.flags & REGEXF_U)) {
            o.flags |= REGEXF_U;
        } else {
            goto regex_analyze_out;
        } break;
        case 'x': if (!(o.flags & REGEXF_X)) {
            o.flags |= REGEXF_X;
        } else {
            goto regex_analyze_out;
        } break;
        case 's': if (!(o.flags & REGEXF_S)) {
            o.flags |= REGEXF_S;
        } else {
            goto regex_analyze_out;
        } break;
        // TODO: handle all flags
        default:
            goto regex_analyze_out;
            break;
        }
    }
    regex_analyze_out:
    //char temp = o.pattern[o.length];
    //o.pattern[o.length] = '\0';
    #if ENABLE_PCRE2
    if (!(flags & REGEX_NOCOMP)) {
        PCRE2_SIZE err_code, err_offset;
        o.re = pcre2_compile(
            (PCRE2_SPTR)o.pattern,
            o.length,
            o.flags,
            (int *)&err_code,
            &err_offset,
            NULL
        );
        if (!o.re) { // if it errors
            return (struct regex_analyze_literal_s){
                .result = o,
                .length = 0
            };
        }
        if (o.ref_id > 0) {

        }
    }
    #endif
    //o.pattern[o.length] = temp;
    return (struct regex_analyze_literal_s){
        .result = o,
        .length = idx
    };
}

#if ENABLE_PCRE2
regex_result_list regex_process(const regex *r, const char *s, size_t max_len, uint8_t flags) {
    regex_result_list o = (regex_result_list){
        .head = NULL,
        .tail = NULL,
        .size = 0
    };

    size_t shift = 0;
    size_t str_len = SIZE_MAX;

    if (flags & REGEX_STRLEN)
        str_len = strlen(s);

    size_t len = min(max_len, str_len);
    size_t remain = len;

    pcre2_match_data *match_data =
        pcre2_match_data_create_from_pattern(r->re, NULL);

    while (shift < len) {
        int rc = pcre2_match(
            r->re,
            s,
            len - shift,
            shift,
            PCRE2_NOTEMPTY | ((shift > 0 && s[shift - 1] != '\n' && s[shift - 1] != '\r') ? PCRE2_NOTBOL : 0),
            match_data,
            NULL
        );

        if (rc <= 0) break;

        PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(match_data); // elaborates the result and returns a tuple?

        struct regex_result_node *n = (struct regex_result_node *)malloc(sizeof(struct regex_result_node));
        n->idx = ovector[0];
        n->len = ovector[1] - ovector[0];

        shift += ovector[1];

        if (o.head == NULL) {
            o.tail->next = n;
            o.tail = n;
            o.size++;
        } else {
            o.head = n;
            o.tail = n;
            o.size = 1;
        }

        if (!r->g) break;
    }

    pcre2_match_data_free(match_data);

    return o;
}
#endif