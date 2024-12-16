#ifndef PDN_Header_pdn_types_character_utility
#define PDN_Header_pdn_types_character_utility

#include <cstddef>
#include <concepts>

#include "pdn_unicode.h"

namespace pdn::types::dev_util
{
	template <typename char_t>
	struct max_code_unit_count_for_character {};
	
	template <::std::same_as<unicode::utf_8_code_unit_t> char_t>
	struct max_code_unit_count_for_character<char_t>
	{
		static constexpr ::std::size_t value = 4;
	};
	
	template <::std::same_as<unicode::utf_16_code_unit_t> char_t>
	struct max_code_unit_count_for_character<char_t>
	{
		static constexpr ::std::size_t value = 2;
	};
	
	template <::std::same_as<unicode::utf_32_code_unit_t> char_t>
	struct max_code_unit_count_for_character<char_t>
	{
		static constexpr ::std::size_t value = 1;
	};

	template <typename char_t>
	inline constexpr ::std::size_t max_code_unit_count_for_character_v
		= max_code_unit_count_for_character<char_t>::value;
}

#endif
