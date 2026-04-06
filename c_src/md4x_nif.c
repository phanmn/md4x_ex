#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <erl_nif.h>
#include "md4x.h"
#include "renderers/md4x-html.h"
#include "renderers/md4x-ast.h"
#include "renderers/md4x-meta.h"
#include "renderers/md4x-ansi.h"
#include "renderers/md4x-text.h"
#include "renderers/md4x-markdown.h"
#include "renderers/md4x-heal.h"

// Output buffer structure for collecting renderer output
typedef struct {
    char *data;
    size_t size;
    size_t capacity;
} OutputBuffer;

// Initialize output buffer
static void init_buffer(OutputBuffer *buf) {
    buf->capacity = 4096;
    buf->data = (char *)malloc(buf->capacity);
    buf->size = 0;
    if (buf->data) {
        buf->data[0] = '\0';
    }
}

// Free output buffer
static void free_buffer(OutputBuffer *buf) {
    if (buf->data) {
        free(buf->data);
        buf->data = NULL;
    }
    buf->size = 0;
    buf->capacity = 0;
}

// Callback wrapper for md_html, md_ast, md_meta
static void output_callback(const MD_CHAR *chunk, MD_SIZE chunk_size, void *userdata) {
    OutputBuffer *buf = (OutputBuffer *)userdata;
    
    if (!buf || !buf->data || !chunk || chunk_size == 0) {
        return;
    }
    
    size_t new_size = buf->size + chunk_size;
    
    // Expand buffer if needed
    if (new_size >= buf->capacity) {
        size_t new_capacity = buf->capacity * 2;
        while (new_capacity <= new_size) {
            new_capacity *= 2;
        }
        
        char *new_data = (char *)realloc(buf->data, new_capacity);
        if (!new_data) {
            return; // Out of memory
        }
        
        buf->data = new_data;
        buf->capacity = new_capacity;
    }
    
    // Append chunk to buffer
    memcpy(buf->data + buf->size, chunk, chunk_size);
    buf->size = new_size;
    buf->data[buf->size] = '\0';
}

// Helper function to create an OK tuple with a binary
static ERL_NIF_TERM make_ok_binary(ErlNifEnv *env, const char *data, size_t size) {
    ERL_NIF_TERM bin_term;
    unsigned char *bin_data = enif_make_new_binary(env, size, &bin_term);
    
    if (!bin_data) {
        fprintf(stderr, "Failed to allocate binary\n");
        return enif_make_tuple2(env, enif_make_atom(env, "error"), 
                                enif_make_atom(env, "enomem"));
    }
    
    memcpy(bin_data, data, size);
    ERL_NIF_TERM ok_atom = enif_make_atom(env, "ok");
    return enif_make_tuple2(env, ok_atom, bin_term);
}



// Helper to parse options map
// For simplicity, we just check if the second argument is a non-empty map
// and assume it contains heal: true
static unsigned parse_options(ErlNifEnv *env, ERL_NIF_TERM opts) {
    unsigned flags = 0;
    
    // Simple approach: if opts is provided and is not an empty list,
    // we enable the heal flag
    // This is a simplified implementation for compatibility
    if (enif_is_map(env, opts)) {
        size_t map_size;
        enif_get_map_size(env, opts, &map_size);
        
        if (map_size > 0) {
            // For now, just enable heal if any options are provided
            // A more sophisticated implementation would parse the actual options
            flags |= 0x0100; // MD_*_FLAG_HEAL
        }
    }
    
    return flags;
}

// NIF: render_to_html
static ERL_NIF_TERM nif_render_to_html(ErlNifEnv *env, int argc, const ERL_NIF_TERM *argv) {
    if (argc < 1 || argc > 2) {
        return enif_make_badarg(env);
    }
    
    ErlNifBinary bin;
    if (enif_inspect_binary(env, argv[0], &bin) == 0) {
        return enif_make_tuple2(env, enif_make_atom(env, "error"), 
                                enif_make_atom(env, "invalid_input"));
    }
    
    size_t markdown_len = bin.size;
    const char *markdown = (const char *)bin.data;
    
    unsigned renderer_flags = 0;
    if (argc == 2) {
        renderer_flags = parse_options(env, argv[1]);
    }
    
    OutputBuffer buf;
    init_buffer(&buf);
    
    if (!buf.data) {
        return enif_make_tuple2(env, enif_make_atom(env, "error"), enif_make_atom(env, "enomem"));
    }
    
    // Render to HTML using callback
    int result = md_html(markdown, markdown_len, output_callback, &buf, 
                          MD_DIALECT_ALL, renderer_flags);
    
    ERL_NIF_TERM term;
    
    if (result == 0) {
        term = make_ok_binary(env, buf.data, buf.size);
    } else {
        term = enif_make_tuple2(env, enif_make_atom(env, "error"), enif_make_atom(env, "render_failed"));
    }
    
    free_buffer(&buf);
    return term;
}

