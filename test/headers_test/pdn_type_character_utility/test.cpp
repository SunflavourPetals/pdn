#include <pdn/pdn_type_character_utility.h>

#include <cstddef>
#include <cstdio>
#include <type_traits>

#include <pdn/pdn_type_config.h>

namespace
{
	namespace d = pdn::type::detail;

	using d::max_code_unit_count_for_character;
	using d::max_code_unit_count_for_character_v;

	// one Unicode scalar value occupies at most this many code units
	static_assert(max_code_unit_count_for_character_v<char8_t>  == 4);
	static_assert(max_code_unit_count_for_character_v<char16_t> == 2);
	static_assert(max_code_unit_count_for_character_v<char32_t> == 1);

	// the variable template is the struct member
	static_assert(max_code_unit_count_for_character<char8_t>::value  == 4);
	static_assert(max_code_unit_count_for_character<char16_t>::value == 2);
	static_assert(max_code_unit_count_for_character<char32_t>::value == 1);
	static_assert(max_code_unit_count_for_character_v<char8_t>
	           == max_code_unit_count_for_character<char8_t>::value);

	// the same values as the unicode aliases
	static_assert(max_code_unit_count_for_character_v<pdn::unicode::u8char_t>  == 4);
	static_assert(max_code_unit_count_for_character_v<pdn::unicode::u16char_t> == 2);
	static_assert(max_code_unit_count_for_character_v<pdn::unicode::u32char_t> == 1);

	// value has type std::size_t, the traits are empty and standard layout
	static_assert(::std::same_as<decltype(max_code_unit_count_for_character_v<char8_t>), const ::std::size_t>);
	static_assert(::std::same_as<decltype(max_code_unit_count_for_character<char8_t>::value), const ::std::size_t>);
	static_assert(::std::is_empty_v<max_code_unit_count_for_character<char8_t>>);
	static_assert(::std::is_standard_layout_v<max_code_unit_count_for_character<char8_t>>);

	// character reserves exactly this many code units
	static_assert(pdn::type::config::character<char8_t>::max_size()  == max_code_unit_count_for_character_v<char8_t>);
	static_assert(pdn::type::config::character<char16_t>::max_size() == max_code_unit_count_for_character_v<char16_t>);
	static_assert(pdn::type::config::character<char32_t>::max_size() == max_code_unit_count_for_character_v<char32_t>);

	// the primary template is deliberately unusable: max_code_unit_count_for_character_v<int>
	// trips a static_assert instead of yielding a value, so it cannot be probed from a
	// concept; only the three specializations above are meant to be named.
}

int main()
{
	const std::size_t counts[] = {
		max_code_unit_count_for_character_v<char8_t>,
		max_code_unit_count_for_character_v<char16_t>,
		max_code_unit_count_for_character_v<char32_t>,
	};
	if (counts[0] != 4 || counts[1] != 2 || counts[2] != 1)
	{
		std::printf("unexpected code unit counts: %zu %zu %zu\n", counts[0], counts[1], counts[2]);
		return 1;
	}
	return 0;
}
