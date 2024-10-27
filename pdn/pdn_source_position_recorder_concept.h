#ifndef PDN_Header_pdn_source_position_recorder_concept
#define PDN_Header_pdn_source_position_recorder_concept

#include <concepts>

#include "pdn_unicode_base.h"
#include "pdn_source_position.h"

namespace pdn::concepts
{
	template <typename type>
	concept source_position_recorder = requires(type pos_recorder, unicode::code_point_t c)
	{
		{ pos_recorder.position() } -> ::std::convertible_to<source_position>;
		pos_recorder.update(c);
	};
}

#endif
