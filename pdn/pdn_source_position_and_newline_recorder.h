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
		constexpr auto position() const noexcept { return pos; }
		constexpr auto CRLF()     const noexcept { return CRLF_count; }
		constexpr auto LF()       const noexcept { return LF_count; }
		constexpr auto CR()       const noexcept { return CR_count; }
		constexpr auto NEL()      const noexcept { return NEL_count; }
		constexpr auto LS()       const noexcept { return LS_count; }
		constexpr auto PS()       const noexcept { return PS_count; }
		constexpr newline_modes newline_mode() const noexcept
		{
			newline_modes result{};
			if (CRLF()) result |= newline_modes::CRLF;
			if (LF())   result |= newline_modes::LF;
			if (CR())   result |= newline_modes::CR;
			if (NEL())  result |= newline_modes::NEL;
			if (LS())   result |= newline_modes::LS;
			if (PS())   result |= newline_modes::PS;
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
		constexpr void update(unicode::code_point_t c)
		{
			switch (c)
			{
			case U'\r': // CR or CRLF
				is_last_CR = true;
				++CR_count; // update CR count
				++pos.line;
				pos.column = 1;
				break;
			case U'\n': // LF of CRLF
				if (!is_last_CR) // LF
				{
					++pos.line;
					++LF_count; // update LF count
					pos.column = 1;
				}
				else // CRLF
				{
					--CR_count; // update CR count (decrease)
					++CRLF_count; // update CRLF count
				}
				is_last_CR = false;
				break;
			case U'\u0085': // NEL
				is_last_CR = false;
				++pos.line;
				++NEL_count;  // update NEL count
				pos.column = 1;
				break;
			case U'\u2028': // LS
				is_last_CR = false;
				++pos.line;
				++LS_count;  // update LS count
				pos.column = 1;
				break;
			case U'\u2029': // PS
				is_last_CR = false;
				++pos.line;
				++PS_count;  // update PS count
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
		::std::size_t   CRLF_count{};
		::std::size_t   LF_count{};
		::std::size_t   CR_count{};
		::std::size_t   NEL_count{};
		::std::size_t   LS_count{};
		::std::size_t   PS_count{};
		bool            is_last_CR{};
	};
	static_assert(concepts::source_position_recorder<source_position_and_newline_recorder>);
}

#endif
