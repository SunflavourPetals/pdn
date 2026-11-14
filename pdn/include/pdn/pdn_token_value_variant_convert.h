#ifndef PDN_Header_pdn_token_value_variant_convert
#define PDN_Header_pdn_token_value_variant_convert

#include <type_traits>
#include <concepts>
#include <utility>
#include <variant>

#include "pdn_type.h"
#include "pdn_proxy.h"
#include "pdn_utf_code_convert.h"
#include "pdn_token_value_variant.h"

namespace pdn::detail
{
	// Re-expresses a token payload in another character type; used by token_convert when tokens
	// flow from one character type to another (e.g. an error message built in UTF-8 out of a
	// u16-parsed document). Three payload kinds need work, everything else is passed through:
	//   character<source_char_t>     - decoded and re-encoded (a 4-unit UTF-8 emoji becomes
	//                                  a 2-unit UTF-16 surrogate pair); content that is not a
	//                                  scalar value becomes U+FFFD in the target type;
	//   proxy<string<source_char_t>> - decoded and re-encoded, then wrapped in a new proxy;
	//   detail::at_iden_string_proxy - already UTF-8 by design, so it stays untouched.
	// The same-character-type call is a plain pass-through and is usable in a constant
	// expression; a real re-encoding allocates, so that path is not.
	template <typename target_char_t, typename source_char_t>
	constexpr auto token_value_variant_convert(token_value_variant<source_char_t> src) -> token_value_variant<target_char_t>
	{
		if constexpr (::std::same_as<target_char_t, source_char_t>)
		{
			return src;
		}
		else
		{
			return ::std::visit([](auto&& arg) -> token_value_variant<target_char_t>
			{
				using target_string = type::string<target_char_t>;
				using source_string = type::string<source_char_t>;
				using arg_t = ::std::decay_t<decltype(arg)>;
				using unicode::code_convert;

				if constexpr (::std::same_as<arg_t, type::character<source_char_t>>)
				{
					auto converted = code_convert<type::string<target_char_t>>(arg.to_string_view());
					return type::character<target_char_t>{ converted.cbegin(), converted.size() };
				}
				else if constexpr (::std::same_as<arg_t, proxy<source_string>>)
				{
					return make_proxy<target_string>(code_convert<target_string>(*arg));
				}
				else
				{
					return ::std::forward<decltype(arg)>(arg);
				}
			}, ::std::move(src));
		}
	}
}

#endif
