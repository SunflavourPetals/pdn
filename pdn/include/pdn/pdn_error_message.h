#ifndef PDN_Header_pdn_error_message
#define PDN_Header_pdn_error_message

#include "pdn_error_code_variant.h"
#include "pdn_source_position.h"
#include "pdn_error_string.h"

namespace pdn
{
	// Payload of error_handler::handle_error: the error code (its active
	// alternative names the layer), where it was seen, and the rendered UTF-8
	// text. The text is "<label>(line:column) <detail>", optionally prefixed with
	// "<file>: " by a function package that knows the file name; consumers print
	// it as is instead of re-deriving the position from the other fields. All
	// three fields describe the same event. A value-initialized instance is a
	// placeholder, not an error: its code is the unnamed 0 of the first variant
	// alternative and its text is empty.
	struct error_message
	{
		error_code_variant error_code;
		source_position    position;
		error_msg_string   error_message;
	};
}

#endif
