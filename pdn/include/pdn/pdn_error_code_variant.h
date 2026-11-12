#ifndef PDN_Header_pdn_error_code_variant
#define PDN_Header_pdn_error_code_variant

#include <variant>

#include "pdn_unicode.h"
#include "pdn_lexical_error_code.h"
#include "pdn_syntax_error_code.h"

namespace pdn
{
	using error_code_variant = ::std::variant<
		unicode::utf8 ::decode_error_code,
		unicode::utf16::decode_error_code,
		unicode::utf32::decode_error_code,
		lexical_error_code,
		syntax_error_code>;
}

#endif
