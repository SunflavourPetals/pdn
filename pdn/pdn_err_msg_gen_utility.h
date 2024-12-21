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

#include "pdn_types.h"
#include "pdn_proxy.h"
#include "pdn_exception.h"
#include "pdn_error_string.h"
#include "pdn_code_convert.h"
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
	namespace raw_details = raw_error_message_type;
	inline auto make_slashes(error_msg_string_view view) -> error_msg_string
	{
		return make_slashes_string<error_msg_string>(unicode::code_convert<unicode::code_point_string>(view));
	}
	inline auto token_value_variant_to_s(const token_value_variant<error_msg_char>& variant) -> error_msg_string
	{
		return ::std::visit([](const auto& arg) -> error_msg_string
		{
			using arg_t = ::std::decay_t<decltype(arg)>;
			if constexpr (::std::same_as<arg_t, types::character<error_msg_char>>)
			{
				return u8"\'"_em + error_msg_string{ arg.to_string_view() } + u8"\'"_em;
			}
			else if constexpr (::std::same_as<arg_t, proxy<types::string<error_msg_char>>>)
			{
				return u8"\""_em + make_slashes(*arg) + u8"\""_em;
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
				return u8"\"@"_em.append(arg.get_id()).append(u8"\""_em);
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
		using namespace literals::error_message_literals;
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
		using namespace literals::error_message_literals;
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
		default:  throw  inner_error{ "get_integer_min_value for non-integer type" }; return 0;
		}
	}

	// for casting_msg from casting_domain_error
	inline auto get_integer_min_value_s(type_code c) -> error_msg_string
	{
		return reinterpret_to_err_msg_str(::std::to_string(get_integer_min_value(c)));
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
		default:  throw  inner_error{ "get_integer_max_value for non-integer type" }; return 0;
		}
	}

	// for casting_msg from casting_domain_error
	inline auto get_integer_max_value_s(type_code c) -> error_msg_string
	{
		return reinterpret_to_err_msg_str(::std::to_string(get_integer_max_value(c)));
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
		return u8"\"@"_em + make_slashes(::std::get<raw_details::identifier>(raw).value) + u8"\""_em;
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
		using namespace literals::error_message_literals;
		auto is_negative = ::std::get<raw_details::unary_operation>(raw).negative;
		return is_negative ? u8"operator-"_em : u8"operator+"_em;
	}

	// for unary_operation
	inline auto get_description_for_unary_operation(raw_err_v_cref raw) -> error_msg_string
	{
		const auto& msg = ::std::get<raw_details::unary_operation>(raw);
		using namespace literals::error_message_literals;
		return is_mono(msg.operand)
			? get_operand_type_name(raw)
			: get_operand_type_name(raw)
			+ u8" with value "_em
			+ token_value_variant_to_s(msg.operand);
	}
}

namespace pdn::dev_util::err_msg_gen_util
{
	template <int base = 10, ::std::size_t width = 1, ::std::size_t buffer_size = 64>
	inline auto to_s(::std::integral auto const val) -> error_msg_string
	{
		using namespace literals::error_message_literals;
		::std::array<char, buffer_size> buffer{};
		auto to_chars_result = ::std::to_chars(buffer.data(), buffer.data() + buffer.size(), val, base);
		if (to_chars_result.ec != ::std::errc{})
		{
			if (to_chars_result.ec == ::std::errc::value_too_large)
			{
				throw inner_error{ "too large value" };
			}
			else
			{
				throw inner_error{ "unknown to_chars error" };
			}
		}
		if (to_chars_result.ptr - buffer.data() < 0)
		{
			throw inner_error{ "to chars error: to_chars_result.ptr < begin(buffer)" };
		}
		auto begin  = reinterpret_cast<error_msg_char*>(buffer.data());
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
}

namespace pdn::dev_util::err_msg_gen_util::lexical_err_msg_gen_util
{
	// for not_unicode_scalar_value
	inline auto get_code_point_hex(raw_err_v_cref raw) -> error_msg_string
	{
		return u8"0x"_em.append(to_s<16, 8>(::std::get<raw_details::not_unicode_scalar_value>(raw).value));
	}
}

#endif
