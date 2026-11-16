#ifndef PDN_Header_pdn_err_msg_gen_en_utf32_dec
#define PDN_Header_pdn_err_msg_gen_en_utf32_dec

#include <cassert>
#include <variant>

#include "pdn_error_string.h"
#include "pdn_utf32_decoder.h"
#include "pdn_source_position.h"
#include "pdn_raw_error_message_type.h"
#include "pdn_raw_error_message_variant.h"

#include "pdn_err_msg_gen_utility.h"

namespace pdn::detail
{
	// English text for the UTF-32 decoding errors, one case per enumerator. There is deliberately no
	// default: a new code without a message must fail -Wswitch instead of falling back to a generic
	// text. The position is not used here; the caller prefixes "code(line:column)".
	using utf32_decode_error_code = unicode::utf32::decode_error_code;

	inline auto err_msg_gen_en(utf32_decode_error_code errc, source_position, raw_err_v_cref raw) -> error_msg_string
	{
		using namespace err_msg_gen_util;
		using namespace error_message_literals;
		using enum utf32_decode_error_code;
		const auto& msg = ::std::get<raw_details::utf32_decode_error>(raw);
		switch (errc)
		{
		case invalid_code_point:
			return u8"invalid code point: 0x"_em + to_s<16, 8>(msg.result.value())
				+ u8", in the sequence at offset "_em + offset_of_leading(msg, 4) + u8" (add 4 if the input starts with a BOM), "_em
				+ to_s(msg.result.distance() + 1)
				+ (msg.result.distance() ? u8" code units were read"_em : u8" code unit was read"_em);
		case non_code_point:
			return u8"not a code point: 0x"_em + to_s<16, 8>(msg.result.value())
				+ u8", in the sequence at offset "_em + offset_of_leading(msg, 4) + u8" (add 4 if the input starts with a BOM), "_em
				+ to_s(msg.result.distance() + 1)
				+ (msg.result.distance() ? u8" code units were read"_em : u8" code unit was read"_em);
		case unexpected_eof: // theoretically unreachable by implementation
			return u8"unexpected end of input while reading a code unit of the sequence at offset "_em
				+ offset_of_leading(msg, 4) + u8" (add 4 if the input starts with a BOM)"_em;
		}
		assert(false && "[pdn] UTF-32 decode error code without an English message");
		return u8"unhandled UTF-32 decode error code"_em;
	}
}

#endif
