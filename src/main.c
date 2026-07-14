#include <stdio.h>
#include "luax.h"

int main() {
    compiled_regex_vector crv = htmw_crv_new(HTMW_INIT_MEMV_SIZE);
    size_t crv_size = 1;

    luax_context ctx = (luax_context){
        .crv = &crv,
        .counters = {
            .rex = &crv_size,
            .xml = NULL
        }
    };

    char s[10000] = "\0";
    FILE *f = fopen("t.luax", "r");

    while (fgets(s + strlen(s), 10000, f));

    fclose(f);

    luax_transpile_result o = luax_transpile(s, 10000, &ctx);
    if (o.error) {
        printf("err! (%ld)\n", o.error);
        exit(1);
    }

    luax_str_view sv = o.result.first;

    f = fopen("t.lua", "w");

    sv.s[sv.len] = '\0';
    //printf("processed characters: %zu\noutput length: %zu\n", o.result.second, o.result.first.len);

    fprintf(f, "%s", sv.s);

    fclose(f);

    return 0;
}