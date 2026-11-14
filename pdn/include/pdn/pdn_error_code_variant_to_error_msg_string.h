#ifndef PDN_Header_pdn_error_code_variant_to_error_msg_string
#define PDN_Header_pdn_error_code_variant_to_error_msg_string

#include <variant>
#include <concepts>
#include <type_traits>

#include "pdn_false.h"
#include "pdn_error_string.h"
#include "pdn_error_code_variant.h"

namespace pdn
{
	// The category label of an error code, without position or details: "lexical error",
	// "syntax error", "utf-8 decode error". The caller (the default message generator) appends
	// "(line:column) " and then the detailed text. The view points into static storage, so it
	// stays valid for the lifetime of the program. An alternative that is not handled here falls
	// into the static_assert below, so adding one to error_code_variant cannot go unnoticed.
	inline auto error_code_variant_to_error_msg_string_view(error_code_variant err_c) -> error_msg_string_view
	{
		return ::std::visit([](auto c) -> error_msg_string_view
		{
			using error_type = ::std::decay_t<decltype(c)>;
			using namespace error_message_literals;

			if constexpr (::std::same_as<error_type, unicode::utf8::decode_error_code>)
			{
				return u8"utf-8 decode error"_emv;
			}
			else if constexpr (::std::same_as<error_type, unicode::utf16::decode_error_code>)
			{
				return u8"utf-16 decode error"_emv;
			}
			else if constexpr (::std::same_as<error_type, unicode::utf32::decode_error_code>)
			{
				return u8"utf-32 decode error"_emv;
			}
			else if constexpr (::std::same_as<error_type, lexical_error_code>)
			{
				return u8"lexical error"_emv;
			}
			else if constexpr (::std::same_as<error_type, syntax_error_code>)
			{
				return u8"syntax error"_emv;
			}
			else
			{
				static_assert(detail::false_v<error_type>, "[pdn] not all cases are enumerated");
				return {};
			}
		}, err_c);
	}

	inline auto error_code_variant_to_error_msg_string(error_code_variant c) -> error_msg_string
	{
		auto sv = error_code_variant_to_error_msg_string_view(c);
		return { sv.data(), sv.size() };
	}
}

#endif
