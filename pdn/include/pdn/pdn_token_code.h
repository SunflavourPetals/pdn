#ifndef PDN_Header_pdn_token_code
#define PDN_Header_pdn_token_code

namespace pdn
{
	enum class pdn_token_code
	{
		invalid,

		eof,
		at_identifier,          // variant(at_iden_string_proxy)
		identifier,             // variant(proxy<string>)
		literal_integer,        // variant(i8..u64)
		literal_floating_point, // variant(f64)
		literal_string,         // variant(proxy<string>)
		literal_character,      // variant(character)
		literal_boolean,        // variant(boolean); no lexer state produces it

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
