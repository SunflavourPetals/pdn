#ifndef PDN_Header_pdn_utf8_base
#define PDN_Header_pdn_utf8_base

#include <string>
#include <memory>

#include "pdn_unicode_base.h"

namespace pdn::unicode::utf8
{
	using code_unit_t = u8char_t;
	using string = u8string;
	using string_view = u8string_view;

	constexpr bool is_trailing(code_unit_t c) noexcept
	{
		return (c & code_unit_t(0xC0)) == code_unit_t(0x80);
	}
}

#endif
