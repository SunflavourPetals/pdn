#ifndef PDN_Header_pdn_constants_generator
#define PDN_Header_pdn_constants_generator

#include <functional> // remove this

#include "pdn_unicode_base.h"
#include "pdn_constants_variant.h"
#include "pdn_constants_generator_concept.h"

namespace pdn // remove this
{
	template <typename char_t>
	using constants_generator = ::std::function<bool(const unicode::utf_8_code_unit_string&, constant_variant<char_t>&)>;
}

#endif
