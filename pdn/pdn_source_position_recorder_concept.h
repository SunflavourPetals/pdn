#ifndef PDN_Header_pdn_source_position_recorder_concept
#define PDN_Header_pdn_source_position_recorder_concept

#include <concepts>
#include <utility>

#include "pdn_unicode_base.h"
#include "pdn_source_position.h"

namespace pdn::concepts
{
	template <typename type>
	concept source_position_getter = requires(type pos_recorder)
	{
		{ pos_recorder.position() } -> ::std::convertible_to<source_position>;
	};

	template <typename type>
	concept source_position_updater = requires(type pos_recorder, unicode::code_point_t c)
	{
		pos_recorder.update(c);
		pos_recorder.update(::std::move(c));
	};

	template <typename type>
	concept source_position_recorder = source_position_getter<type> && source_position_updater<type>;
}

#endif
