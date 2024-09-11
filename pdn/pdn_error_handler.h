#ifndef PDN_Header_pdn_error_handler
#define PDN_Header_pdn_error_handler

#include <functional>

#include "pdn_error_message.h"

namespace pdn
{
	using error_handler = ::std::function<void(const error_message&)>;
}

#endif
