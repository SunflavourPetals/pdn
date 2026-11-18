#ifndef PDN_Header_pdn_stoppable_concept
#define PDN_Header_pdn_stoppable_concept

#include <concepts>
#include <type_traits>

#include "pdn_stop_reason.h"

namespace pdn::concepts
{
	// Models a type that can be polled for a stop request: every layer tests stop_requested() in
	// its increment loop and reports stop_reason() upwards once it turns true. Both members must
	// be callable on a const object and return exactly bool and stop_reason_code, so a member
	// returning int or void* cannot satisfy the concept. The reference is stripped first, so that
	// stoppable<T&>, stoppable<const T&> and stoppable<T&&> all mean the same as stoppable<T>.
	// The check is purely structural: the library's default_function_package, its iterators and
	// default_threshold_error_handler model it.
	template <typename type>
	concept stoppable = requires(::std::add_const_t<::std::remove_reference_t<type>> stop)
	{
		{ stop.stop_requested() } -> ::std::same_as<bool>;
		{ stop.stop_reason() } -> ::std::same_as<stop_reason_code>;
	};
}

#endif
