#ifndef PDN_Header_pdn_type_code_to_error_msg_string
#define PDN_Header_pdn_type_code_to_error_msg_string

#include "pdn_type_code.h"
#include "pdn_error_string.h"

namespace pdn
{
	inline constexpr error_msg_string type_code_to_error_msg_string(type_code type_c)
	{
		using namespace error_message_literals;
		using enum type_code;
		switch (type_c)
		{
		case unknown:
			return u8"unknown"_em;
		case i8:
			return u8"i8"_em;
		case i16:
			return u8"i16"_em;
		case i32:
			return u8"i32"_em;
		case i64:
			return u8"i64"_em;
		case u8:
			return u8"u8"_em;
		case u16:
			return u8"u16"_em;
		case u32:
			return u8"u32"_em;
		case u64:
			return u8"u64"_em;
		case f32:
			return u8"f32"_em;
		case f64:
			return u8"f64"_em;
		case boolean:
			return u8"boolean"_em;
		case character:
			return u8"character"_em;
		case string:
			return u8"string"_em;
		case list:
			return u8"list"_em;
		case object:
			return u8"object"_em;
		default:
			break;
		}
		return u8"unknown"_em;
	}
}

#endif
