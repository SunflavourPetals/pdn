#ifndef PDN_Header_pdn_error_message_generator
#define PDN_Header_pdn_error_message_generator

#include <type_traits>
#include <utility>
#include <string>

#include "pdn_raw_error_message.h"
#include "pdn_error_code_variant.h"
#include "pdn_error_code_variant_to_error_msg_string.h"
#include "pdn_error_string.h"
#include "pdn_error_message_generator_concept.h"
#include "pdn_error_message_generator_en.h"

namespace pdn
{
	class default_error_message_generator
	{
	public:
		static auto generate_error_message(raw_error_message src) -> error_msg_string
		{
			using namespace literals::error_message_literals;
			auto errc = src.error_code;
			auto pos  = src.position;
			return error_code_variant_to_error_msg_string(errc)
				.append(u8"("_em)
				.append(reinterpret_to_err_msg_str(::std::to_string(pos.line)))
				.append(u8":"_em)
				.append(reinterpret_to_err_msg_str(::std::to_string(pos.column)))
				.append(u8") "_em)
				.append(error_message_generator_en(::std::move(src)));
		}
	};
	static_assert(concepts::error_message_generator<default_error_message_generator>);
}

#endif
