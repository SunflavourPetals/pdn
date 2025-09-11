#ifndef PDN_Header_pdn_utf_16_base
#define PDN_Header_pdn_utf_16_base

#include <array>
#include <string>
#include <memory>
#include <cstdint>

#include "pdn_unicode_base.h"

namespace pdn::unicode::utf_16
{
	using code_unit_t = utf_16_code_unit_t;

	template <typename traits = ::std::char_traits<code_unit_t>, typename alloc = ::std::allocator<code_unit_t>>
	using basic_code_unit_string = basic_utf_16_code_unit_string<traits, alloc>;

	template <typename traits = ::std::char_traits<code_unit_t>>
	using basic_code_unit_string_view = basic_utf_16_code_unit_string_view<traits>;

	using code_unit_string = basic_code_unit_string<>;
	using code_unit_string_view = basic_code_unit_string_view<>;

	constexpr auto to_le_bytes(code_unit_t code_unit) noexcept -> ::std::array<::std::uint8_t, 2>
	{
		return { ::std::uint8_t(code_unit), ::std::uint8_t(code_unit >> 8) };
	}

	constexpr auto to_be_bytes(code_unit_t code_unit) noexcept -> ::std::array<::std::uint8_t, 2>
	{
		return { ::std::uint8_t(code_unit >> 8), ::std::uint8_t(code_unit) };
	}
}

#endif
