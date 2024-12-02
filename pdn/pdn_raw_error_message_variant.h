#ifndef PDN_Header_pdn_raw_error_message_variant
#define PDN_Header_pdn_raw_error_message_variant

#include <variant>

#include "pdn_raw_error_message_type.h"

namespace pdn::raw_error_message_type::dev_util
{
	using raw_error_message_variant = ::std::variant<
		::std::monostate,
		utf_8_decode_error,
		utf_8_encode_error,
		utf_16_decode_error,
		utf_16_encode_error,
		utf_32_decode_error,
		utf_32_encode_error,
		identifier,
		error_token,
		unary_operation,
		casting_msg,

		error_msg_string>; // remove it
}

namespace pdn
{
	using raw_error_message_type::dev_util::raw_error_message_variant;
}

#endif
