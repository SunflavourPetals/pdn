#ifndef PDN_Header_pdn_error_message_generator
#define PDN_Header_pdn_error_message_generator

#include <functional> // remove this

#include <concepts>
#include <utility>

#include "pdn_error_code_variant.h"
#include "pdn_error_string.h"
#include "pdn_error_message_generator_concept.h"
#include "pdn_error_message_generator_en.h"

namespace pdn
{
	class default_error_message_generator
	{
	public:
		error_msg_string generate_error_message(error_code_variant errc, error_msg_string src)
		{
			return error_message_generator_en_function(::std::move(errc), ::std::move(src));
		}
	};
	static_assert(concepts::error_message_generator<default_error_message_generator>);
}

namespace pdn // remove this
{
	using error_message_generator = ::std::function<error_msg_string(error_code_variant, error_msg_string)>;
}

namespace pdn // remove this
{
	inline const error_message_generator error_message_generator_en{ error_message_generator_en_function };
}

#endif
