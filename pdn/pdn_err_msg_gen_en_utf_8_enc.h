#ifndef PDN_Header_pdn_err_msg_gen_en_utf_8_enc
#define PDN_Header_pdn_err_msg_gen_en_utf_8_enc

#include <variant>

#include "pdn_exception.h"
#include "pdn_error_string.h"
#include "pdn_utf_8_encoder.h"
#include "pdn_source_position.h"
#include "pdn_raw_error_message_type.h"
#include "pdn_raw_error_message_variant.h"

#include "pdn_err_msg_gen_utility.h"

namespace pdn::dev_util
{
	using utf_8_encode_error_code = unicode::utf_8::encode_error_code;

	inline auto err_msg_gen_en(utf_8_encode_error_code errc, source_position, raw_err_v_cref raw) -> error_msg_string
	{
		return {};
	}
	inline constexpr error_msg_string err_msg_gen_en_remove(unicode::utf_8::encode_error_code errc, error_msg_string src)
	{
		using namespace error_message_literals;
		using enum unicode::utf_8::encode_error_code;
		switch (errc)
		{
	//	case success:
	//		return u8"unicode::utf_8::encode_error == success: \""_em + src + u8"\""_em;
		case not_scalar_value:
			return u8"UTF-8 encode error not scalar value: \""_em + src + u8"\""_em;
		default:
			break;
		}
		return u8"UTF-8 encode error, error_message_generator_en unresolved: \""_em + src + u8"\""_em;
	}
}

#endif
