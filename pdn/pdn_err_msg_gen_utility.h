#ifndef PDN_Header_pdn_err_msg_gen_utility
#define PDN_Header_pdn_err_msg_gen_utility

#include <type_traits>
#include <concepts>
#include <variant>
#include <string>
#include <array>
#include <limits>
#include <cstddef>
#include <charconv>
#include <cassert>

#include "pdn_types.h"
#include "pdn_proxy.h"
#include "pdn_error_string.h"
#include "pdn_utf_code_convert.h"
#include "pdn_make_slashes_string.h"
#include "pdn_raw_error_message_type.h"
#include "pdn_raw_error_message_variant.h"
#include "pdn_type_code_to_type.h"
#include "pdn_type_code_to_error_msg_string.h"
#include "pdn_token_code_to_error_msg_string.h"

#include "pdn_unicode.h"

namespace pdn::dev_util
{
	using raw_err_v_cref = const raw_error_message_variant&;
}

namespace pdn::dev_util::err_msg_gen_util
{
	using namespace literals::error_message_literals;
	inline auto add_quote(error_msg_string_view src) -> error_msg_string
	{
		return u8"\""_em.append(src).append(u8"\"");
	}
	inline auto add_single_quote(error_msg_string_view src) -> error_msg_string
	{
		return u8"'"_em.append(src).append(u8"'");
	}
	template <int base = 10, ::std::size_t width = 1, ::std::size_t buffer_size = 64>
	inline auto to_s(::std::integral auto const val) -> error_msg_string
	{
		::std::array<char, buffer_size> buffer{};
		auto to_chars_result = ::std::to_chars(buffer.data(), buffer.data() + buffer.size(), val, base);
		if (to_chars_result.ec != ::std::errc{})
		{
			if (to_chars_result.ec == ::std::errc::value_too_large)
			{
				assert(0 && "too large value");
			}
			else
			{
				assert(0 && "unknown to_chars error");
			}
		}
		assert(to_chars_result.ptr > buffer.data());
		auto begin = reinterpret_cast<error_msg_char*>(buffer.data());
		auto length = static_cast<::std::size_t>(to_chars_result.ptr - buffer.data());
		auto result = error_msg_string{};
		for (auto i = length; i < width; ++i)
		{
			result += u8'0';
		}
		result.append(begin, length);
		return result;
	}
	// for utf_8|16|32_decode_error
	inline auto offset_of_leading(const auto& msg, const int multi = 1) -> error_msg_string
	{
		return u8"0x"_em.append(to_s<16, 4>((msg.last_code_unit_offset - msg.result.distance()) * multi));
	}

	namespace raw_details = raw_error_message_type;
	inline auto make_slashes(error_msg_string_view view) -> error_msg_string
	{
		return make_slashes_string<error_msg_string>(unicode::code_convert<unicode::ucpstring>(view));
	}
	inline auto token_value_variant_to_s(const token_value_variant<error_msg_char>& variant) -> error_msg_string
	{
		return ::std::visit([](const auto& arg) -> error_msg_string
		{
			using arg_t = ::std::decay_t<decltype(arg)>;
			if constexpr (::std::same_as<arg_t, types::character<error_msg_char>>)
			{
				return add_single_quote(arg.to_string_view());
			}
			else if constexpr (::std::same_as<arg_t, proxy<types::string<error_msg_char>>>)
			{
				return add_quote(make_slashes(*arg));
			}
			else if constexpr (::std::same_as<arg_t, types::boolean>)
			{
				return arg ? u8"true"_em : u8"false"_em;
			}
			else if constexpr (::std::same_as<arg_t, ::std::monostate>)
			{
				return u8"monostate"_em;
			}
			else if constexpr (::std::same_as<arg_t, dev_util::at_iden_string_proxy>)
			{
				return add_quote(u8"@"_em.append(arg.get_id()));
			}
			else
			{
				return reinterpret_to_err_msg_str(::std::to_string(arg));
			}
		}, variant);
	}
	inline auto is_mono(const token_value_variant<error_msg_char>& variant) -> bool
	{
		return static_cast<bool>(::std::get_if<::std::monostate>(&variant));
	}
	inline auto description_of(const token<error_msg_char>& token_val) -> error_msg_string
	{
		return is_mono(token_val.value)
			? token_code_to_error_msg_string(token_val.code)
			: token_code_to_error_msg_string(token_val.code)
			+ u8" with value "_em
			+ token_value_variant_to_s(token_val.value);
	}
}

