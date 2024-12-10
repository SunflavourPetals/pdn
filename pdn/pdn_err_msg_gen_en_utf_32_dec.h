#ifndef PDN_Header_pdn_err_msg_gen_en_utf_32_dec
#define PDN_Header_pdn_err_msg_gen_en_utf_32_dec

#include <variant>

#include "pdn_exception.h"
#include "pdn_error_string.h"
#include "pdn_utf_32_decoder.h"
#include "pdn_source_position.h"
#include "pdn_raw_error_message_type.h"
#include "pdn_raw_error_message_variant.h"

#include "pdn_err_msg_gen_utility.h"

namespace pdn::dev_util
{
	using utf_32_decode_error_code = unicode::utf_32::decode_error_code;
	inline auto err_msg_gen_en(utf_32_decode_error_code errc, source_position pos, raw_error_message_variant raw) -> error_msg_string
	{
		using namespace err_msg_gen_util;
		using namespace error_message_literals;
		using enum utf_32_decode_error_code;
		auto& msg = ::std::get<raw_error_message_type::utf_32_decode_error>(raw);
		switch (errc)
		{
		case not_scalar_value:
			return u8"not scalar value: 0x"_em + to_s<16, 8>(msg.result.value())
				+ u8", sequence at offset "_em + offset_of_leading(msg, 4) + u8", "_em
				+ to_s(msg.result.distance())
				+ (msg.result.distance() == 1 ? u8" code unit was read"_em : u8" code units were read"_em);
		case eof_when_read_code_unit:
			return u8"eof when read code unit, sequence at offset "_em + offset_of_leading(msg, 4);
		default:
			throw inner_error{ "UTF-32 decode error and error_message_generator_en unresolved" };
			return u8"UTF-32 decode error, error_message_generator_en unresolved"_em;
		}
	}
}

#endif
