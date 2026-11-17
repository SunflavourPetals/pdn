#include <pdn/pdn_lexer_utility.h>

#include <pdn/pdn_unicode_xid.h>

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

	// 1. Identifier sets: the ASCII fast paths plus the XID_Start / XID_Continue properties that
	//    docs/pdn-reference.md:214-230 defines identifiers with. The tables themselves are tested
	//    by pdn_unicode_xid; here the point is that the lexer predicates are exactly the
	//    documented union.
	int starter_errors = 0;
	int in_identifier_errors = 0;
	for (char32_t c = 0; c <= last_code_point; ++c)
	{
		const bool ascii_letter = (U'A' <= c && c <= U'Z') || (U'a' <= c && c <= U'z');
		const bool ascii_digit = U'0' <= c && c <= U'9';
		const bool expected_starter = ascii_letter || c == U'_' || pdn::unicode::is_xid_start(c);
		const bool expected_in_identifier = ascii_letter || ascii_digit || c == U'_' || pdn::unicode::is_xid_continue(c);
		if (lu::is_allowed_as_first_char_of_identifier(c) != expected_starter)
		{
			++starter_errors;
		}
		if (lu::is_allowed_in_identifier(c) != expected_in_identifier)
		{
			++in_identifier_errors;
		}
	}
	assert(starter_errors == 0);
	assert(in_identifier_errors == 0);

	// The directional formatting characters are not XID_Start. The previous, MSVC-derived
	// table accepted the whole 202A-202E range; the XID rule accepts none of them.
	assert(!lu::is_allowed_as_first_char_of_identifier(U'\u202A'));
	assert(!lu::is_allowed_as_first_char_of_identifier(U'\u202B'));
	assert(!lu::is_allowed_as_first_char_of_identifier(U'\u202C'));
	assert(!lu::is_allowed_as_first_char_of_identifier(U'\u202D'));
	assert(!lu::is_allowed_as_first_char_of_identifier(U'\u202E'));
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
