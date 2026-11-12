#ifndef PDN_Header_pdn_constant_generator_concept
#define PDN_Header_pdn_constant_generator_concept

#include <optional>
#include <utility>

#include "pdn_decayed_same_as_concept.h"
#include "pdn_unicode_base.h"
#include "pdn_entity.h"

namespace pdn::concepts
{
	template <typename type, typename target_char_t>
	concept constant_generator = requires (type constants_gen, unicode::u8string iden)
	{
		{ constants_gen.generate_constant(iden) } -> detail::decayed_same_as<::std::optional<entity<target_char_t>>>;
		{ constants_gen.generate_constant(::std::move(iden)) } -> detail::decayed_same_as<::std::optional<entity<target_char_t>>>;
	};
}

#endif
