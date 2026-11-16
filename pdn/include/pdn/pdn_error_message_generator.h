#ifndef PDN_Header_pdn_error_message_generator
#define PDN_Header_pdn_error_message_generator

#include <utility>
#include <string>

#include "pdn_raw_error_message.h"
#include "pdn_error_code_variant_to_error_msg_string.h"
#include "pdn_error_string.h"
#include "pdn_error_message_generator_concept.h"
#include "pdn_error_message_generator_en.h"

namespace pdn
{
	// The built-in message generator: "<category>(<line>:<column>) <detail>", for example
	// "syntax error(3:7) expected entity name (identifier), but got integer-val with value 3".
	// Positions are 1-based (see source_position); a filename, when the function package knows one,
	// is prepended separately by detail::error_msg_for_filename (pdn_error_handler.h).
	class default_error_message_generator
	{
	public:
		static auto generate_error_message(raw_error_message src) -> error_msg_string
		{
			using namespace literals::error_message_literals;
			auto errc = src.error_code;
			auto pos = src.position;
			auto result = error_code_variant_to_error_msg_string(errc)
				.append(u8"("_emv)
				.append(reinterpret_to_err_msg_str(::std::to_string(pos.line)))
				.append(u8":"_emv)
				.append(reinterpret_to_err_msg_str(::std::to_string(pos.column)))
				.append(u8") "_emv)
				.append(error_message_generator_en(::std::move(src)));
			return result;
		}
	};
	static_assert(concepts::error_message_generator<default_error_message_generator>);
}

#endif
