#ifndef PDN_Header_pdn_token_value_variant_convert
#define PDN_Header_pdn_token_value_variant_convert

#include <type_traits>
#include <concepts>
#include <variant>

#include "pdn_types.h"
#include "pdn_proxy.h"
#include "pdn_utf_code_convert.h"
#include "pdn_token_value_variant.h"

namespace pdn::detail
{
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
				using target_string = types::string<target_char_t>;
				using source_string = types::string<source_char_t>;
				using arg_t = ::std::decay_t<decltype(arg)>;
				using unicode::code_convert;

				if constexpr (::std::same_as<arg_t, types::character<source_char_t>>)
				{
					auto converted = code_convert<types::string<target_char_t>>(arg.to_string_view());
					return types::character<target_char_t>{ converted.cbegin(), converted.size() };
				}
				else if constexpr (::std::same_as<arg_t, proxy<source_string>>)
				{
					return make_proxy<target_string>(code_convert<target_string>(*arg));
				}
				else
				{
					return arg;
				}
			}, ::std::move(src));
		}
	}
}

#endif
