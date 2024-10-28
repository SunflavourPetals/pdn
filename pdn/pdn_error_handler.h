#ifndef PDN_Header_pdn_error_handler
#define PDN_Header_pdn_error_handler

#include <functional> // remove this

#include <iostream>

#include "pdn_error_message.h"
#include "pdn_error_handler_concept.h"

namespace pdn
{
	class default_error_handler
	{
	public:
		void handle_error(const error_message&) const
		{
			// todo
			::std::cout << "default_error_handler::handle_error called, pdn error(function uncompleted)\n";
		}
	};
	static_assert(concepts::error_handler<default_error_handler>);
}

namespace pdn // remove this
{
	using error_handler = ::std::function<void(const error_message&)>;
}

#endif
