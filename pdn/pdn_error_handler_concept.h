#ifndef PDN_Header_pdn_error_handler_concept
#define PDN_Header_pdn_error_handler_concept

#include <utility>

#include "pdn_error_message.h"

namespace pdn::concepts
{
	template <typename type>
	concept error_handler = requires (type err_handler, error_message err_msg)
	{
		err_handler.handle_error(err_msg);
		err_handler.handle_error(::std::move(err_msg));
	};
}

#endif
