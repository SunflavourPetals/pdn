#ifndef PDN_Header_pdn_constants_generator
#define PDN_Header_pdn_constants_generator

#include <optional>

#include "pdn_unicode_base.h"
#include "pdn_constants_variant.h"
#include "pdn_constants_generator_concept.h"
#include "pdn_constants_generator_std.h"

namespace pdn
{
	template <typename char_t>
	class default_constants_generator
	{
	public:
		auto generate_constant(const unicode::utf_8_code_unit_string& iden) const -> ::std::optional<constant_variant<char_t>>
		{
			constant_variant<char_t> r = 0;
			if (constants_generator_std_function(iden, r))
			{
				return r;
			}
			return ::std::nullopt;
		}
	};
}

#endif
