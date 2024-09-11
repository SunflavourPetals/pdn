#ifndef PDN_Header_pdn_source_position
#define PDN_Header_pdn_source_position

#include <cstddef>

namespace pdn
{
	struct source_position
	{
		using text_position_type = ::std::size_t;
		text_position_type line{ 1 };
		text_position_type column{ 1 };
	};
}

#endif
