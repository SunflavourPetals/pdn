#ifndef PDN_Header_pdn_newline_modes
#define PDN_Header_pdn_newline_modes

#include <type_traits>

namespace pdn
{
	enum class newline_modes
	{
		NONE = 0x00,
		CRLF = 0x01,
		LF   = 0x02,
		CR   = 0x04,
		NEL  = 0x08,
		LS   = 0x10,
		PS   = 0x20,
	};

	constexpr newline_modes& operator|=(newline_modes& lhs, newline_modes rhs) noexcept
	{
		using ut = ::std::underlying_type_t<newline_modes>;
		return lhs = static_cast<newline_modes>(static_cast<ut>(lhs) | static_cast<ut>(rhs));
	}

	constexpr newline_modes& operator&=(newline_modes& lhs, newline_modes rhs) noexcept
	{
		using ut = ::std::underlying_type_t<newline_modes>;
		return lhs = static_cast<newline_modes>(static_cast<ut>(lhs) & static_cast<ut>(rhs));
	}

	constexpr newline_modes operator|(newline_modes lhs, newline_modes rhs) noexcept
	{
		return lhs |= rhs;
	}

	constexpr newline_modes operator&(newline_modes lhs, newline_modes rhs) noexcept
	{
		return lhs &= rhs;
	}
}

#endif
