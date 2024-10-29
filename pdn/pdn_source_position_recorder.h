#ifndef PDN_Header_pdn_source_position_recorder
#define PDN_Header_pdn_source_position_recorder

#include "pdn_source_position.h"
#include "pdn_unicode_base.h"
#include "pdn_source_position_recorder_concept.h"

namespace pdn
{
	class source_position_recorder
	{
	public:
		constexpr source_position position() const noexcept
		{
			return pos;
		}
		constexpr void update(unicode::code_point_t c) noexcept
		{
			switch (c)
			{
			case U'\r':
				is_last_CR = true;
				++pos.line;
				pos.column = 1;
				break;
			case U'\n':
				if (!is_last_CR)
				{
					++pos.line;
					pos.column = 1;
				}
				is_last_CR = false;
				break;
			case U'\u0085':
			case U'\u2028':
			case U'\u2029':
				is_last_CR = false;
				++pos.line;
				pos.column = 1;
				break;
			default:
				is_last_CR = false;
				++pos.column;
				break;
			}
		}
	private:
		source_position pos{};
		bool            is_last_CR{};
	};
	static_assert(concepts::source_position_recorder<source_position_recorder>);
}

#endif
