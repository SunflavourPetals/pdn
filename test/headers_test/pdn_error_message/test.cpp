// Tests for pdn/include/pdn/pdn_error_message.h.
//
// What is pinned down here:
//   * error_message is a plain aggregate of (code, position, text); it can be built
//     positionally and with designated initializers in declaration order, which is how
//     the lexer, the parser and the code point iterator build it;
//   * the text is the fully rendered UTF-8 message: "<label>(line:column) <detail>", and
//     a function package that knows the file name prepends "<file>: " without touching
//     the code or the position;
//   * a value-initialized instance is a placeholder, not an error: the variant holds the
//     unnamed 0 of its first alternative and the text is empty;
//   * the rendered text is delivered only through handle_error; parse_result::what()
//     keeps the coarse parse_error_code name.

#include <cstddef>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

#include <pdn/pdn_error_message.h>
#include <pdn/pdn_error_handler.h>
#include <pdn/pdn_error_handler_concept.h>
#include <pdn/spdn.h>

namespace
{
	using pdn::error_code_variant;
	using pdn::error_message;
	using pdn::error_msg_string;
	using pdn::source_position;
	using pdn::unicode::utf8::decode_error_code;

	int failures = 0;

	void check(bool ok, const char* what)
	{
		if (!ok)
		{
			std::printf("FAILED: %s\n", what);
			++failures;
		}
	}

	std::string as_ascii(const error_msg_string& text)
	{
		return std::string{ text.cbegin(), text.cend() };
	}

	// Redirects std::cerr for the lifetime of the object and keeps what was written.
	class cerr_capture
	{
	public:
		cerr_capture() : old_buf{ std::cerr.rdbuf(buffer.rdbuf()) } {}
		~cerr_capture() { std::cerr.rdbuf(old_buf); }
		cerr_capture(const cerr_capture&) = delete;
		cerr_capture& operator=(const cerr_capture&) = delete;
		std::string str() const { return buffer.str(); }
	private:
		std::ostringstream buffer;
		std::streambuf*    old_buf;
	};

	// Accepts both value categories, which is what concepts::error_handler asks for.
	struct recording_handler
	{
		void handle_error(const error_message& msg) { last = msg; ++count; }
		auto error_count() const noexcept -> ::std::size_t { return count; }
		error_message last{};
		::std::size_t count{};
	};

	// Same shape except that it only takes a non-const lvalue reference.
	struct non_const_handler
	{
		void handle_error(error_message&) {}
		auto error_count() const noexcept -> ::std::size_t { return 0; }
	};

	void test_shape()
	{
		static_assert(std::is_aggregate_v<error_message>);
		static_assert(std::is_copy_constructible_v<error_message>);
		static_assert(std::is_copy_assignable_v<error_message>);
		static_assert(std::is_nothrow_move_constructible_v<error_message>);
		static_assert(std::is_nothrow_move_assignable_v<error_message>);
		static_assert(std::is_nothrow_destructible_v<error_message>);
		static_assert(!std::is_trivially_copyable_v<error_message>);

		static_assert(std::is_same_v<decltype(::std::declval<error_message&>().error_code), error_code_variant>);
		static_assert(std::is_same_v<decltype(::std::declval<error_message&>().position), source_position>);
		static_assert(std::is_same_v<decltype(::std::declval<error_message&>().error_message), error_msg_string>);

		// The member is named like the class; the pointer-to-member type still separates them.
		static_assert(std::is_same_v<decltype(&error_message::error_message), error_msg_string error_message::*>);

		static_assert(pdn::concepts::error_handler<recording_handler>);
		static_assert(pdn::concepts::error_handler<pdn::default_error_handler>);
		static_assert(!pdn::concepts::error_handler<non_const_handler>);
	}

	void test_placeholder_state()
	{
		const error_message placeholder{};
		check(placeholder.error_code.index() == 0, "value-initialized code is the first alternative");
		check(std::holds_alternative<decode_error_code>(placeholder.error_code), "first alternative is the utf-8 decode error code");
		check(std::get<decode_error_code>(placeholder.error_code) == decode_error_code{},
			"the placeholder code is the unnamed 0, not a real error");
		check(placeholder.position.line == 1 && placeholder.position.column == 1, "the placeholder position is 1:1");
		check(placeholder.error_message.empty(), "the placeholder text is empty");
	}

	void test_aggregate_initialization()
	{
		const error_message positional{
			decode_error_code::non_shortest_sequence,
			source_position{ 3, 7 },
			error_msg_string{ u8"non-shortest" }
		};
		check(std::holds_alternative<decode_error_code>(positional.error_code), "positional init keeps the decode alternative");
		check(std::get<decode_error_code>(positional.error_code) == decode_error_code::non_shortest_sequence, "positional init keeps the code");
		check(positional.position.line == 3 && positional.position.column == 7, "positional init keeps the position");
		check(as_ascii(positional.error_message) == "non-shortest", "positional init keeps the text");

		// This is the form pdn_error_handler.h uses when it re-packs the message.
		const error_message designated{
			.error_code    = pdn::syntax_error_code::expect_comma,
			.position      = source_position{ 9, 2 },
			.error_message = error_msg_string{ u8"x: expect-comma" }
		};
		check(std::holds_alternative<pdn::syntax_error_code>(designated.error_code), "designated init keeps the syntax alternative");
		check(std::get<pdn::syntax_error_code>(designated.error_code) == pdn::syntax_error_code::expect_comma, "designated init keeps the code");
		check(designated.position.line == 9 && designated.position.column == 2, "designated init keeps the position");
		check(as_ascii(designated.error_message) == "x: expect-comma", "designated init keeps the text");
	}

