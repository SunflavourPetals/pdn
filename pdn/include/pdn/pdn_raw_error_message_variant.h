#ifndef PDN_Header_pdn_raw_error_message_variant
#define PDN_Header_pdn_raw_error_message_variant

#include <variant>

#include "pdn_raw_error_message_type.h"

namespace pdn::raw_error_message_type::detail
{
	using raw_error_message_variant = ::std::variant<
		::std::monostate,
		// for code point iterator
		utf8_decode_error,
		utf8_encode_error,
		utf16_decode_error,
		utf16_encode_error,
		utf32_decode_error,
		utf32_encode_error,
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
