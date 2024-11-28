#ifndef PDN_Header_pdn_raw_error_message_variant
#define PDN_Header_pdn_raw_error_message_variant

#include <cstddef>
#include <variant>

#include "pdn_unicode.h"
#include "pdn_error_string.h"
#include "pdn_token.h"
#include "pdn_type_code.h"

namespace pdn::raw_error_message_type
{
	struct utf_8_decode_error final
	{
		unicode::utf_8::decode_result result;
		unicode::utf_8::code_unit_t   point;
		::std::size_t                 code_unit_offset;
	};
	struct utf_16_decode_error final
	{
		unicode::utf_16::decode_result result;
		unicode::utf_16::code_unit_t   point;
		::std::size_t                  code_unit_offset;
	};
	struct utf_32_decode_error final
	{
		unicode::utf_32::decode_result result;
		unicode::utf_32::code_unit_t   point;
		::std::size_t                  code_unit_offset;
	};
	struct utf_8_encode_error final
	{
		unicode::utf_8::encode_result result;
		unicode::code_point_t         source;
	};
	struct utf_16_encode_error final
	{
		unicode::utf_16::encode_result result;
		unicode::code_point_t          source;
	};
	struct utf_32_encode_error final
	{
		unicode::utf_32::encode_result result;
		unicode::code_point_t          source;
	};
}

namespace pdn::raw_error_message_type 
{
	struct identifier final // for flag 1
	{
		error_msg_string value;
	};
	struct error_token final // for flag 2
	{
		token<error_msg_char> value;
	};
	struct unary_operation final
	{
		token<error_msg_char> operand;
		type_code             operand_type;
		bool                  negative;
	};
	struct casting_msg final
	{
		token<error_msg_char> operand;
		type_code             source_type;
		type_code             target_type;
	};
}

namespace pdn::dev_util::raw_error_message
{
	using namespace raw_error_message_type;
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
	using dev_util::raw_error_message::raw_error_message_variant;
}

#endif
