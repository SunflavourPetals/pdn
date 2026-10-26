#ifndef PDN_Header_pdn_type_character_utility
#define PDN_Header_pdn_type_character_utility

#include <cstddef>
#include <concepts>

#include "pdn_unicode.h"
#include "pdn_false.h"

namespace pdn::type::detail
{
	template <typename char_t>
	struct max_code_unit_count_for_character
	{
		static_assert(pdn::detail::false_v<char_t>, "[pdn] char_t is not one of unicode::u8char_t, u16char_t, u32char_t");
	};
	
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
