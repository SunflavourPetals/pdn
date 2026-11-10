#ifndef PDN_Header_pdn_nesting_depth_limiter_concept
#define PDN_Header_pdn_nesting_depth_limiter_concept

#include <concepts>
#include <type_traits>

namespace pdn::concepts
{
	template <typename type>
	concept nesting_depth_limiter = requires(::std::add_const_t<type> limiter)
	{
		{ limiter.nesting_depth_limit() } -> ::std::same_as<::std::size_t>;
	};
}

#endif
