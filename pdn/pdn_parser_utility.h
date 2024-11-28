#ifndef PDN_Header_pdn_parser_utility
#define PDN_Header_pdn_parser_utility

#include "pdn_token_code.h"
#include "pdn_type_code.h"

namespace pdn::parser_utility
{
	constexpr bool is_unary_operator(pdn_token_code code) noexcept
	{
		return code == pdn_token_code::minus || code == pdn_token_code::plus;
	};
	constexpr bool is_negative_sign(pdn_token_code code) noexcept
	{
		return code == pdn_token_code::minus;
	};
	constexpr auto operand_type(pdn_token_code code) noexcept -> type_code
	{
		using enum pdn_token_code;
		using enum type_code;
		switch (code)
		{
		case literal_boolean:     return boolean;
		case literal_character:   return character;
		case literal_string:      return string;
		case left_brackets:       return list;
		case left_curly_brackets: return object;
		default:                  return unknown;
		}
	}
}

#endif
