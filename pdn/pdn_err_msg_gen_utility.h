#ifndef PDN_Header_pdn_err_msg_gen_utility
#define PDN_Header_pdn_err_msg_gen_utility

#include <type_traits>
#include <concepts>
#include <variant>
#include <string>
#include <limits>

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

namespace pdn::dev_util::err_msg_gen_util
{
	inline auto make_slashes(error_msg_string_view view) -> error_msg_string
	{
		return make_slashes_string<error_msg_string>(unicode::code_convert<unicode::code_point_string>(view));
	}
}

namespace pdn::dev_util::err_msg_gen_util::syntax_err_msg_gen_util
{
	// for identifier
	inline auto get_slashes_iden(const raw_error_message_variant& raw) -> error_msg_string
	{
		return make_slashes(::std::get<raw_error_message_type::identifier>(raw).value);
	}

	// for casting_msg
	inline auto get_source_type_name(const raw_error_message_variant& raw) -> error_msg_string
	{
		return type_code_to_error_msg_string(::std::get<raw_error_message_type::casting_msg>(raw).source_type);
	}
	// for casting_msg
	inline auto get_target_type_name(const raw_error_message_variant& raw) -> error_msg_string
	{
		return type_code_to_error_msg_string(::std::get<raw_error_message_type::casting_msg>(raw).target_type);
	}

	// for casting_msg
	inline auto is_source_type_list_or_object(const raw_error_message_variant& raw) -> bool
	{
		const auto src_type_c = ::std::get<raw_error_message_type::casting_msg>(raw).source_type;
		return src_type_c == type_code::list || src_type_c == type_code::object;
	}

	// for casting_msg
	inline auto get_casting_operand(const raw_error_message_variant& raw) -> error_msg_string
	{
		using namespace literals::error_message_literals;
		const auto src_type_c = ::std::get<raw_error_message_type::casting_msg>(raw).source_type;
		if (src_type_c == type_code::list)
		{
			return u8"list"_em;
		}
		else if (src_type_c == type_code::object)
		{
			return u8"object"_em;
		}
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
				throw inner_error{ "get value from monostate" };
				return u8"error"_em;
			}
			else
			{
				return reinterpret_to_err_msg_str(::std::to_string(arg));
			}
		}, ::std::get<raw_error_message_type::casting_msg>(raw).operand);
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
	inline auto get_target_min_s(const raw_error_message_variant& raw) -> error_msg_string
	{
		return get_integer_min_value_s(::std::get<raw_error_message_type::casting_msg>(raw).target_type);
	}

	// for casting_msg from casting_domain_error
	inline auto get_target_max_s(const raw_error_message_variant& raw) -> error_msg_string
	{
		return get_integer_max_value_s(::std::get<raw_error_message_type::casting_msg>(raw).target_type);
	}

	// for error_token
	inline auto is_error_token_with_value(const raw_error_message_variant& raw) -> bool
	{
		return !::std::get_if<::std::monostate>(&::std::get<raw_error_message_type::error_token>(raw).value.value);
	}


}

#endif
