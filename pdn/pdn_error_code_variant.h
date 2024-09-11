#ifndef PDN_Header_pdn_error_code_variant
#define PDN_Header_pdn_error_code_variant

#include <variant>

#include "pdn_unicode.h"
#include "pdn_lexical_error_code.h"
#include "pdn_syntax_error_code.h"

namespace pdn::dev_util
{
	using error_code_variant = ::std::variant<
		unicode::utf_8 ::decode_error_code,
		unicode::utf_8 ::encode_error_code,
		unicode::utf_16::decode_error_code,
		unicode::utf_16::encode_error_code,
		unicode::utf_32::decode_error_code,
		unicode::utf_32::encode_error_code,
		lexical_error_code,
		syntax_error_code>;
}

namespace pdn
{
	using error_code_variant = dev_util::error_code_variant;
}

#endif
