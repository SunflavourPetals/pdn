#ifndef PDN_Header_pdn_parser_utility
#define PDN_Header_pdn_parser_utility

#include <type_traits>
#include <concepts>
#include <cstddef>

#include "pdn_entity.h"
#include "pdn_types.h"
#include "pdn_proxy.h"
#include "pdn_token_code.h"
#include "pdn_token.h"
#include "pdn_exception.h"
#include "pdn_type_code.h"
#include "pdn_error_string.h"
#include "pdn_code_convert.h"
#include "pdn_raw_error_message_type.h"
#include "pdn_source_position.h"

namespace pdn::dev_util
{
	template <typename type, typename target_type>
	concept remove_cvref_same_as = ::std::is_same_v<::std::remove_cvref_t<type>, ::std::remove_cvref_t<target_type>>;
}

namespace pdn::concepts
{
	template <typename type, typename char_t>
	concept token_iterator = requires (type it)
	{
		{ *it } -> ::std::convertible_to<token<char_t>>;
		++it;
	};

	template <typename type>
	concept utf_8_code_unit_iterator = requires (type it)
	{
		{ *it } -> dev_util::remove_cvref_same_as<unicode::utf_8_code_unit_t>;
		++it;
	};
	template <typename type>
	concept utf_16_code_unit_iterator = requires (type it)
	{
		{ *it } -> dev_util::remove_cvref_same_as<unicode::utf_16_code_unit_t>;
		++it;
	};
	template <typename type>
	concept utf_32_code_unit_iterator = requires (type it)
	{
		{ *it } -> dev_util::remove_cvref_same_as<unicode::utf_32_code_unit_t>;
		++it;
	};
	template <typename type>
	concept utf_code_unit_iterator
		 = utf_8_code_unit_iterator<type>
		|| utf_16_code_unit_iterator<type>
		|| utf_32_code_unit_iterator<type>;
}

namespace pdn::parser_utility
{
	constexpr bool is_unary_operator(pdn_token_code code) noexcept
	{
		return code == pdn_token_code::minus || code == pdn_token_code::plus;
	}

	constexpr bool is_negative_sign(pdn_token_code code) noexcept
	{
		return code == pdn_token_code::minus;
	}

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

	constexpr bool is_list_element_first(pdn_token_code c) noexcept
	{
		return is_expr_first(c) || c == pdn_token_code::identifier;
	}

	template <typename code_unit_t>
	constexpr auto default_entity_value(type_code type_c) -> entity<code_unit_t>
	{
		using char_t   = types::character<code_unit_t>;
		using string_t = types::string<code_unit_t>;
		using list_t   = types::list<code_unit_t>;
		using object_t = types::object<code_unit_t>;
		using enum type_code;
		switch (type_c)
		{
		case i8:        return types::i8{};
		case i16:       return types::i16{};
		case i32:       return types::i32{};
		case i64:       return types::i64{};
		case u8:        return types::u8{};
		case u16:       return types::u16{};
		case u32:       return types::u32{};
		case u64:       return types::u64{};
		case f32:       return types::f32{};
		case f64:       return types::f64{};
		case boolean:   return types::boolean{};
		case character: return char_t{};
		case string:    return make_proxy<string_t>();
		case list:      return make_proxy<list_t>();
		case object:    return make_proxy<object_t>();
		default:        return types::cppint{};
		}
	}

	template <typename char_t>
	constexpr auto to_raw_error_token(token<char_t> src) -> raw_error_message_type::error_token
	{
		if constexpr (::std::same_as<char_t, error_msg_char>)
		{
			return raw_error_message_type::error_token{ ::std::move(src) };
		}
		else
		{
			return raw_error_message_type::error_token{ dev_util::token_convert<error_msg_char>(::std::move(src)) };
		}
	}

	struct unary_record
	{
		source_position last_positive_sign_pos{}; // P-pos
		source_position last_negative_sign_pos{}; // N-pos
		::std::size_t   negative_sign_count{};
		bool            is_last_sign_negative{};  // true => N-pos is valid, false => P-pos is valis
		bool            has_sign{};               // true => this object is valid
		explicit unary_record(bool v) : has_sign{ v } {}
	};
}

#endif
