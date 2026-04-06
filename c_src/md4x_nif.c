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



// Helper to parse boolean value from Erlang term
static int get_bool_value(ErlNifEnv *env, ERL_NIF_TERM val, int default_val) {
    char atom_buf[64];
    
    // Try to get the atom as a string
    if (enif_get_atom(env, val, atom_buf, sizeof(atom_buf), ERL_NIF_LATIN1)) {
        if (strcmp(atom_buf, "true") == 0) {
            return 1;
        } else if (strcmp(atom_buf, "false") == 0) {
            return 0;
        }
    }
    
    return default_val;
}

// Helper to get boolean value from keyword list (list of tuples)
// Elixir keyword lists are passed as [{key, value}, ...]
static int kwlist_get_bool(ErlNifEnv *env, ERL_NIF_TERM list, const char *key, int default_val) {
    if (!enif_is_list(env, list)) {
        return default_val;
    }
    
    // Check if it's an empty list
    ERL_NIF_TERM head, tail = list;
    if (!enif_get_list_cell(env, tail, &head, &tail)) {
        return default_val;
    }
    
    // Iterate through the list
    do {
        // Check if head is a 2-element tuple
        int arity = 2;
        const ERL_NIF_TERM *tuple_items;
        if (enif_get_tuple(env, head, &arity, &tuple_items) && arity == 2) {
            char atom_buf[64];
            if (enif_get_atom(env, tuple_items[0], atom_buf, sizeof(atom_buf), ERL_NIF_LATIN1)) {
                if (strcmp(atom_buf, key) == 0) {
                    return get_bool_value(env, tuple_items[1], default_val);
                }
            }
        }
    } while (enif_get_list_cell(env, tail, &head, &tail));
    
    return default_val;
}

// Parse HTML renderer options from keyword list
static unsigned parse_html_options(ErlNifEnv *env, ERL_NIF_TERM opts) {
    unsigned flags = 0;
    
    if (kwlist_get_bool(env, opts, "heal", 0)) {
        flags |= 0x0100; // MD_HTML_FLAG_HEAL
    }
    
    if (kwlist_get_bool(env, opts, "full", 0)) {
        flags |= 0x0008; // MD_HTML_FLAG_FULL_HTML
    }
    
    if (kwlist_get_bool(env, opts, "debug", 0)) {
        flags |= 0x0001; // MD_HTML_FLAG_DEBUG
    }
    
    if (kwlist_get_bool(env, opts, "verbatim_entities", 0)) {
        flags |= 0x0002; // MD_HTML_FLAG_VERBATIM_ENTITIES
    }
    
    if (kwlist_get_bool(env, opts, "skip_utf8_bom", 0)) {
        flags |= 0x0004; // MD_HTML_FLAG_SKIP_UTF8_BOM
    }
    
    if (kwlist_get_bool(env, opts, "code_meta", 0)) {
        flags |= 0x0010; // MD_HTML_FLAG_CODE_META
    }
    
    return flags;
}

// Parse AST renderer options from keyword list
static unsigned parse_ast_options(ErlNifEnv *env, ERL_NIF_TERM opts) {
    unsigned flags = 0;
    
    if (kwlist_get_bool(env, opts, "debug", 0)) {
        flags |= 0x0001; // MD_AST_FLAG_DEBUG
    }
    
    if (kwlist_get_bool(env, opts, "skip_utf8_bom", 0)) {
        flags |= 0x0002; // MD_AST_FLAG_SKIP_UTF8_BOM
    }
    
    if (kwlist_get_bool(env, opts, "heal", 0)) {
        flags |= 0x0100; // MD_AST_FLAG_HEAL
    }
    
    return flags;
}

// Parse Meta renderer options from keyword list
static unsigned parse_meta_options(ErlNifEnv *env, ERL_NIF_TERM opts) {
    unsigned flags = 0;
    
    if (kwlist_get_bool(env, opts, "debug", 0)) {
        flags |= 0x0001; // MD_META_FLAG_DEBUG
    }
    
    if (kwlist_get_bool(env, opts, "skip_utf8_bom", 0)) {
        flags |= 0x0002; // MD_META_FLAG_SKIP_UTF8_BOM
    }
    
    if (kwlist_get_bool(env, opts, "heal", 0)) {
        flags |= 0x0100; // MD_META_FLAG_HEAL
    }
    
    return flags;
}

// Parse ANSI renderer options from keyword list
static unsigned parse_ansi_options(ErlNifEnv *env, ERL_NIF_TERM opts) {
    unsigned flags = 0;
    
    if (kwlist_get_bool(env, opts, "heal", 0)) {
        flags |= 0x0100; // MD_ANSI_FLAG_HEAL
    }
    
    if (kwlist_get_bool(env, opts, "show_urls", 0)) {
        flags |= 0x0010; // MD_ANSI_FLAG_SHOW_URLS
    }
    
    if (kwlist_get_bool(env, opts, "show_frontmatter", 0)) {
        flags |= 0x0020; // MD_ANSI_FLAG_SHOW_FRONTMATTER
    }
    
    if (kwlist_get_bool(env, opts, "debug", 0)) {
        flags |= 0x0001; // MD_ANSI_FLAG_DEBUG
    }
    
    if (kwlist_get_bool(env, opts, "skip_utf8_bom", 0)) {
        flags |= 0x0002; // MD_ANSI_FLAG_SKIP_UTF8_BOM
    }
    
    if (kwlist_get_bool(env, opts, "no_color", 0)) {
        flags |= 0x0004; // MD_ANSI_FLAG_NO_COLOR
    }
    
    if (kwlist_get_bool(env, opts, "code_meta", 0)) {
        flags |= 0x0008; // MD_ANSI_FLAG_CODE_META
    }
    
    return flags;
}

// Parse Text renderer options from keyword list
static unsigned parse_text_options(ErlNifEnv *env, ERL_NIF_TERM opts) {
    unsigned flags = 0;
    
    if (kwlist_get_bool(env, opts, "debug", 0)) {
        flags |= 0x0001; // MD_TEXT_FLAG_DEBUG
    }
    
    if (kwlist_get_bool(env, opts, "skip_utf8_bom", 0)) {
        flags |= 0x0002; // MD_TEXT_FLAG_SKIP_UTF8_BOM
    }
    
    if (kwlist_get_bool(env, opts, "heal", 0)) {
        flags |= 0x0100; // MD_TEXT_FLAG_HEAL
    }
    
    return flags;
}

// Parse Markdown renderer options from keyword list
static unsigned parse_markdown_options(ErlNifEnv *env, ERL_NIF_TERM opts) {
    unsigned flags = 0;
    
    if (kwlist_get_bool(env, opts, "debug", 0)) {
        flags |= 0x0001; // MD_MARKDOWN_FLAG_DEBUG
    }
    
    if (kwlist_get_bool(env, opts, "skip_utf8_bom", 0)) {
        flags |= 0x0002; // MD_MARKDOWN_FLAG_SKIP_UTF8_BOM
    }
    
    if (kwlist_get_bool(env, opts, "heal", 0)) {
        flags |= 0x0100; // MD_MARKDOWN_FLAG_HEAL
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
        renderer_flags = parse_html_options(env, argv[1]);
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
        renderer_flags = parse_ast_options(env, argv[1]);
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
        renderer_flags = parse_meta_options(env, argv[1]);
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
        renderer_flags = parse_ansi_options(env, argv[1]);
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
        renderer_flags = parse_text_options(env, argv[1]);
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
        renderer_flags = parse_markdown_options(env, argv[1]);
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
