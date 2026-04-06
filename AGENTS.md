# MD4xEx

> **Elixir/NIF wrapper for the md4x markdown parser**

Elixir bindings for the md4x markdown parser library, providing fast markdown-to-HTML/AST/ANSI/text rendering via NIF (Native Implemented Function).

## Project Status

**Current Version:** Development (based on md4x 0.5.2)

**Last Updated:** 2026-04-07

## Goal

Extend the `md4x_ex` Elixir/NIF wrapper to fully support all renderer options available in the original md4x library, ensuring feature parity with the JavaScript/TypeScript version.

## Architecture

- **NIF Implementation:** `/home/phanmn/codings/md4x_ex/c_src/md4x_nif.c` - Core C code that interfaces with md4x library
- **Elixir Module:** `/home/phanmn/codings/md4x_ex/lib/md4x_ex.ex` - Elixir API layer
- **Renderers:** HTML, AST (JSON), Meta, ANSI, Text, Markdown
- **Build System:** Mix with C source compilation via `c_src/Makefile`

## Key Technical Decisions

### Option Handling

- **Elixir convention:** All options use snake_case (e.g., `:debug`, `:skip_utf8_bom`, `:show_urls`)
- **Keyword lists only:** Elixir always passes keyword lists (`[{key, value}, ...]`), so map parsing code has been removed
- **String comparison:** Uses `strcmp()` instead of `enif_compare()` for atom comparison due to Erlang 27.3.4 bug with atom comparison

### Renderer Options

All renderers support these common options:

- `:debug` - Enable debug output
- `:skip_utf8_bom` - Skip UTF-8 BOM in output
- `:heal` - Auto-heal malformed markdown

**HTML-specific:**

- `:full` - Generate full HTML document
- `:verbatim_entities` - Use verbatim entities
- `:code_meta` - Include code metadata

**ANSI-specific:**

- `:show_urls` - Display URLs after hyperlinks
- `:show_frontmatter` - Include frontmatter in output
- `:no_color` - Disable ANSI colors
- `:code_meta` - Include code metadata

## Build & Test

```bash
# Build
mix compile

# Run all tests
mix test

# Build in release mode
MIX_ENV=release mix compile
```

## File Structure

```
md4x_ex/
├─ c_src/
│  ├─ md4x_nif.c              # NIF implementation (keyword-list-only option parsers)
│  ├─ Makefile                # C compilation rules
│  └─ renderers/              # md4x renderer headers (md4x-html.h, md4x-ast.h, etc.)
├─ lib/
│  └─ md4x_ex.ex              # Main Elixir module with public API
├─ test/
│  ├─ test_helper.exs
│  └─ md4x_ex_test.exs        # Test suite (45 tests covering all renderers + options)
├─ README.md                  # Complete API documentation with examples
├─ mix.exs                    # Project configuration
└─ .agent                    # This file
```

## API Quick Reference

```elixir
# HTML rendering
Md4xEx.to_html("# Hello", heal: true, full: true)

# AST (JSON) rendering
Md4xEx.to_ast("# Hello", debug: true, skip_utf8_bom: true)

# ANSI terminal rendering
Md4xEx.to_ansi("# Hello", show_urls: true, no_color: false)

# Plain text extraction
Md4xEx.to_text("# Hello", debug: true)

# Metadata extraction
Md4xEx.extract_meta("---\ntitle: Test\n---\n# Hello", debug: true)

# Markdown round-trip
Md4xEx.to_markdown("# Hello", debug: true)

# Heal incomplete markdown
Md4xEx.heal("* incomplete list")
```

## Testing Guidelines

- All option names must use snake_case in tests
- Test keyword list options (not maps)
- Verify both `true` and `false` values for boolean options
- Empty keyword list `[]` should work as default options

## Dependencies

- **md4x C library:** Fork of mity/md4c, vendored in `c_src/`
- **Erlang/OTP:** 27.3.4+ (tested on 27.3.4)
- **Elixir:** 1.17+

## Notes for Future Development

1. **Option parsing:** The `kwlist_get_bool()` helper in `md4x_nif.c` handles keyword list parsing. It uses `strcmp()` for atom comparison to avoid Erlang 27.3.4 bugs.

2. **Memory management:** Output buffers are allocated in C and freed after conversion to Erlang binaries. No memory leaks detected in current implementation.

3. **Error handling:** Returns `{:error, reason}` tuples for invalid input, render failures, or memory allocation errors.

4. **Thread safety:** NIF functions are stateless and thread-safe. Each call allocates its own output buffer.

5. **UTF-8 handling:** All input/output is UTF-8. The `:skip_utf8_bom` flag controls whether BOM is included in output.

6. **Dialect support:** Currently uses `MD_DIALECT_ALL` (all extensions enabled). Future work may add dialect options.