	void test_copy_is_deep()
	{
		error_message original{ .error_code = decode_error_code::trailing_as_start,
		                        .position = source_position{ 1, 1 },
		                        .error_message = error_msg_string{ u8"abc" } };
		error_message copy = original;
		copy.error_message.append(u8"def");

		check(as_ascii(original.error_message) == "abc", "copying does not share the text buffer");
		check(as_ascii(copy.error_message) == "abcdef", "the copy owns its own text");
	}

	void test_filename_prefix()
	{
		const error_message original{ .error_code = decode_error_code::trailing_as_start,
		                              .position = source_position{ 4, 5 },
		                              .error_message = error_msg_string{ u8"utf-8 decode error(4:5) trailing" } };

		const auto renamed = pdn::detail::error_msg_for_filename(error_msg_string{ u8"a.spdn" }, original);

		check(as_ascii(renamed.error_message) == "a.spdn: utf-8 decode error(4:5) trailing",
			"the file name is prepended to the already rendered text");
		check(renamed.position.line == 4 && renamed.position.column == 5, "the file name prefix keeps the position");
		check(std::holds_alternative<decode_error_code>(renamed.error_code), "the file name prefix keeps the code alternative");
		check(as_ascii(original.error_message) == "utf-8 decode error(4:5) trailing", "the source message is not modified");
	}

	void test_rendered_text_is_utf8()
	{
		error_message msg{ .error_code = decode_error_code::trailing_as_start,
		                   .position = source_position{ 1, 1 },
		                   .error_message = error_msg_string{ u8"\u4E2D" } };
		check(msg.error_message.size() == 3, "text is a UTF-8 byte string: U+4E2D needs three bytes");
		check(static_cast<unsigned char>(msg.error_message[0]) == 0xE4
			&& static_cast<unsigned char>(msg.error_message[1]) == 0xB8
			&& static_cast<unsigned char>(msg.error_message[2]) == 0xAD, "the three bytes are E4 B8 AD");
	}

	void test_default_handler()
	{
		const error_message first{ .error_code = decode_error_code::trailing_as_start,
		                           .position = source_position{ 1, 2 },
		                           .error_message = error_msg_string{ u8"first error" } };
		const error_message second{ .error_code = pdn::lexical_error_code::string_missing_terminating_character,
		                            .position = source_position{ 2, 3 },
		                            .error_message = error_msg_string{ u8"second error" } };

		pdn::default_error_handler handler;
		std::ostringstream out;
		handler.handle_error(first, out);
		handler.handle_error(second, out);
		handler.handle_error();

		check(handler.error_count() == 3, "each handle_error call counts once, including the no-message overload");
		check(out.str() == "first error\nsecond error\n", "the handler prints the text and a newline, without the position");
	}

	void test_end_to_end()
	{
		std::string lexical_text;
		{
			const cerr_capture captured;
			const auto result = pdn::parse(::std::u8string_view{ u8"name \"abc" }, pdn::utf8_tag);
			check(result.error_code() == pdn::parse_error_code::parse_error, "a recoverable error is reported as parse_error");
			check(std::string_view{ result.what() } == "parse_error", "what() keeps the coarse parse code name, not the detail");
			lexical_text = captured.str();
		}
		check(lexical_text.rfind("lexical error(1:6) ", 0) == 0, "rendered text starts with the layer label and the position");
		check(lexical_text.find("string literal is missing its terminating") != std::string::npos, "rendered text carries the detail");

		std::string syntax_text;
		{
			const cerr_capture captured;
			const auto result = pdn::parse(::std::u8string_view{ u8"x : bool 'A'" }, pdn::utf8_tag);
			check(result.error_code() == pdn::parse_error_code::parse_error, "the cast error is a recoverable error");
			syntax_text = captured.str();
		}
		check(syntax_text.rfind("syntax error(1:5) ", 0) == 0, "rendered text starts with the syntax layer label");
		check(syntax_text.find("cannot cast") != std::string::npos, "rendered text carries the cast detail");

		recording_handler handler;
		const error_message sample{ .error_code = decode_error_code::trailing_as_start,
		                            .position = source_position{ 1, 1 },
		                            .error_message = error_msg_string{ u8"sample" } };
		handler.handle_error(sample);
		check(handler.count == 1, "a handler that stores the message receives it unchanged");
		check(as_ascii(handler.last.error_message) == "sample", "the stored message keeps its text");
	}
}

int main()
{
	test_shape();
	test_placeholder_state();
	test_aggregate_initialization();
	test_copy_is_deep();
	test_filename_prefix();
	test_rendered_text_is_utf8();
	test_default_handler();
	test_end_to_end();

	if (failures != 0)
	{
		std::printf("%d check(s) failed\n", failures);
		return 1;
	}

	std::printf("all checks passed\n");
	return 0;
}
