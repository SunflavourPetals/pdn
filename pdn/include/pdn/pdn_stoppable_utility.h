#ifndef PDN_Header_pdn_stoppable_utility
#define PDN_Header_pdn_stoppable_utility

#include <utility>
#include <type_traits>

#include "pdn_stoppable_concept.h"

namespace pdn::detail
{
	// These answer 'does the wrapped type's stop_requested()/stop_reason() promise not to throw'. They are
	// constrained by concepts::stoppable, so they can only be named for a type that has both members, and they
	// probe a const object because that is what the concept requires. Forwarding layers use them to keep their
	// own noexcept specifier exact: pdn_code_unit_iterator.h:379/384 repeats the noexcept-ness of the iterator
	// it wraps instead of claiming noexcept unconditionally.
	template <concepts::stoppable type>
	struct noexcept_stop_requested : ::std::bool_constant<noexcept(::std::declval<const type&>().stop_requested())> {};

	template <concepts::stoppable type>
	constexpr bool noexcept_stop_requested_v = noexcept_stop_requested<type>::value;

	template <concepts::stoppable type>
	struct noexcept_stop_reason : ::std::bool_constant<noexcept(::std::declval<const type&>().stop_reason())> {};

	template <concepts::stoppable type>
	constexpr bool noexcept_stop_reason_v = noexcept_stop_reason<type>::value;
}

#endif
