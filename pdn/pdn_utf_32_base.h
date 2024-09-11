#ifndef PDN_Header_pdn_utf_32_base
#define PDN_Header_pdn_utf_32_base

#include <array>
#include <string>
#include <memory>
#include <cstdint>

#include "pdn_unicode_base.h"

namespace pdn::unicode::utf_32
{
	using code_unit_t = utf_32_code_unit_t;

	template <typename traits = ::std::char_traits<code_unit_t>, typename alloc = ::std::allocator<code_unit_t>>
	using basic_code_unit_string = ::std::basic_string<code_unit_t, traits, alloc>;

	template <typename traits = ::std::char_traits<code_unit_t>>
	using basic_code_unit_string_view = ::std::basic_string_view<code_unit_t, traits>;

	using code_unit_string = basic_code_unit_string<>;

	using code_unit_string_view = basic_code_unit_string_view<>;


	inline constexpr ::std::array<::std::uint8_t, 4> to_le_bytes(code_unit_t code_unit) noexcept
	{
		return
		{
			::std::uint8_t(code_unit),
			::std::uint8_t(code_unit >> 8),
			::std::uint8_t(code_unit >> 16),
			::std::uint8_t(code_unit >> 24)
		};
	}

	inline constexpr ::std::array<::std::uint8_t, 4> to_be_bytes(code_unit_t code_unit) noexcept
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
