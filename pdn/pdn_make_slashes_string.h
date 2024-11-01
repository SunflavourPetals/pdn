#ifndef PDN_Header_pdn_pdn_make_slashes_string
#define PDN_Header_pdn_pdn_make_slashes_string

#include <type_traits>

#include "pdn_unicode_base.h"
#include "pdn_code_convert.h"
#include "pdn_types.h"

namespace pdn
{
	template <typename string_t>
	constexpr string_t make_slashes_string(unicode::code_point_t c)
	{
		using char_t = ::std::remove_cv_t<typename string_t::value_type>;
		
		string_t s{};
		if (!unicode::is_scalar_value(c))
		{
			return s;
		}
		switch (c)
		{
		case U'\'': s.push_back(char_t('\\')); s.push_back(char_t('\'')); break;
		case U'\"': s.push_back(char_t('\\')); s.push_back(char_t('\"')); break;
		case U'\\': s.push_back(char_t('\\')); s.push_back(char_t('\\')); break;
		case U'\a': s.push_back(char_t('\\')); s.push_back(char_t('a'));  break;
		case U'\b': s.push_back(char_t('\\')); s.push_back(char_t('b'));  break;
		case U'\f': s.push_back(char_t('\\')); s.push_back(char_t('f'));  break;
		case U'\n': s.push_back(char_t('\\')); s.push_back(char_t('n'));  break;
		case U'\r': s.push_back(char_t('\\')); s.push_back(char_t('r'));  break;
		case U'\t': s.push_back(char_t('\\')); s.push_back(char_t('t'));  break;
		case U'\v': s.push_back(char_t('\\')); s.push_back(char_t('v'));  break;
		case U'\0': s.push_back(char_t('\\')); s.push_back(char_t('0'));  break;
		case U'\u0085': // NEL | NEXT LINE
			s.push_back(char_t('\\'));
			s.push_back(char_t('u'));
			s.push_back(char_t('0'));
			s.push_back(char_t('0'));
			s.push_back(char_t('8'));
			s.push_back(char_t('5'));
			break;
		case U'\u2028': // LINE SEPARATOR | LS
			s.push_back(char_t('\\'));
			s.push_back(char_t('u'));
			s.push_back(char_t('2'));
			s.push_back(char_t('0'));
			s.push_back(char_t('2'));
			s.push_back(char_t('8'));
			break;
		case U'\u2029': // PARAGRAPH SEPARATOR | PS
			s.push_back(char_t('\\'));
			s.push_back(char_t('u'));
			s.push_back(char_t('2'));
			s.push_back(char_t('0'));
			s.push_back(char_t('2'));
			s.push_back(char_t('9'));
			break;
		default:
			s += unicode::code_convert<string_t>(unicode::code_point_string_view{ &c, 1 });
		}
		return s;
	}
	template <typename string_t>
	constexpr string_t make_slashes_string(unicode::code_point_string_view sv)
	{
		string_t r{};
		for (auto c : sv)
		{
			r += make_slashes_string<string_t>(c);
		}
		return r;
	}
}

#endif
