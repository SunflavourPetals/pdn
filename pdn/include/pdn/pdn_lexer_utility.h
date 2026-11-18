#ifndef PDN_Header_pdn_lexer_utility
#define PDN_Header_pdn_lexer_utility

#include "pdn_unicode_base.h"
#include "pdn_unicode_xid.h"

// set of tool functions for lexer & dfa that serves pdn
// these tools are based on the Unicode character set, but serve the implementation of pdn
// these functions are not for features of Unicode
namespace pdn::lexer_utility
{
	using unicode::code_point_t;
	// is c in [first, last]
	constexpr bool in_range(code_point_t c, code_point_t first, code_point_t last) noexcept
	{
		return c >= first && c <= last;
	}
	constexpr bool is_uppercase(code_point_t c) noexcept
	{
		return in_range(c, U'A', U'Z');
	}
	constexpr bool is_lowercase(code_point_t c) noexcept
	{
		return in_range(c, U'a', U'z');
	}
	constexpr bool is_alpha(code_point_t c) noexcept
	{
		return is_lowercase(c) || is_uppercase(c);
	}
	constexpr bool is_digit(code_point_t c) noexcept
	{
		return in_range(c, U'0', U'9');
	}
	constexpr bool is_oct(code_point_t c) noexcept
	{
		return in_range(c, U'0', U'7');
	}
	constexpr bool is_hex_uppercase(code_point_t c) noexcept
	{
		return is_digit(c) || in_range(c, U'A', U'F');
	}
	constexpr bool is_hex_lowercase(code_point_t c) noexcept
	{
		return is_digit(c) || in_range(c, U'a', U'f');
	}
	constexpr bool is_hex(code_point_t c) noexcept
	{
		return is_hex_lowercase(c) || is_hex_uppercase(c);
	}
	// is whitespace
	// https://www.unicode.org/charts/collation/chart_Whitespace.html
	constexpr bool is_whitespace(code_point_t c) noexcept
	{
		switch (c)
		{
		case U'\u0009': // character tabulation
		case U'\u000A': // line feed
		case U'\u000B': // line tabulation
		case U'\u000C': // form feed
		case U'\u000D': // carriage return
		case U'\u0020': // space
		case U'\u0085': // next line
		case U'\u00A0': // no-break space
		case U'\u1680': // ogham space mark
		case U'\u2000': // en quad
		case U'\u2001': // em quad
		case U'\u2002': // en space
		case U'\u2003': // em space
		case U'\u2004': // three-per-em space
		case U'\u2005': // four-per-em space
		case U'\u2006': // six-per-em space
		case U'\u2007': // figure space
		case U'\u2008': // punctuation space
		case U'\u2009': // thin space
		case U'\u200A': // hair space
		case U'\u2028': // line separator
		case U'\u2029': // paragraph separator
		case U'\u202F': // narrow no-break space
		case U'\u205F': // medium mathematical space
		case U'\u3000': // ideographic space
			return true;
		default:
			return false;
		}
	}
	// is c token separator, except comments (whitespace)
	constexpr bool is_token_separator(code_point_t c) noexcept
	{
		return is_whitespace(c);
	}
	// is c allowed as first character of identifier
	// https://en.cppreference.com/cpp/language/identifiers
	constexpr bool is_allowed_as_first_char_of_identifier(code_point_t c) noexcept
	{
		return is_alpha(c) || c == U'_' || unicode::is_xid_start(c);
	}
	// is c allowed in identifier
	// https://en.cppreference.com/cpp/language/identifiers
	constexpr bool is_allowed_in_identifier(code_point_t c) noexcept
	{
		return is_alpha(c) || c == U'_' || is_digit(c) || unicode::is_xid_continue(c);
	}
	// is c in basic character set (ref c++26)
	// https://en.cppreference.com/w/cpp/language/charset#Basic_character_set
	constexpr bool is_in_basic_character_set(code_point_t c) noexcept
	{
		return in_range(c, U'\u0020', U'\u007E') || in_range(c, U'\u0009', U'\u000C');
	}
	// is c in basic literal character set (ref c++26)
	// https://en.cppreference.com/w/cpp/language/charset#Basic_literal_character_set
	constexpr bool is_in_basic_literal_character_set(code_point_t c) noexcept
	{
		if (is_in_basic_character_set(c)) return true;
		switch (c)
		{
		case U'\u0000': // NUL
		case U'\u0007': // BEL
		case U'\u0008': // backspace
		case U'\u000D': // CR
			return true;
		default:
			return false;
		}
	}
	// is c in translation character set (ref c++23)
	// https://en.cppreference.com/w/cpp/language/charset#Translation_character_set
	constexpr bool is_in_translation_character_set(code_point_t c) noexcept
	{
		return unicode::is_scalar_value(c);
	}
	// is new-line, such as LF, CR, LS etc.
	constexpr bool is_new_line(code_point_t c) noexcept
	{
		switch (c)
		{
		case U'\n': // LINE FEED | NEW LINE | END OF LINE | LF | NL | EOL
		case U'\r': // CARRIAGE RETURN | CR
		case U'\u0085': // NEL | NEXT LINE
		case U'\u2028': // LINE SEPARATOR | LS
		case U'\u2029': // PARAGRAPH SEPARATOR | PS
			return true;
		default:
			return false;
		}
	}
	// is c allowed in delimiter sequence for raw string
	// https://en.cppreference.com/w/cpp/language/string_literal
	constexpr bool is_allowed_in_raw_string_d_sequence(code_point_t c) noexcept
	{
		switch (c)
		{
		case U'\\':
		case U'(':
		case U')':
			return false;
		default:
			return is_whitespace(c) ? false : is_in_basic_character_set(c);
		}
	}
}

#endif
