#ifndef PDN_Header_pdn_err_msg_gen_en_utf8_dec
#define PDN_Header_pdn_err_msg_gen_en_utf8_dec

#include <cassert>
#include <variant>

#include "pdn_error_string.h"
#include "pdn_utf8_decoder.h"
#include "pdn_source_position.h"
#include "pdn_raw_error_message_type.h"
#include "pdn_raw_error_message_variant.h"

#include "pdn_err_msg_gen_utility.h"

namespace pdn::detail
{
	// English text for the UTF-8 decoding errors, one case per enumerator. There is deliberately no
	// default: a new code without a message must fail -Wswitch instead of falling back to a generic
	// text. The position is not used here; the caller prefixes "code(line:column)".
	using utf8_decode_error_code = unicode::utf8::decode_error_code;

	inline auto err_msg_gen_en(utf8_decode_error_code errc, source_position, raw_err_v_cref raw) -> error_msg_string
	{
		using namespace err_msg_gen_util;
		using namespace error_message_literals;
		using enum utf8_decode_error_code;
		const auto& msg = ::std::get<raw_details::utf8_decode_error>(raw);
		switch (errc)
		{
		case invalid_code_point:
			return u8"invalid code point: 0x"_em + to_s<16, 4>(msg.result.value())
				+ u8", in the sequence at offset "_em + offset_of_leading(msg) + u8" (add 3 if the input starts with a BOM), "_em
				+ to_s(msg.result.distance() + 1)
				+ (msg.result.distance() ? u8" code units were read"_em : u8" code unit was read"_em);
		case non_code_point:
			return u8"not a code point: 0x"_em + to_s<16, 4>(msg.result.value())
				+ u8", in the sequence at offset "_em + offset_of_leading(msg) + u8" (add 3 if the input starts with a BOM), "_em
				+ to_s(msg.result.distance() + 1)
				+ (msg.result.distance() ? u8" code units were read"_em : u8" code unit was read"_em);
		case incomplete_sequence:
			return u8"incomplete sequence (expected a UTF-8 trailing code unit), code unit: 0x"_em + to_s<16, 2>(msg.last_code_unit)
				+ u8", in the sequence at offset "_em + offset_of_leading(msg) + u8" (add 3 if the input starts with a BOM), "_em
				+ to_s(msg.result.distance() + 1)
				+ (msg.result.distance() ? u8" code units were read"_em : u8" code unit was read"_em);
		case trailing_as_start:
			return u8"trailing code unit where a UTF-8 leading code unit is expected, code unit: 0x"_em + to_s<16, 2>(msg.last_code_unit)
				+ u8", in the sequence at offset "_em + offset_of_leading(msg) + u8" (add 3 if the input starts with a BOM), "_em
				+ to_s(msg.result.distance() + 1)
				+ (msg.result.distance() ? u8" code units were read"_em : u8" code unit was read"_em);
		case unsupported_leading:
			return u8"unsupported UTF-8 leading code unit: 0x"_em + to_s<16, 2>(msg.last_code_unit)
				+ u8", in the sequence at offset "_em + offset_of_leading(msg) + u8" (add 3 if the input starts with a BOM), "_em
				+ to_s(msg.result.distance() + 1)
				+ (msg.result.distance() ? u8" code units were read"_em : u8" code unit was read"_em);
		case non_shortest_sequence:
			return u8"non-shortest UTF-8 encoding"_em
				+ u8", in the sequence at offset "_em + offset_of_leading(msg) + u8" (add 3 if the input starts with a BOM), "_em
				+ to_s(msg.result.distance() + 1)
				+ (msg.result.distance() ? u8" code units were read"_em : u8" code unit was read"_em);
		case unexpected_eof_offset0: // theoretically unreachable by implementation
			return u8"unexpected end of input while reading code unit 1 of the sequence at offset "_em
				+ offset_of_leading(msg) + u8" (add 3 if the input starts with a BOM)"_em;
		case unexpected_eof_offset1:
			return u8"unexpected end of input while reading code unit 2 of the sequence at offset "_em
				+ offset_of_leading(msg) + u8" (add 3 if the input starts with a BOM)"_em;
		case unexpected_eof_offset2:
			return u8"unexpected end of input while reading code unit 3 of the sequence at offset "_em
				+ offset_of_leading(msg) + u8" (add 3 if the input starts with a BOM)"_em;
		case unexpected_eof_offset3:
			return u8"unexpected end of input while reading code unit 4 of the sequence at offset "_em
				+ offset_of_leading(msg) + u8" (add 3 if the input starts with a BOM)"_em;
		case unexpected_eof_offset4:
			return u8"unexpected end of input while reading code unit 5 of the sequence at offset "_em
				+ offset_of_leading(msg) + u8" (add 3 if the input starts with a BOM)"_em;
		case unexpected_eof_offset5:
			return u8"unexpected end of input while reading code unit 6 of the sequence at offset "_em
				+ offset_of_leading(msg) + u8" (add 3 if the input starts with a BOM)"_em;
		}
		assert(false && "[pdn] UTF-8 decode error code without an English message");
		return u8"unhandled UTF-8 decode error code"_em;
	}
}

#endif
