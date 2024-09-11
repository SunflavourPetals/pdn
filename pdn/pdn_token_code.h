#ifndef PDN_Header_pdn_token_code
#define PDN_Header_pdn_token_code

namespace pdn
{
	enum class pdn_token_code
	{
		invalid,

		eof,
		identifier,
		at_identifier,
		literal_integer,
		literal_floating_point,
		literal_string,
		literal_character,
		literal_boolean,

		tilde,                // ~
		exclamation_mark,     // !
		at_sign,              // @
		hash,                 // #
		dollar,               // $
		percent,              // %
		caret,                // ^
		ampersand,            // &
		asterisk,             // *
		left_parentheses,     // (
		right_parentheses,    // )
		left_brackets,        // [
		right_brackets,       // ]
		left_curly_brackets,  // {
		right_curly_brackets, // }
		minus,                // -
		plus,                 // +
		equal,                // =
		slash,                // (/)
		back_slash,           // (\)
		bar,                  // |
		colon,                // :
		semicolon,            // ;
		less_than,            // <
		greater_than,         // >
		comma,                // ,
		dot,                  // .
		question_mark,        // ?
	};
}

#endif