#ifndef PDN_Header_pdn_nesting_depth_limiter_concept
#define PDN_Header_pdn_nesting_depth_limiter_concept

#include <concepts>
#include <cstddef>
#include <type_traits>

namespace pdn::concepts
{
	// The member must be callable on a const object and return exactly std::size_t; a package that
	// returns another integer type does not model the concept, so the parser silently keeps its
	// own default limit instead of failing to compile.
	template <typename type>
	concept nesting_depth_limiter = requires(::std::add_const_t<type> limiter)
	{
		{ limiter.nesting_depth_limit() } -> ::std::same_as<::std::size_t>;
	};
}

#endif
