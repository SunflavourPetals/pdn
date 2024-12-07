#ifndef PDN_Header_pdn_err_msg_gen_en_utf_8_dec
#define PDN_Header_pdn_err_msg_gen_en_utf_8_dec

#include "pdn_error_string.h"
#include "pdn_error_message.h"
#include "pdn_utf_8_decoder.h"

namespace pdn::dev_util
{
	inline constexpr error_msg_string err_msg_gen_en(unicode::utf_8::decode_error_code errc, error_msg_string src)
	{
		using namespace error_message_literals;
		using enum unicode::utf_8::decode_error_code;
		switch (errc)
		{
	//	case success:
	//		return u8"unicode::utf_8::decode_error == success: \""_em + src + u8"\""_em;
		case not_scalar_value:
			return u8"UTF-8 decode error not scalar value: \""_em + src + u8"\""_em;
		case eof_when_read_1st_code_unit:
			return u8"UTF-8 decode error eof when read 1st code unit: \""_em + src + u8"\""_em;
		case eof_when_read_2nd_code_unit:
			return u8"UTF-8 decode error eof when read 2nd code unit: \""_em + src + u8"\""_em;
		case eof_when_read_3rd_code_unit:
			return u8"UTF-8 decode error eof when read 3rd code unit: \""_em + src + u8"\""_em;
		case eof_when_read_4th_code_unit:
			return u8"UTF-8 decode error eof when read 4th code unit: \""_em + src + u8"\""_em;
		case eof_when_read_5th_code_unit:
			return u8"UTF-8 decode error eof when read 5th code unit: \""_em + src + u8"\""_em;
		case eof_when_read_6th_code_unit:
			return u8"UTF-8 decode error eof when read 6th code unit: \""_em + src + u8"\""_em;
		case requires_utf_8_trailing:
			return u8"UTF-8 decode error requires trailing and read one which not trailing: \""_em + src + u8"\""_em;
		case requires_utf_8_leading:
			return u8"UTF-8 decode error requires leading and read one which not leading: \""_em + src + u8"\""_em;
		case unsupported_utf_8_leading:
			return u8"UTF-8 decode unsupported utf-8 leading: \""_em + src + u8"\""_em;
		default:
			break;
		}
		return u8"UTF-8 decode error, error_message_generator_en unresolved: \""_em + src + u8"\""_em;
	}
}

#endif
