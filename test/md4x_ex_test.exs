defmodule Md4xExTest do
  use ExUnit.Case
  doctest Md4xEx

  describe "render_to_html/1" do
    test "renders headings" do
      assert {:ok, "<h1>Hello</h1>\n"} == Md4xEx.render_to_html("# Hello")
      assert {:ok, "<h2>World</h2>\n"} == Md4xEx.render_to_html("## World")
    end

    test "renders paragraphs" do
      assert {:ok, "<p>Hello World</p>\n"} == Md4xEx.render_to_html("Hello World")
    end

    test "renders lists" do
      markdown = """
      - Item 1
      - Item 2
      """

      assert {:ok, html} = Md4xEx.render_to_html(markdown)
      assert String.contains?(html, "<li>Item 1</li>")
      assert String.contains?(html, "<li>Item 2</li>")
    end

    test "renders links" do
      assert {:ok, "<p><a href=\"https://example.com\">Example</a></p>\n"} ==
               Md4xEx.render_to_html("[Example](https://example.com)")
    end

    test "renders code blocks" do
      markdown = """
      ```elixir
      def hello do
        IO.puts "Hello"
      end
      ```
      """

      assert {:ok, html} = Md4xEx.render_to_html(markdown)
      assert String.contains?(html, "<pre>")
      assert String.contains?(html, "<code")
    end
  end

  describe "render_to_ast/1" do
    test "renders simple heading to JSON AST" do
      assert {:ok, json} = Md4xEx.render_to_ast("# Hello")
      assert String.contains?(json, "h1")
      assert String.contains?(json, "Hello")
    end

    test "renders paragraph to JSON AST" do
      assert {:ok, json} = Md4xEx.render_to_ast("Hello World")
      assert String.contains?(json, "p")
    end
  end

  describe "extract_meta/1" do
    test "extracts frontmatter" do
      markdown = """
      ---
      title: My Document
      author: John Doe
      ---

      # Hello World
      """

      assert {:ok, json} = Md4xEx.extract_meta(markdown)
      assert String.contains?(json, "My Document")
      assert String.contains?(json, "John Doe")
    end

    test "extracts headings" do
      markdown = """
      ---
      title: Test
      ---

      # Heading 1
      ## Heading 2
      """

      assert {:ok, json} = Md4xEx.extract_meta(markdown)
      assert String.contains?(json, "headings")
    end

    test "handles markdown without frontmatter" do
      assert {:ok, json} = Md4xEx.extract_meta("# Hello")
      assert String.contains?(json, "headings")
    end
  end

  describe "render_to_ansi/1" do
    test "renders heading with ANSI codes" do
      assert {:ok, ansi} = Md4xEx.render_to_ansi("# Hello")
      assert is_binary(ansi)
      assert byte_size(ansi) > 0
    end

    test "renders paragraph with ANSI codes" do
      assert {:ok, ansi} = Md4xEx.render_to_ansi("Hello World")
      assert is_binary(ansi)
      assert ansi =~ "Hello World"
    end

    test "renders bold text with ANSI codes" do
      assert {:ok, ansi} = Md4xEx.render_to_ansi("**bold text**")
      assert is_binary(ansi)
      assert ansi =~ "bold text"
    end

    test "renders italic text with ANSI codes" do
      assert {:ok, ansi} = Md4xEx.render_to_ansi("*italic text*")
      assert is_binary(ansi)
      assert ansi =~ "italic text"
    end
  end

  describe "render_to_text/1" do
    test "strips all formatting" do
      assert {:ok, text} = Md4xEx.render_to_text("# Hello World")
      assert is_binary(text)
      assert text =~ "Hello World"
    end

    test "removes markdown syntax" do
      markdown = """
      # Heading

      **Bold** and *italic* text

      - List item 1
      - List item 2
      """

      assert {:ok, text} = Md4xEx.render_to_text(markdown)
      assert is_binary(text)
      assert text =~ "Heading"
      assert text =~ "Bold"
      assert text =~ "italic"
    end

    test "handles links by extracting text" do
      assert {:ok, text} = Md4xEx.render_to_text("[Example](https://example.com)")
      assert is_binary(text)
      assert text =~ "Example"
      refute text =~ "https://example.com"
    end
  end

  describe "render_to_markdown/1" do
    test "normalizes markdown" do
      assert {:ok, md} = Md4xEx.render_to_markdown("# Hello")
      assert is_binary(md)
      assert md =~ "Hello"
    end

    test "preserves markdown structure" do
      markdown = """
      ## Heading

      - Item 1
      - Item 2
      """

      assert {:ok, md} = Md4xEx.render_to_markdown(markdown)
      assert is_binary(md)
      assert md =~ "Heading"
      assert md =~ "Item 1"
      assert md =~ "Item 2"
    end

    test "handles complex markdown" do
      markdown = """
      # Title

      **Bold** and *italic*

      [Link](https://example.com)

      ```elixir
      def hello do
        IO.puts "Hello"
      end
      ```
      """

      assert {:ok, md} = Md4xEx.render_to_markdown(markdown)
      assert is_binary(md)
      assert md =~ "Title"
    end
  end

  describe "render_to_html/2 with options" do
    test "renders with heal option" do
      markdown = "Hello **world"
      assert {:ok, html} = Md4xEx.render_to_html(markdown, heal: true)
      assert is_binary(html)
    end

    test "renders with full document option" do
      markdown = "# Hello"
      assert {:ok, html} = Md4xEx.render_to_html(markdown, full: true)
      assert is_binary(html)
      assert html =~ "<!DOCTYPE html>"
      assert html =~ "<html"
      assert html =~ "<head>"
      assert html =~ "<body>"
    end

    test "renders with heal and full options" do
      markdown = "Hello **world"
      assert {:ok, html} = Md4xEx.render_to_html(markdown, heal: true, full: true)
      assert is_binary(html)
      assert html =~ "<!DOCTYPE html>"
    end

    test "renders with empty options" do
      assert {:ok, html} = Md4xEx.render_to_html("# Hello", [])
      assert html =~ "<h1>Hello</h1>"
    end

    test "renders without full html by default" do
      assert {:ok, html} = Md4xEx.render_to_html("# Hello")
      refute html =~ "<!DOCTYPE html>"
      refute html =~ "<html"
    end
  end

  describe "render_to_ast/2 with options" do
    test "renders with heal option" do
      markdown = "Hello **world"
      assert {:ok, json} = Md4xEx.render_to_ast(markdown, heal: true)
      assert is_binary(json)
    end

    test "renders with empty options" do
      assert {:ok, json} = Md4xEx.render_to_ast("# Hello", [])
      assert json =~ "h1"
    end
  end

  describe "extract_meta/2 with options" do
    test "extracts with heal option" do
      markdown = "---\ntitle: Test\n---\n# Hello"
      assert {:ok, json} = Md4xEx.extract_meta(markdown, heal: true)
      assert is_binary(json)
    end

    test "extracts with empty options" do
      markdown = "---\ntitle: Test\n---\n# Hello"
      assert {:ok, json} = Md4xEx.extract_meta(markdown, [])
      assert json =~ "Test"
    end
  end

  describe "render_to_ansi/2 with options" do
    test "renders with heal option" do
      markdown = "Hello **world"
      assert {:ok, ansi} = Md4xEx.render_to_ansi(markdown, heal: true)
      assert is_binary(ansi)
    end

    test "renders with empty options" do
      assert {:ok, ansi} = Md4xEx.render_to_ansi("# Hello", [])
      assert is_binary(ansi)
    end

    test "renders with show_urls option" do
      markdown = "[Example](https://example.com)"
      assert {:ok, ansi} = Md4xEx.render_to_ansi(markdown, show_urls: true)
      assert is_binary(ansi)
      assert ansi =~ "Example"
      assert ansi =~ "https://example.com"
    end

    test "renders without visible URLs by default" do
      markdown = "[Example](https://example.com)"
      assert {:ok, ansi} = Md4xEx.render_to_ansi(markdown)
      assert ansi =~ "Example"
      # URLs are embedded in OSC 8 hyperlinks but not shown as visible text
      # The visible URL text appears as " (URL)" when showUrls is true
      refute ansi =~ "(https://example.com)"
    end

    test "renders with show_frontmatter option" do
      markdown = """
      ---
      title: My Document
      author: John Doe
      ---

      # Hello World
      """

      assert {:ok, ansi} = Md4xEx.render_to_ansi(markdown, show_frontmatter: true)
      assert is_binary(ansi)
      assert ansi =~ "title"
      assert ansi =~ "My Document"
    end

    test "hides frontmatter by default" do
      markdown = """
      ---
      title: My Document
      ---

      # Hello World
      """

      assert {:ok, ansi} = Md4xEx.render_to_ansi(markdown)
      refute ansi =~ "title"
      refute ansi =~ "My Document"
    end

    test "renders with heal, show_urls, and show_frontmatter options" do
      markdown = """
      ---
      title: Test
      ---

      Hello **world [link](https://example.com)
      """

      assert {:ok, ansi} =
               Md4xEx.render_to_ansi(markdown,
                 heal: true,
                 show_urls: true,
                 show_frontmatter: true
               )

      assert is_binary(ansi)
      assert ansi =~ "title"
      assert ansi =~ "https://example.com"
    end
  end

  describe "render_to_text/2 with options" do
    test "renders with heal option" do
      markdown = "Hello **world"
      assert {:ok, text} = Md4xEx.render_to_text(markdown, heal: true)
      assert is_binary(text)
    end

    test "renders with empty options" do
      assert {:ok, text} = Md4xEx.render_to_text("# Hello", [])
      assert text =~ "Hello"
    end
  end

  describe "render_to_markdown/2 with options" do
    test "renders with heal option" do
      markdown = "Hello **world"
      assert {:ok, md} = Md4xEx.render_to_markdown(markdown, heal: true)
      assert is_binary(md)
    end

    test "renders with empty options" do
      assert {:ok, md} = Md4xEx.render_to_markdown("# Hello", [])
      assert md =~ "Hello"
    end
  end

  describe "heal/1" do
    test "closes unclosed bold markers" do
      assert {:ok, healed} = Md4xEx.heal("Hello **world")
      assert healed =~ "**"
    end

    test "closes unclosed italic markers" do
      assert {:ok, healed} = Md4xEx.heal("Hello *world")
      assert healed =~ "*"
    end

    test "handles complete markdown unchanged" do
      assert {:ok, healed} = Md4xEx.heal("Hello **world**")
      assert healed == "Hello **world**"
    end

    test "closes unclosed code blocks" do
      markdown = """
      ```elixir
      def hello do
        IO.puts "Hello"
      """

      assert {:ok, healed} = Md4xEx.heal(markdown)
      assert healed =~ "```"
    end

    test "handles empty input" do
      assert {:ok, healed} = Md4xEx.heal("")
      assert healed == ""
    end
  end

  describe "bang versions" do
    test "render_to_html!/1 returns html directly" do
      assert "<h1>Hello</h1>\n" == Md4xEx.render_to_html!("# Hello")
    end

    test "render_to_html!/2 with options" do
      assert html = Md4xEx.render_to_html!("# Hello", full: true)
      assert html =~ "<!DOCTYPE html>"
    end

    test "render_to_ast!/1 returns json directly" do
      assert json = Md4xEx.render_to_ast!("# Hello")
      assert json =~ "h1"
    end

    test "render_to_ansi!/1 returns ansi directly" do
      assert ansi = Md4xEx.render_to_ansi!("# Hello")
      assert is_binary(ansi)
    end

    test "render_to_text!/1 returns text directly" do
      assert text = Md4xEx.render_to_text!("# Hello")
      assert text =~ "Hello"
    end

    test "render_to_markdown!/1 returns markdown directly" do
      assert md = Md4xEx.render_to_markdown!("# Hello")
      assert md =~ "Hello"
    end

    test "extract_meta!/1 returns meta directly" do
      markdown = "---\ntitle: Test\n---\n# Hello"
      assert meta = Md4xEx.extract_meta!(markdown)
      assert meta =~ "Test"
    end

    test "heal!/1 returns healed markdown directly" do
      assert "Hello **world**" == Md4xEx.heal!("Hello **world**")
    end
  end
end
