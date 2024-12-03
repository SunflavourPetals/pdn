#ifndef PDN_Header_pdn_err_msg_gen_en_syn
#define PDN_Header_pdn_err_msg_gen_en_syn

#include <variant>

#include "pdn_exception.h"
#include "pdn_error_string.h"
#include "pdn_syntax_error_code.h"
#include "pdn_source_position.h"
#include "pdn_raw_error_message_type.h"
#include "pdn_raw_error_message_variant.h"

#include "pdn_err_msg_gen_utility.h"

namespace pdn::dev_util
{
	inline auto err_msg_gen_en(syntax_error_code errc, source_position pos, raw_error_message_variant raw) -> error_msg_string
	{
		using namespace err_msg_gen_util::syntax_err_msg_gen_util;
		using namespace error_message_literals;
		using enum syntax_error_code;
		switch (errc)
		{
		case success:
			throw inner_error{ "generate error message for syntax error code success" };

		case entity_redefine:
			return u8"entity redefine: \""_em + get_slashes_id(raw) + u8"\""_em;
		case casting_domain_error:
			return u8"domain error in casting: cast "_em + get_casting_operand(raw)
				+ u8" with type "_em + get_source_type_name(raw)
				+ u8" to type "_em + get_target_type_name(raw)
				+ u8"["_em + get_target_min_s(raw) + u8", "_em + get_target_max_s(raw) + u8"]"_em;
		case illegal_cast:
			if (is_source_type_list_or_object(raw))
			{
				return u8"illegal cast: cast "_em + get_source_type_name(raw)
					+ u8" to type "_em + get_target_type_name(raw);
			}
			else
			{
				return u8"illegal cast: cast "_em + get_casting_operand(raw)
					+ u8" with type "_em + get_source_type_name(raw)
					+ u8" to type "_em + get_target_type_name(raw);
			}
		case expect_entity_name:
		//	return u8"expect name(identifier) but receiving \""_em + src + u8"\" before entity"_em;
		case expect_type_name:
		//	return u8"expect type-name(identifier) but receiving \""_em + src + u8"\""_em;
		case expect_expression:
		//	return u8"expect expression but receiving \""_em + src + u8"\""_em;
		case expect_comma:
		//	return u8"expect comma but receiving \""_em + src + u8"\""_em;
		case expect_colon:
		//	return u8"expect colon([type-name: val, ...]) but receiving \""_em + src + u8"\""_em;

		case invalid_unary_operation:
		//	return u8"invalid unary operation: \""_em + src + u8"\""_em;

		case unexpected_token:
		//	return u8"unexpected token: \""_em + src + u8"\""_em;

		case unknown_type:
		//	return u8"no type named \""_em + src + u8"\""_em;

		case missing_right_brackets:
		//	return u8"missing right brackets: \"]\""_em;
		case missing_right_curly_brackets:
		//	return u8"missing right curly brackets: \"}\""_em;

		default:
			// todo throw inner_error
			return u8"NEW SYNTAX ERROR MESSAGE GENERATOR"_em;
		}

	//	return u8"syntax error, error_message_generator_en unresolved: \""_em + src + u8"\""_em;
	}
}

#endif
