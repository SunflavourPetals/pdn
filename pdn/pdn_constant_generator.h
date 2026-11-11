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
	static_assert(concepts::constant_generator<default_constant_generator<unicode::u8char_t>,  unicode::u8char_t>);
	static_assert(concepts::constant_generator<default_constant_generator<unicode::u16char_t>, unicode::u16char_t>);
	static_assert(concepts::constant_generator<default_constant_generator<unicode::u32char_t>, unicode::u32char_t>);
}

#endif
