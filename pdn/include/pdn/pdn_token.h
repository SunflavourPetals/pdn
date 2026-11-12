#ifndef PDN_Header_pdn_token
#define PDN_Header_pdn_token

#include "pdn_source_position.h"
#include "pdn_token_code.h"
#include "pdn_token_value_variant.h"

namespace pdn
{
	template <typename char_t>
	struct token
	{
		source_position             position{};
		pdn_token_code              code{};
		token_value_variant<char_t> value{};
	};
}

#endif
