#include <pdn/pdn_token_code_to_error_msg_string.h>

#include <array>
#include <cassert>
#include <cstddef>

namespace
{
	struct token_name_case
	{
		pdn::pdn_token_code       code;
		pdn::error_msg_string_view name;
	};

	// Same order as the enumerators in pdn_token_code.h, so the table also documents the numbering.
	constexpr ::std::array<token_name_case, 37> token_names{ {
		{ pdn::pdn_token_code::invalid,                u8"code-invalid"        },
		{ pdn::pdn_token_code::eof,                    u8"eof"                 },
		{ pdn::pdn_token_code::at_identifier,          u8"@identifier"         },
		{ pdn::pdn_token_code::identifier,             u8"identifier"          },
		{ pdn::pdn_token_code::literal_integer,        u8"integer-val"         },
		{ pdn::pdn_token_code::literal_floating_point, u8"floating-point-val"  },
		{ pdn::pdn_token_code::literal_string,         u8"string-val"          },
		{ pdn::pdn_token_code::literal_character,      u8"character-val"       },
		{ pdn::pdn_token_code::literal_boolean,        u8"boolean-val"         },
		{ pdn::pdn_token_code::tilde,                  u8"~"                   },
		{ pdn::pdn_token_code::exclamation_mark,       u8"!"                   },
		{ pdn::pdn_token_code::at_sign,                u8"@"                   },
		{ pdn::pdn_token_code::hash,                   u8"#"                   },
		{ pdn::pdn_token_code::dollar,                 u8"$"                   },
		{ pdn::pdn_token_code::percent,                u8"%"                   },
		{ pdn::pdn_token_code::caret,                  u8"^"                   },
		{ pdn::pdn_token_code::ampersand,              u8"&"                   },
		{ pdn::pdn_token_code::asterisk,               u8"*"                   },
		{ pdn::pdn_token_code::left_parentheses,       u8"("                   },
		{ pdn::pdn_token_code::right_parentheses,      u8")"                   },
		{ pdn::pdn_token_code::left_brackets,          u8"["                   },
		{ pdn::pdn_token_code::right_brackets,         u8"]"                   },
		{ pdn::pdn_token_code::left_curly_brackets,    u8"{"                   },
		{ pdn::pdn_token_code::right_curly_brackets,   u8"}"                   },
		{ pdn::pdn_token_code::minus,                  u8"-"                   },
		{ pdn::pdn_token_code::plus,                   u8"+"                   },
		{ pdn::pdn_token_code::equal,                  u8"="                   },
		{ pdn::pdn_token_code::slash,                  u8"/"                   },
		{ pdn::pdn_token_code::back_slash,             u8"\\"                  },
		{ pdn::pdn_token_code::bar,                    u8"|"                   },
		{ pdn::pdn_token_code::colon,                  u8":"                   },
		{ pdn::pdn_token_code::semicolon,              u8";"                   },
		{ pdn::pdn_token_code::less_than,              u8"<"                   },
		{ pdn::pdn_token_code::greater_than,           u8">"                   },
		{ pdn::pdn_token_code::comma,                  u8","                   },
		{ pdn::pdn_token_code::dot,                    u8"."                   },
		{ pdn::pdn_token_code::question_mark,          u8"?"                   },
	} };
}

static_assert(pdn::token_code_to_error_msg_string_view(pdn::pdn_token_code::semicolon) == u8";");
static_assert(pdn::token_code_to_error_msg_string_view(pdn::pdn_token_code::back_slash) == u8"\\");
static_assert(noexcept(pdn::token_code_to_error_msg_string_view(pdn::pdn_token_code::eof)));
// An out of range code degrades to the marker instead of trapping.
static_assert(pdn::token_code_to_error_msg_string_view(static_cast<pdn::pdn_token_code>(99)) == u8"unknown-code");

int main()
{
	// 1. Every enumerator maps to exactly its expected spelling, and both overloads agree.
	for (const auto& c : token_names)
	{
		const auto view = pdn::token_code_to_error_msg_string_view(c.code);
		assert(view == c.name);
		assert(pdn::token_code_to_error_msg_string(c.code) == c.name);
	}

	// 2. No defined enumerator falls through to the default branch. This is the guard that matters:
	//    adding an enumerator without a case would silently be spelled "unknown-code".
	for (int i = 0; i < static_cast<int>(token_names.size()); ++i)
	{
		assert(pdn::token_code_to_error_msg_string_view(static_cast<pdn::pdn_token_code>(i)) != u8"unknown-code");
	}

	// 3. The spellings are spliced into sentences, so they must be pairwise distinct, ASCII and free of
	//    whitespace: 'but receiving <spelling>' has to stay unambiguous.
	for (::std::size_t i = 0; i < token_names.size(); ++i)
	{
		for (::std::size_t j = i + 1; j < token_names.size(); ++j)
		{
			assert(token_names[i].name != token_names[j].name);
		}
		for (const auto ch : token_names[i].name)
		{
			assert(static_cast<unsigned>(ch) < 0x80);
			assert(ch != u8' ' && ch != u8'\t' && ch != u8'\n' && ch != u8'\r');
		}
	}

	// 4. Values past the last enumerator are not tokens; they get the marker, never a crash.
	assert(pdn::token_code_to_error_msg_string_view(static_cast<pdn::pdn_token_code>(37)) == u8"unknown-code");
	assert(pdn::token_code_to_error_msg_string_view(static_cast<pdn::pdn_token_code>(-1)) == u8"unknown-code");
	assert(pdn::token_code_to_error_msg_string(static_cast<pdn::pdn_token_code>(99)) == u8"unknown-code");
}