#ifndef PDN_Header_pdn_utf_32_base
#define PDN_Header_pdn_utf_32_base

#include <array>
#include <string>
#include <memory>
#include <cstdint>

#include "pdn_unicode_base.h"

namespace pdn::unicode::utf_32
{
	using code_unit_t = u32char_t;
	using string = u32string;
	using string_view = u32string_view;

	constexpr auto to_le_bytes(code_unit_t code_unit) noexcept -> ::std::array<::std::uint8_t, 4>
	{
		return
		{
			::std::uint8_t(code_unit),
			::std::uint8_t(code_unit >> 8),
			::std::uint8_t(code_unit >> 16),
			::std::uint8_t(code_unit >> 24)
		};
	}

	constexpr auto to_be_bytes(code_unit_t code_unit) noexcept -> ::std::array<::std::uint8_t, 4>
	{
		return
		{
			::std::uint8_t(code_unit >> 24),
			::std::uint8_t(code_unit >> 16),
			::std::uint8_t(code_unit >> 8),
			::std::uint8_t(code_unit)
		};
	}
}

#endif
