#ifndef PDN_Header_pdn_raw_error_message_type
#define PDN_Header_pdn_raw_error_message_type

#include <system_error>
#include <cstddef>

#include "pdn_unicode_base.h"
#include "pdn_utf_decoder.h"
#include "pdn_error_string.h"
#include "pdn_token_value_variant.h"
#include "pdn_token.h"
#include "pdn_type_code.h"

// The payload types carried by error codes. Decoding payloads are passed on by the code point iterator;
// lexical and syntax payloads are named by the "extra(T)" annotation next to the enumerator. The message
// generator reports an error by reading the struct that matches the code (pdn_raw_error_message_variant.h
// wraps them into one std::variant). The types are plain aggregates: their fields are exactly what the
// reporting site knows and the message text needs.
namespace pdn::raw_error_message_type
{
	// A decoding failure carries the codec result plus the code unit that could not be consumed and its
	// offset (the decoder stops at the last unit of the malformed sequence). There is no encoding payload:
	// encoding is only applied to code points a decoder produced, so it cannot fail while parsing.
	struct utf8_decode_error final
	{
		unicode::utf8::decode_result result;
		unicode::utf8::code_unit_t   last_code_unit;
		::std::size_t                last_code_unit_offset;
	};
	struct utf16_decode_error final
	{
		unicode::utf16::decode_result result;
		unicode::utf16::code_unit_t   last_code_unit;
		::std::size_t                 last_code_unit_offset;
	};
	struct utf32_decode_error final
	{
		unicode::utf32::decode_result result;
		unicode::utf32::code_unit_t   last_code_unit;
		::std::size_t                 last_code_unit_offset;
	};
}

// for parser
namespace pdn::raw_error_message_type
{
	struct identifier final
	{
		error_msg_string value;
	};
	struct error_token final
	{
		token<error_msg_char> value;
	};
	struct unary_operation final
	{
		token_value_variant<error_msg_char> operand;
		type_code operand_type;
		bool      negative;
	};
	struct casting_msg final
	{
		token_value_variant<error_msg_char> operand;
		type_code source_type;
		type_code target_type;
	};
}

// for lexer
namespace pdn::raw_error_message_type
{
	struct not_unicode_scalar_value final
	{
		unicode::code_point_t value;
	};
	struct error_string final
	{
		error_msg_string value;
	};
	struct character_length_error final
	{
		error_msg_string value;
		::std::size_t    code_point_count;
	};
	enum class number_type
	{
		bin_integer,
		oct_integer,
		dec_integer,
		hex_integer,
		dec_floating,
		hex_floating,
	};
	struct number_end_with_separator final
	{
		error_msg_string number_sequence;
		number_type      literal_type;
	};
	struct from_chars_error final // n: for number sequence
	{
		error_msg_string sequence;
		::std::ptrdiff_t offset; // from_chars_result.ptr - begin
		::std::errc      ec;
		number_type      type;
	};
	struct escape_not_unicode_scalar_value final
	{
		error_msg_string      escape_sequence; // escape sequence except \, e.g. "\u{}" -> "u{}"
		unicode::code_point_t code_point;      // result of parsing
	};
	struct missing_terminating_sequence final
	{
		error_msg_string content;
		error_msg_string d_seq;
		bool is_raw_identifier_string;
	};
	struct delimiter_error final
	{
		error_msg_string d_seq;
		bool is_raw_identifier_string; // true -> @`d_seq(...)d_seq`; false -> @"d_seq(...)d_seq"
	};
}

#endif
