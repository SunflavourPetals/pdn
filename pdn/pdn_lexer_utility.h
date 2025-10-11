#ifndef PDN_Header_pdn_lexer_utility
#define PDN_Header_pdn_lexer_utility

#include "pdn_unicode_base.h"

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
	// https://learn.microsoft.com/en-us/cpp/cpp/identifiers-cpp
	constexpr bool is_allowed_as_first_char_of_identifier(code_point_t c) noexcept
	{
		if (is_alpha(c) || c == U'_')
		{
			return true;
		}
		switch (c)
		{
		case U'\u00A8':
		case U'\u00AA':
		case U'\u00AD':
		case U'\u00AF':
			return true;
		default:
			break;
		}
		if (in_range(c, U'\U00010000', U'\U000EFFFD'))
		{
			//	10000-1FFFD,
			//	20000-2FFFD,
			//	30000-3FFFD,
			//	40000-4FFFD,
			//	50000-5FFFD,
			//	60000-6FFFD,
			//	70000-7FFFD,
			//	80000-8FFFD,
			//	90000-9FFFD,
			//	A0000-AFFFD,
			//	B0000-BFFFD,
			//	C0000-CFFFD,
			//	D0000-DFFFD,
			//	E0000-EFFFD
			return (c & U'\uFFFE') == U'\uFFFE' ? false : true;
		}
		if (c >= U'\u3000')
		{
			if (in_range(c, U'\u3004', U'\u3007')  // 3004-3007,
			 || in_range(c, U'\u3021', U'\u302F')  // 3021-302F,
			 || in_range(c, U'\u3031', U'\uD7FF')) // 3031-303F, 3040-D7FF,
			{
				return true;
			}
			if (in_range(c, U'\uF900', U'\uFFFD'))
			{
				//	F900-FD3D,
				//	FD40-FDCF,
				//	FDF0-FE1F,
				//	FE30-FE44,
				//	FE47-FFFD,
				switch (c)
				{
				case U'\uFD3E':
				case U'\uFD3F':
				case U'\uFE45':
				case U'\uFE46':
					return false;
				default:
					return in_range(c, U'\uFDD0', U'\uFDEF') || in_range(c, U'\uFD20', U'\uFD2F') ? false : true;
				}
			}
			return false;
		}
		if (c >= U'\u2000')
		{
			switch (c)
			{
			//	200B-200D,
			//	202A-202E,
			//	203F-2040,
			//	2054,
			case U'\u200B':
			case U'\u200C':
			case U'\u200D':
			case U'\u202A':
			case U'\u202D':
			case U'\u202E':
			case U'\u203F':
			case U'\u2040':
			case U'\u2054':
				return true;
			default:
				break;
			}
			if (in_range(c, U'\u2060', U'\u20CF')  // 2060-206F, 2070 - 20CF,
			 || in_range(c, U'\u2100', U'\u218F')  // 2100-218F,
			 || in_range(c, U'\u2460', U'\u24FF')  // 2460-24FF,
			 || in_range(c, U'\u2776', U'\u2793')  // 2776-2793,
			 || in_range(c, U'\u2C00', U'\u2DFF')  // 2C00-2DFF,
			 || in_range(c, U'\u2E80', U'\u2FFF')) // 2E80-2FFF,
			{
				return true;
			}
			return false;
		}

		if (in_range(c, U'\u00B2', U'\u02FF'))
		{
			//	00B2-00B5,
			//	00B7-00BA,
			//	00BC-00BE,
			//	00C0-00D6,
			//	00D8-00F6,
			//	00F8-00FF, 0100-02FF,
			switch (c)
			{
			case U'\u00B6':
			case U'\u00BB':
			case U'\u00BF':
			case U'\u00D7':
			case U'\u00F7':
				return false;
			default:
				return true;
			}
		}
		if (in_range(c, U'\u0370', U'\u1DBF'))
		{
			//	0370-167F,
			//	1681-180D,
			//	180F-1DBF,
			switch (c)
			{
			case U'\u1680':
			case U'\u180E':
				return false;
			default:
				return true;
			}
		}
		if (in_range(c, U'\u1E00', U'\u1FFF')) // 1E00-1FFF,
		{
			return true;
		}
		return false;
	}
	// is c allowed in identifier
	// https://learn.microsoft.com/en-us/cpp/cpp/identifiers-cpp
	constexpr bool is_allowed_in_identifier(code_point_t c) noexcept
	{
		//   is_allowed_in_identifier_first
		// | '0'-'9'
		// | 0300-036F
		// | 1DC0-1DFF
		// | 20D0-20FF
		// | FE20-FE2F
		return is_alpha(c) || c == U'_' // for optimization, ASCII compatible characters are preferentially processed. 
			|| is_digit(c)
			|| is_allowed_as_first_char_of_identifier(c)
			|| in_range(c, U'\u0300', U'\u036F')
			|| in_range(c, U'\u1DC0', U'\u1DFF')
			|| in_range(c, U'\u20D0', U'\u20FF')
			|| in_range(c, U'\uFE20', U'\uFE2F');
	}
	// is c in basic character set (ref c++26)
	// https://en.cppreference.com/w/cpp/language/charset#Basic_character_set
	constexpr bool is_in_basic_character_set(code_point_t c) noexcept
	{
		return in_range(c, U'\u0020', U'\u007E') || in_range(c, U'\u0009', U'\u000C') ? true : false;
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
