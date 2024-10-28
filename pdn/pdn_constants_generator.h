#ifndef PDN_Header_pdn_constants_generator
#define PDN_Header_pdn_constants_generator

#include <functional> // remove this

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
		auto generate_constant(unicode::utf_8_code_unit_string iden) const -> ::std::optional<constant_variant<char_t>>
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

namespace pdn // remove this
{
	template <typename char_t>
	using constants_generator = ::std::function<bool(const unicode::utf_8_code_unit_string&, constant_variant<char_t>&)>;

	template <typename char_t>
	inline const constants_generator<char_t> constants_generator_std{ constants_generator_std_function<char_t> };
}

#endif
