#ifndef PDN_Header_pdn_error_message
#define PDN_Header_pdn_error_message

#include "pdn_source_position.h"
#include "pdn_error_code_variant.h"
#include "pdn_error_string.h"

namespace pdn
{
	struct error_message
	{
		source_position position;
		error_code_variant error_code;
		error_msg_string error_message;
	};
}

#endif
