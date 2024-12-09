#ifndef PDN_Header_pdn_err_msg_gen_en_utf_8_dec
#define PDN_Header_pdn_err_msg_gen_en_utf_8_dec

#include <variant>

#include "pdn_exception.h"
#include "pdn_error_string.h"
#include "pdn_utf_8_decoder.h"
#include "pdn_source_position.h"
#include "pdn_raw_error_message_type.h"
#include "pdn_raw_error_message_variant.h"

#include "pdn_err_msg_gen_utility.h"

namespace pdn::dev_util
{
	using utf_8_decode_error_code = unicode::utf_8::decode_error_code;

	inline auto err_msg_gen_en(utf_8_decode_error_code errc, source_position pos, raw_error_message_variant raw) -> error_msg_string
	{
		using namespace dev_util::err_msg_gen_util;
		using namespace error_message_literals;
		using enum utf_8_decode_error_code;
		auto& err_msg = ::std::get<raw_error_message_type::utf_8_decode_error>(raw);
		switch (errc)
		{
		case not_scalar_value:
			return u8"not scalar value: 0x"_em + to_s<16, 4>(err_msg.result.value())
				+ u8", sequence at offset "_em + offset_of_leading(err_msg) + u8", "_em
				+ to_s(err_msg.last_code_unit_offset)
				+ (err_msg.last_code_unit_offset == 1 ? u8" code unit was read"_em : u8" code units were read"_em);
		case eof_when_read_1st_code_unit:
			return u8"eof when read 1st code unit, sequence at offset "_em + offset_of_leading(err_msg);
		case eof_when_read_2nd_code_unit:
			return u8"eof when read 2nd code unit, sequence at offset "_em + offset_of_leading(err_msg);
		case eof_when_read_3rd_code_unit:
			return u8"eof when read 3rd code unit, sequence at offset "_em + offset_of_leading(err_msg);
		case eof_when_read_4th_code_unit:
			return u8"eof when read 4th code unit, sequence at offset "_em + offset_of_leading(err_msg);
		case eof_when_read_5th_code_unit:
			return u8"eof when read 5th code unit, sequence at offset "_em + offset_of_leading(err_msg);
		case eof_when_read_6th_code_unit:
			return u8"eof when read 6th code unit, sequence at offset "_em + offset_of_leading(err_msg);
		case requires_utf_8_trailing:
			return u8"requires trailing and read one which not trailing, code unit: 0x"_em + to_s<16, 2>(err_msg.result.value())
				+ u8", sequence at offset "_em + offset_of_leading(err_msg) + u8", "_em
				+ to_s(err_msg.last_code_unit_offset)
				+ (err_msg.last_code_unit_offset == 1 ? u8" code unit was read"_em : u8" code units were read"_em);
		case requires_utf_8_leading:
			return u8"requires leading and read one which not leading, code unit: 0x"_em + to_s<16, 2>(err_msg.result.value())
				+ u8", sequence at offset "_em + offset_of_leading(err_msg) + u8", "_em
				+ to_s(err_msg.last_code_unit_offset)
				+ (err_msg.last_code_unit_offset == 1 ? u8" code unit was read"_em : u8" code units were read"_em);
		case unsupported_utf_8_leading:
			return u8"unsupported utf-8 leading, code unit: 0x"_em + to_s<16, 2>(err_msg.result.value())
				+ u8", sequence at offset "_em + offset_of_leading(err_msg) + u8", "_em
				+ to_s(err_msg.last_code_unit_offset)
				+ (err_msg.last_code_unit_offset == 1 ? u8" code unit was read"_em : u8" code units were read"_em);
		default:
			throw inner_error{ "UTF-8 decode error and error_message_generator_en unresolved" };
			return u8"UTF-8 decode error, error_message_generator_en unresolved"_em;
		}
	}
}

#endif
