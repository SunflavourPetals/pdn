#ifndef PDN_Header_pdn_constant_generator_concept
#define PDN_Header_pdn_constant_generator_concept

#include <optional>
#include <utility>

#include "pdn_remove_cvref_same_as_concept.h"
#include "pdn_unicode_base.h"
#include "pdn_entity_forward_decl.h"

namespace pdn::concepts
{
	// A generator that turns the text of an at-identifier (the part after '@', always UTF-8) into the entity
	// it denotes. std::nullopt means "no such constant", which the parser reports as at_value_not_found.
	//
	// The two probes are the lvalue and rvalue forms of the same call: a by-value or const-reference
	// parameter satisfies both, and so does a static member function (the built-in generator is static). The
	// result must be exactly std::optional<entity<target_char_t>>, so a generator parameterised on another
	// character type does not satisfy this concept.
	//
	// entity appears only inside a type identity check, hence the forward declaration is enough here; the
	// definition in pdn_entity.h is not needed to test the concept.
	template <typename type, typename target_char_t>
	concept constant_generator = requires (type constants_gen, unicode::u8string iden)
	{
		{ constants_gen.generate_constant(iden) } -> detail::remove_cvref_same_as<::std::optional<entity<target_char_t>>>;
		{ constants_gen.generate_constant(::std::move(iden)) } -> detail::remove_cvref_same_as<::std::optional<entity<target_char_t>>>;
	};
}

#endif
