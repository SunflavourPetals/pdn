#ifndef PDN_Header_pdn_source_position_and_newline_recorder
#define PDN_Header_pdn_source_position_and_newline_recorder

#include <cstddef>

#include "pdn_source_position.h"
#include "pdn_newline_modes.h"
#include "pdn_unicode_base.h"
#include "pdn_source_position_recorder_concept.h"

namespace pdn
{
	class source_position_and_newline_recorder
	{
	public:
		constexpr auto position()   const noexcept { return pos; }
		constexpr auto CRLF_count() const noexcept { return CRLF_cnt; }
		constexpr auto LF_count()   const noexcept { return LF_cnt; }
		constexpr auto CR_count()   const noexcept { return CR_cnt; }
		constexpr auto NEL_count()  const noexcept { return NEL_cnt; }
		constexpr auto LS_count()   const noexcept { return LS_cnt; }
		constexpr auto PS_count()   const noexcept { return PS_cnt; }
		constexpr newline_modes newline_mode() const noexcept
		{
			newline_modes result{};
			if (CRLF_count()) result |= newline_modes::CRLF;
			if (LF_count())   result |= newline_modes::LF;
			if (CR_count())   result |= newline_modes::CR;
			if (NEL_count())  result |= newline_modes::NEL;
			if (LS_count())   result |= newline_modes::LS;
			if (PS_count())   result |= newline_modes::PS;
			return result;
		}
		constexpr bool is_no_newline() const noexcept
		{
			return newline_mode() == newline_modes::NONE;
		}
		constexpr bool is_coincident_newline() const noexcept
		{
			switch (newline_mode())
			{
			case newline_modes::NONE:
			case newline_modes::CRLF:
			case newline_modes::LF:
			case newline_modes::CR:
			case newline_modes::NEL:
			case newline_modes::LS:
			case newline_modes::PS:
				return true;
			default:
				return false;
			}
		}
		constexpr bool is_mixed_newline() const noexcept
		{
			return !is_coincident_newline();
		}
		constexpr void update(unicode::code_point_t c) noexcept
		{
			switch (c)
			{
			case U'\r': // CR or CRLF
				is_last_CR = true;
				++CR_cnt; // update CR count
				++pos.line;
				pos.column = 1;
				break;
			case U'\n': // LF of CRLF
				if (!is_last_CR) // LF
				{
					++pos.line;
					++LF_cnt; // update LF count
					pos.column = 1;
				}
				else // CRLF
				{
					--CR_cnt; // update CR count (decrease)
					++CRLF_cnt; // update CRLF count
				}
				is_last_CR = false;
				break;
			case U'\u0085': // NEL
				is_last_CR = false;
				++pos.line;
				++NEL_cnt;  // update NEL count
				pos.column = 1;
				break;
			case U'\u2028': // LS
				is_last_CR = false;
				++pos.line;
				++LS_cnt;  // update LS count
				pos.column = 1;
				break;
			case U'\u2029': // PS
				is_last_CR = false;
				++pos.line;
				++PS_cnt;  // update PS count
				pos.column = 1;
				break;
			default:
				is_last_CR = false;
				++pos.column;
				break;
			}
		}
		source_position_and_newline_recorder() = default;
	private:
		source_position pos{};
		::std::size_t   CRLF_cnt{};
		::std::size_t   LF_cnt{};
		::std::size_t   CR_cnt{};
		::std::size_t   NEL_cnt{};
		::std::size_t   LS_cnt{};
		::std::size_t   PS_cnt{};
		bool            is_last_CR{};
	};
	static_assert(concepts::source_position_recorder<source_position_and_newline_recorder>);
}

#endif
