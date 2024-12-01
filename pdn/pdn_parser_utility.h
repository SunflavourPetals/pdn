#ifndef PDN_Header_pdn_parser_utility
#define PDN_Header_pdn_parser_utility

#include "pdn_token_code.h"
#include "pdn_type_code.h"

namespace pdn::dev_util
{
	template <typename type, typename target_type>
	concept remove_cvref_same_as = ::std::is_same_v<::std::remove_cvref_t<type>, ::std::remove_cvref_t<target_type>>;
}

namespace pdn::concepts
{
	template <typename type, typename char_t>
	concept token_iterator = requires (type it)
	{
		{ *it } -> ::std::convertible_to<token<char_t>>;
		++it;
	};

	template <typename type>
	concept utf_8_code_unit_iterator = requires (type it)
	{
		{ *it } -> dev_util::remove_cvref_same_as<unicode::utf_8_code_unit_t>;
		++it;
	};
	template <typename type>
	concept utf_16_code_unit_iterator = requires (type it)
	{
		{ *it } -> dev_util::remove_cvref_same_as<unicode::utf_16_code_unit_t>;
		++it;
	};
	template <typename type>
	concept utf_32_code_unit_iterator = requires (type it)
	{
		{ *it } -> dev_util::remove_cvref_same_as<unicode::utf_32_code_unit_t>;
		++it;
	};
	template <typename type>
	concept utf_code_unit_iterator
		 = utf_8_code_unit_iterator<type>
		|| utf_16_code_unit_iterator<type>
		|| utf_32_code_unit_iterator<type>;
}

namespace pdn::parser_utility
{
	constexpr bool is_unary_operator(pdn_token_code code) noexcept
	{
		return code == pdn_token_code::minus || code == pdn_token_code::plus;
	}
	constexpr bool is_negative_sign(pdn_token_code code) noexcept
	{
		return code == pdn_token_code::minus;
	}
}

#endif
