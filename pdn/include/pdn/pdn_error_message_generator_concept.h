#ifndef PDN_Header_pdn_error_message_generator_concept
#define PDN_Header_pdn_error_message_generator_concept

#include <concepts>
#include <utility>

#include "pdn_raw_error_message.h"
#include "pdn_error_string.h"

namespace pdn::concepts
{
	// Turns one raw error (code, position, payload) into the text the handler prints. The two
	// requires-expressions call the function through an object, so 'generate_error_message' may be
	// static or a plain member function (the guide only requires it to exist). Both value categories
	// are required because the default generator consumes the payload while composing the message.
	template <typename type>
	concept error_message_generator = requires (type gen, raw_error_message raw_err_msg)
	{
		{ gen.generate_error_message(raw_err_msg) } -> ::std::convertible_to<error_msg_string>;
		{ gen.generate_error_message(::std::move(raw_err_msg)) } -> ::std::convertible_to<error_msg_string>;
	};
}

#endif
