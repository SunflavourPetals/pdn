#ifndef PDN_Header_pdn_err_msg_gen_en_utf_16_dec
#define PDN_Header_pdn_err_msg_gen_en_utf_16_dec

#include <cassert>
#include <variant>

#include "pdn_error_string.h"
#include "pdn_utf_16_decoder.h"
#include "pdn_source_position.h"
#include "pdn_raw_error_message_type.h"
#include "pdn_raw_error_message_variant.h"

#include "pdn_err_msg_gen_utility.h"

namespace pdn::detail
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
		case invalid_code_point: // theoretically unreachable, UTF-16 should not make this error
			return u8"invalid code point: 0x"_em + to_s<16, 4>(msg.result.value())
				+ u8", sequence at offset "_em + offset_of_leading(msg, 2) + u8"(if with BOM then +2), "_em
				+ to_s(msg.result.distance() + 1)
				+ (msg.result.distance() ? u8" code units were read"_em : u8" code unit was read"_em);
		case non_code_point: // theoretically unreachable, UTF-16 should not make this error
			return u8"non code point: 0x"_em + to_s<16, 4>(msg.result.value())
				+ u8", sequence at offset "_em + offset_of_leading(msg, 2) + u8"(if with BOM then +2), "_em
				+ to_s(msg.result.distance() + 1)
				+ (msg.result.distance() ? u8" code units were read"_em : u8" code unit was read"_em);
		case incomplete_sequence:
			return u8"incomplete sequence(expect utf-16 trailing surrogate), code unit: 0x"_em + to_s<16, 4>(msg.last_code_unit)
				+ u8", sequence at offset "_em + offset_of_leading(msg, 2) + u8"(if with BOM then +2), "_em
				+ to_s(msg.result.distance() + 1)
				+ (msg.result.distance() ? u8" code units were read"_em : u8" code unit was read"_em);
		case trailing_as_start:
			return u8"trailing as start(alone utf-16 trailing surrogate), code unit: 0x"_em + to_s<16, 4>(msg.last_code_unit)
				+ u8", sequence at offset "_em + offset_of_leading(msg, 2) + u8"(if with BOM then +2), "_em
				+ to_s(msg.result.distance() + 1)
				+ (msg.result.distance() ? u8" code units were read"_em : u8" code unit was read"_em);
		case unexpected_eof_offset0: // theoretically unreachable by implementation
			return u8"eof when read code unit(or utf-16 leading surrogate), sequence at offset "_em
				+ offset_of_leading(msg, 2) + u8"(if with BOM then +2)"_em;
		case unexpected_eof_offset1:
			return u8"eof when read utf-16 trailing surrogate, sequence at offset "_em
				+ offset_of_leading(msg, 2) + u8"(if with BOM then +2)"_em;
		default:
			assert(0 && "UTF-16 decode error and error_message_generator_en unresolved");
			return u8"UTF-16 decode error and error_message_generator_en unresolved"_em;
		}
	}
}

#endif
