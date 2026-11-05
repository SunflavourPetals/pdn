#ifndef PDN_Header_pdn_stoppable_utility
#define PDN_Header_pdn_stoppable_utility

#include <utility>
#include <type_traits>

#include "pdn_stoppable_concept.h"

namespace pdn::detail
{
	template <concepts::stoppable type>
	struct noexcept_stop_requested : ::std::conditional_t<
		noexcept(::std::declval<const type&>().stop_requested()),
		::std::true_type,
		::std::false_type> {};

	template <concepts::stoppable type>
	constexpr bool noexcept_stop_requested_v = noexcept_stop_requested<type>::value;

	template <concepts::stoppable type>
	struct noexcept_stop_reason : ::std::conditional_t<
		noexcept(::std::declval<const type&>().stop_reason()),
		::std::true_type,
		::std::false_type> {};

	template <concepts::stoppable type>
	constexpr bool noexcept_stop_reason_v = noexcept_stop_reason<type>::value;
}

#endif
