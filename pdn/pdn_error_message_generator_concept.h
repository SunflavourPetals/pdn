#ifndef PDN_Header_pdn_error_message_generator_concept
#define PDN_Header_pdn_error_message_generator_concept

#include <concepts>
#include <utility>

#include "pdn_error_code_variant.h"
#include "pdn_error_string.h"

namespace pdn::concepts
{
	template <typename type>
	concept error_message_generator = requires (type err_msg_gen, error_code_variant errc_variant, error_msg_string err_msg_str)
	{
		{ err_msg_gen.generate_error_message(errc_variant, err_msg_str) }
			-> ::std::convertible_to<error_msg_string>;
		{ err_msg_gen.generate_error_message(::std::move(errc_variant), err_msg_str) }
			-> ::std::convertible_to<error_msg_string>;
		{ err_msg_gen.generate_error_message(errc_variant, ::std::move(err_msg_str)) }
			-> ::std::convertible_to<error_msg_string>;
		{ err_msg_gen.generate_error_message(::std::move(errc_variant), ::std::move(err_msg_str)) }
			-> ::std::convertible_to<error_msg_string>;
	};
}

#endif
