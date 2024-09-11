#ifndef PDN_Header_pdn_utf_8_base
#define PDN_Header_pdn_utf_8_base

#include <string>
#include <memory>

#include "pdn_unicode_base.h"

namespace pdn::unicode::utf_8
{
	using code_unit_t = utf_8_code_unit_t;

	template <typename traits = ::std::char_traits<code_unit_t>, typename alloc = ::std::allocator<code_unit_t>>
	using basic_code_unit_string = ::std::basic_string<code_unit_t, traits, alloc>;

	template <typename traits = ::std::char_traits<code_unit_t>>
	using basic_code_unit_string_view = ::std::basic_string_view<code_unit_t, traits>;

	using code_unit_string = basic_code_unit_string<>;

	using code_unit_string_view = basic_code_unit_string_view<>;

	inline bool is_trailing(code_unit_t c) noexcept
	{
		return (c & code_unit_t(0xC0)) == code_unit_t(0x80);
	}
}

#endif
