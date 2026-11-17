// Tests for pdn/include/pdn/pdn_syntax_error_code.h.
//
// The header is a plain enum: no includes, no behaviour. What is worth pinning is the numbering
// contract the rest of the library relies on -- 0 is not an error, the fourteen codes are 1..14 in
// the documented order, and each name is a distinct value (they are switched on by the English
// message generator and stored in error_code_variant).

#include <pdn/pdn_syntax_error_code.h>

namespace
{
	using code = pdn::syntax_error_code;

	constexpr int value(code c) noexcept
	{
		return static_cast<int>(c);
	}

	static_assert(value(code::entity_redefine) == 1);
	static_assert(value(code::casting_domain_error) == 2);
	static_assert(value(code::illegal_cast) == 3);
	static_assert(value(code::at_value_not_found) == 4);
	static_assert(value(code::expect_entity_name) == 5);
	static_assert(value(code::expect_expression) == 6);
	static_assert(value(code::expect_comma) == 7);
	static_assert(value(code::expect_colon) == 8);
	static_assert(value(code::expect_definition_of_named_entity) == 9);
	static_assert(value(code::expect_definition_of_list_element) == 10);
	static_assert(value(code::invalid_unary_operation) == 11);
	static_assert(value(code::unknown_type) == 12);
	static_assert(value(code::missing_right_brackets) == 13);
	static_assert(value(code::missing_right_curly_brackets) == 14);

	// 1..14 with no holes and no reuse.
	static_assert(value(code::missing_right_curly_brackets) - value(code::entity_redefine) + 1 == 14);

	// 0 stays out of the enumerator list, so a value-initialised code names no error.
	static_assert(static_cast<code>(0) != code::entity_redefine);
	static_assert(static_cast<code>(0) != code::missing_right_curly_brackets);

	// Codes are comparable and switchable as scoped enum values.
	constexpr bool is_terminal(code c) noexcept
	{
		return c == code::missing_right_brackets || c == code::missing_right_curly_brackets;
	}

	static_assert(!is_terminal(code::expect_comma));
	static_assert(is_terminal(code::missing_right_brackets));
}

int main()
{
	return value(pdn::syntax_error_code::missing_right_curly_brackets) - 14;
}
