#ifndef PDN_Header_pdn_err_msg_gen_en_syn
#define PDN_Header_pdn_err_msg_gen_en_syn

#include <cassert>

#include "pdn_error_string.h"
#include "pdn_syntax_error_code.h"
#include "pdn_source_position.h"
#include "pdn_raw_error_message_type.h"
#include "pdn_raw_error_message_variant.h"

#include "pdn_err_msg_gen_utility.h"

namespace pdn::detail
{
	// English text for the syntax errors, one case per enumerator. There is deliberately no default:
	// a new code without a message must fail -Wswitch instead of falling back to a generic text.
	// The position is not used here; the caller prefixes the message with "code(line:column)".
	inline auto err_msg_gen_en(syntax_error_code errc, source_position, raw_err_v_cref raw) -> error_msg_string
	{
		using namespace err_msg_gen_util::syntax_err_msg_gen_util;
		using namespace error_message_literals;
		using enum syntax_error_code;
		switch (errc)
		{
		case entity_redefine:
			return u8"entity \"" + get_slashes_iden(raw) + u8"\" is redefined in this scope"_em;
		case casting_domain_error:
			return u8"cannot cast "_em + get_casting_operand(raw)
				+ u8" from "_em + get_source_type_name(raw)
				+ u8" to "_em   + get_target_type_name(raw)
				+ u8": out of range ["_em + get_target_min_s(raw) + u8", "_em + get_target_max_s(raw) + u8"]"_em;
		case illegal_cast:
			return is_source_type_list_or_object(raw)
				? u8"cannot cast "_em + get_source_type_name(raw)
				+ u8" to "_em   + get_target_type_name(raw)
				: u8"cannot cast "_em + get_casting_operand(raw)
				+ u8" ("_em      + get_source_type_name(raw)
				+ u8") to "_em    + get_target_type_name(raw);
		case at_value_not_found:
			return u8"at value "_em + get_at_iden_s(raw) + u8" not found"_em;
		case expect_entity_name:
			return u8"expected entity name (identifier), but got "_em
				+ get_description_for_error_token(raw);
		case expect_expression:
			return u8"expected expression, but got "_em + get_description_for_error_token(raw);
		case expect_comma:
			return u8"expected comma (,), but got "_em + get_description_for_error_token(raw);
		case expect_colon:
			return u8"expected colon (:), but got "_em + get_description_for_error_token(raw);
		case expect_definition_of_named_entity:
			return u8"expected named entity definition, but got "_em + get_description_for_error_token(raw);
		case expect_definition_of_list_element:
			return u8"expected list element, but got "_em + get_description_for_error_token(raw);
		case invalid_unary_operation:
			return u8"invalid unary operation: "_em
				+ get_unary_operator_s(raw)
				+ u8" on "_em
				+ get_description_for_unary_operation(raw);
		case unknown_type:
			return u8"type \""_em + get_slashes_iden(raw) + u8"\" not found"_em;
		case missing_right_brackets:
			return u8"missing right bracket: ]"_em;
		case missing_right_curly_brackets:
			return u8"missing right curly bracket: }"_em;
		}
		assert(false && "[pdn] syntax error code without an English message");
		return u8"unhandled syntax error code"_em;
	}
}

#endif
