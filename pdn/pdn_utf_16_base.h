#ifndef PDN_Header_pdn_utf_16_base
#define PDN_Header_pdn_utf_16_base

#include <array>
#include <string>
#include <memory>
#include <cstdint>

#include "pdn_unicode_base.h"

namespace pdn::unicode::utf_16
{
	using code_unit_t                 = utf_16_code_unit_t;
	template <typename traits         = ::std::char_traits<code_unit_t>, typename alloc = ::std::allocator<code_unit_t>>
	using basic_code_unit_string      = ::std::basic_string<code_unit_t, traits, alloc>;
	using code_unit_string            = basic_code_unit_string<>;
	template <typename traits         = ::std::char_traits<code_unit_t>>
	using basic_code_unit_string_view = ::std::basic_string_view<code_unit_t, traits>;
	using code_unit_string_view       = basic_code_unit_string_view<>;

	constexpr ::std::array<::std::uint8_t, 2> to_le_bytes(code_unit_t code_unit) noexcept
	{
		return { ::std::uint8_t(code_unit), ::std::uint8_t(code_unit >> 8) };
	}

	constexpr ::std::array<::std::uint8_t, 2> to_be_bytes(code_unit_t code_unit) noexcept
	{
		return { ::std::uint8_t(code_unit >> 8), ::std::uint8_t(code_unit) };
	}
}

#endif
