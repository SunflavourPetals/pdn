#ifndef PDN_Header_pdn_error_code_variant
#define PDN_Header_pdn_error_code_variant

#include <variant>

#include "pdn_utf_decoder.h"
#include "pdn_lexical_error_code.h"
#include "pdn_syntax_error_code.h"

namespace pdn
{
	// The closed set of recoverable errors a parse can report: decoding, lexical and syntax.
	// It deliberately has no alternative for encoding errors: encoders are only used on code
	// points that came out of a decoder and are always valid scalars, so encoding cannot fail
	// while parsing. The alternative order is an implementation detail; do not rely on
	// variant::index().
	using error_code_variant = ::std::variant<
		unicode::utf8::decode_error_code,
		unicode::utf16::decode_error_code,
		unicode::utf32::decode_error_code,
		lexical_error_code,
		syntax_error_code>;
}

#endif