namespace pdn::dev_util::err_msg_gen_util::syntax_err_msg_gen_util
{
	// for identifier
	inline auto get_slashes_iden(raw_err_v_cref raw) -> error_msg_string
	{
		return make_slashes(::std::get<raw_details::identifier>(raw).value);
	}

	// for casting_msg
	inline auto get_source_type_name(raw_err_v_cref raw) -> error_msg_string
	{
		return type_code_to_error_msg_string(::std::get<raw_details::casting_msg>(raw).source_type);
	}

	// for casting_msg
	inline auto get_target_type_name(raw_err_v_cref raw) -> error_msg_string
	{
		return type_code_to_error_msg_string(::std::get<raw_details::casting_msg>(raw).target_type);
	}

	// for casting_msg
	inline auto is_source_type_list_or_object(raw_err_v_cref raw) -> bool
	{
		const auto src_type_c = ::std::get<raw_details::casting_msg>(raw).source_type;
		return src_type_c == type_code::list || src_type_c == type_code::object;
	}

	// for casting_msg
	inline auto get_casting_operand(raw_err_v_cref raw) -> error_msg_string
	{
		using raw_details::casting_msg;

		switch (::std::get<raw_details::casting_msg>(raw).source_type)
		{
		case type_code::list:   return u8"list"_em;
		case type_code::object: return u8"object"_em;
		default:                return token_value_variant_to_s(::std::get<casting_msg>(raw).operand);
		}
	}

	// for casting_msg from casting_domain_error
	inline auto get_integer_min_value(type_code c) -> types::i64
	{
		using char_t = error_msg_char;
		using enum type_code;
		switch (c)
		{
		case i8:  return ::std::numeric_limits<type_code_to_type_t<i8,  char_t>>::min();
		case i16: return ::std::numeric_limits<type_code_to_type_t<i16, char_t>>::min();
		case i32: return ::std::numeric_limits<type_code_to_type_t<i32, char_t>>::min();
		case i64: return ::std::numeric_limits<type_code_to_type_t<i64, char_t>>::min();
		case u8:  return ::std::numeric_limits<type_code_to_type_t<u8,  char_t>>::min();
		case u16: return ::std::numeric_limits<type_code_to_type_t<u16, char_t>>::min();
		case u32: return ::std::numeric_limits<type_code_to_type_t<u32, char_t>>::min();
		case u64: return ::std::numeric_limits<type_code_to_type_t<u64, char_t>>::min();
		default:  assert(0 && "get_integer_min_value for non-integer type"); return 0;
		}
	}

	// for casting_msg from casting_domain_error
	inline auto get_integer_min_value_s(type_code c) -> error_msg_string
	{
		return to_s(get_integer_min_value(c));
	}

	// for casting_msg from casting_domain_error
	inline auto get_integer_max_value(type_code c) -> types::u64
	{
		using char_t = error_msg_char;
		using enum type_code;
		switch (c)
		{
		case i8:  return ::std::numeric_limits<type_code_to_type_t<i8,  char_t>>::max();
		case i16: return ::std::numeric_limits<type_code_to_type_t<i16, char_t>>::max();
		case i32: return ::std::numeric_limits<type_code_to_type_t<i32, char_t>>::max();
		case i64: return ::std::numeric_limits<type_code_to_type_t<i64, char_t>>::max();
		case u8:  return ::std::numeric_limits<type_code_to_type_t<u8,  char_t>>::max();
		case u16: return ::std::numeric_limits<type_code_to_type_t<u16, char_t>>::max();
		case u32: return ::std::numeric_limits<type_code_to_type_t<u32, char_t>>::max();
		case u64: return ::std::numeric_limits<type_code_to_type_t<u64, char_t>>::max();
		default:  assert(0 && "get_integer_max_value for non-integer type"); return 0;
		}
	}

