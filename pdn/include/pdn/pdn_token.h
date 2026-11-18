#ifndef PDN_Header_pdn_token
#define PDN_Header_pdn_token

#include "pdn_source_position.h"
#include "pdn_token_code.h"
#include "pdn_token_value_variant.h"

namespace pdn
{
	// One lexeme. Deliberately a plain aggregate so that both positional and designated
	// initialization work (the parser builds its eof token with designated initializers).
	// position - the source position of the first code point of the lexeme, not the position
	//            after it; callers must not read it as a cursor.
	// code     - pdn_token_code::invalid only in a default-constructed token, as the "no token
	//            read yet" marker; every token the lexer produces carries a real code.
	// value    - the payload that belongs to code: literals carry their typed value, identifier
	//            and string carry proxy<type::string<char_t>> (moved out by the consumer),
	//            at_identifier carries detail::at_iden_string_proxy (always UTF-8, independent of
	//            char_t), and eof / punctuators leave it monostate.
	template <typename char_t>
	struct token
	{
		source_position             position{};
		pdn_token_code              code{};
		token_value_variant<char_t> value{};
	};
}

#endif
