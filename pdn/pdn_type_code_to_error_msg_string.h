#ifndef PDN_Header_pdn_type_code_to_error_msg_string
#define PDN_Header_pdn_type_code_to_error_msg_string

#include "pdn_type_code.h"
#include "pdn_error_string.h"

namespace pdn
{
	constexpr auto type_code_to_error_msg_string_view(type_code c) noexcept -> error_msg_string_view
	{
		using namespace error_message_literals;
		using enum type_code;
		switch (c)
		{
		case unknown:   return u8"unknown"_emv;
		case i8:        return u8"i8"_emv;
		case i16:       return u8"i16"_emv;
		case i32:       return u8"i32"_emv;
		case i64:       return u8"i64"_emv;
		case u8:        return u8"u8"_emv;
		case u16:       return u8"u16"_emv;
		case u32:       return u8"u32"_emv;
		case u64:       return u8"u64"_emv;
		case f32:       return u8"f32"_emv;
		case f64:       return u8"f64"_emv;
		case boolean:   return u8"boolean"_emv;
		case character: return u8"character"_emv;
		case string:    return u8"string"_emv;
		case list:      return u8"list"_emv;
		case object:    return u8"object"_emv;
		default:        return u8"unknown"_emv;
		}
	}

	constexpr auto type_code_to_error_msg_string(type_code c) -> error_msg_string
	{
		auto sv = type_code_to_error_msg_string_view(c);
		return { sv.data(), sv.size() };
	}
}

#endif
