#ifndef PDN_Header_pdn_raw_error_message_variant
#define PDN_Header_pdn_raw_error_message_variant

#include <variant>

#include "pdn_raw_error_message_type.h"

namespace pdn::raw_error_message_type::detail
{
	// The closed set of raw payloads a diagnostic can carry, grouped by the layer that
	// produces them (code point iterator, parser, lexer). Only decoding can fail there:
	// encoding is only applied to code points a decoder produced, so it never fails while
	// parsing. The error code stored next to a payload decides which alternative to read:
	// the message generators pick it with std::get, so a payload that does not match its
	// code is a std::bad_variant_access (terminate when exceptions are disabled). monostate
	// is the "no payload" alternative; it only keeps a default-constructed
	// raw_error_message valid and is never read.
	using raw_error_message_variant = ::std::variant<
		::std::monostate,
		// for code point iterator
		utf8_decode_error,
		utf16_decode_error,
		utf32_decode_error,
		// for parser
		identifier,
		error_token,
		unary_operation,
		casting_msg,
		// for lexer
		not_unicode_scalar_value,
		error_string,
		character_length_error,
		number_end_with_separator,
		from_chars_error,
		escape_not_unicode_scalar_value,
		missing_terminating_sequence,
		delimiter_error>;
}

namespace pdn
{
	using raw_error_message_type::detail::raw_error_message_variant;
}

#endif
