#ifndef PDN_Header_pdn_error_code_variant_to_error_msg_string
#define PDN_Header_pdn_error_code_variant_to_error_msg_string

#include <variant>
#include <concepts>
#include <type_traits>

#include "pdn_error_string.h"
#include "pdn_error_code_variant.h"

namespace pdn
{
	inline error_msg_string error_code_variant_to_error_msg_string(error_code_variant err_c)
	{
		return ::std::visit([](auto c) -> error_msg_string
		{
			using error_type = ::std::decay_t<decltype(c)>;
			using namespace error_message_literals;

			if constexpr (::std::same_as<error_type, unicode::utf_8::decode_error_code>)
			{
				return u8"utf-8 decode error"_s;
			}
			else if constexpr (::std::same_as<error_type, unicode::utf_8::encode_error_code>)
			{
				return u8"utf-8 encode error"_s;
			}
			else if constexpr (::std::same_as<error_type, unicode::utf_16::decode_error_code>)
			{
				return u8"utf-16 decode error"_s;
			}
			else if constexpr (::std::same_as<error_type, unicode::utf_16::encode_error_code>)
			{
				return u8"utf-16 encode error"_s;
			}
			else if constexpr (::std::same_as<error_type, unicode::utf_32::decode_error_code>)
			{
				return u8"utf-32 decode error"_s;
			}
			else if constexpr (::std::same_as<error_type, unicode::utf_32::encode_error_code>)
			{
				return u8"utf-32 encode error"_s;
			}
			else if constexpr (::std::same_as<error_type, lexical_error_code>)
			{
				return u8"lexical error"_s;
			}
			else if constexpr (::std::same_as<error_type, syntax_error_code>)
			{
				return u8"syntax error"_s;
			}
			else
			{
				static_assert(false, "[pdn] not all cases are enumerated");
				return {};
			}
		}, err_c);
	}
}

#endif
