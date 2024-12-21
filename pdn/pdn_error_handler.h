#ifndef PDN_Header_pdn_error_handler
#define PDN_Header_pdn_error_handler

#include <iostream>
#include <string>

#include "pdn_error_message.h"
#include "pdn_error_handler_concept.h"

namespace pdn
{
	class default_error_handler
	{
	public:
		void handle_error(const error_message& e) const
		{
			handle_error(e, ::std::cerr);
		}
		void handle_error(const error_message& e, ::std::ostream& out) const
		{
			out << ::std::string_view{ reinterpret_cast<const char*>(e.error_message.data()), e.error_message.size() } << "\n";
		}
	};
	static_assert(concepts::error_handler<default_error_handler>);
}

#endif