// NIF: render_to_ast
static ERL_NIF_TERM nif_render_to_ast(ErlNifEnv *env, int argc, const ERL_NIF_TERM *argv) {
    if (argc < 1 || argc > 2) {
        return enif_make_badarg(env);
    }
    
    ErlNifBinary bin;
    if (enif_inspect_binary(env, argv[0], &bin) == 0) {
        return enif_make_tuple2(env, enif_make_atom(env, "error"), 
                                enif_make_atom(env, "invalid_input"));
    }
    
    size_t markdown_len = bin.size;
    const char *markdown = (const char *)bin.data;
    
    unsigned renderer_flags = 0;
    if (argc == 2) {
        renderer_flags = parse_options(env, argv[1]);
    }
    
    OutputBuffer buf;
    init_buffer(&buf);
    
    if (!buf.data) {
        return enif_make_tuple2(env, enif_make_atom(env, "error"), enif_make_atom(env, "enomem"));
    }
    
    // Render to AST (JSON) using callback
    int result = md_ast(markdown, markdown_len, output_callback, &buf,
                         MD_DIALECT_ALL, renderer_flags);
    
    ERL_NIF_TERM term;
    
    if (result == 0) {
        term = make_ok_binary(env, buf.data, buf.size);
    } else {
        term = enif_make_tuple2(env, enif_make_atom(env, "error"), enif_make_atom(env, "render_failed"));
    }
    
    free_buffer(&buf);
    return term;
}

// NIF: extract_meta
static ERL_NIF_TERM nif_extract_meta(ErlNifEnv *env, int argc, const ERL_NIF_TERM *argv) {
    if (argc < 1 || argc > 2) {
        return enif_make_badarg(env);
    }
    
    ErlNifBinary bin;
    if (enif_inspect_binary(env, argv[0], &bin) == 0) {
        return enif_make_tuple2(env, enif_make_atom(env, "error"), 
                                enif_make_atom(env, "invalid_input"));
    }
    
    size_t markdown_len = bin.size;
    const char *markdown = (const char *)bin.data;
    
    unsigned renderer_flags = 0;
    if (argc == 2) {
        renderer_flags = parse_options(env, argv[1]);
    }
    
    OutputBuffer buf;
    init_buffer(&buf);
    
    if (!buf.data) {
        return enif_make_tuple2(env, enif_make_atom(env, "error"), enif_make_atom(env, "enomem"));
    }
    
    // Extract metadata using callback
    int result = md_meta(markdown, markdown_len, output_callback, &buf,
                          MD_DIALECT_ALL, renderer_flags);
    
    ERL_NIF_TERM term;
    
    if (result == 0) {
        term = make_ok_binary(env, buf.data, buf.size);
    } else {
        term = enif_make_tuple2(env, enif_make_atom(env, "error"), enif_make_atom(env, "render_failed"));
    }
    
    free_buffer(&buf);
    return term;
}

// NIF: render_to_ansi
static ERL_NIF_TERM nif_render_to_ansi(ErlNifEnv *env, int argc, const ERL_NIF_TERM *argv) {
    if (argc < 1 || argc > 2) {
        return enif_make_badarg(env);
    }
    
    ErlNifBinary bin;
    if (enif_inspect_binary(env, argv[0], &bin) == 0) {
        return enif_make_tuple2(env, enif_make_atom(env, "error"), 
                                enif_make_atom(env, "invalid_input"));
    }
    
    size_t markdown_len = bin.size;
    const char *markdown = (const char *)bin.data;
    
    unsigned renderer_flags = 0;
    if (argc == 2) {
        renderer_flags = parse_options(env, argv[1]);
    }
    
    OutputBuffer buf;
    init_buffer(&buf);
    
    if (!buf.data) {
        return enif_make_tuple2(env, enif_make_atom(env, "error"), enif_make_atom(env, "enomem"));
    }
    
    int result = md_ansi(markdown, markdown_len, output_callback, &buf,
                         MD_DIALECT_ALL, renderer_flags);
    
    ERL_NIF_TERM term;
    
    if (result == 0) {
        term = make_ok_binary(env, buf.data, buf.size);
    } else {
        term = enif_make_tuple2(env, enif_make_atom(env, "error"), enif_make_atom(env, "render_failed"));
    }
    
    free_buffer(&buf);
    return term;
}

