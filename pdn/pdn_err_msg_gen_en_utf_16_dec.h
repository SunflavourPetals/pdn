#ifndef PDN_Header_pdn_err_msg_gen_en_utf_16_dec
#define PDN_Header_pdn_err_msg_gen_en_utf_16_dec

#include "pdn_error_string.h"
#include "pdn_error_message.h"
#include "pdn_error_message_generator.h"
#include "pdn_utf_16_decoder.h"

namespace pdn::dev_util
{
	inline constexpr error_msg_string err_msg_gen_en(unicode::utf_16::decode_error_code errc, error_msg_string src)
	{
		using namespace error_message_literals;
		using enum unicode::utf_16::decode_error_code;
		switch (errc)
		{
		case success:
			return u8"unicode::utf_16::decode_error == success: \""_em + src + u8"\""_em;
		case not_scalar_value:
			return u8"UTF-16 decode error not scalar value(UTF-16 should not make this error): \""_em + src + u8"\""_em;
		case eof_when_read_code_unit:
			return u8"UTF-16 decode error eof when read code unit(or leading surrogate): \""_em + src + u8"\""_em;
		case alone_trailing_surrogate:
			return u8"UTF-16 decode error alone trailing surrogate(trailing surrogate is first code unit by reading): \""_em + src + u8"\""_em;
		case eof_when_read_trailing_surrogate:
			return u8"UTF-16 decode error eof when read trailing surrogate: \""_em + src + u8"\""_em;
		case requires_trailing_surrogate:
			return u8"UTF-16 decode error requires trailing surrogate: \""_em + src + u8"\""_em;
		default:
			break;
		}
		return u8"UTF-16 decode error, error_message_generator_en unresolved: \""_em + src + u8"\""_em;
	}
}

#endif
