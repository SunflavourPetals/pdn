#include <pdn/pdn_error_message_generator.h>

#include <cstdio>
#include <type_traits>
#include <variant>

namespace
{
	int fails = 0;

	void check(bool ok, const char* what)
	{
		if (!ok)
		{
			++fails;
			std::printf("FAILED: %s\n", what);
		}
	}

	namespace rt   = pdn::raw_error_message_type;
	namespace utf8 = pdn::unicode::utf8;
	namespace utf16 = pdn::unicode::utf16;
	namespace utf32 = pdn::unicode::utf32;
	using pdn::default_error_message_generator;
	using pdn::error_msg_string;
	using pdn::lexical_error_code;
	using pdn::raw_error_message;
	using pdn::source_position;
	using pdn::syntax_error_code;
	using variant_t = pdn::raw_error_message_variant;

	static_assert(pdn::concepts::error_message_generator<default_error_message_generator>,
		"the built-in generator satisfies the public concept");
	static_assert(::std::is_same_v<decltype(default_error_message_generator::generate_error_message(
		::std::declval<raw_error_message>())), error_msg_string>,
		"generate_error_message returns the final text type");


	auto gen(raw_error_message raw) -> error_msg_string
	{
		return default_error_message_generator::generate_error_message(::std::move(raw));
	}

	void test_format()
	{
		// "<category>(<line>:<column>) <detail>": the category comes from the error code variant, the
		// position from the raw error, and the detail from the English generator.
		check(gen(raw_error_message{ syntax_error_code::entity_redefine, source_position{ 3, 7 },
				variant_t{ rt::identifier{ u8"x" } } })
			== u8"syntax error(3:7) entity \"x\" is redefined in this scope", "syntax error format");

		// A value initialized position is { 1, 1 }, which is what a diagnostic at the very start of the
		// input reports.
		check(gen(raw_error_message{ syntax_error_code::unknown_type, source_position{},
				variant_t{ rt::identifier{ u8"y" } } })
			== u8"syntax error(1:1) type \"y\" not found", "the default position is 1:1");
	}

	void test_categories()
	{
		check(gen(raw_error_message{ lexical_error_code::unacceptable_character, source_position{ 1, 2 },
				variant_t{ rt::error_string{ u8"x" } } })
			== u8"lexical error(1:2) unacceptable character: \"x\"", "lexical error category");
		check(gen(raw_error_message{ utf8::decode_error_code::invalid_code_point, source_position{ 1, 1 },
				variant_t{ rt::utf8_decode_error{ utf8::decode_result{}, utf8::code_unit_t{ 0x80 }, 4 } } })
			== u8"utf-8 decode error(1:1) invalid code point: 0x0000, in the sequence at offset 0x0004 (add 3 if the input starts with a BOM), 1 code unit was read",
			"utf-8 decode error category");
		check(gen(raw_error_message{ utf16::decode_error_code::incomplete_sequence, source_position{ 2, 5 },
				variant_t{ rt::utf16_decode_error{ utf16::decode_result{}, utf16::code_unit_t{ 0xdc00 }, 4 } } })
			== u8"utf-16 decode error(2:5) incomplete sequence (expected a UTF-16 trailing surrogate), code unit: 0xdc00, in the sequence at offset 0x0008 (add 2 if the input starts with a BOM), 1 code unit was read",
			"utf-16 decode error category");
		check(gen(raw_error_message{ utf32::decode_error_code::invalid_code_point, source_position{ 9, 9 },
				variant_t{ rt::utf32_decode_error{ utf32::decode_result{}, utf32::code_unit_t{}, 8 } } })
			== u8"utf-32 decode error(9:9) invalid code point: 0x00000000, in the sequence at offset 0x0020 (add 4 if the input starts with a BOM), 1 code unit was read",
			"utf-32 decode error category");
	}

	void test_multiple_digits_and_repeated_calls()
	{
		// The line and the column go through std::to_string, so multi-digit positions must render
		// without padding or separators, and the call must not consume anything that a second call
		// would miss.
		const auto raw = raw_error_message{ syntax_error_code::at_value_not_found, source_position{ 120, 3456 },
			variant_t{ rt::identifier{ u8"a b" } } };
		check(gen(raw) == u8"syntax error(120:3456) at value \"@a b\" not found", "multi-digit position");
		check(gen(raw) == gen(raw), "the generator is idempotent");
	}

	void test_the_filename_prefix_is_not_added_here()
	{
		// detail::error_msg_for_filename (pdn_error_handler.h) prepends "<filename>: " when the
		// function package knows the file name; the generator itself never emits it.
		const auto text = gen(raw_error_message{ syntax_error_code::entity_redefine, source_position{ 1, 1 },
			variant_t{ rt::identifier{ u8"x" } } });
		check(text.find(u8": ") != 0, "no filename prefix");
	}
}

int main()
{
	test_format();
	test_categories();
	test_multiple_digits_and_repeated_calls();
	test_the_filename_prefix_is_not_added_here();
	return fails == 0 ? 0 : 1;
}
