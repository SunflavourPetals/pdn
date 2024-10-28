#ifndef PDN_Header_pdn_type_generator
#define PDN_Header_pdn_type_generator

#include <functional> // remove this

#include "pdn_types.h"
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
		auto generate_type(types::string<char_t> iden) const -> type_code
		{
			return type_generator_function<char_t>(::std::move(iden));
		}
	};
}

namespace pdn // remove this
{
	template <typename char_t>
	using type_generator = ::std::function<type_code(const types::string<char_t>&)>;

	template <typename char_t>
	inline const type_generator<char_t> type_generator_std{ type_generator_function<char_t> };
}

#endif