	// for casting_msg from casting_domain_error
	inline auto get_integer_max_value_s(type_code c) -> error_msg_string
	{
		return to_s(get_integer_max_value(c));
	}

	// for casting_msg from casting_domain_error
	inline auto get_target_min_s(raw_err_v_cref raw) -> error_msg_string
	{
		return get_integer_min_value_s(::std::get<raw_details::casting_msg>(raw).target_type);
	}

	// for casting_msg from casting_domain_error
	inline auto get_target_max_s(raw_err_v_cref raw) -> error_msg_string
	{
		return get_integer_max_value_s(::std::get<raw_details::casting_msg>(raw).target_type);
	}

	// for at_value_not_found
	inline auto get_at_iden_s(raw_err_v_cref raw) -> error_msg_string
	{
		return add_quote(u8"@"_em.append(make_slashes(::std::get<raw_details::identifier>(raw).value)));
	}

	// for error_token
	inline auto get_description_for_error_token(raw_err_v_cref raw) -> error_msg_string
	{
		return description_of(::std::get<raw_details::error_token>(raw).value);
	}

	// for unary_operation
	inline auto get_operand_type_name(raw_err_v_cref raw) -> error_msg_string
	{
		return type_code_to_error_msg_string(::std::get<raw_details::unary_operation>(raw).operand_type);
	}

	// for unary_operation
	inline auto is_operand_type_list_or_object(raw_err_v_cref raw) -> bool
	{
		const auto src_type_c = ::std::get<raw_details::unary_operation>(raw).operand_type;
		return src_type_c == type_code::list || src_type_c == type_code::object;
	}

	// for unary_operation
	inline auto get_unary_operator_s(raw_err_v_cref raw) -> error_msg_string
	{
		auto is_negative = ::std::get<raw_details::unary_operation>(raw).negative;
		return is_negative ? u8"operator-"_em : u8"operator+"_em;
	}

	// for unary_operation
	inline auto get_description_for_unary_operation(raw_err_v_cref raw) -> error_msg_string
	{
		const auto& msg = ::std::get<raw_details::unary_operation>(raw);
		return is_mono(msg.operand)
			? get_operand_type_name(raw)
			: get_operand_type_name(raw)
			+ u8" with value "_em
			+ token_value_variant_to_s(msg.operand);
	}
}

