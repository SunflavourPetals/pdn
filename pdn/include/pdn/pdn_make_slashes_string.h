#ifndef PDN_Header_pdn_make_slashes_string
#define PDN_Header_pdn_make_slashes_string

#include <type_traits>

#include "pdn_unicode_base.h"
#include "pdn_utf_code_convert.h"

namespace pdn::detail
{
	// An octal escape takes up to three digits, so \0 followed by an octal digit would be read
	// as one longer escape. NUL therefore uses the short \0 form only when the next character
	// written is not an octal digit, and the brace delimited \o{0} otherwise. Unlike \u0000, the
	// brace form is not a restricted C++ universal character name.
	[[nodiscard]] constexpr auto is_octal_digit(unicode::code_point_t c) noexcept -> bool
	{
		return U'0' <= c && c <= U'7';
	}
	// Whether the character after the one at it is an octal digit. Only called for positions inside
	// the view, so the increment itself stays in range.
	[[nodiscard]] constexpr auto next_is_octal_digit_at(
		unicode::ucpstring_view::const_iterator it,
		unicode::ucpstring_view::const_iterator end) noexcept -> bool
	{
		++it;
		return it != end && is_octal_digit(*it);
	}
	// The twelve code points with the Unicode Bidi_Control property. They take no room but can
	// reorder how the rest of the line is displayed, so a diagnostic must not carry them verbatim.
	[[nodiscard]] constexpr auto is_bidi_control(unicode::code_point_t c) noexcept -> bool
	{
		switch (c)
		{
		case U'\u061c':  // ARABIC LETTER MARK
		case U'\u200e':  // LEFT-TO-RIGHT MARK
		case U'\u200f':  // RIGHT-TO-LEFT MARK
		case U'\u202a':  // LEFT-TO-RIGHT EMBEDDING
		case U'\u202b':  // RIGHT-TO-LEFT EMBEDDING
		case U'\u202c':  // POP DIRECTIONAL FORMATTING
		case U'\u202d':  // LEFT-TO-RIGHT OVERRIDE
		case U'\u202e':  // RIGHT-TO-LEFT OVERRIDE
		case U'\u2066':  // LEFT-TO-RIGHT ISOLATE
		case U'\u2067':  // RIGHT-TO-LEFT ISOLATE
		case U'\u2068':  // FIRST STRONG ISOLATE
		case U'\u2069':  // POP DIRECTIONAL ISOLATE
			return true;
		default:
			return false;
		}
	}
	// A message is one line of text, so a non-printable code point must not reach it: C0/C1 controls
	// and DEL would be invisible or break the line, and a Bidi_Control code point can reorder it.
	// PDN has escapes for the ones a literal can write; the others become the fixed width \uXXXX
	// form. Every other code point is written as itself: the message only has to point at the error.
	[[nodiscard]] constexpr auto is_printable(unicode::code_point_t c) noexcept -> bool
	{
		return c >= U' ' && c != U'\u007f' && (c < U'\u0080' || c > U'\u009f') && !is_bidi_control(c);
	}
	// The fixed width form, used where a variable width escape could swallow what follows.
	template <typename string_t>
	constexpr void append_u_escape(string_t& s, unicode::code_point_t c)
	{
		using char_t = ::std::remove_cv_t<typename string_t::value_type>;

		s += { char_t('\\'), char_t('u') };
		for (auto shift = 16; shift;)
		{
			shift -= 4;
			const auto digit = (static_cast<unsigned>(c) >> shift) & 0xFu;
			s += static_cast<char_t>(digit < 10u ? U'0' + digit : U'a' + digit - 10u);
		}
	}
	// Write one code point back as the body of a string or character literal: a backslash escape
	// when PDN has one, the code point itself otherwise.
	template <typename string_t, bool escape_every_non_printable = false>
	constexpr void append_slashes_string(string_t& s, unicode::code_point_t c, bool next_is_octal_digit)
	{
		using char_t = ::std::remove_cv_t<typename string_t::value_type>;
		constexpr auto slash = char_t('\\');
		if (!unicode::is_scalar_value(c))
		{
			return;
		}
		switch (c)
		{
		case U'\'': s += { slash, char_t('\'') }; break;
		case U'\"': s += { slash, char_t('\"') }; break;
		case U'\\': s += { slash, char_t('\\') }; break;
		case U'\a': s += { slash, char_t('a')  }; break;
		case U'\b': s += { slash, char_t('b')  }; break;
		case U'\f': s += { slash, char_t('f')  }; break;
		case U'\n': s += { slash, char_t('n')  }; break;
		case U'\r': s += { slash, char_t('r')  }; break;
		case U'\t': s += { slash, char_t('t')  }; break;
		case U'\v': s += { slash, char_t('v')  }; break;
		case U'\0':  // NULL
			if (next_is_octal_digit)
			{
				s += { slash, char_t('o'), char_t('{'), char_t('0'), char_t('}') };
			}
			else
			{
				s += { slash, char_t('0') };
			}
			break;
		case U'\u0085':  // NEL | NEXT LINE
		case U'\u2028':  // LINE SEPARATOR | LS
		case U'\u2029':  // PARAGRAPH SEPARATOR | PS
			append_u_escape<string_t>(s, c);
			break;
		default:
			if constexpr (escape_every_non_printable)
			{
				if (!is_printable(c)) [[unlikely]]
				{
					append_u_escape<string_t>(s, c);
					return;
				}
			}
			s += unicode::code_convert<string_t>(unicode::ucpstring_view{ &c, 1 });
		}
	}
	// Same, for the body of a string-form identifier (backtick), which additionally escapes the
	// backtick itself.
	template <typename string_t, bool escape_every_non_printable = false>
	constexpr void append_slashes_id_string(string_t& s, unicode::code_point_t c, bool next_is_octal_digit)
	{
		using char_t = ::std::remove_cv_t<typename string_t::value_type>;

		if (c == U'`')
		{
			s += { char_t('\\'), char_t('`') };
			return;
		}
		append_slashes_string<string_t, escape_every_non_printable>(s, c, next_is_octal_digit);
	}
}

namespace pdn
{
	template <typename string_t, bool escape_every_non_printable = false>
	constexpr string_t make_slashes_string(unicode::code_point_t c)
	{
		auto s = string_t{};
		detail::append_slashes_string<string_t, escape_every_non_printable>(s, c, false);
		return s;
	}
	template <typename string_t, bool escape_every_non_printable = false>
	constexpr string_t make_slashes_string(unicode::ucpstring_view sv)
	{
		constexpr auto cond = escape_every_non_printable;
		auto r = string_t{};
		r.reserve(sv.size());
		for (auto it = sv.cbegin(); it != sv.cend(); ++it)
		{
			detail::append_slashes_string<string_t, cond>(r, *it, detail::next_is_octal_digit_at(it, sv.cend()));
		}
		return r;
	}
	template <typename string_t, bool escape_every_non_printable = false>
	constexpr string_t make_slashes_id_string(unicode::code_point_t c)
	{
		auto s = string_t{};
		detail::append_slashes_id_string<string_t, escape_every_non_printable>(s, c, false);
		return s;
	}
	template <typename string_t, bool escape_every_non_printable = false>
	constexpr string_t make_slashes_id_string(unicode::ucpstring_view sv)
	{
		constexpr auto cond = escape_every_non_printable;
		auto r = string_t{};
		r.reserve(sv.size());
		for (auto it = sv.cbegin(); it != sv.cend(); ++it)
		{
			detail::append_slashes_id_string<string_t, cond>(r, *it, detail::next_is_octal_digit_at(it, sv.cend()));
		}
		return r;
	}
}

#endif
