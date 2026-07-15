#include <stdio.h>
#include <stdint.h>
#include <luax.h>

#define BUFFER_SIZE 100000
#define DISPLAY_VER (1 << 0)

typedef struct {
    char *input_file_path;
    char *output_file_path; // if null apply the input file path with the new extension (.lua)
    uint8_t flags;
} luax_config;

typedef struct {
    size_t path;
    size_t extension;
} path_lengths;

path_lengths get_path_lengths(const char *path) {
    path_lengths len = {0, 0};

    for (len.path = 0; path[len.path] != '\0'; len.path++)
        if (path[len.path] == '.')
            len.extension = len.path;

    len.extension = len.path - len.extension;
    return len;
}

luax_config init_config(int argc, char *argv[]) {
    luax_config config = {
        NULL, NULL, 0
    };
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            switch (argv[i][1])
            {
            case '-':
                fprintf(stderr, "luaxc error: long flags are not supported yet\n");
                exit(1);
                break;
            case 'v': if ((config.flags & DISPLAY_VER) == 0) {
                printf("LuaX 0.3 by Woxell\n\n");
                config.flags |= DISPLAY_VER;
            } break;
            default:
                break;
            }
        } else {
            if (config.input_file_path == NULL) {
                config.input_file_path = argv[i];
            } else {
                fprintf(stderr, "luaxc error: input file already specified\n");
                exit(1);
            }
        }
    }

    if (config.input_file_path != NULL) {
        path_lengths current_path_lenghts = get_path_lengths(config.input_file_path);
        config.output_file_path = (char *)malloc((current_path_lenghts.path) * sizeof(char));
        sprintf(config.output_file_path, "%.*s.%s", (int)(current_path_lenghts.path - current_path_lenghts.extension), config.input_file_path, "lua");
    }

    return config;
}

int main(int argc, char *argv[]) {
    luax_config config = init_config(argc, argv);

    if (config.input_file_path == NULL) {
        if (config.flags & DISPLAY_VER)
            exit(0);

        fprintf(stderr, "luaxc error: no input file specified\n");
        exit(1);
    }

    compiled_regex_vector crv = htmw_crv_new(HTMW_INIT_MEMV_SIZE);
    size_t crv_size = 1;

    luax_context ctx = (luax_context){
        .crv = &crv,
        .counters = {
            .rex = &crv_size,
            .xml = NULL
        }
    };

    char s[BUFFER_SIZE] = "";
    FILE *f = fopen(config.input_file_path, "r");

    while (fgets(s + strlen(s), BUFFER_SIZE, f));

    fclose(f);

    luax_transpile_result o = luax_transpile(s, BUFFER_SIZE, &ctx);
    if (o.error) {
        printf("err! (%ld)\n", o.error);
        exit(1);
    }

    luax_str_view sv = o.result.first;

    f = fopen(config.output_file_path, "w");

    sv.s[sv.len] = '\0';
    //printf("processed characters: %zu\noutput length: %zu\n", o.result.second, o.result.first.len);

    fprintf(f, "%s", sv.s);

    fclose(f);

    return 0;
}