#ifndef PDN_Header_pdn_stoppable_concept
#define PDN_Header_pdn_stoppable_concept

#include <concepts>

#include "pdn_stop_reason.h"

namespace pdn::concepts
{
	template <typename t>
	concept stoppable = requires(t stop)
	{
		{ stop.stop_requested() } -> ::std::convertible_to<bool>;
	};

	template <typename t>
	concept stop_reasonable = stoppable<t> && requires(t stop)
	{
		{ stop.stop_reason() } -> ::std::convertible_to<stop_reason_code>;
	};
}

#endif
