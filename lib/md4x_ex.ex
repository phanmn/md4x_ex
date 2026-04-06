defmodule Md4xEx do
  @moduledoc """
  Elixir bindings for md4x - a fast markdown parser library.

  Uses elixir_make to compile the C library and communicates via NIFs.
  """

  # Load the NIF on module compilation
  @on_load :load_nif

  def load_nif do
    nif_path = :code.priv_dir(:md4x_ex) |> Path.join("md4x_nif") |> to_charlist()
    :erlang.load_nif(nif_path, 0)
  end

  @doc """
  Renders markdown to HTML.

  ## Parameters

  - markdown - The markdown string to render
  - opts - Optional keyword list of options:
    - `:heal` - Enable healing of incomplete markdown (default: false)
    - `:full` - Generate full HTML document with DOCTYPE, html, head, body tags (default: false)

  ## Returns

  - {:ok, html} on success
  - {:error, reason} on failure

  ## Examples

      iex> Md4xEx.render_to_html("# Hello")
      {:ok, "<h1>Hello</h1>\\n"}
  """
  def render_to_html(markdown, opts \\ []) when is_binary(markdown) and is_list(opts) do
    :erlang.nif_error(:nif_not_loaded)
  end

  @doc """
  Renders markdown to JSON (AST representation).

  ## Parameters

  - markdown - The markdown string to render
  - opts - Optional keyword list of options:
    - `:heal` - Enable healing of incomplete markdown (default: false)

  ## Returns

  - {:ok, json} on success
  - {:error, reason} on failure
  """
  def render_to_ast(markdown, opts \\ []) when is_binary(markdown) and is_list(opts) do
    :erlang.nif_error(:nif_not_loaded)
  end

  @doc """
  Extracts metadata (frontmatter) from markdown.

  ## Parameters

  - markdown - The markdown string with YAML frontmatter
  - opts - Optional keyword list of options:
    - `:heal` - Enable healing of incomplete markdown (default: false)

  ## Returns

  - {:ok, %{frontmatter: map, content: binary}} on success
  - {:error, reason} on failure
  """
  def extract_meta(markdown, opts \\ []) when is_binary(markdown) and is_list(opts) do
    :erlang.nif_error(:nif_not_loaded)
  end

  @doc """
  Renders markdown to ANSI terminal output with colors and styling.

  ## Parameters

  - markdown - The markdown string to render
  - opts - Optional keyword list of options:
    - `:heal` - Enable healing of incomplete markdown (default: false)
    - `:show_urls` - Show link URLs after link text (default: false)
    - `:show_frontmatter` - Show frontmatter content as dim text (default: false)

  ## Returns

  - {:ok, ansi} on success
  - {:error, reason} on failure

  ## Examples

      iex> {:ok, ansi} = Md4xEx.render_to_ansi("# Hello")
      iex> is_binary(ansi)
      true
  """
  def render_to_ansi(markdown, opts \\ []) when is_binary(markdown) and is_list(opts) do
    :erlang.nif_error(:nif_not_loaded)
  end

  @doc """
  Renders markdown to plain text (strips all formatting).

  ## Parameters

  - markdown - The markdown string to render
  - opts - Optional keyword list of options:
    - `:heal` - Enable healing of incomplete markdown (default: false)

  ## Returns

  - {:ok, plain_text} on success
  - {:error, reason} on failure
  """
  def render_to_text(markdown, opts \\ []) when is_binary(markdown) and is_list(opts) do
    :erlang.nif_error(:nif_not_loaded)
  end

  @doc """
  Renders markdown back to markdown (normalization/canonicalization).

  ## Parameters

  - markdown - The markdown string to render
  - opts - Optional keyword list of options:
    - `:heal` - Enable healing of incomplete markdown (default: false)

  ## Returns

  - {:ok, markdown} on success
  - {:error, reason} on failure
  """
  def render_to_markdown(markdown, opts \\ []) when is_binary(markdown) and is_list(opts) do
    :erlang.nif_error(:nif_not_loaded)
  end

  @doc """
  Heals (fixes/completes) incomplete streaming markdown.

  This function closes unclosed formatting markers, completes incomplete
  links/images, closes open code blocks, and fixes other partial markdown
  so it renders correctly mid-stream.

  ## Parameters

  - markdown - The potentially incomplete markdown string to heal

  ## Returns

  - {:ok, healed} on success
  - {:error, reason} on failure

  ## Examples

      iex> Md4xEx.heal("Hello **world")
      {:ok, "Hello **world**"}
  """
  def heal(markdown) when is_binary(markdown) do
    :erlang.nif_error(:nif_not_loaded)
  end
end
