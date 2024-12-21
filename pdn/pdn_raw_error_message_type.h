#ifndef PDN_Header_pdn_raw_error_message_type
#define PDN_Header_pdn_raw_error_message_type

#include <cstddef>

#include "pdn_types.h"
#include "pdn_unicode.h"
#include "pdn_error_string.h"
#include "pdn_token_value_variant.h"
#include "pdn_token.h"
#include "pdn_type_code.h"

namespace pdn::raw_error_message_type
{
	struct utf_8_decode_error final
	{
		unicode::utf_8::decode_result result;
		unicode::utf_8::code_unit_t   last_code_unit;
		::std::size_t                 last_code_unit_offset;
	};
	struct utf_16_decode_error final
	{
		unicode::utf_16::decode_result result;
		unicode::utf_16::code_unit_t   last_code_unit;
		::std::size_t                  last_code_unit_offset;
	};
	struct utf_32_decode_error final
	{
		unicode::utf_32::decode_result result;
		unicode::utf_32::code_unit_t   last_code_unit;
		::std::size_t                  last_code_unit_offset;
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
}

#endif
