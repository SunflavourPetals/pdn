#include <pdn/pdn_error_message_generator_en.h>

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
	using pdn::error_msg_string;
	using pdn::lexical_error_code;
	using pdn::raw_error_message;
	using pdn::source_position;
	using pdn::syntax_error_code;
	using variant_t = pdn::raw_error_message_variant;

	static_assert(::std::is_same_v<decltype(pdn::error_message_generator_en(::std::declval<raw_error_message>())), error_msg_string>,
		"error_message_generator_en returns the final text type");

	// One raw error per layer: the generator must forward each to the matching detail overload.
	auto syntax_raw(syntax_error_code code, variant_t payload, source_position pos = {}) -> raw_error_message
	{
		return raw_error_message{ code, pos, ::std::move(payload) };
	}
	auto lexical_raw(lexical_error_code code, variant_t payload, source_position pos = {}) -> raw_error_message
	{
		return raw_error_message{ code, pos, ::std::move(payload) };
	}
	auto utf8_raw(utf8::decode_error_code code, variant_t payload, source_position pos = {}) -> raw_error_message
	{
		return raw_error_message{ code, pos, ::std::move(payload) };
	}
	auto utf16_raw(utf16::decode_error_code code, variant_t payload, source_position pos = {}) -> raw_error_message
	{
		return raw_error_message{ code, pos, ::std::move(payload) };
	}
	auto utf32_raw(utf32::decode_error_code code, variant_t payload, source_position pos = {}) -> raw_error_message
	{
		return raw_error_message{ code, pos, ::std::move(payload) };
	}

	void test_dispatch_matches_the_detail_functions()
	{
		const auto syntax_pos = source_position{ 3, 7 };
		const auto syn        = syntax_raw(syntax_error_code::entity_redefine, variant_t{ rt::identifier{ u8"x" } }, syntax_pos);
		check(pdn::error_message_generator_en(syn)
			== u8"entity \"x\" is redefined in this scope", "syntax detail");
		check(pdn::error_message_generator_en(syn)
			== pdn::detail::err_msg_gen_en(syntax_error_code::entity_redefine, syntax_pos, syn.raw_error_message),
			"syntax dispatch equals the detail call");

		const auto lex = lexical_raw(lexical_error_code::not_unicode_scalar_value,
			variant_t{ rt::not_unicode_scalar_value{ 0xD800u } });
		check(pdn::error_message_generator_en(lex)
			== u8"PDN lexer requires a Unicode scalar value: 0x0000d800", "lexical detail");
		check(pdn::error_message_generator_en(lex)
			== pdn::detail::err_msg_gen_en(lexical_error_code::not_unicode_scalar_value, source_position{},
				lex.raw_error_message),
			"lexical dispatch equals the detail call");

		const auto u8_raw = utf8_raw(utf8::decode_error_code::invalid_code_point,
			variant_t{ rt::utf8_decode_error{ utf8::decode_result{}, utf8::code_unit_t{ 0x80 }, 4 } });
		check(pdn::error_message_generator_en(u8_raw)
			== u8"invalid code point: 0x0000, in the sequence at offset 0x0004 (add 3 if the input starts with a BOM), 1 code unit was read",
			"utf-8 detail");
		check(pdn::error_message_generator_en(u8_raw)
			== pdn::detail::err_msg_gen_en(utf8::decode_error_code::invalid_code_point, source_position{},
				u8_raw.raw_error_message),
			"utf-8 dispatch equals the detail call");

		const auto u16_raw = utf16_raw(utf16::decode_error_code::incomplete_sequence,
			variant_t{ rt::utf16_decode_error{ utf16::decode_result{}, utf16::code_unit_t{ 0xdc00 }, 4 } });
		check(pdn::error_message_generator_en(u16_raw)
			== u8"incomplete sequence (expected a UTF-16 trailing surrogate), code unit: 0xdc00, in the sequence at offset 0x0008 (add 2 if the input starts with a BOM), 1 code unit was read",
			"utf-16 detail");

		const auto u32_raw = utf32_raw(utf32::decode_error_code::invalid_code_point,
			variant_t{ rt::utf32_decode_error{ utf32::decode_result{}, utf32::code_unit_t{}, 8 } });
		check(pdn::error_message_generator_en(u32_raw)
			== u8"invalid code point: 0x00000000, in the sequence at offset 0x0020 (add 4 if the input starts with a BOM), 1 code unit was read",
			"utf-32 detail");
	}

	void test_position_is_not_part_of_the_text()
	{
		// The category name and the position are the caller's job (default_error_message_generator),
		// so the position must not leak into the text even when it is not the default.
		const auto raw = syntax_raw(syntax_error_code::entity_redefine, variant_t{ rt::identifier{ u8"x" } },
			source_position{ 12, 34 });
		const auto text = pdn::error_message_generator_en(raw);
		check(text.find(u8"(12:34)") == error_msg_string::npos, "no position prefix");
	}

	void test_accepts_lvalue_and_rvalue()
	{
		auto raw = lexical_raw(lexical_error_code::unacceptable_character, variant_t{ rt::error_string{ u8"x" } });
		const auto from_lvalue = pdn::error_message_generator_en(raw);
		const auto from_rvalue = pdn::error_message_generator_en(::std::move(raw));
		check(from_lvalue == from_rvalue, "lvalue and rvalue calls agree");
		check(from_lvalue == u8"unacceptable character: \"x\"", "the payload survives both calls");
	}
}

int main()
{
	test_dispatch_matches_the_detail_functions();
	test_position_is_not_part_of_the_text();
	test_accepts_lvalue_and_rvalue();
	return fails == 0 ? 0 : 1;
}
