#ifndef PDN_Header_pdn_error_handler_concept
#define PDN_Header_pdn_error_handler_concept

#include <utility>
#include <cstddef>
#include <concepts>
#include <type_traits>

#include "pdn_error_message.h"

namespace pdn::concepts
{
	// The counting half of an error handler: a const-callable error_count() that returns exactly
	// std::size_t (the error threshold comparison and parse_result::error_count() are typed on it).
	template <typename type>
	concept error_counter = requires (::std::add_const_t<::std::remove_reference_t<type>> err_counter)
	{
		{ err_counter.error_count() } -> ::std::same_as<::std::size_t>;
	};
	// The handling half: a mutable handler that accepts a const message (the package forwards the
	// message it received) and a temporary (the package re-wraps it after prepending the filename),
	// on top of the counting half above.
	template <typename type>
	concept error_handler = error_counter<type> && requires (type err_handler, error_message msg, const error_message cmsg)
	{
		err_handler.handle_error(cmsg);
		err_handler.handle_error(::std::move(msg));
	};
}

#endif
