#ifndef PDN_Header_pdn_raw_error_message
#define PDN_Header_pdn_raw_error_message

#include "pdn_source_position.h"
#include "pdn_error_code_variant.h"
#include "pdn_raw_error_message_variant.h"

// The data a message generator needs: the error code (which layer failed and what happened), the
// position, and the payload named by the code's "extra(T)" annotation. Reporting sites build this
// aggregate directly - pdn_code_point_iterator.h for decoding errors, pdn_lexer.h for lexical
// errors and pdn_parser.h for syntax errors - so the member order below is the initialization
// order those sites use.
namespace pdn
{
	struct raw_error_message
	{
		// What went wrong. The active alternative names the layer that produced the error.
		error_code_variant        error_code;
		// Where it went wrong.
		source_position           position;
		// What the message text needs beyond the code: a raw_error_message_type struct. The member is
		// named after its own type, shadowing the injected-class-name inside this struct.
		raw_error_message_variant raw_error_message;
	};
}

#endif
