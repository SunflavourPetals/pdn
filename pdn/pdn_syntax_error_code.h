#ifndef PDN_Header_pdn_syntax_error_code
#define PDN_Header_pdn_syntax_error_code

namespace pdn
{
	enum class syntax_error_code
	{
		success, // extra(monostate)

		entity_redefine, // extra(identifier)

		casting_domain_error,
		illegal_cast,

		expect_entity_name, // extra(error_token)
		expect_type_name, // extra(error_token)
		expect_expression, // extra(error_token)
		expect_comma, // extra(error_token)
		expect_colon, // extra(error_token)

		invalid_unary_operation, // extra(unary_operation)

		unexpected_token, // extra(error_token)

		unknown_type, // extra(identifier)

		missing_right_brackets, // extra(monostate)
		missing_right_curly_brackets, // extra(monostate)
	};
}

#endif
