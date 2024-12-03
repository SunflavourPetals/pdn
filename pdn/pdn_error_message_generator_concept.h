#ifndef PDN_Header_pdn_error_message_generator_concept
#define PDN_Header_pdn_error_message_generator_concept

#include <concepts>
#include <utility>

#include "pdn_error_code_variant.h"
#include "pdn_raw_error_message.h"
#include "pdn_error_string.h"

namespace pdn::concepts
{
	template <typename type>
	concept error_message_generator = requires (type gen, raw_error_message raw_err_msg)
	{
		{ gen.generate_error_message(raw_err_msg) } -> ::std::convertible_to<error_msg_string>;
		{ gen.generate_error_message(::std::move(raw_err_msg)) } -> ::std::convertible_to<error_msg_string>;
	};
}

#endif
