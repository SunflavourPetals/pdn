#ifndef PDN_Header_pdn_stoppable_concept
#define PDN_Header_pdn_stoppable_concept

#include <concepts>

#include "pdn_stop_reason.h"

namespace pdn::concepts
{
	template <typename type>
	concept stoppable = requires(const type stop)
	{
		{ stop.stop_requested() } -> ::std::convertible_to<bool>;
	};

	template <typename type>
	concept stop_reasonable = stoppable<type> && requires(const type stop)
	{
		{ stop.stop_reason() } -> ::std::convertible_to<stop_reason_code>;
	};
}

#endif
