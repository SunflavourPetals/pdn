#ifndef PDN_Header_pdn_type_generator
#define PDN_Header_pdn_type_generator

#include <functional>

#include "pdn_types.h"
#include "pdn_type_code.h"
#include "pdn_unicode_base.h"

namespace pdn
{
	template <typename char_t>
	using type_generator = ::std::function<type_code(const types::string<char_t>&)>;
}

#endif
