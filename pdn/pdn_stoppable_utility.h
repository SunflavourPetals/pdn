#ifndef PDN_Header_pdn_stoppable_utility
#define PDN_Header_pdn_stoppable_utility

#include <utility>
#include <concepts>
#include <type_traits>

#include "pdn_stop_reason.h"
#include "pdn_stoppable_concept.h"

namespace pdn::detail
{
	template <typename type>
	struct noexcept_stop_requested : ::std::false_type {};

	template <concepts::stoppable type>
	struct noexcept_stop_requested<type> : ::std::conditional_t<
		noexcept(::std::declval<const type&>().stop_requested()),
		::std::true_type,
		::std::false_type> {};

	template <typename type>
	constexpr bool noexcept_stop_requested_v = noexcept_stop_requested<type>::value;

	template <typename type>
	struct noexcept_stop_reasonable : ::std::false_type {};

	template <concepts::stop_reasonable type>
	struct noexcept_stop_reasonable<type> : ::std::conditional_t<
		noexcept(::std::declval<const type&>().stop_reason()),
		::std::true_type,
		::std::false_type> {};

	template <typename type>
	constexpr bool noexcept_stop_reasonable_v = noexcept_stop_reasonable<type>::value;
}

#endif
