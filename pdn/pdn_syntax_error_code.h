#ifndef PDN_Header_pdn_syntax_error_code
#define PDN_Header_pdn_syntax_error_code

namespace pdn
{
	enum class syntax_error_code
	{
		success,

		entity_redefine, // extra(redefined_identifier)

		casting_domain_error,
		illegal_cast,

		expect_entity_name, // extra(error_token)
		expect_type_name,
		expect_expression,
		expect_comma,
		expect_colon,

		invalid_operation,

		unexpected_token,

		unknown_type,

		missing_right_brackets,
		missing_right_curly_brackets,

		inner_error_token_have_no_value,
		inner_error_parse_terminated,
	};
}

#endif
