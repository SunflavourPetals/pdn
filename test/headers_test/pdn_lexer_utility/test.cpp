#include <pdn/pdn_lexer_utility.h>

#include <cassert>
#include <cstdint>

namespace
{
	namespace lu = pdn::lexer_utility;

	struct code_point_range
	{
		char32_t first;
		char32_t last;
	};

	// docs/pdn-reference.md:225 -- the Unicode part of the identifier starter set
	// (the ASCII letters and '_' are checked separately).
	constexpr code_point_range starter_ranges[] =
	{
		{ 0x00A8, 0x00A8 }, { 0x00AA, 0x00AA }, { 0x00AD, 0x00AD }, { 0x00AF, 0x00AF },
		{ 0x00B2, 0x00B5 }, { 0x00B7, 0x00BA }, { 0x00BC, 0x00BE }, { 0x00C0, 0x00D6 },
		{ 0x00D8, 0x00F6 }, { 0x00F8, 0x00FF }, { 0x0100, 0x02FF }, { 0x0370, 0x167F },
		{ 0x1681, 0x180D }, { 0x180F, 0x1DBF }, { 0x1E00, 0x1FFF }, { 0x200B, 0x200D },
		{ 0x202A, 0x202E }, { 0x203F, 0x2040 }, { 0x2054, 0x2054 }, { 0x2060, 0x206F },
		{ 0x2070, 0x20CF }, { 0x2100, 0x218F }, { 0x2460, 0x24FF }, { 0x2776, 0x2793 },
		{ 0x2C00, 0x2DFF }, { 0x2E80, 0x2FFF }, { 0x3004, 0x3007 }, { 0x3021, 0x302F },
		{ 0x3031, 0x303F }, { 0x3040, 0xD7FF }, { 0xF900, 0xFD3D }, { 0xFD40, 0xFDCF },
		{ 0xFDF0, 0xFE1F }, { 0xFE30, 0xFE44 }, { 0xFE47, 0xFFFD },
		{ 0x10000, 0x1FFFD }, { 0x20000, 0x2FFFD }, { 0x30000, 0x3FFFD }, { 0x40000, 0x4FFFD },
		{ 0x50000, 0x5FFFD }, { 0x60000, 0x6FFFD }, { 0x70000, 0x7FFFD }, { 0x80000, 0x8FFFD },
		{ 0x90000, 0x9FFFD }, { 0xA0000, 0xAFFFD }, { 0xB0000, 0xBFFFD }, { 0xC0000, 0xCFFFD },
		{ 0xD0000, 0xDFFFD }, { 0xE0000, 0xEFFFD },
	};

	// docs/pdn-reference.md:234 -- characters allowed anywhere but the first position.
	constexpr code_point_range non_starter_ranges[] =
	{
		{ 0x0030, 0x0039 }, { 0x0300, 0x036F }, { 0x1DC0, 0x1DFF }, { 0x20D0, 0x20FF }, { 0xFE20, 0xFE2F },
	};

	// Unicode White_Space=Yes: 25 code points.
	constexpr code_point_range whitespace_ranges[] =
	{
		{ 0x0009, 0x000D }, { 0x0020, 0x0020 }, { 0x0085, 0x0085 }, { 0x00A0, 0x00A0 },
		{ 0x1680, 0x1680 }, { 0x2000, 0x200A }, { 0x2028, 0x2029 }, { 0x202F, 0x202F },
		{ 0x205F, 0x205F }, { 0x3000, 0x3000 },
	};

	// The five line terminators pdn accepts: LF CR NEL LS PS.
	constexpr code_point_range new_line_code_points[] =
	{
		{ 0x000A, 0x000A }, { 0x000D, 0x000D }, { 0x0085, 0x0085 }, { 0x2028, 0x2028 }, { 0x2029, 0x2029 },
	};

	template <::std::size_t size>
	constexpr bool in_any(char32_t c, const code_point_range(&ranges)[size]) noexcept
	{
		for (const auto& range : ranges)
		{
			if (range.first <= c && c <= range.last)
			{
				return true;
			}
		}
		return false;
	}

	constexpr bool doc_starter(char32_t c) noexcept
	{
		return c == U'_' || (U'A' <= c && c <= U'Z') || (U'a' <= c && c <= U'z') || in_any(c, starter_ranges);
	}

	constexpr bool doc_in_identifier(char32_t c) noexcept
	{
		return doc_starter(c) || in_any(c, non_starter_ranges);
	}

	// C++26 basic character set: the graphic ASCII characters, space, HT, LF, VT and FF.
	constexpr bool doc_basic_character_set(char32_t c) noexcept
	{
		return (U'\u0020' <= c && c <= U'\u007E') || (U'\u0009' <= c && c <= U'\u000C');
	}

	// C++26 basic literal character set: the basic character set plus NUL, BEL, BS and CR.
	constexpr bool doc_basic_literal_character_set(char32_t c) noexcept
	{
		return doc_basic_character_set(c) || c == U'\u0000' || c == U'\u0007' || c == U'\u0008' || c == U'\u000D';
	}
}

