#ifndef PDN_Header_pdn_constants_generator_concept
#define PDN_Header_pdn_constants_generator_concept

#include <concepts>
#include <optional>
#include <utility>

#include "pdn_unicode_base.h"
#include "pdn_constants_variant.h"

namespace pdn::concepts
{
	template <typename type, typename target_char_t>
	concept constants_generator = requires (type constants_gen, unicode::utf_8_code_unit_string iden)
	{
		{ constants_gen.generate_constant(iden) } -> ::std::convertible_to<::std::optional<constant_variant<target_char_t>>>;
		{ constants_gen.generate_constant(::std::move(iden)) } -> ::std::convertible_to<::std::optional<constant_variant<target_char_t>>>;
	};
}

#endif
