#ifndef PDN_Header_pdn_err_msg_gen_en_syn
#define PDN_Header_pdn_err_msg_gen_en_syn

#include "pdn_error_string.h"
#include "pdn_error_message.h"
#include "pdn_error_message_generator.h"
#include "pdn_syntax_error_code.h"

namespace pdn::dev_util
{
	inline constexpr error_msg_string err_msg_gen_en(syntax_error_code errc, error_msg_string src)
	{
		using namespace error_message_literals;
		using enum syntax_error_code;
		switch (errc)
		{
		case success:
			return u8"syntax_error_code == success: \""_em + src + u8"\""_em;

		case entity_redefine:
			return u8"entity redefine: \""_em + src + u8"\""_em;
		case casting_domain_error:
			return u8"domain error in casting: \""_em + src + u8"\""_em;
		case illegal_cast:
			return u8"illegal cast: \""_em + src + u8"\""_em;
			
		case expect_entity_name:
			return u8"expect name(identifier) but receiving \""_em + src + u8"\" before entity"_em;
		case expect_type_name:
			return u8"expect type-name(identifier) but receiving \""_em + src + u8"\""_em;
		case expect_expression:
			return u8"expect expression but receiving \""_em + src + u8"\""_em;
		case expect_comma:
			return u8"expect comma but receiving \""_em + src + u8"\""_em;
		case expect_colon:
			return u8"expect colon([type-name: val, ...]) but receiving \""_em + src + u8"\""_em;

		case invalid_unary_operation:
			return u8"invalid unary operation: \""_em + src + u8"\""_em;

		case unexpected_token:
			return u8"unexpected token: \""_em + src + u8"\""_em;

		case unknown_type:
			return u8"no type named \""_em + src + u8"\""_em;

		case missing_right_brackets:
			return u8"missing right brackets: \"]\""_em;
		case missing_right_curly_brackets:
			return u8"missing right curly brackets: \"}\""_em;

		case inner_error_token_have_no_value:
			return u8"inner parser error token have no value"_em;
		case inner_error_parse_terminated:
			return u8"inner parser error parse terminated by unknown reason"_em;

		default:
			break;
		}

		return u8"syntax error, error_message_generator_en unresolved: \""_em + src + u8"\""_em;
	}
}

#endif