int main()
{
	constexpr char32_t last_code_point = 0x10FFFF;

	// 1. Identifier starter set: must be exactly the set documented in the reference. This is the
	//    exhaustive form of the examples in docs/pdn-reference.md:238-245.
	int starter_errors = 0;
	int in_identifier_errors = 0;
	for (char32_t c = 0; c <= last_code_point; ++c)
	{
		if (lu::is_allowed_as_first_char_of_identifier(c) != doc_starter(c))
		{
			++starter_errors;
		}
		if (lu::is_allowed_in_identifier(c) != doc_in_identifier(c))
		{
			++in_identifier_errors;
		}
	}
	assert(starter_errors == 0);
	assert(in_identifier_errors == 0);

	// The two directional formatting characters that used to be dropped from the 202A-202E range.
	assert(lu::is_allowed_as_first_char_of_identifier(U'\u202A'));
	assert(lu::is_allowed_as_first_char_of_identifier(U'\u202B'));
	assert(lu::is_allowed_as_first_char_of_identifier(U'\u202C'));
	assert(lu::is_allowed_as_first_char_of_identifier(U'\u202D'));
	assert(lu::is_allowed_as_first_char_of_identifier(U'\u202E'));
	assert(!lu::is_allowed_as_first_char_of_identifier(U'\u2029'));
	assert(!lu::is_allowed_as_first_char_of_identifier(U'\u202F'));

	// 2. Whitespace: exactly White_Space=Yes, so VT and FF are separators as well.
	int whitespace_errors = 0;
	for (char32_t c = 0; c <= last_code_point; ++c)
	{
		if (lu::is_whitespace(c) != in_any(c, whitespace_ranges))
		{
			++whitespace_errors;
		}
	}
	assert(whitespace_errors == 0);
	assert(lu::is_whitespace(U'\u000B'));  // VT
	assert(lu::is_whitespace(U'\u000C'));  // FF
	assert(!lu::is_whitespace(U'\u200B')); // ZERO WIDTH SPACE, not White_Space

	// 3. Line terminators: exactly LF CR NEL LS PS, and a subset of the whitespace set.
	int new_line_errors = 0;
	for (char32_t c = 0; c <= last_code_point; ++c)
	{
		if (lu::is_new_line(c) != in_any(c, new_line_code_points))
		{
			++new_line_errors;
		}
		if (lu::is_new_line(c) && !lu::is_whitespace(c))
		{
			++new_line_errors;
		}
	}
	assert(new_line_errors == 0);
	assert(!lu::is_new_line(U'\u000B'));
	assert(!lu::is_new_line(U'\u000C'));

	// 4. Token separators are the whitespace characters; comments are tokens of their own.
	int separator_errors = 0;
	for (char32_t c = 0; c <= last_code_point; ++c)
	{
		if (lu::is_token_separator(c) != lu::is_whitespace(c))
		{
			++separator_errors;
		}
	}
	assert(separator_errors == 0);

	// 5. Character classes.
	int class_errors = 0;
	for (char32_t c = 0; c <= last_code_point; ++c)
	{
		const bool uppercase = (U'A' <= c && c <= U'Z');
		const bool lowercase = (U'a' <= c && c <= U'z');
		const bool digit = (U'0' <= c && c <= U'9');
		const bool octal = (U'0' <= c && c <= U'7');
		const bool hex_uppercase = digit || (U'A' <= c && c <= U'F');
		const bool hex_lowercase = digit || (U'a' <= c && c <= U'f');
		if (lu::is_uppercase(c) != uppercase) { ++class_errors; }
		if (lu::is_lowercase(c) != lowercase) { ++class_errors; }
		if (lu::is_alpha(c) != (uppercase || lowercase)) { ++class_errors; }
		if (lu::is_digit(c) != digit) { ++class_errors; }
		if (lu::is_oct(c) != octal) { ++class_errors; }
		if (lu::is_hex_uppercase(c) != hex_uppercase) { ++class_errors; }
		if (lu::is_hex_lowercase(c) != hex_lowercase) { ++class_errors; }
		if (lu::is_hex(c) != (hex_uppercase || hex_lowercase)) { ++class_errors; }
		if (lu::is_digit(c) && !lu::is_hex(c)) { ++class_errors; }
	}
	assert(class_errors == 0);

	// 6. Character sets.
	int set_errors = 0;
	for (char32_t c = 0; c <= last_code_point; ++c)
	{
		if (lu::is_in_basic_character_set(c) != doc_basic_character_set(c))
		{
			++set_errors;
		}
		if (lu::is_in_basic_literal_character_set(c) != doc_basic_literal_character_set(c))
		{
			++set_errors;
		}
		if (lu::is_in_translation_character_set(c) != pdn::unicode::is_scalar_value(c))
		{
			++set_errors;
		}
	}
	assert(set_errors == 0);
	assert(!lu::is_in_basic_character_set(U'\u0000'));
	assert(lu::is_in_basic_literal_character_set(U'\u0000'));
	assert(lu::is_in_basic_literal_character_set(U'\r'));
	assert(!lu::is_in_basic_character_set(U'\r'));
	assert(!lu::is_in_basic_character_set(U'\u007F'));
	assert(lu::is_in_basic_character_set(U'$'));
	assert(lu::is_in_basic_character_set(U'@'));
	assert(lu::is_in_basic_character_set(U'`'));
	assert(!lu::is_in_translation_character_set(0xD800));
	assert(!lu::is_in_translation_character_set(0xDFFF));
	assert(lu::is_in_translation_character_set(0x10FFFF));
	assert(!lu::is_in_translation_character_set(0x110000));

	// 7. Raw string delimiter sequence: basic character set minus parentheses, backslash and whitespace.
	int raw_d_seq_errors = 0;
	for (char32_t c = 0; c <= last_code_point; ++c)
	{
		const bool expected = doc_basic_character_set(c)
			&& !lu::is_whitespace(c)
			&& c != U'(' && c != U')' && c != U'\\';
		if (lu::is_allowed_in_raw_string_d_sequence(c) != expected)
		{
			++raw_d_seq_errors;
		}
	}
	assert(raw_d_seq_errors == 0);
	assert(lu::is_allowed_in_raw_string_d_sequence(U'#'));

	// 8. in_range is inclusive on both ends.
	assert(lu::in_range(U'a', U'a', U'z'));
	assert(lu::in_range(U'z', U'a', U'z'));
	assert(!lu::in_range(U'A', U'a', U'z'));
}