namespace pdn::dev_util::err_msg_gen_util::lexical_err_msg_gen_util
{
	// for not_unicode_scalar_value
	inline auto get_code_point_hex(raw_err_v_cref raw) -> error_msg_string
	{
		return u8"0x"_em.append(to_s<16, 8>(::std::get<raw_details::not_unicode_scalar_value>(raw).value));
	}
	// for error_string
	inline auto get_slashes_s(raw_err_v_cref raw) -> error_msg_string
	{
		return make_slashes(::std::get<raw_details::error_string>(raw).value);
	}
	// for error_string
	inline auto get_err_s(raw_err_v_cref raw) -> error_msg_string
	{
		return ::std::get<raw_details::error_string>(raw).value;
	}
	// for error_string
	inline auto get_quoted_slashes_s(raw_err_v_cref raw) -> error_msg_string
	{
		const auto& err_s_ref = ::std::get<raw_details::error_string>(raw).value;
		return add_quote(make_slashes(err_s_ref));
	}
	// for error_string escape error
	inline auto get_esc_quoted_s(raw_err_v_cref raw) -> error_msg_string
	{
		const auto& s_ref = ::std::get<raw_details::error_string>(raw).value;
		return add_quote(u8"\\"_em.append(make_slashes(s_ref)));
	}
	// for error_string
	inline auto get_single_quoted_slashes_s(raw_err_v_cref raw) -> error_msg_string
	{
		const auto& err_s_ref = ::std::get<raw_details::error_string>(raw).value;
		return add_single_quote(make_slashes(err_s_ref));
	}
	// for character_length_error
	inline auto get_single_quoted_slashes_s_for_cle(raw_err_v_cref raw) -> error_msg_string
	{
		const auto& err_s_ref = ::std::get<raw_details::character_length_error>(raw).value;
		return add_single_quote(make_slashes(err_s_ref));
	}
	// for character_length_error
	inline auto get_cle_cp_length(raw_err_v_cref raw) -> error_msg_string
	{
		return to_s(::std::get<raw_details::character_length_error>(raw).code_point_count);
	}
	// for error_string
	inline auto get_quoted_slashes_s_for_mts(raw_err_v_cref raw) -> error_msg_string
	{
		const auto& msg = ::std::get<raw_details::missing_terminating_sequence>(raw);
		const auto full = u8"@"_em
			.append(msg.is_raw_identifier_string ? u8"`"_em : u8"\""_em)
			.append(make_slashes(msg.d_seq))
			.append(u8"("_em)
			.append(make_slashes(msg.content));
		return add_quote(full);
	}
	// for error_string
	inline auto get_slashes_d_seq_for_mts(raw_err_v_cref raw) -> error_msg_string
	{
		const auto& err_s_ref = ::std::get<raw_details::missing_terminating_sequence>(raw).d_seq;
		return make_slashes(err_s_ref);
	}
	// for number_end_with_separator
	inline auto get_s_for_ews(raw_err_v_cref raw) -> error_msg_string
	{
		return ::std::get<raw_details::number_end_with_separator>(raw).number_sequence;
	}
	// for from_chars_error
	inline auto get_quoted_from_chars_src_s(raw_err_v_cref raw) -> error_msg_string
	{
		return add_quote(make_slashes(::std::get<raw_details::from_chars_error>(raw).sequence));
	}
	// for from_chars_error
	inline auto get_quoted_from_chars_incomplete_s(raw_err_v_cref raw) -> error_msg_string
	{
		const auto& msg = ::std::get<raw_details::from_chars_error>(raw);
		const auto view = error_msg_string_view{ msg.sequence.data(), msg.sequence.data() + msg.offset };
		return add_quote(make_slashes(view));
	}
	// for from_chars_error
	inline auto get_from_chars_desc_s(raw_err_v_cref raw) -> error_msg_string
	{
		const auto& msg = ::std::get<raw_details::from_chars_error>(raw);
		using enum raw_details::number_type;
		switch (msg.type)
		{
		case bin_integer:  return u8"integer, base 2"_em;
		case oct_integer:  return u8"integer, base 8"_em;
		case dec_integer:  return u8"integer, base 10"_em;
		case hex_integer:  return u8"integer, base 16"_em;
		case dec_floating: return u8"floating, default chars_format(general)"_em;
		case hex_floating: return u8"floating, chars_format hex"_em;
		default: assert(0 && "enum unhandled"); return u8"unknown"_em;
		}
	}
	// for from_chars_error
	inline auto get_from_chars_errc_s(raw_err_v_cref raw) -> error_msg_string
	{
		return to_s(static_cast<int>(::std::get<raw_details::from_chars_error>(raw).ec));
	}
	// for escape_not_unicode_scalar_value
	inline auto get_value_from_esc_not_u_scalar(raw_err_v_cref raw) -> error_msg_string
	{
		return u8"0x"_em.append(to_s<16, 4>(static_cast<int>(::std::get<raw_details::escape_not_unicode_scalar_value>(raw).code_point)));
	}
	// for escape_not_unicode_scalar_value
	inline auto get_quoted_seq_from_esc_not_u_scalar(raw_err_v_cref raw) -> error_msg_string
	{
		const auto& s_ref = ::std::get<raw_details::escape_not_unicode_scalar_value>(raw).escape_sequence;
		return add_quote(u8"\\"_em.append(make_slashes(s_ref)));
	}
	// for delimiter_error
	inline auto get_prefix_and_d_seq(raw_err_v_cref raw) -> error_msg_string
	{
		const auto& msg = ::std::get<raw_details::delimiter_error>(raw);
		return make_slashes((msg.is_raw_identifier_string ? u8"@`"_em : u8"@\""_em) + msg.d_seq);
	}
	// for delimiter_error
	inline auto get_prefix_and_d_seq_with_par(raw_err_v_cref raw) -> error_msg_string
	{
		return get_prefix_and_d_seq(raw) + u8"("_em;
	}
}

#endif
