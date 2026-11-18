#ifndef PDN_Header_pdn_parser_utility
#define PDN_Header_pdn_parser_utility

#include <type_traits>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <utility>

#include "pdn_entity.h"
#include "pdn_type.h"
#include "pdn_proxy.h"
#include "pdn_token_code.h"
#include "pdn_token.h"
#include "pdn_token_convert.h"
#include "pdn_type_code.h"
#include "pdn_error_string.h"
#include "pdn_raw_error_message_type.h"
#include "pdn_source_position.h"
#include "pdn_remove_cvref_same_as_concept.h"

// Shared vocabulary of the parser layer: what the parser accepts as a token stream and as a string
// of code units, plus the small helpers that turn a token code into a parse decision and a type
// code into the entity value used when an error forces the parser to continue.

namespace pdn::concepts
{
	// Iterators over a token stream of the requested character type. Only what the parser reads from
	// one element is covered here; comparing against the end of the stream is left to the caller,
	// so the sentinel type is not part of the concept.
	template <typename type, typename char_t>
	concept token_iterator = requires (type it)
	{
		{ *it } -> ::std::convertible_to<token<char_t>>;
		++it;
	};

	// Iterators over the code units of one UTF encoding: the input the code point iterators consume.
	template <typename type>
	concept utf8_code_unit_iterator = requires (type it)
	{
		{ *it } -> detail::remove_cvref_same_as<unicode::u8char_t>;
		++it;
	};
	template <typename type>
	concept utf16_code_unit_iterator = requires (type it)
	{
		{ *it } -> detail::remove_cvref_same_as<unicode::u16char_t>;
		++it;
	};
	template <typename type>
	concept utf32_code_unit_iterator = requires (type it)
	{
		{ *it } -> detail::remove_cvref_same_as<unicode::u32char_t>;
		++it;
	};

	// Any of the three code unit iterator kinds.
	template <typename type>
	concept utf_code_unit_iterator
		 = utf8_code_unit_iterator<type>
		|| utf16_code_unit_iterator<type>
		|| utf32_code_unit_iterator<type>;

	// A string-like object readable as code units of one UTF encoding. A C array is rejected below
	// because it carries a terminating null code unit that is not part of the text.
	template <typename type>
	concept utf_code_unit_string_view
		 = requires (const type& sv) { ::std::cbegin(sv); ::std::cend(sv); }
		&& !::std::is_array_v<type>
		&& utf_code_unit_iterator<decltype(::std::cbegin(::std::declval<const type&>()))>;
}

namespace pdn::parser_utility
{
	// A sign may precede an expression; the parser records it separately from the operand.
	constexpr bool is_unary_operator(pdn_token_code code) noexcept
	{
		return code == pdn_token_code::minus || code == pdn_token_code::plus;
	}

	// Minus is the only sign that changes the value of the operand; plus only has to be consumed.
	constexpr bool is_negative_sign(pdn_token_code code) noexcept
	{
		return code == pdn_token_code::minus;
	}

	// Token codes that can start an expression: literals, at constants, signs and both bracket
	// kinds. literal_boolean is listed for completeness although the lexer never produces it.
	constexpr bool is_expr_first(pdn_token_code c) noexcept
	{
		using enum pdn_token_code;
		switch (c)
		{
		case at_identifier:
		case minus:
		case plus:
		case literal_boolean:
		case literal_character:
		case literal_string:
		case literal_floating_point:
		case literal_integer:
		case left_brackets:
		case left_curly_brackets:
			return true;
		default:
			return false;
		}
	}

	// A list element may also be a named entity definition ("name: type value").
	constexpr bool is_list_element_first(pdn_token_code c) noexcept
	{
		return is_expr_first(c) || c == pdn_token_code::identifier;
	}

	// The value a definition gets when an error prevents its value from being parsed: zero for
	// numbers, false for boolean, a character holding U+0000, and an empty proxy for
	// string/list/object. Without a usable type specifier (unknown) the value is i32 zero.
	template <typename code_unit_t>
	auto default_entity_value(type_code type_c) -> entity<code_unit_t>
	{
		using character_t = type::character<code_unit_t>;
		using string_t    = type::string<code_unit_t>;
		using list_t      = type::list<code_unit_t>;
		using object_t    = type::object<code_unit_t>;
		using enum type_code;
		switch (type_c)
		{
		case i8:        return type::i8{};
		case i16:       return type::i16{};
		case i32:       return type::i32{};
		case i64:       return type::i64{};
		case u8:        return type::u8{};
		case u16:       return type::u16{};
		case u32:       return type::u32{};
		case u64:       return type::u64{};
		case f32:       return type::f32{};
		case f64:       return type::f64{};
		case boolean:   return type::boolean{};
		case character: return character_t{};
		case string:    return make_proxy<string_t>();
		case list:      return make_proxy<list_t>();
		case object:    return make_proxy<object_t>();
		default:        return type::auto_int{};
		}
	}

	// Raw error payloads carry their token in the message character type (UTF-8); convert only when
	// the lexer tokenized another character type.
	template <typename char_t>
	auto to_raw_error_token(token<char_t> src) -> raw_error_message_type::error_token
	{
		if constexpr (::std::same_as<char_t, error_msg_char>)
		{
			return raw_error_message_type::error_token{ ::std::move(src) };
		}
		else
		{
			return raw_error_message_type::error_token{ detail::token_convert<error_msg_char>(::std::move(src)) };
		}
	}

	// The unary signs recorded for one operand: where the last plus and the last minus were seen
	// (an error must point at the sign that decided the result), how many minus signs were seen
	// (its parity is the sign of the value), and whether any sign was seen at all.
	struct unary_record
	{
		source_position last_positive_sign_pos{}; // the last '+' seen
		source_position last_negative_sign_pos{}; // the last '-' seen
		::std::size_t   negative_sign_count{};
		bool            is_last_sign_negative{};  // true: the last sign seen was a '-'
		bool            has_sign{};               // true: at least one sign was seen
		explicit unary_record(bool v) : has_sign{ v } {}
	};
}

#endif
