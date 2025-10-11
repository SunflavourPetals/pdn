#ifndef PDN_Header_pdn_err_msg_gen_en_utf_8_dec
#define PDN_Header_pdn_err_msg_gen_en_utf_8_dec

#include <cassert>
#include <variant>

#include "pdn_error_string.h"
#include "pdn_utf_8_decoder.h"
#include "pdn_source_position.h"
#include "pdn_raw_error_message_type.h"
#include "pdn_raw_error_message_variant.h"

#include "pdn_err_msg_gen_utility.h"

namespace pdn::detail
{
	using utf_8_decode_error_code = unicode::utf_8::decode_error_code;

	inline auto err_msg_gen_en(utf_8_decode_error_code errc, source_position, raw_err_v_cref raw) -> error_msg_string
	{
		using namespace err_msg_gen_util;
		using namespace error_message_literals;
		using enum utf_8_decode_error_code;
		const auto& msg = ::std::get<raw_details::utf_8_decode_error>(raw);
		switch (errc)
		{
		case invalid_code_point:
			return u8"invalid code point: 0x"_em + to_s<16, 4>(msg.result.value())
				+ u8", sequence at offset "_em + offset_of_leading(msg) + u8"(if with BOM then +3), "_em
				+ to_s(msg.result.distance() + 1)
				+ (msg.result.distance() ? u8" code units were read"_em : u8" code unit was read"_em);
		case non_code_point:
			return u8"non-code point: 0x"_em + to_s<16, 4>(msg.result.value())
				+ u8", sequence at offset "_em + offset_of_leading(msg) + u8"(if with BOM then +3), "_em
				+ to_s(msg.result.distance() + 1)
				+ (msg.result.distance() ? u8" code units were read"_em : u8" code unit was read"_em);
		case incomplete_sequence:
			return u8"incomplete sequence(expect utf-8 trailing), code unit: 0x"_em + to_s<16, 2>(msg.last_code_unit)
				+ u8", sequence at offset "_em + offset_of_leading(msg) + u8"(if with BOM then +3), "_em
				+ to_s(msg.result.distance() + 1)
				+ (msg.result.distance() ? u8" code units were read"_em : u8" code unit was read"_em);
		case trailing_as_start:
			return u8"trailing as start(expect utf-8 leading), code unit: 0x"_em + to_s<16, 2>(msg.last_code_unit)
				+ u8", sequence at offset "_em + offset_of_leading(msg) + u8"(if with BOM then +3), "_em
				+ to_s(msg.result.distance() + 1)
				+ (msg.result.distance() ? u8" code units were read"_em : u8" code unit was read"_em);
		case unsupported_leading:
			return u8"unsupported utf-8 leading, code unit: 0x"_em + to_s<16, 2>(msg.last_code_unit)
				+ u8", sequence at offset "_em + offset_of_leading(msg) + u8"(if with BOM then +3), "_em
				+ to_s(msg.result.distance() + 1)
				+ (msg.result.distance() ? u8" code units were read"_em : u8" code unit was read"_em);
		case non_shortest_sequence:
			return u8"non-shortest utf-8 sequence, sequence at offset "_em + offset_of_leading(msg) + u8"(if with BOM then +3), "_em
				+ to_s(msg.result.distance() + 1)
				+ (msg.result.distance() ? u8" code units were read"_em : u8" code unit was read"_em);
		case unexpected_eof_offset0: // theoretically unreachable by implementation
			return u8"eof when read 1st code unit, sequence at offset "_em + offset_of_leading(msg) + u8"(if with BOM then +3)"_em;
		case unexpected_eof_offset1:
			return u8"eof when read 2nd code unit, sequence at offset "_em + offset_of_leading(msg) + u8"(if with BOM then +3)"_em;
		case unexpected_eof_offset2:
			return u8"eof when read 3rd code unit, sequence at offset "_em + offset_of_leading(msg) + u8"(if with BOM then +3)"_em;
		case unexpected_eof_offset3:
			return u8"eof when read 4th code unit, sequence at offset "_em + offset_of_leading(msg) + u8"(if with BOM then +3)"_em;
		case unexpected_eof_offset4:
			return u8"eof when read 5th code unit, sequence at offset "_em + offset_of_leading(msg) + u8"(if with BOM then +3)"_em;
		case unexpected_eof_offset5:
			return u8"eof when read 6th code unit, sequence at offset "_em + offset_of_leading(msg) + u8"(if with BOM then +3)"_em;
		default:
			assert(0 && "UTF-8 decode error and error_message_generator_en unresolved");
			return u8"UTF-8 decode error and error_message_generator_en unresolved"_em;
		}
	}
}

#endif
