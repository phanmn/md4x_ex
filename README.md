# Md4xEx

Elixir wrapper for the [md4x](https://github.com/mity/md4c) markdown parser library using NIFs (Native Implemented Functions) for high-performance markdown rendering.

## Features

- **Render markdown to HTML** - Convert markdown to HTML with full CommonMark 0.31.2 support
- **Render markdown to AST (JSON)** - Get structured JSON representation of the markdown AST
- **Extract metadata** - Extract frontmatter YAML and document metadata (headings, etc.)
- **Render to ANSI terminal** - Generate ANSI escape codes for colored terminal output
- **Render to plain text** - Strip all markdown formatting to get plain text
- **Normalize markdown** - Render markdown back to normalized/canonicalized markdown
- **Heal incomplete markdown** - Fix unclosed formatting markers and incomplete structures for streaming
- **High performance** - Uses NIFs for direct C library integration without port overhead

## Installation

Add `md4x_ex` to your `mix.exs` dependencies:

```elixir
def deps do
  [
    {:md4x_ex, "~> 0.1.0"}
  ]
end
```

## Usage

### Render Markdown to HTML

```elixir
{:ok, html} = Md4xEx.render_to_html("# Hello World")
# => {:ok, "<h1>Hello World</h1>\n"}

{:ok, html} = Md4xEx.render_to_html("""
# Title

This is a paragraph with **bold** and *italic* text.

- Item 1
- Item 2
""")
```

### Render Markdown to AST (JSON)

```elixir
{:ok, ast_json} = Md4xEx.render_to_ast("# Hello World")
# => {:ok, "{\"nodes\":[[\"h1\",{},\"Hello World\"]],\"frontmatter\":{},\"meta\":{}}\n"}

# Parse the JSON if needed
{:ok, ast} = Jason.decode(ast_json)
# => %{nodes: [[["h1", %{}, "Hello World"]]], frontmatter: %{}, meta: %{}}}
```

### Extract Metadata

```elixir
markdown = """
---
title: My Document
author: John Doe
date: 2024-01-01
---

# Introduction

Some content here.

## Details

More content.
"""

{:ok, meta_json} = Md4xEx.extract_meta(markdown)
# => {:ok, "{\"frontmatter\":{\"title\":\"My Document\",\"author\":\"John Doe\",\"date\":\"2024-01-01\"},\"meta\":{\"headings\":[{\"level\":1,\"content\":\"Introduction\"},{\"level\":2,\"content\":\"Details\"}]}}\n"}

# Parse the JSON if needed
{:ok, meta} = Jason.decode(meta_json)
# => %{
#      frontmatter: %{title: "My Document", author: "John Doe", date: "2024-01-01"},
#      meta: %{headings: [%{level: 1, content: "Introduction"}, %{level: 2, content: "Details"}]}
#    }
```

## API

### `render_to_html/2`

Renders markdown to HTML.

**Parameters:**

- `markdown` (String.t()) - The markdown string to render
- `opts` (keyword list, optional) - Rendering options:
  - `:heal` (boolean) - Enable healing of incomplete markdown (default: false)
  - `:full` (boolean) - Generate full HTML document with `<!DOCTYPE>`, `<html>`, `<head>`, and `<body>` tags (default: false)
  - `:debug` (boolean) - Enable debug output to stderr (default: false)
  - `:verbatim_entities` (boolean) - Output HTML entities as verbatim character references (default: false)
  - `:skip_utf8_bom` (boolean) - Skip UTF-8 BOM in output (default: false)
  - `:code_meta` (boolean) - Include code block metadata in output (default: false)

**Returns:**

- `{:ok, html}` (String.t()) - The rendered HTML
- `{:error, reason}` - Error tuple if rendering fails

**Examples:**

```elixir
{:ok, html} = Md4xEx.render_to_html("# Hello", [heal: true])
{:ok, html} = Md4xEx.render_to_html("# Hello", [full: true])
{:ok, html} = Md4xEx.render_to_html("# Hello", [heal: true, full: true])
```

### `render_to_ast/2`

Renders markdown to a JSON AST representation.

**Parameters:**

- `markdown` (String.t()) - The markdown string to render
- `opts` (keyword list, optional) - Rendering options:
  - `:debug` (boolean) - Enable debug output to stderr (default: false)
  - `:skip_utf8_bom` (boolean) - Skip UTF-8 BOM in output (default: false)
  - `:heal` (boolean) - Enable healing of incomplete markdown (default: false)

**Returns:**

- `{:ok, json}` (String.t()) - JSON string containing the AST with structure: `{"nodes":[...],"frontmatter":{...},"meta":{}}`
- `{:error, reason}` - Error tuple if rendering fails

**Examples:**

```elixir
{:ok, json} = Md4xEx.render_to_ast("# Hello")
{:ok, json} = Md4xEx.render_to_ast("# Hello", [heal: true])
{:ok, json} = Md4xEx.render_to_ast("# Hello", [debug: true])
```

### `extract_meta/2`

Extracts metadata (frontmatter YAML and document structure) from markdown.

**Parameters:**

- `markdown` (String.t()) - The markdown string to extract metadata from
- `opts` (keyword list, optional) - Rendering options:
  - `:debug` (boolean) - Enable debug output to stderr (default: false)
  - `:skip_utf8_bom` (boolean) - Skip UTF-8 BOM in output (default: false)
  - `:heal` (boolean) - Enable healing of incomplete markdown (default: false)

**Returns:**

- `{:ok, json}` (String.t()) - JSON string containing frontmatter and metadata
- `{:error, reason}` - Error tuple if extraction fails

**Examples:**

```elixir
{:ok, json} = Md4xEx.extract_meta(markdown)
{:ok, json} = Md4xEx.extract_meta(markdown, [heal: true])
{:ok, json} = Md4xEx.extract_meta(markdown, [debug: true])
```

### `render_to_ansi/2`

Renders markdown to ANSI terminal output with colors and styling.

**Parameters:**

- `markdown` (String.t()) - The markdown string to render
- `opts` (keyword list, optional) - Rendering options:
  - `:heal` (boolean) - Enable healing of incomplete markdown (default: false)
  - `:show_urls` (boolean) - Show URLs as visible text after links (default: false)
  - `:show_frontmatter` (boolean) - Include frontmatter YAML in output (default: false)
  - `:debug` (boolean) - Enable debug output to stderr (default: false)
  - `:skip_utf8_bom` (boolean) - Skip UTF-8 BOM in output (default: false)
  - `:no_color` (boolean) - Disable ANSI color codes (default: false)
  - `:code_meta` (boolean) - Include code block metadata in output (default: false)

**Returns:**

- `{:ok, ansi}` (String.t()) - ANSI escape code string for terminal rendering
- `{:error, reason}` - Error tuple if rendering fails

**Examples:**

```elixir
{:ok, ansi} = Md4xEx.render_to_ansi("# Hello")
{:ok, ansi} = Md4xEx.render_to_ansi("# Hello", [heal: true])
{:ok, ansi} = Md4xEx.render_to_ansi("[link](https://example.com)", [show_urls: true])
{:ok, ansi} = Md4xEx.render_to_ansi(markdown, [show_frontmatter: true])
```

### `render_to_text/2`

Renders markdown to plain text by stripping all formatting.

**Parameters:**

- `markdown` (String.t()) - The markdown string to render
- `opts` (keyword list, optional) - Rendering options:
  - `:debug` (boolean) - Enable debug output to stderr (default: false)
  - `:skip_utf8_bom` (boolean) - Skip UTF-8 BOM in output (default: false)
  - `:heal` (boolean) - Enable healing of incomplete markdown (default: false)

**Returns:**

- `{:ok, text}` (String.t()) - Plain text output
- `{:error, reason}` - Error tuple if rendering fails

**Example:**

```elixir
{:ok, text} = Md4xEx.render_to_text("# Hello", [heal: true])
{:ok, text} = Md4xEx.render_to_text("# Hello", [debug: true])
```

### `render_to_markdown/2`

Renders markdown back to normalized/canonicalized markdown.

**Parameters:**

- `markdown` (String.t()) - The markdown string to normalize
- `opts` (keyword list, optional) - Rendering options:
  - `:debug` (boolean) - Enable debug output to stderr (default: false)
  - `:skip_utf8_bom` (boolean) - Skip UTF-8 BOM in output (default: false)
  - `:heal` (boolean) - Enable healing of incomplete markdown (default: false)

**Returns:**

- `{:ok, markdown}` (String.t()) - Normalized markdown
- `{:error, reason}` - Error tuple if rendering fails

**Example:**

```elixir
{:ok, md} = Md4xEx.render_to_markdown("# Hello", [heal: true])
{:ok, md} = Md4xEx.render_to_markdown("# Hello", [debug: true])
```

### `heal/1`

Heals (fixes/completes) incomplete streaming markdown by closing unclosed formatting markers, completing incomplete links/images, and closing open code blocks.

**Parameters:**

- `markdown` (String.t()) - The potentially incomplete markdown string

**Returns:**

- `{:ok, healed}` (String.t()) - Healed markdown with all markers properly closed
- `{:error, reason}` - Error tuple if healing fails

**Example:**

```elixir
{:ok, healed} = Md4xEx.heal("Hello **world")
# => {:ok, "Hello **world**"}
```

## Building from Source

This project uses `elixir_make` to compile the C library. Ensure you have a C compiler installed:

```bash
mix deps.get
mix compile
```

## Testing

```bash
mix test
```

## License

MIT License - See the original [md4x](https://github.com/mity/md4c) project for details.