// NIF: render_to_text
static ERL_NIF_TERM nif_render_to_text(ErlNifEnv *env, int argc, const ERL_NIF_TERM *argv) {
    if (argc < 1 || argc > 2) {
        return enif_make_badarg(env);
    }
    
    ErlNifBinary bin;
    if (enif_inspect_binary(env, argv[0], &bin) == 0) {
        return enif_make_tuple2(env, enif_make_atom(env, "error"), 
                                enif_make_atom(env, "invalid_input"));
    }
    
    size_t markdown_len = bin.size;
    const char *markdown = (const char *)bin.data;
    
    unsigned renderer_flags = 0;
    if (argc == 2) {
        renderer_flags = parse_options(env, argv[1]);
    }
    
    OutputBuffer buf;
    init_buffer(&buf);
    
    if (!buf.data) {
        return enif_make_tuple2(env, enif_make_atom(env, "error"), enif_make_atom(env, "enomem"));
    }
    
    int result = md_text(markdown, markdown_len, output_callback, &buf,
                         MD_DIALECT_ALL, renderer_flags);
    
    ERL_NIF_TERM term;
    
    if (result == 0) {
        term = make_ok_binary(env, buf.data, buf.size);
    } else {
        term = enif_make_tuple2(env, enif_make_atom(env, "error"), enif_make_atom(env, "render_failed"));
    }
    
    free_buffer(&buf);
    return term;
}

// NIF: render_to_markdown
static ERL_NIF_TERM nif_render_to_markdown(ErlNifEnv *env, int argc, const ERL_NIF_TERM *argv) {
    if (argc < 1 || argc > 2) {
        return enif_make_badarg(env);
    }
    
    ErlNifBinary bin;
    if (enif_inspect_binary(env, argv[0], &bin) == 0) {
        return enif_make_tuple2(env, enif_make_atom(env, "error"), 
                                enif_make_atom(env, "invalid_input"));
    }
    
    size_t markdown_len = bin.size;
    const char *markdown = (const char *)bin.data;
    
    unsigned renderer_flags = 0;
    if (argc == 2) {
        renderer_flags = parse_options(env, argv[1]);
    }
    
    OutputBuffer buf;
    init_buffer(&buf);
    
    if (!buf.data) {
        return enif_make_tuple2(env, enif_make_atom(env, "error"), enif_make_atom(env, "enomem"));
    }
    
    int result = md_markdown(markdown, markdown_len, output_callback, &buf,
                             MD_DIALECT_ALL, renderer_flags);
    
    ERL_NIF_TERM term;
    
    if (result == 0) {
        term = make_ok_binary(env, buf.data, buf.size);
    } else {
        term = enif_make_tuple2(env, enif_make_atom(env, "error"), enif_make_atom(env, "render_failed"));
    }
    
    free_buffer(&buf);
    return term;
}

// NIF: heal
static ERL_NIF_TERM nif_heal(ErlNifEnv *env, int argc, const ERL_NIF_TERM *argv) {
    if (argc != 1) {
        return enif_make_badarg(env);
    }
    
    ErlNifBinary bin;
    if (enif_inspect_binary(env, argv[0], &bin) == 0) {
        return enif_make_tuple2(env, enif_make_atom(env, "error"), 
                                enif_make_atom(env, "invalid_input"));
    }
    
    size_t markdown_len = bin.size;
    const char *markdown = (const char *)bin.data;
    
    OutputBuffer buf;
    init_buffer(&buf);
    
    if (!buf.data) {
        return enif_make_tuple2(env, enif_make_atom(env, "error"), enif_make_atom(env, "enomem"));
    }
    
    int result = md_heal(markdown, markdown_len, output_callback, &buf);
    
    ERL_NIF_TERM term;
    
    if (result == 0) {
        term = make_ok_binary(env, buf.data, buf.size);
    } else {
        term = enif_make_tuple2(env, enif_make_atom(env, "error"), enif_make_atom(env, "heal_failed"));
    }
    
    free_buffer(&buf);
    return term;
}

// NIF load callback
static int load(ErlNifEnv *env, void **priv_data, ERL_NIF_TERM load_info) {
    (void)env;
    (void)priv_data;
    (void)load_info;
    return 0;
}

// NIF unload callback
static void unload(ErlNifEnv *env, void *priv_data) {
    (void)env;
    (void)priv_data;
}

// NIF function list
static ErlNifFunc nif_funcs[] = {
    {"render_to_html", 2, nif_render_to_html, 0},
    {"render_to_ast", 2, nif_render_to_ast, 0},
    {"extract_meta", 2, nif_extract_meta, 0},
    {"render_to_ansi", 2, nif_render_to_ansi, 0},
    {"render_to_text", 2, nif_render_to_text, 0},
    {"render_to_markdown", 2, nif_render_to_markdown, 0},
    {"heal", 1, nif_heal, 0}
};

// NIF module initialization
ERL_NIF_INIT(Elixir.Md4xEx, nif_funcs, load, NULL, NULL, unload)
