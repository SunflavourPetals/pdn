#ifndef PDN_Header_pdn_source_position
#define PDN_Header_pdn_source_position

#include <cstddef>

namespace pdn
{
	// Line and column start at 1; the column counts Unicode code points, not bytes or UTF-16 code units.
	// A value is the position of the next code point to be consumed: { 1, 1 } at the very beginning of
	// the text, and after a line break the line increases by one while the column returns to 1.
	struct source_position
	{
		using text_position_type = ::std::size_t;
		text_position_type line  { 1 };
		text_position_type column{ 1 };
		friend constexpr bool operator==(const source_position&, const source_position&) noexcept = default;
	};
}

#endif
