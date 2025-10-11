#ifndef PDN_Header_pdn_types_character_utility
#define PDN_Header_pdn_types_character_utility

#include <cstddef>
#include <concepts>

#include "pdn_unicode.h"

namespace pdn::types::detail
{
	template <typename char_t>
	struct max_code_unit_count_for_character {};
	
	template <::std::same_as<unicode::u8char_t> char_t>
	struct max_code_unit_count_for_character<char_t>
	{
		static constexpr ::std::size_t value = 4;
	};
	
	template <::std::same_as<unicode::u16char_t> char_t>
	struct max_code_unit_count_for_character<char_t>
	{
		static constexpr ::std::size_t value = 2;
	};
	
	template <::std::same_as<unicode::u32char_t> char_t>
	struct max_code_unit_count_for_character<char_t>
	{
		static constexpr ::std::size_t value = 1;
	};

	template <typename char_t>
	inline constexpr ::std::size_t max_code_unit_count_for_character_v
		= max_code_unit_count_for_character<char_t>::value;
}

#endif
