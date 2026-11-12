#ifndef PDN_Header_pdn_token_convert
#define PDN_Header_pdn_token_convert

#include "pdn_token.h"
#include "pdn_token_value_variant_convert.h"

namespace pdn::detail
{
	template <typename target_char_t, typename source_char_t>
	static constexpr auto token_convert(token<source_char_t> src) -> token<target_char_t>
	{
		if constexpr (::std::same_as<source_char_t, target_char_t>)
		{
			return src;
		}
		else
		{
			return { src.position, src.code, token_value_variant_convert<target_char_t>(::std::move(src.value)) };
		}
	}
}

#endif
