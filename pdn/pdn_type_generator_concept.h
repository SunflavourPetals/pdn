#ifndef PDN_Header_pdn_type_generator_concept
#define PDN_Header_pdn_type_generator_concept

#include <concepts>
#include <utility>

#include "pdn_types.h"
#include "pdn_type_code.h"

namespace pdn::concepts
{
	template <typename type, typename char_t>
	concept type_generator = requires (type type_gen, types::string<char_t> iden)
	{
		{ type_gen.generate_type(iden) } -> ::std::convertible_to<type_code>;
		{ type_gen.generate_type(::std::move(iden)) } -> ::std::convertible_to<type_code>;
	};
}

#endif
