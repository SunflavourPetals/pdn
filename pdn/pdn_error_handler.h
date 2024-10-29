#ifndef PDN_Header_pdn_error_handler
#define PDN_Header_pdn_error_handler

#include <iostream>
#include <format>

#include "pdn_error_message.h"
#include "pdn_error_handler_concept.h"

#include "pdn_error_code_variant_to_error_msg_string.h"

namespace pdn
{
	class default_error_handler
	{
	public:
		void handle_error(const error_message& e) const
		{
			handle_error(e, ::std::cout);
		}
		void handle_error(const error_message& e, ::std::ostream& out) const
		{
			auto error_type_s = pdn::error_code_variant_to_error_msg_string(e.error_code);
			out << std::format("{}({}:{}) {}\n",
				reinterpret_cast<const char*>(error_type_s.c_str()),
				e.position.line,
				e.position.column,
				reinterpret_cast<const char*>(e.error_message.c_str()));
		}
	};
	static_assert(concepts::error_handler<default_error_handler>);
}

#endif
