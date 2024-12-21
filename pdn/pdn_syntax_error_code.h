#ifndef PDN_Header_pdn_syntax_error_code
#define PDN_Header_pdn_syntax_error_code

namespace pdn
{
	enum class syntax_error_code
	{
		entity_redefine, // extra(identifier)

		casting_domain_error, // extra(casting_msg), source_type and target_type belong to { i8, i16, i32, i64, u8, u16, u32, u64 }
		illegal_cast, // extra(casting_msg)

		at_value_not_found, // extra(identifier)

		expect_entity_name, // extra(error_token)
		expect_expression, // extra(error_token)
		expect_comma, // extra(error_token)
		expect_colon, // extra(error_token)
		expect_definition_of_named_entity, // extra(error_token)
		expect_definition_of_list_element, // extra(error_token)

		invalid_unary_operation, // extra(unary_operation)

		unknown_type, // extra(identifier)

		missing_right_brackets,
		missing_right_curly_brackets,
	};
}

#endif
