#ifndef PDN_Header_pdn_error_message_generator_concept
#define PDN_Header_pdn_error_message_generator_concept

#include <concepts>
#include <utility>

#include "pdn_error_code_variant.h"
#include "pdn_raw_error_message_variant.h"
#include "pdn_error_string.h"

namespace pdn::concepts
{
	template <typename type>
	concept error_message_generator = requires (type err_msg_gen, error_code_variant errc_variant, raw_error_message_variant raw_err_msg)
	{
		{ err_msg_gen.generate_error_message(errc_variant, raw_err_msg) }
			-> ::std::convertible_to<error_msg_string>;
		{ err_msg_gen.generate_error_message(::std::move(errc_variant), raw_err_msg) }
			-> ::std::convertible_to<error_msg_string>;
		{ err_msg_gen.generate_error_message(errc_variant, ::std::move(raw_err_msg)) }
			-> ::std::convertible_to<error_msg_string>;
		{ err_msg_gen.generate_error_message(::std::move(errc_variant), ::std::move(raw_err_msg)) }
			-> ::std::convertible_to<error_msg_string>;
	};
}

#endif
