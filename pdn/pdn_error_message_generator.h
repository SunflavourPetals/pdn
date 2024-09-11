#ifndef PDN_Header_pdn_error_message_generator
#define PDN_Header_pdn_error_message_generator

#include <functional>

#include "pdn_error_string.h"
#include "pdn_error_code_variant.h"

namespace pdn
{
	using error_message_generator = ::std::function<error_msg_string(error_code_variant, error_msg_string)>;
}

#endif
