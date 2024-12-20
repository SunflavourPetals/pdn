#ifndef PDN_Header_pdn_err_msg_gen_en_syn
#define PDN_Header_pdn_err_msg_gen_en_syn

#include "pdn_exception.h"
#include "pdn_error_string.h"
#include "pdn_syntax_error_code.h"
#include "pdn_source_position.h"
#include "pdn_raw_error_message_type.h"
#include "pdn_raw_error_message_variant.h"

#include "pdn_err_msg_gen_utility.h"

namespace pdn::dev_util
{
	inline auto err_msg_gen_en(syntax_error_code errc, source_position, raw_err_v_cref raw) -> error_msg_string
	{
		using namespace err_msg_gen_util::syntax_err_msg_gen_util;
		using namespace error_message_literals;
		using enum syntax_error_code;
		switch (errc)
		{
		case entity_redefine:
			return u8"entity \"" + get_slashes_iden(raw) + u8"\" redefine in this scope"_em;
		case casting_domain_error:
			return u8"domain error in casting: cast "_em + get_casting_operand(raw)
				+ u8" with type "_em + get_source_type_name(raw)
				+ u8" to type "_em   + get_target_type_name(raw)
				+ u8"["_em + get_target_min_s(raw) + u8", "_em + get_target_max_s(raw) + u8"]"_em;
		case illegal_cast:
			return is_source_type_list_or_object(raw)
				? u8"illegal cast: cast "_em + get_source_type_name(raw)
				+ u8" to type "_em           + get_target_type_name(raw)
				: u8"illegal cast: cast "_em + get_casting_operand(raw)
				+ u8" with type "_em         + get_source_type_name(raw)
				+ u8" to type "_em           + get_target_type_name(raw);
		case at_value_not_found:
			// todo
			return u8"error"_em;
		case expect_entity_name:
			return u8"expect entity-name(identifier) but receiving "_em
				+ get_description_for_error_token(raw)
				+ u8" before entity"_em;
		case expect_type_name:
			return u8"expect type-name(identifier) but receiving "_em + get_description_for_error_token(raw);
		case expect_expression:
			return u8"expect expression but receiving "_em + get_description_for_error_token(raw);
		case expect_comma:
			return u8"expect comma(,) but receiving "_em + get_description_for_error_token(raw);
		case expect_colon:
			return u8"expect colon(:) but receiving "_em + get_description_for_error_token(raw);
		case expect_definition_of_named_entity:
			return u8"expect named entity but receiving "_em + get_description_for_error_token(raw);
		case expect_definition_of_list_element:
			return u8"expect element of list but receiving "_em + get_description_for_error_token(raw);
		case invalid_unary_operation:
			return u8"invalid unary operation: "_em
				+ get_unary_operator_s(raw)
				+ u8" on "_em
				+ get_description_for_unary_operation(raw);
		case unknown_type:
			return u8"type \""_em + get_slashes_iden(raw) + u8"\" not found"_em;
		case missing_right_brackets:
			return u8"missing right brackets: ]"_em;
		case missing_right_curly_brackets:
			return u8"missing right curly brackets: }"_em;
		default:
			throw inner_error{ "syntax error and error_message_generator_en unresolved" };
			return u8"syntax error, error_message_generator_en unresolved"_em;
		}
	}
}

#endif
