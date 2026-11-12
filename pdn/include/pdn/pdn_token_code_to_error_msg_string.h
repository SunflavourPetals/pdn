#ifndef PDN_Header_pdn_token_code_to_error_msg_string
#define PDN_Header_pdn_token_code_to_error_msg_string

#include "pdn_error_string.h"
#include "pdn_token_code.h"

namespace pdn
{
	constexpr auto token_code_to_error_msg_string_view(pdn_token_code c) noexcept -> error_msg_string_view
	{
		using namespace error_message_literals;
		using enum pdn_token_code;
		switch (c)
		{
		case eof:                    return u8"eof"_emv;
		case at_identifier:          return u8"@identifier"_emv;
		case identifier:             return u8"identifier"_emv;
		case literal_integer:        return u8"integer-val"_emv;
		case literal_floating_point: return u8"floating-point-val"_emv;
		case literal_string:         return u8"string-val"_emv;
		case literal_character:      return u8"character-val"_emv;
		case literal_boolean:        return u8"boolean-val"_emv;
		case tilde:                  return u8"~"_emv;
		case exclamation_mark:       return u8"!"_emv;
		case at_sign:                return u8"@"_emv;
		case hash:                   return u8"#"_emv;
		case dollar:                 return u8"$"_emv;
		case percent:                return u8"%"_emv;
		case caret:                  return u8"^"_emv;
		case ampersand:              return u8"&"_emv;
		case asterisk:               return u8"*"_emv;
		case left_parentheses:       return u8"("_emv;
		case right_parentheses:      return u8")"_emv;
		case left_brackets:          return u8"["_emv;
		case right_brackets:         return u8"]"_emv;
		case left_curly_brackets:    return u8"{"_emv;
		case right_curly_brackets:   return u8"}"_emv;
		case minus:                  return u8"-"_emv;
		case plus:                   return u8"+"_emv;
		case equal:                  return u8"="_emv;
		case slash:                  return u8"/"_emv;
		case back_slash:             return u8"\\"_emv;
		case bar:                    return u8"|"_emv;
		case colon:                  return u8":"_emv;
		case semicolon:              return u8";"_emv;
		case less_than:              return u8"<"_emv;
		case greater_than:           return u8">"_emv;
		case comma:                  return u8","_emv;
		case dot:                    return u8"."_emv;
		case question_mark:          return u8"?"_emv;
		case invalid:                return u8"code-invalid"_emv;
		default:                     return u8"unknown-code"_emv;
		}
	}
	constexpr auto token_code_to_error_msg_string(pdn_token_code c) -> error_msg_string
	{
		auto sv = token_code_to_error_msg_string_view(c);
		return { sv.data(), sv.size() };
	}
}

#endif
