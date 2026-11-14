#ifndef PDN_Header_pdn_type_generator
#define PDN_Header_pdn_type_generator

#include "pdn_type.h"
#include "pdn_type_code.h"
#include "pdn_unicode_base.h"
#include "pdn_type_generator_concept.h"
#include "pdn_type_generator_std.h"

namespace pdn
{
	// The default generator behind the parser: maps the type name written in the source to the
	// type_code the parser will hand to the entity, by looking the name up in the type table of
	// pdn_type_generator_std.h. Names that are not PDN type names yield type_code::unknown, which
	// the parser reports as the unknown_type syntax error (see pdn_parser.h).
	template <typename char_t>
	class default_type_generator
	{
	public:
		static auto generate_type(const type::string<char_t>& iden) -> type_code
		{
			return type_generator_function<char_t>(iden);
		}
	};
	static_assert(concepts::type_generator<default_type_generator<unicode::u8char_t>, unicode::u8char_t>);
	static_assert(concepts::type_generator<default_type_generator<unicode::u16char_t>, unicode::u16char_t>);
	static_assert(concepts::type_generator<default_type_generator<unicode::u32char_t>, unicode::u32char_t>);
}

#endif
