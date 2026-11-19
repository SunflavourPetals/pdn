// Review test for pdn_token_code.h.
// The enumerator set is the lexer's whole vocabulary. The checks below pin its size, the zero
// value, and the names the parser prints when it meets a token it did not expect.

#include <array>
#include <cstddef>

#include <pdn/pdn_token.h>
#include <pdn/pdn_token_code.h>
#include <pdn/pdn_token_code_to_error_msg_string.h>

namespace
{
	using pdn::pdn_token_code;
	using namespace pdn::error_message_literals;

	constexpr ::std::array<pdn_token_code, 37> all_codes{
		pdn_token_code::invalid,
		pdn_token_code::eof,
		pdn_token_code::at_identifier,
		pdn_token_code::identifier,
		pdn_token_code::literal_integer,
		pdn_token_code::literal_floating_point,
		pdn_token_code::literal_string,
		pdn_token_code::literal_character,
		pdn_token_code::literal_boolean,
		pdn_token_code::tilde,
		pdn_token_code::exclamation_mark,
		pdn_token_code::at_sign,
		pdn_token_code::hash,
		pdn_token_code::dollar,
		pdn_token_code::percent,
		pdn_token_code::caret,
		pdn_token_code::ampersand,
		pdn_token_code::asterisk,
		pdn_token_code::left_parentheses,
		pdn_token_code::right_parentheses,
		pdn_token_code::left_brackets,
		pdn_token_code::right_brackets,
		pdn_token_code::left_curly_brackets,
		pdn_token_code::right_curly_brackets,
		pdn_token_code::minus,
		pdn_token_code::plus,
		pdn_token_code::equal,
		pdn_token_code::slash,
		pdn_token_code::back_slash,
		pdn_token_code::bar,
		pdn_token_code::colon,
		pdn_token_code::semicolon,
		pdn_token_code::less_than,
		pdn_token_code::greater_than,
		pdn_token_code::comma,
		pdn_token_code::dot,
		pdn_token_code::question_mark,
	};
	static_assert(all_codes.size() == 37);

	// invalid is the zero value, so a default-constructed token carries it and means "no token".
	static_assert(pdn::token<char8_t>{}.code == pdn_token_code::invalid);
	static_assert(static_cast<int>(pdn_token_code::invalid) == 0);

	// Every code has a non-empty name, and no two codes share one.
	constexpr bool names_are_distinct()
	{
		for (::std::size_t i = 0; i < all_codes.size(); ++i)
		{
			if (pdn::token_code_to_error_msg_string_view(all_codes[i]).empty()) return false;
			for (::std::size_t j = i + 1; j < all_codes.size(); ++j)
			{
				if (pdn::token_code_to_error_msg_string_view(all_codes[i]) ==
				    pdn::token_code_to_error_msg_string_view(all_codes[j])) return false;
			}
		}
		return true;
	}
	static_assert(names_are_distinct());

	// Spot check: literals are named by role, punctuation is printed as itself, and the
	// backslash is escaped again so the text is unambiguous in a message.
	static_assert(pdn::token_code_to_error_msg_string_view(pdn_token_code::literal_integer) == u8"integer-val"_emv);
	static_assert(pdn::token_code_to_error_msg_string_view(pdn_token_code::at_identifier) == u8"@identifier"_emv);
	static_assert(pdn::token_code_to_error_msg_string_view(pdn_token_code::question_mark) == u8"?"_emv);
	static_assert(pdn::token_code_to_error_msg_string_view(pdn_token_code::back_slash) == u8"\\"_emv);
	static_assert(pdn::token_code_to_error_msg_string_view(pdn_token_code::invalid) == u8"code-invalid"_emv);
}

int main()
{
	return 0;
}
