#ifndef PDN_Header_pdn_raw_error_message
#define PDN_Header_pdn_raw_error_message

#include "pdn_source_position.h"
#include "pdn_error_code_variant.h"
#include "pdn_raw_error_message_variant.h"

namespace pdn
{
	struct raw_error_message
	{
		error_code_variant        error_code;
		source_position           position;
		raw_error_message_variant raw_error_message;
	};
}

#endif
