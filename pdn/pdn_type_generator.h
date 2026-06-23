#ifndef PDN_Header_pdn_type_generator
#define PDN_Header_pdn_type_generator

#include "pdn_type.h"
#include "pdn_type_code.h"
#include "pdn_unicode_base.h"
#include "pdn_type_generator_concept.h"
#include "pdn_type_generator_std.h"

namespace pdn
{
	template <typename char_t>
	class default_type_generator
	{
	public:
		static auto generate_type(type::string<char_t> iden) -> type_code
		{
			return type_generator_function<char_t>(::std::move(iden));
		}
	};
}

#endif
