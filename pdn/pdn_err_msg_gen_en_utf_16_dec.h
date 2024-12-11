#ifndef PDN_Header_pdn_err_msg_gen_en_utf_16_dec
#define PDN_Header_pdn_err_msg_gen_en_utf_16_dec

#include <variant>

#include "pdn_exception.h"
#include "pdn_error_string.h"
#include "pdn_utf_16_decoder.h"
#include "pdn_source_position.h"
#include "pdn_raw_error_message_type.h"
#include "pdn_raw_error_message_variant.h"

#include "pdn_err_msg_gen_utility.h"

namespace pdn::dev_util
{
	using utf_16_decode_error_code = unicode::utf_16::decode_error_code;
	inline auto err_msg_gen_en(utf_16_decode_error_code errc, source_position, raw_err_v_cref raw) -> error_msg_string
	{
		using namespace err_msg_gen_util;
		using namespace error_message_literals;
		using enum utf_16_decode_error_code;
		const auto& msg = ::std::get<raw_details::utf_16_decode_error>(raw);
		switch (errc)
		{
		case not_scalar_value: // unreachable UTF-16 should not make this error
			return u8"not scalar value: 0x"_em + to_s<16, 4>(msg.result.value())
				+ u8", sequence at offset "_em + offset_of_leading(msg, 2) + u8"(if with BOM then +2), "_em
				+ to_s(msg.result.distance() + 1)
				+ (msg.result.distance() ? u8" code units were read"_em : u8" code unit was read"_em);
		case eof_when_read_code_unit: // unreachable
			return u8"eof when read code unit(or leading surrogate), sequence at offset "_em
				+ offset_of_leading(msg, 2) + u8"(if with BOM then +2)"_em;
		case alone_trailing_surrogate:
			return u8"alone trailing surrogate, code unit: 0x"_em + to_s<16, 4>(msg.last_code_unit)
				+ u8", sequence at offset "_em + offset_of_leading(msg, 2) + u8"(if with BOM then +2), "_em
				+ to_s(msg.result.distance() + 1)
				+ (msg.result.distance() ? u8" code units were read"_em : u8" code unit was read"_em);
		case eof_when_read_trailing_surrogate:
			return u8"eof when read trailing surrogate, sequence at offset "_em
				+ offset_of_leading(msg, 2) + u8"(if with BOM then +2)"_em;
		case requires_trailing_surrogate:
			return u8"requires trailing surrogate, code unit: 0x"_em + to_s<16, 4>(msg.last_code_unit)
				+ u8", sequence at offset "_em + offset_of_leading(msg, 2) + u8"(if with BOM then +2), "_em
				+ to_s(msg.result.distance() + 1)
				+ (msg.result.distance() ? u8" code units were read"_em : u8" code unit was read"_em);
		default:
			throw inner_error{ "UTF-16 decode error and error_message_generator_en unresolved" };
			return u8"UTF-16 decode error, error_message_generator_en unresolved"_em;
		}
	}
}

#endif
