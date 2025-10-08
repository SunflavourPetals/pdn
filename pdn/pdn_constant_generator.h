#ifndef PDN_Header_pdn_constant_generator
#define PDN_Header_pdn_constant_generator

#include <optional>

#include "pdn_unicode_base.h"
#include "pdn_entity.h"
#include "pdn_constant_generator_concept.h"
#include "pdn_constant_generator_std.h"

namespace pdn
{
	template <typename char_t>
	class default_constant_generator
	{
	public:
		static auto generate_constant(const unicode::u8string& iden) -> ::std::optional<entity<char_t>>
		{
			return constant_generator_std_function<char_t>(iden);
		}
	};
}

#endif
