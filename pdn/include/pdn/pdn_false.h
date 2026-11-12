#ifndef PDN_Header_pdn_false
#define PDN_Header_pdn_false

namespace pdn::detail
{
	// Dependent false for static_assert inside templates: a plain
	// static_assert(false) is already ill-formed at definition time in C++20, so the
	// condition must depend on a template parameter. Used in two shapes: default
	// bodies of primary templates and of reject-only specializations (type_table,
	// cppint_to_type_code, proxy<type[]>), and the trailing else of if constexpr
	// chains (entity::as, serializer, error-message generation). Never read as a value.
	template <typename>
	constexpr bool false_v = false;
}

#endif
